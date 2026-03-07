/// @file VolumetricClouds.cpp
/// @brief Volumetric Clouds implementation
///
/// Features: 3D noise textures, temporal reprojection, half-res rendering.
#include "pch_graphics.h"
#include "Graphics/3D/VolumetricClouds.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"
#include <cmath>

namespace gx
{

// ============================================================================
// ノイズ生成ヘルパー（CPU側）
// ============================================================================

namespace
{

/// HLSL frac() equivalent: always returns [0,1), unlike std::fmod which can return negative values.
inline float hlslFrac(float x) { return x - std::floor(x); }

float hashF(float x, float y, float z)
{
    float p0 = hlslFrac(x * 0.3183099f + 0.1f);
    float p1 = hlslFrac(y * 0.3183099f + 0.1f);
    float p2 = hlslFrac(z * 0.3183099f + 0.1f);
    p0 *= 17.0f; p1 *= 17.0f; p2 *= 17.0f;
    return hlslFrac(p0 * p1 * p2 * (p0 + p1 + p2));
}

struct Float3 { float x, y, z; };

Float3 hash3F(float ix, float iy, float iz)
{
    float px = hlslFrac(ix * 0.1031f);
    float py = hlslFrac(iy * 0.1030f);
    float pz = hlslFrac(iz * 0.0973f);
    // dot(p, p.yzx + 33.33)
    float d = px * (py + 33.33f) + py * (pz + 33.33f) + pz * (px + 33.33f);
    px += d; py += d; pz += d;
    // frac((p.xxy + p.yxx) * p.zyx) — matches HLSL hash3()
    // xxy+yxx = (x+y, x+x, y+x), zyx = (z, y, x)
    return { hlslFrac((px + py) * pz),
             hlslFrac((px + px) * py),
             hlslFrac((py + px) * px) };
}

float worleyCPU(float px, float py, float pz)
{
    float ix = std::floor(px), iy = std::floor(py), iz = std::floor(pz);
    float fx = px - ix, fy = py - iy, fz = pz - iz;
    float minDist = 1.0f;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    for (int z = -1; z <= 1; z++)
    {
        Float3 h = hash3F(ix + x, iy + y, iz + z);
        float dx = x + h.x - fx;
        float dy = y + h.y - fy;
        float dz = z + h.z - fz;
        float d = dx * dx + dy * dy + dz * dz;
        if (d < minDist) minDist = d;
    }
    return std::sqrt(minDist);
}

float valueNoiseCPU(float px, float py, float pz)
{
    float ix = std::floor(px), iy = std::floor(py), iz = std::floor(pz);
    float fx = px - ix, fy = py - iy, fz = pz - iz;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    fz = fz * fz * (3.0f - 2.0f * fz);

    auto h = [&](float ox, float oy, float oz) { return hashF(ix + ox, iy + oy, iz + oz); };
    float a = h(0,0,0) * (1-fx) + h(1,0,0) * fx;
    float b = h(0,1,0) * (1-fx) + h(1,1,0) * fx;
    float c = h(0,0,1) * (1-fx) + h(1,0,1) * fx;
    float d = h(0,1,1) * (1-fx) + h(1,1,1) * fx;
    float e = a * (1-fy) + b * fy;
    float f = c * (1-fy) + d * fy;
    return e * (1-fz) + f * fz;
}

float fbmCPU(float px, float py, float pz, int octaves)
{
    float value = 0.0f, amp = 0.5f, freq = 1.0f;
    for (int i = 0; i < octaves; ++i)
    {
        value += amp * valueNoiseCPU(px * freq, py * freq, pz * freq);
        freq *= 2.0f;
        amp *= 0.5f;
    }
    return value;
}

uint8_t toU8(float v) { return static_cast<uint8_t>((std::max)(0.0f, (std::min)(v, 1.0f)) * 255.0f + 0.5f); }

} // anonymous namespace

// ============================================================================
// Destructor — ensure background noise thread is joined
// ============================================================================

VolumetricClouds::~VolumetricClouds()
{
    if (m_noiseThread.joinable())
        m_noiseThread.join();
}

// ============================================================================
// Initialize
// ============================================================================

bool VolumetricClouds::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // SRV heap: 4 textures (scene+depth+baseNoise+detailNoise) x 2 frames = 8 slots
    if (!m_srvHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true))
        return false;

    if (!m_shader.Initialize())
        return false;

    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // Root signature: [0] CBV(b0), [1] DescTable(t0..t3, 4 SRVs), s0(linear clamp), s1(linear wrap)
    {
        RootSignatureBuilder rsb;
        m_rootSignature = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL,
                                D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_rootSignature) return false;
    }

    if (!CreatePipelines(device))
        return false;

    // 3Dノイズテクスチャをバックグラウンドスレッドで生成（CPU側ハッシュはHLSL frac()互換に修正済み）
    m_noiseThread = std::thread([this]() { GenerateNoiseData(); });

    // Temporal reprojection resources
    CreateTemporalPipeline(device);

    ShaderLibrary::Instance().RegisterPSORebuilder(
        L"Shaders/VolumetricClouds.hlsl",
        [this](ID3D12Device* dev) { return CreatePipelines(dev); }
    );

    GX_LOG_INFO("VolumetricClouds initialized (%dx%d) with 3D noise + temporal reprojection", width, height);
    return true;
}

