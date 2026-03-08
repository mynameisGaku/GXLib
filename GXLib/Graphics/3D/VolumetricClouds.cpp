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

struct Float3 { float x, y, z; };

/// Ken Perlin 標準順列テーブル (256 entries, duplicated to 512)
static constexpr int s_perm[512] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
};

/// 順列テーブルベースの決定論的3Dハッシュ（3独立チェーン、素数オフセットで非相関化）
Float3 hash3Perm(int ix, int iy, int iz)
{
    // Chain 0
    int a0 = s_perm[ix & 255];
    int b0 = s_perm[(a0 + iy) & 255];
    int c0 = s_perm[(b0 + iz) & 255];
    // Chain 1 (prime offsets: 97, 53, 131)
    int a1 = s_perm[(ix + 97) & 255];
    int b1 = s_perm[(a1 + iy + 53) & 255];
    int c1 = s_perm[(b1 + iz + 131) & 255];
    // Chain 2 (prime offsets: 173, 211, 67)
    int a2 = s_perm[(ix + 173) & 255];
    int b2 = s_perm[(a2 + iy + 211) & 255];
    int c2 = s_perm[(b2 + iz + 67) & 255];
    return { static_cast<float>(c0) / 255.0f,
             static_cast<float>(c1) / 255.0f,
             static_cast<float>(c2) / 255.0f };
}

// ---------- タイル可能 Perlin 3D ----------

inline int floorToInt(float x)
{
    int xi = static_cast<int>(x);
    return (x < static_cast<float>(xi)) ? xi - 1 : xi;
}

inline float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
inline float lerpF(float a, float b, float t) { return a + t * (b - a); }

inline float grad3D(int hash, float gx, float gy, float gz)
{
    int h = hash & 15;
    float u = h < 8 ? gx : gy;
    float v = h < 4 ? gy : (h == 12 || h == 14 ? gx : gz);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

/// 任意周期でタイルするPerlin 3Dノイズ (戻り値 [-1, 1])
/// 8頂点すべてのハッシュをラップ済み座標から独立計算（継ぎ目バグ修正）
float perlinTileable3D(float x, float y, float z, int period)
{
    auto wrap = [&](int i) { return ((i % period) + period) % period; };

    int xi0 = wrap(floorToInt(x));
    int yi0 = wrap(floorToInt(y));
    int zi0 = wrap(floorToInt(z));
    int xi1 = wrap(xi0 + 1);
    int yi1 = wrap(yi0 + 1);
    int zi1 = wrap(zi0 + 1);

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float zf = z - std::floor(z);
    float u = fade(xf), v = fade(yf), w = fade(zf);

    // 各頂点のハッシュを正しくラップした座標で計算
    auto ph = [](int xi, int yi, int zi) -> int {
        return s_perm[(s_perm[(s_perm[xi & 255] + yi) & 255] + zi) & 255];
    };
    int h000 = ph(xi0, yi0, zi0), h100 = ph(xi1, yi0, zi0);
    int h010 = ph(xi0, yi1, zi0), h110 = ph(xi1, yi1, zi0);
    int h001 = ph(xi0, yi0, zi1), h101 = ph(xi1, yi0, zi1);
    int h011 = ph(xi0, yi1, zi1), h111 = ph(xi1, yi1, zi1);

    float x1, x2, y1, y2;
    x1 = lerpF(grad3D(h000, xf,        yf,        zf),
               grad3D(h100, xf - 1.0f, yf,        zf), u);
    x2 = lerpF(grad3D(h010, xf,        yf - 1.0f, zf),
               grad3D(h110, xf - 1.0f, yf - 1.0f, zf), u);
    y1 = lerpF(x1, x2, v);

    x1 = lerpF(grad3D(h001, xf,        yf,        zf - 1.0f),
               grad3D(h101, xf - 1.0f, yf,        zf - 1.0f), u);
    x2 = lerpF(grad3D(h011, xf,        yf - 1.0f, zf - 1.0f),
               grad3D(h111, xf - 1.0f, yf - 1.0f, zf - 1.0f), u);
    y2 = lerpF(x1, x2, v);

    return lerpF(y1, y2, w);
}

/// タイル可能 FBM 3D (各オクターブで period を 2 倍にして周期維持)
float fbmTileable3D(float x, float y, float z, int octaves, int basePeriod)
{
    float sum = 0.0f, amplitude = 1.0f, frequency = 1.0f;
    int period = basePeriod;
    for (int i = 0; i < octaves; ++i)
    {
        sum       += amplitude * perlinTileable3D(x * frequency, y * frequency, z * frequency, period);
        amplitude *= 0.5f;
        frequency *= 2.0f;
        period    *= 2;
    }
    return sum;
}

// ---------- タイル可能 Worley ----------

/// 周期的Worleyノイズ（period で自動タイル）
float worleyTileable(float px, float py, float pz, int period)
{
    float ix = std::floor(px), iy = std::floor(py), iz = std::floor(pz);
    float fx = px - ix, fy = py - iy, fz = pz - iz;
    int ixi = static_cast<int>(ix), iyi = static_cast<int>(iy), izi = static_cast<int>(iz);
    float minDist = 1.0f;
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    for (int z = -1; z <= 1; z++)
    {
        int cx = ((ixi + x) % period + period) % period;
        int cy = ((iyi + y) % period + period) % period;
        int cz = ((izi + z) % period + period) % period;
        Float3 h = hash3Perm(cx, cy, cz);
        float dx = x + h.x - fx;
        float dy = y + h.y - fy;
        float dz = z + h.z - fz;
        float d = dx * dx + dy * dy + dz * dz;
        if (d < minDist) minDist = d;
    }
    return std::sqrt(minDist);
}

uint8_t toU8(float v) { return static_cast<uint8_t>((std::max)(0.0f, (std::min)(v, 1.0f)) * 255.0f + 0.5f); }

} // anonymous namespace