// ============================================================================
// CreatePipelines — PSMain (full composite) + PSCloudOnly (cloud-only for temporal)
// ============================================================================

bool VolumetricClouds::CreatePipelines(ID3D12Device* device)
{
    auto vs = m_shader.CompileFromFile(L"Shaders/VolumetricClouds.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;
    auto vsBytecode = vs.GetBytecode();

    // PSMain — full composite (scene + clouds)
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/VolumetricClouds.hlsl", L"PSMain", L"ps_6_0");
        if (!ps.valid) return false;

        PipelineStateBuilder b;
        m_pso = b.SetRootSignature(m_rootSignature.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_pso) return false;
    }

    // PSCloudOnly — cloud-only output for temporal path
    {
        auto ps = m_shader.CompileFromFile(L"Shaders/VolumetricClouds.hlsl", L"PSCloudOnly", L"ps_6_0");
        if (!ps.valid) return false;

        PipelineStateBuilder b;
        m_cloudOnlyPSO = b.SetRootSignature(m_rootSignature.Get())
            .SetVertexShader(vsBytecode)
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_cloudOnlyPSO) return false;
    }

    return true;
}

// ============================================================================
// 3D Noise Texture Generation (CPU)
// ============================================================================

void VolumetricClouds::GenerateNoiseData()
{
    // --- Base shape noise: 128³ RGBA8 ---
    constexpr uint32_t BASE_SIZE = 128;
    constexpr uint32_t BASE_TEXELS = BASE_SIZE * BASE_SIZE * BASE_SIZE;
    m_baseNoiseData.resize(BASE_TEXELS * 4);

    for (uint32_t z = 0; z < BASE_SIZE; z++)
    for (uint32_t y = 0; y < BASE_SIZE; y++)
    for (uint32_t x = 0; x < BASE_SIZE; x++)
    {
        float fx = static_cast<float>(x) / BASE_SIZE;
        float fy = static_cast<float>(y) / BASE_SIZE;
        float fz = static_cast<float>(z) / BASE_SIZE;

        // R = Perlin-Worley (base shape)
        float perlin = fbmCPU(fx * 4.0f, fy * 4.0f, fz * 4.0f, 4);
        float wor1 = worleyCPU(fx * 4.0f, fy * 4.0f, fz * 4.0f);
        float pw = (std::max)(0.0f, perlin - wor1 * 0.35f);

        // G = Worley 1x
        float w1 = worleyCPU(fx * 8.0f, fy * 8.0f, fz * 8.0f);

        // B = Worley 2x
        float w2 = worleyCPU(fx * 16.0f, fy * 16.0f, fz * 16.0f);

        // A = Worley 4x
        float w4 = worleyCPU(fx * 32.0f, fy * 32.0f, fz * 32.0f);

        uint32_t idx = (z * BASE_SIZE * BASE_SIZE + y * BASE_SIZE + x) * 4;
        m_baseNoiseData[idx + 0] = toU8(pw);
        m_baseNoiseData[idx + 1] = toU8(w1);
        m_baseNoiseData[idx + 2] = toU8(w2);
        m_baseNoiseData[idx + 3] = toU8(w4);
    }

    // --- Detail noise: 32³ RGBA8 ---
    constexpr uint32_t DETAIL_SIZE = 32;
    constexpr uint32_t DETAIL_TEXELS = DETAIL_SIZE * DETAIL_SIZE * DETAIL_SIZE;
    m_detailNoiseData.resize(DETAIL_TEXELS * 4);

    for (uint32_t z = 0; z < DETAIL_SIZE; z++)
    for (uint32_t y = 0; y < DETAIL_SIZE; y++)
    for (uint32_t x = 0; x < DETAIL_SIZE; x++)
    {
        float fx = static_cast<float>(x) / DETAIL_SIZE;
        float fy = static_cast<float>(y) / DETAIL_SIZE;
        float fz = static_cast<float>(z) / DETAIL_SIZE;

        float w1 = worleyCPU(fx * 8.0f, fy * 8.0f, fz * 8.0f);
        float w2 = worleyCPU(fx * 16.0f, fy * 16.0f, fz * 16.0f);
        float w4 = worleyCPU(fx * 32.0f, fy * 32.0f, fz * 32.0f);

        uint32_t idx = (z * DETAIL_SIZE * DETAIL_SIZE + y * DETAIL_SIZE + x) * 4;
        m_detailNoiseData[idx + 0] = toU8(w1);
        m_detailNoiseData[idx + 1] = toU8(w2);
        m_detailNoiseData[idx + 2] = toU8(w4);
        m_detailNoiseData[idx + 3] = 255;
    }

    m_noiseDataReady.store(true, std::memory_order_release);
    GX_LOG_INFO("VolumetricClouds: noise data generated (128^3 + 32^3) on background thread");
}

void VolumetricClouds::CreateNoiseResources(ID3D12Device* device)
{
    if (!device) return;

    auto createTex3D = [&](uint32_t size, const uint8_t* data,
                           ComPtr<ID3D12Resource>& texOut, ComPtr<ID3D12Resource>& uploadOut)
    {
        // Default heap texture
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        texDesc.Width     = size;
        texDesc.Height    = size;
        texDesc.DepthOrArraySize = static_cast<uint16_t>(size);
        texDesc.MipLevels = 1;
        texDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout    = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags     = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES defaultHeap = {};
        defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

        HRESULT hr = device->CreateCommittedResource(
            &defaultHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texOut));
        if (FAILED(hr)) return;

        // Calculate upload buffer size with alignment
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT64 totalBytes = 0;
        device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, &totalBytes);

        D3D12_RESOURCE_DESC uploadDesc = {};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Width     = totalBytes;
        uploadDesc.Height    = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.Format    = DXGI_FORMAT_UNKNOWN;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.Layout    = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES uploadHeap = {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

        hr = device->CreateCommittedResource(
            &uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadOut));
        if (FAILED(hr)) return;

        // Copy data to upload buffer respecting row pitch alignment
        uint8_t* mapped = nullptr;
        hr = uploadOut->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
        if (FAILED(hr)) return;

        uint32_t srcRowPitch = size * 4;
        uint32_t dstRowPitch = footprint.Footprint.RowPitch;
        uint32_t numRows = size;
        uint32_t numSlices = size;

        for (uint32_t z = 0; z < numSlices; z++)
        {
            for (uint32_t y = 0; y < numRows; y++)
            {
                const uint8_t* src = data + (z * size * size + y * size) * 4;
                uint8_t* dst = mapped + footprint.Offset + z * dstRowPitch * numRows + y * dstRowPitch;
                memcpy(dst, src, srcRowPitch);
            }
        }
        uploadOut->Unmap(0, nullptr);
    };

    constexpr uint32_t BASE_SIZE = 128;
    constexpr uint32_t DETAIL_SIZE = 32;

    createTex3D(BASE_SIZE, m_baseNoiseData.data(),
                m_baseNoiseTexture, m_baseNoiseUpload);
    createTex3D(DETAIL_SIZE, m_detailNoiseData.data(),
                m_detailNoiseTexture, m_detailNoiseUpload);

    // Free CPU-side noise data — no longer needed after upload buffer is populated
    m_baseNoiseData.clear();
    m_baseNoiseData.shrink_to_fit();
    m_detailNoiseData.clear();
    m_detailNoiseData.shrink_to_fit();

    GX_LOG_INFO("VolumetricClouds: D3D12 noise resources created");
}