// ============================================================================
// Destructor — ensure background noise thread is joined
// ============================================================================

VolumetricClouds::~VolumetricClouds() = default;

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

    // 3Dノイズテクスチャを同期生成 + GPUリソース作成（Initialize中に~2秒ブロック、シーン表示前なので問題なし）
    GenerateNoiseData();
    CreateNoiseResources(device);

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
    // R = Perlin-Worley (tileable FBM @4 + invWorley @5, 非整合周波数)
    // G = Worley @7  (素数, 非整合: 4の倍数ではない → グリッド整列しない)
    // B = Worley @13 (素数, 非整合, オフセット付きで G とも非整列)
    // A = Worley @23 (素数, 非整合)
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

        // R = tileable Perlin FBM @4 + inverted Worley @5 (非整合で自然な形状)
        float perlin = fbmTileable3D(fx * 4.0f, fy * 4.0f, fz * 4.0f, 4, 4);
        float perlin01 = perlin * 0.5f + 0.5f;
        float wInv = 1.0f - worleyTileable(fx * 5.0f, fy * 5.0f, fz * 5.0f, 5);
        float pw = (std::max)(0.0f, (std::min)(1.0f, perlin01 * 0.7f + wInv * 0.3f));

        // G = Worley @7 + offset (素数セル数、非整合グリッド)
        float g = worleyTileable(fx * 7.0f + 0.37f, fy * 7.0f + 0.83f, fz * 7.0f + 0.17f, 7);

        // B = Worley @13 + offset
        float b = worleyTileable(fx * 13.0f + 0.71f, fy * 13.0f + 0.29f, fz * 13.0f + 0.53f, 13);

        // A = Worley @23 + offset
        float a = worleyTileable(fx * 23.0f + 0.13f, fy * 23.0f + 0.61f, fz * 23.0f + 0.41f, 23);

        uint32_t idx = (z * BASE_SIZE * BASE_SIZE + y * BASE_SIZE + x) * 4;
        m_baseNoiseData[idx + 0] = toU8(pw);
        m_baseNoiseData[idx + 1] = toU8(g);
        m_baseNoiseData[idx + 2] = toU8(b);
        m_baseNoiseData[idx + 3] = toU8(a);
    }

    // --- Detail noise: 32³ RGBA8 ---
    // R = Worley @3 + offset (素数)
    // G = Worley @7 + offset (素数、base G と周波数同じだが異なるオフセット)
    // B = Worley @11 + offset (素数)
    // A = 255
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

        float w1 = worleyTileable(fx * 3.0f + 0.31f, fy * 3.0f + 0.73f, fz * 3.0f + 0.19f, 3);
        float w2 = worleyTileable(fx * 7.0f + 0.57f, fy * 7.0f + 0.43f, fz * 7.0f + 0.89f, 7);
        float w4 = worleyTileable(fx * 11.0f + 0.67f, fy * 11.0f + 0.11f, fz * 11.0f + 0.47f, 11);

        uint32_t idx = (z * DETAIL_SIZE * DETAIL_SIZE + y * DETAIL_SIZE + x) * 4;
        m_detailNoiseData[idx + 0] = toU8(w1);
        m_detailNoiseData[idx + 1] = toU8(w2);
        m_detailNoiseData[idx + 2] = toU8(w4);
        m_detailNoiseData[idx + 3] = 255;
    }

    GX_LOG_INFO("VolumetricClouds: noise data generated (128^3 + 32^3)");
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
    cb.coverageVariation = self.GetCoverageVariation();
    cb.cloudType         = self.GetCloudType();

    // AC7品質向上パラメータ
    cb.diffusivity       = self.GetDiffusivity();
    cb.upperDensity      = self.GetUpperDensity();
    cb.shadowDarkness    = self.GetShadowDarkness();
    cb.shadowBias        = self.GetShadowBias();
    cb.atmosphereBlueShift = self.GetAtmosphereBlueShift();
    cb.detailFadeDistance = self.GetDetailFadeDistance();

    // sun elevation → ambient color を自動計算
    float sunElev = (std::max)(sunDirF.y, 0.0f);
    Vector3 twilight = { 0.2f, 0.15f, 0.35f };   // 薄明
    Vector3 sunset   = { 0.8f, 0.35f, 0.2f };     // 夕焼け
    Vector3 daytime  = { 0.35f, 0.45f, 0.6f };    // 昼間
    Vector3 ambient;
    if (sunElev < 0.15f)
        ambient = Vector3::Lerp(twilight, sunset, sunElev / 0.15f);
    else
        ambient = Vector3::Lerp(sunset, daytime, (std::min)((sunElev - 0.15f) / 0.35f, 1.0f));
    cb.ambientSkyR = ambient.x;
    cb.ambientSkyG = ambient.y;
    cb.ambientSkyB = ambient.z;

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
    // GPUリソースはInitialize()で作成済み、初回Execute()でアップロードのみ
    if (!m_noiseUploaded)
        UploadNoiseTextures(cmdList);

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