void VolumetricClouds::UploadNoiseTextures(ID3D12GraphicsCommandList* cmdList)
{
    if (m_noiseUploaded) return;
    if (!m_baseNoiseTexture || !m_detailNoiseTexture) return;

    auto copyTex3D = [&](ID3D12Resource* tex, ID3D12Resource* upload, uint32_t size)
    {
        D3D12_RESOURCE_DESC texDesc = tex->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        m_device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr);

        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = tex;
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = upload;
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = footprint;

        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = tex;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    };

    copyTex3D(m_baseNoiseTexture.Get(), m_baseNoiseUpload.Get(), 128);
    copyTex3D(m_detailNoiseTexture.Get(), m_detailNoiseUpload.Get(), 32);

    m_noiseUploaded = true;
}

// ============================================================================
// Temporal Pipeline
// ============================================================================

void VolumetricClouds::CreateTemporalPipeline(ID3D12Device* device)
{
    if (!device) return;

    // Half-res cloud RT
    uint32_t halfW = (std::max)(1u, m_width / 2);
    uint32_t halfH = (std::max)(1u, m_height / 2);
    m_cloudRT.Create(device, halfW, halfH, DXGI_FORMAT_R16G16B16A16_FLOAT);

    // Full-res history double buffer
    m_historyRT[0].Create(device, m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_historyRT[1].Create(device, m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT);

    // Temporal SRV heap: 4 SRVs x 2 frames = 8 slots
    m_temporalHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true);

    // Temporal CB
    m_temporalCB.Initialize(device, 256, 256);

    // Temporal root signature: CBV(b0) + DescTable(t0..t3, 4 SRVs) + s0(linear clamp) + s1(point clamp)
    {
        RootSignatureBuilder rsb;
        m_temporalRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL,
                                D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
    }

    // Temporal resolve PSO
    if (m_temporalRS)
    {
        auto vs = m_shader.CompileFromFile(L"Shaders/VolumetricCloudsTemporal.hlsl", L"FullscreenVS", L"vs_6_0");
        auto ps = m_shader.CompileFromFile(L"Shaders/VolumetricCloudsTemporal.hlsl", L"PSTemporalResolve", L"ps_6_0");
        if (vs.valid && ps.valid)
        {
            PipelineStateBuilder b;
            m_temporalPSO = b.SetRootSignature(m_temporalRS.Get())
                .SetVertexShader(vs.GetBytecode())
                .SetPixelShader(ps.GetBytecode())
                .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
                .SetDepthEnable(false)
                .SetCullMode(D3D12_CULL_MODE_NONE)
                .Build(device);
        }
    }

    m_hasHistory = false;
    m_hasPreviousVP = false;
    m_historyWriteIdx = 0;
}

// ============================================================================
// UpdateSRVHeap — 4 SRVs per frame: scene + depth + baseNoise + detailNoise
// ============================================================================

void VolumetricClouds::UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex)
{
    uint32_t base = frameIndex * 4;

    // [base+0] = scene HDR
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = srcHDR.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 0));
    }

    // [base+1] = depth
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        m_device->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                            m_srvHeap.GetCPUHandle(base + 1));
    }

    // [base+2] = baseNoise (3D) or null SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (m_baseNoiseTexture)
        {
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MipLevels       = 1;
            srvDesc.Texture3D.MostDetailedMip = 0;
            m_device->CreateShaderResourceView(m_baseNoiseTexture.Get(), &srvDesc,
                                                m_srvHeap.GetCPUHandle(base + 2));
        }
        else
        {
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MipLevels       = 1;
            m_device->CreateShaderResourceView(nullptr, &srvDesc,
                                                m_srvHeap.GetCPUHandle(base + 2));
        }
    }

    // [base+3] = detailNoise (3D) or null SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (m_detailNoiseTexture)
        {
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MipLevels       = 1;
            srvDesc.Texture3D.MostDetailedMip = 0;
            m_device->CreateShaderResourceView(m_detailNoiseTexture.Get(), &srvDesc,
                                                m_srvHeap.GetCPUHandle(base + 3));
        }
        else
        {
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MipLevels       = 1;
            m_device->CreateShaderResourceView(nullptr, &srvDesc,
                                                m_srvHeap.GetCPUHandle(base + 3));
        }
    }
}

// ============================================================================
// CloudConstants helper — fills CB from member params
// ============================================================================

static CloudConstants BuildCloudCB(const VolumetricClouds& self,
                                    const Camera3D& camera, float elapsedTime,
                                    float screenW, float screenH, bool noiseUploaded)
{
    XMMATRIX vp_mat = ToXMMATRIX(camera.GetViewProjectionMatrix());
    XMMATRIX invVP  = XMMatrixInverse(nullptr, vp_mat);

    XMVECTOR sunDir = XMVector3Normalize(XMVectorNegate(XMLoadFloat3(XM(&self.GetSunDirection()))));
    Vector3 sunDirF;
    XMStoreFloat3(XM(&sunDirF), sunDir);

    CloudConstants cb = {};
    XMStoreFloat4x4(XM(&cb.invViewProjection), XMMatrixTranspose(invVP));
    cb.cameraPosition   = camera.GetPosition();
    cb.time             = elapsedTime;
    cb.sunDirection     = sunDirF;
    cb.cloudBottom      = self.GetCloudBottom();
    cb.cloudTop         = self.GetCloudTop();
    cb.coverage         = self.GetCoverage();
    cb.densityMul       = self.GetDensity();
    cb.windSpeed        = self.GetWindSpeed();
    cb.silverLining     = self.GetSilverLiningIntensity();
    cb.windDirection    = self.GetWindDirection();
    cb.marchSteps       = self.GetMarchSteps();
    cb.lightSteps       = self.GetLightSteps();
    cb.screenDimensions = { screenW, screenH };
    cb.msOctaves        = self.GetMSOctaves();
    cb.msAttenuation    = self.GetMSAttenuation();
    cb.msContribution   = self.GetMSContribution();
    cb.msEccentricity   = self.GetMSEccentricity();
    cb.powderAmount     = self.GetPowderAmount();
    cb.ambientBottom    = self.GetAmbientBottom();
    cb.ambientTop       = self.GetAmbientTop();
    cb.atmosphereDensity = self.GetAtmosphereDensity();
    cb.useNoiseTextures = noiseUploaded ? 1 : 0;
    return cb;
}

// ============================================================================
// Execute — main entry point, dispatches temporal or single-pass
// ============================================================================

void VolumetricClouds::Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                RenderTarget& srcHDR, RenderTarget& destHDR,
                                DepthBuffer& depth, const Camera3D& camera,
                                float elapsedTime)
{
    // 非同期ノイズ生成完了チェック → GPUリソース作成 + アップロード
    if (!m_noiseUploaded && m_noiseDataReady.load(std::memory_order_acquire))
    {
        if (!m_baseNoiseTexture)
            CreateNoiseResources(m_device);
        UploadNoiseTextures(cmdList);
    }

    // Temporal path: half-res cloud → temporal resolve + bilateral upsample → composite
    if (m_temporalEnabled && m_temporalPSO && m_cloudOnlyPSO)
    {
        // Pass 1: half-res cloud-only → m_cloudRT
        ExecuteCloudOnly(cmdList, frameIndex, depth, camera, elapsedTime);

        // Pass 2: temporal resolve + bilateral upsample + scene composite → destHDR
        ExecuteTemporalResolve(cmdList, frameIndex, srcHDR, destHDR, depth, camera);

        // Save VP for next frame
        XMMATRIX vpMat = ToXMMATRIX(camera.GetViewProjectionMatrix());
        XMStoreFloat4x4(XM(&m_previousVP), XMMatrixTranspose(vpMat));
        m_hasPreviousVP = true;

        depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        return;
    }

    // Fallback: single-pass full-res (legacy path)
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    UpdateSRVHeap(srcHDR, depth, frameIndex);

    auto destRTV = destHDR.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_pso.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    CloudConstants cb = BuildCloudCB(*this, camera, elapsedTime,
                                     static_cast<float>(m_width), static_cast<float>(m_height),
                                     m_noiseUploaded);
    cb.sunColor = m_sunColor;

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &cb, sizeof(cb));
        m_cb.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex * 4));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

// ============================================================================
// ExecuteCloudOnly — half-res cloud-only render
// ============================================================================

void VolumetricClouds::ExecuteCloudOnly(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                         DepthBuffer& depth, const Camera3D& camera,
                                         float elapsedTime)
{
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cloudRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Populate SRV heap for cloud-only pass:
    // t0 = null SRV (scene unused by PSCloudOnly, but validation requires valid descriptor)
    // t1 = depth, t2 = baseNoise, t3 = detailNoise
    {
        uint32_t base = frameIndex * 4;

        // [base+0] = null SRV (PSCloudOnly doesn't read scene texture)
        D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
        nullDesc.Format                    = DXGI_FORMAT_R16G16B16A16_FLOAT;
        nullDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullDesc.Texture2D.MipLevels       = 1;
        m_device->CreateShaderResourceView(nullptr, &nullDesc,
                                            m_srvHeap.GetCPUHandle(base + 0));

        // [base+1] = depth
        D3D12_SHADER_RESOURCE_VIEW_DESC depthDesc = {};
        depthDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
        depthDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        depthDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        depthDesc.Texture2D.MipLevels       = 1;
        m_device->CreateShaderResourceView(depth.GetResource(), &depthDesc,
                                            m_srvHeap.GetCPUHandle(base + 1));

        // [base+2] = baseNoise (3D) or null SRV
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC noiseDesc = {};
            noiseDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
            noiseDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (m_baseNoiseTexture)
            {
                noiseDesc.ViewDimension         = D3D12_SRV_DIMENSION_TEXTURE3D;
                noiseDesc.Texture3D.MipLevels   = 1;
                m_device->CreateShaderResourceView(m_baseNoiseTexture.Get(), &noiseDesc,
                                                    m_srvHeap.GetCPUHandle(base + 2));
            }
            else
            {
                noiseDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE3D;
                noiseDesc.Texture3D.MipLevels       = 1;
                m_device->CreateShaderResourceView(nullptr, &noiseDesc,
                                                    m_srvHeap.GetCPUHandle(base + 2));
            }
        }

        // [base+3] = detailNoise (3D) or null SRV
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC noiseDesc = {};
            noiseDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
            noiseDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            if (m_detailNoiseTexture)
            {
                noiseDesc.ViewDimension         = D3D12_SRV_DIMENSION_TEXTURE3D;
                noiseDesc.Texture3D.MipLevels   = 1;
                m_device->CreateShaderResourceView(m_detailNoiseTexture.Get(), &noiseDesc,
                                                    m_srvHeap.GetCPUHandle(base + 3));
            }
            else
            {
                noiseDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE3D;
                noiseDesc.Texture3D.MipLevels       = 1;
                m_device->CreateShaderResourceView(nullptr, &noiseDesc,
                                                    m_srvHeap.GetCPUHandle(base + 3));
            }
        }
    }

    auto rtv = m_cloudRT.GetRTVHandle();

    // Clear to (0,0,0,1) = no scattering + transmittance=1 (fully transparent)
    // Prevents black screen if any pixels miss the cloud-only draw
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    uint32_t halfW = m_cloudRT.GetWidth();
    uint32_t halfH = m_cloudRT.GetHeight();

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(halfW);
    vp.Height = static_cast<float>(halfH);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(halfW);
    sc.bottom = static_cast<LONG>(halfH);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_cloudOnlyPSO.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    CloudConstants cb = BuildCloudCB(*this, camera, elapsedTime,
                                     static_cast<float>(halfW), static_cast<float>(halfH),
                                     m_noiseUploaded);
    cb.sunColor = m_sunColor;

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &cb, sizeof(cb));
        m_cb.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap.GetGPUHandle(frameIndex * 4));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}

// ============================================================================
// ExecuteTemporalResolve — bilateral upsample + temporal accumulation + scene composite
// ============================================================================

void VolumetricClouds::ExecuteTemporalResolve(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                               RenderTarget& srcHDR, RenderTarget& destHDR,
                                               DepthBuffer& depth, const Camera3D& camera)
{
    uint32_t readIdx  = 1 - m_historyWriteIdx;
    uint32_t writeIdx = m_historyWriteIdx;

    // Transitions
    m_cloudRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_historyRT[readIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Update temporal SRV heap
    uint32_t base = frameIndex * 4;

    auto createSRV2D = [&](uint32_t slot, ID3D12Resource* res, DXGI_FORMAT fmt) {
        D3D12_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = fmt;
        sd.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        sd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        sd.Texture2D.MipLevels = 1;
        m_device->CreateShaderResourceView(res, &sd, m_temporalHeap.GetCPUHandle(slot));
    };

    // t0 = half-res cloud RT
    createSRV2D(base + 0, m_cloudRT.GetResource(), DXGI_FORMAT_R16G16B16A16_FLOAT);
    // t1 = history (previous frame, full-res)
    createSRV2D(base + 1, m_historyRT[readIdx].GetResource(), DXGI_FORMAT_R16G16B16A16_FLOAT);
    // t2 = depth (full-res)
    createSRV2D(base + 2, depth.GetResource(), DXGI_FORMAT_R32_FLOAT);
    // t3 = scene HDR (full-res)
    createSRV2D(base + 3, srcHDR.GetResource(), srcHDR.GetFormat());

    auto destRTV = destHDR.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width  = static_cast<float>(m_width);
    vp.Height = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);

    cmdList->SetPipelineState(m_temporalPSO.Get());
    cmdList->SetGraphicsRootSignature(m_temporalRS.Get());

    ID3D12DescriptorHeap* heaps[] = { m_temporalHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Build temporal CB
    XMMATRIX vpMat = ToXMMATRIX(camera.GetViewProjectionMatrix());
    XMMATRIX invVP = XMMatrixInverse(nullptr, vpMat);

    CloudTemporalConstants tcb = {};
    tcb.prevViewProjection = m_previousVP;  // Already stored transposed
    XMStoreFloat4x4(XM(&tcb.invViewProjection), XMMatrixTranspose(invVP));
    tcb.alpha        = m_temporalAlpha;
    tcb.screenWidth  = static_cast<float>(m_width);
    tcb.screenHeight = static_cast<float>(m_height);
    tcb.hasHistory   = (m_hasHistory && m_hasPreviousVP) ? 1.0f : 0.0f;
    tcb.halfWidth    = static_cast<float>(m_cloudRT.GetWidth());
    tcb.halfHeight   = static_cast<float>(m_cloudRT.GetHeight());
    tcb.depthThreshold = 0.01f;

    void* p = m_temporalCB.Map(frameIndex);
    if (p)
    {
        memcpy(p, &tcb, sizeof(tcb));
        m_temporalCB.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_temporalCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_temporalHeap.GetGPUHandle(base));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // Copy result to history buffer for next frame
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_historyRT[writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->CopyResource(m_historyRT[writeIdx].GetResource(), destHDR.GetResource());

    // Flip history buffer and restore states
    m_historyWriteIdx = 1 - m_historyWriteIdx;
    m_hasHistory = true;

    // Restore destHDR for subsequent post-effects
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

// ============================================================================
// OnResize
// ============================================================================

void VolumetricClouds::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    // Recreate temporal RTs
    if (device && m_temporalPSO)
    {
        uint32_t halfW = (std::max)(1u, width / 2);
        uint32_t halfH = (std::max)(1u, height / 2);
        m_cloudRT.Create(device, halfW, halfH, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_historyRT[0].Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_historyRT[1].Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
        m_hasHistory = false;
        m_hasPreviousVP = false;
    }
}

} // namespace gx
