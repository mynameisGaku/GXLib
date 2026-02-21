/// @file RTGI.cpp
/// @brief DXR グローバルイルミネーション実装
#include "pch.h"
#include "Graphics/RayTracing/RTGI.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Device/BarrierBatch.h"
#include "Core/Logger.h"

namespace GX
{

bool RTGI::Initialize(ID3D12Device5* device, uint32_t width, uint32_t height)
{
    m_device5    = device;
    m_width      = width;
    m_height     = height;
    m_halfWidth  = (std::max)(width / 2, 1u);
    m_halfHeight = (std::max)(height / 2, 1u);

    // アクセラレーション構造 (自前, 共有時は上書き)
    if (!m_ownAccelStruct.Initialize(device))
        return false;
    m_accelStruct = &m_ownAccelStruct;

    // GIパイプライン (パラメータ化されたRTPipeline)
    if (!m_giPipeline.Initialize(device,
            L"Shaders/RTGlobalIllumination.hlsl",
            L"GIRayGen", L"GIClosestHit", L"GIMiss", L"GIShadowMiss", L"GIHitGroup"))
        return false;

    // 半解像度UAV
    CreateHalfResUAV();

    // ディスパッチ用SRV/UAVヒープ
    if (!m_dispatchHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, k_DispatchHeapSize, true))
        return false;

    // 定数バッファ
    if (!m_cb.Initialize(device, 256, 256))
        return false;
    if (!m_lightCB.Initialize(device, 1280, 1280))
        return false;
    if (!m_instanceDataCB.Initialize(device, 24576, 24576))
        return false;
    m_instanceData.reserve(k_MaxInstances);

    // テンポラル蓄積リソース
    CreateTemporalResources();

    // 空間フィルタリソース
    CreateSpatialResources();

    // デノイズパイプライン
    if (!m_denoiseShader.Initialize())
        return false;
    if (!CreateDenoisePipelines(device))
        return false;

    // コンポジットパイプライン
    if (!m_compositeShader.Initialize())
        return false;
    if (!CreateCompositePipeline(device))
        return false;

    // テンポラル用定数バッファ
    if (!m_temporalCB.Initialize(device, 256, 256))
        return false;
    // テンポラル用ヒープ (4 SRV × 2 frames = 8)
    if (!m_temporalHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true))
        return false;

    // 空間フィルタ用定数バッファ (256B × k_MaxSpatialIter, 全イテレーション分を一括書き込み)
    if (!m_spatialCB.Initialize(device, 256 * k_MaxSpatialIter, 256))
        return false;
    // 空間フィルタ用ヒープ (3 SRV × k_MaxSpatialIter = 各イテレーション固有スロット)
    if (!m_spatialHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3 * k_MaxSpatialIter, true))
        return false;

    // コンポジット用
    if (!m_compositeCB.Initialize(device, 256, 256))
        return false;
    if (!m_compositeHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true))
        return false;

    // 初期VP行列を単位行列に
    XMStoreFloat4x4(&m_previousVP, XMMatrixIdentity());

    GX_LOG_INFO("RTGI initialized (%dx%d, half-res dispatch %dx%d)", width, height, m_halfWidth, m_halfHeight);
    return true;
}

void RTGI::CreateHalfResUAV()
{
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width            = m_halfWidth;
    texDesc.Height           = m_halfHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels        = 1;
    texDesc.Format           = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_giUAV.Reset();
    m_device5->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr, IID_PPV_ARGS(&m_giUAV));
    m_giUAVState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

void RTGI::CreateTemporalResources()
{
    for (int i = 0; i < 2; ++i)
        m_temporalHistory[i].Create(m_device5, m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT);

    // 前フレーム深度コピー
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width            = m_width;
    texDesc.Height           = m_height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels        = 1;
    texDesc.Format           = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags            = D3D12_RESOURCE_FLAG_NONE;

    m_prevDepthCopy.Reset();
    m_device5->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr, IID_PPV_ARGS(&m_prevDepthCopy));
}

void RTGI::CreateSpatialResources()
{
    for (int i = 0; i < 2; ++i)
        m_spatialPingPong[i].Create(m_device5, m_width, m_height, DXGI_FORMAT_R16G16B16A16_FLOAT);
}

void RTGI::SetSharedAccelerationStructure(RTAccelerationStructure* shared)
{
    m_accelStruct = shared ? shared : &m_ownAccelStruct;
}

void RTGI::BeginFrame()
{
    // 共有モードでは外部がBeginFrameを呼ぶ。自前モードのみ
    if (m_accelStruct == &m_ownAccelStruct)
        m_accelStruct->BeginFrame();

    m_instanceData.clear();
    m_textureLookup.clear();
    m_textureResources.clear();
    m_nextTextureSlot = 0;
}

int RTGI::BuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                     ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                     ID3D12Resource* ib, uint32_t indexCount,
                     DXGI_FORMAT indexFormat)
{
    if (indexFormat != DXGI_FORMAT_R32_UINT)
        return -1;

    int idx = m_accelStruct->BuildBLAS(cmdList, vb, vertexCount, vertexStride, ib, indexCount, indexFormat);
    if (idx >= 0)
    {
        m_blasLookup[vb] = idx;
        if (static_cast<int>(m_blasGeometry.size()) <= idx)
            m_blasGeometry.resize(idx + 1);
        m_blasGeometry[idx].vb = vb;
        m_blasGeometry[idx].ib = ib;
        m_blasGeometry[idx].vertexStride = vertexStride;
    }
    return idx;
}

int RTGI::FindOrBuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                           ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                           ID3D12Resource* ib, uint32_t indexCount,
                           DXGI_FORMAT indexFormat)
{
    auto it = m_blasLookup.find(vb);
    if (it != m_blasLookup.end())
        return it->second;
    return BuildBLAS(cmdList, vb, vertexCount, vertexStride, ib, indexCount, indexFormat);
}

void RTGI::AddInstance(int blasIndex, const XMMATRIX& worldMatrix,
                        const XMFLOAT3& albedo, float metallic, float roughness,
                        ID3D12Resource* albedoTex, uint32_t instanceFlags)
{
    if (m_instanceData.size() >= k_MaxInstances)
        return;

    m_accelStruct->AddInstance(blasIndex, worldMatrix, 0, 0xFF, instanceFlags);

    float texIdx = -1.0f;
    float hasTexture = 0.0f;
    if (albedoTex)
    {
        auto it = m_textureLookup.find(albedoTex);
        if (it != m_textureLookup.end())
        {
            texIdx = static_cast<float>(it->second);
        }
        else if (m_nextTextureSlot < k_MaxTextures)
        {
            uint32_t slot = m_nextTextureSlot++;
            m_textureLookup[albedoTex] = slot;
            m_textureResources.push_back(ComPtr<ID3D12Resource>(albedoTex));

            auto resDesc = albedoTex->GetDesc();
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = resDesc.MipLevels;
            m_device5->CreateShaderResourceView(albedoTex, &srvDesc,
                m_dispatchHeap.GetCPUHandle(k_TextureSlotsBase + slot));
            texIdx = static_cast<float>(slot);
        }
        hasTexture = 1.0f;
    }

    float vertexStride = 0.0f;
    if (blasIndex >= 0 && blasIndex < static_cast<int>(m_blasGeometry.size()))
        vertexStride = static_cast<float>(m_blasGeometry[blasIndex].vertexStride);

    InstancePBR inst;
    inst.albedoMetallic = { albedo.x, albedo.y, albedo.z, metallic };
    inst.roughnessGeom  = { roughness, static_cast<float>(blasIndex), texIdx, hasTexture };
    inst.extraData      = { vertexStride, 0.0f, 0.0f, 0.0f };
    m_instanceData.push_back(inst);
}

void RTGI::CreateGeometrySRVs()
{
    // null SRV で全スロット初期化
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc = {};
        nullDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        nullDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullDesc.Buffer.NumElements = 1;
        nullDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        for (uint32_t s = 0; s < k_GeomSlotsCount; ++s)
            m_device5->CreateShaderResourceView(nullptr, &nullDesc,
                m_dispatchHeap.GetCPUHandle(k_GeomSlotsBase + s));
    }

    for (size_t i = 0; i < m_blasGeometry.size(); ++i)
    {
        auto& geo = m_blasGeometry[i];
        if (geo.vb)
        {
            auto vbDesc = geo.vb->GetDesc();
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.NumElements = static_cast<uint32_t>(vbDesc.Width / 4);
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
            m_device5->CreateShaderResourceView(geo.vb.Get(), &srvDesc,
                m_dispatchHeap.GetCPUHandle(k_GeomSlotsBase + static_cast<uint32_t>(i) * 2));
        }
        if (geo.ib)
        {
            auto ibDesc = geo.ib->GetDesc();
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.NumElements = static_cast<uint32_t>(ibDesc.Width / 4);
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
            m_device5->CreateShaderResourceView(geo.ib.Get(), &srvDesc,
                m_dispatchHeap.GetCPUHandle(k_GeomSlotsBase + static_cast<uint32_t>(i) * 2 + 1));
        }
    }

    // テクスチャスロット null初期化
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC nullTexDesc = {};
        nullTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        nullTexDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        nullTexDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        nullTexDesc.Texture2D.MipLevels = 1;
        for (uint32_t s = 0; s < k_MaxTextures; ++s)
            m_device5->CreateShaderResourceView(nullptr, &nullTexDesc,
                m_dispatchHeap.GetCPUHandle(k_TextureSlotsBase + s));
    }

    GX_LOG_INFO("RTGI: Created geometry SRVs for %zu BLASes", m_blasGeometry.size());
}

void RTGI::SetLights(const LightData* lights, uint32_t count, const XMFLOAT3& ambient)
{
    m_lightConstants = {};
    uint32_t n = (std::min)(count, LightConstants::k_MaxLights);
    for (uint32_t i = 0; i < n; ++i)
        m_lightConstants.lights[i] = lights[i];
    m_lightConstants.numLights = n;
    m_lightConstants.ambientColor = ambient;
}

void RTGI::SetSkyColors(const XMFLOAT3& top, const XMFLOAT3& bottom)
{
    m_skyTopColor    = top;
    m_skyBottomColor = bottom;
}

bool RTGI::CreateDenoisePipelines(ID3D12Device* device)
{
    constexpr auto kVolatile = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    // テンポラルパス: b0 + t0..t3 (currentGI, history, depth, prevDepth) + s0 + s1
    {
        RootSignatureBuilder rsb;
        m_temporalRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL, kVolatile)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_temporalRS) return false;

        auto vs = m_denoiseShader.CompileFromFile(L"Shaders/RTGIDenoise.hlsl", L"FullscreenVS", L"vs_6_0");
        if (!vs.valid) return false;

        auto ps = m_denoiseShader.CompileFromFile(L"Shaders/RTGIDenoise.hlsl", L"TemporalPS", L"ps_6_0");
        if (!ps.valid) return false;

        PipelineStateBuilder b;
        m_temporalPSO = b.SetRootSignature(m_temporalRS.Get())
            .SetVertexShader(vs.GetBytecode())
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_temporalPSO) return false;
    }

    // 空間フィルタパス: b0 + t0..t2 (input, depth, normal) + s0 + s1
    {
        RootSignatureBuilder rsb;
        m_spatialRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL, kVolatile)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_spatialRS) return false;

        auto vs = m_denoiseShader.CompileFromFile(L"Shaders/RTGIDenoise.hlsl", L"FullscreenVS", L"vs_6_0");
        if (!vs.valid) return false;

        auto ps = m_denoiseShader.CompileFromFile(L"Shaders/RTGIDenoise.hlsl", L"SpatialPS", L"ps_6_0");
        if (!ps.valid) return false;

        PipelineStateBuilder b;
        m_spatialPSO = b.SetRootSignature(m_spatialRS.Get())
            .SetVertexShader(vs.GetBytecode())
            .SetPixelShader(ps.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_spatialPSO) return false;
    }

    return true;
}

bool RTGI::CreateCompositePipeline(ID3D12Device* device)
{
    // b0 + t0..t3 (scene, gi, depth, albedo) + s0 + s1
    {
        constexpr auto kV = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        RootSignatureBuilder rsb;
        m_compositeRS = rsb
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 4, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL, kV)
            .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .AddStaticSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
                              D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                              D3D12_COMPARISON_FUNC_NEVER)
            .Build(device);
        if (!m_compositeRS) return false;
    }

    auto vs = m_compositeShader.CompileFromFile(L"Shaders/RTGIComposite.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_compositeShader.CompileFromFile(L"Shaders/RTGIComposite.hlsl", L"PSMain", L"ps_6_0");
    if (!ps.valid) return false;

    PipelineStateBuilder b;
    m_compositePSO = b.SetRootSignature(m_compositeRS.Get())
        .SetVertexShader(vs.GetBytecode())
        .SetPixelShader(ps.GetBytecode())
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .Build(device);
    if (!m_compositePSO) return false;

    return true;
}

void RTGI::Execute(ID3D12GraphicsCommandList4* cmdList4, uint32_t frameIndex,
                    RenderTarget& srcHDR, RenderTarget& destHDR,
                    DepthBuffer& depth, const Camera3D& camera,
                    RenderTarget& albedoRT)
{
    auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(cmdList4);

    // === Pass 1: DispatchRays (半解像度) ===
    // 1spp コサイン半球サンプルで間接照明を計算。半解像度で負荷を軽減し、
    // テンポラルとA-Trousフィルタで品質を回復する

    // TLAS構築 (自前モードのみ; 共有モードではRTReflectionsが既にビルド済み)
    if (m_accelStruct == &m_ownAccelStruct)
        m_accelStruct->BuildTLAS(cmdList4, frameIndex);

    // リソース遷移
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if (m_normalRT)
        m_normalRT->TransitionTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    // テクスチャリソース遷移
    if (!m_textureResources.empty())
    {
        std::vector<D3D12_RESOURCE_BARRIER> texBarriers(m_textureResources.size());
        for (size_t i = 0; i < m_textureResources.size(); ++i)
        {
            texBarriers[i] = {};
            texBarriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            texBarriers[i].Transition.pResource   = m_textureResources[i].Get();
            texBarriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            texBarriers[i].Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            texBarriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        }
        cmdList->ResourceBarrier(static_cast<UINT>(texBarriers.size()), texBarriers.data());
    }

    // UAV状態にする
    if (m_giUAVState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_giUAV.Get();
        barrier.Transition.StateBefore = m_giUAVState;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        m_giUAVState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    // ディスパッチヒープ更新
    uint32_t heapBase = k_PerFrameBase + frameIndex * k_PerFrameCount;
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = srcHDR.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        m_device5->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                                             m_dispatchHeap.GetCPUHandle(heapBase + 0));

        // Depth SRV
        {
            DXGI_FORMAT depthFormat = depth.GetFormat();
            DXGI_FORMAT depthSrvFormat = DXGI_FORMAT_R32_FLOAT;
            if (depthFormat == DXGI_FORMAT_D24_UNORM_S8_UINT)
                depthSrvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            srvDesc.Format = depthSrvFormat;
        }
        m_device5->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                             m_dispatchHeap.GetCPUHandle(heapBase + 1));

        // Normal SRV
        srvDesc.Format = m_normalRT ? m_normalRT->GetFormat() : DXGI_FORMAT_R16G16B16A16_FLOAT;
        m_device5->CreateShaderResourceView(
            m_normalRT ? m_normalRT->GetResource() : nullptr, &srvDesc,
            m_dispatchHeap.GetCPUHandle(heapBase + 2));

        // Output UAV
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
        uavDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        m_device5->CreateUnorderedAccessView(m_giUAV.Get(), nullptr, &uavDesc,
                                              m_dispatchHeap.GetCPUHandle(heapBase + 3));
    }

    // 定数バッファ更新
    XMMATRIX viewMat = camera.GetViewMatrix();
    XMMATRIX projMat = camera.GetProjectionMatrix();
    XMMATRIX vpMat   = viewMat * projMat;
    XMMATRIX invVP   = XMMatrixInverse(nullptr, vpMat);
    XMMATRIX invProj = XMMatrixInverse(nullptr, projMat);

    RTGIConstants constants = {};
    XMStoreFloat4x4(&constants.invViewProjection, XMMatrixTranspose(invVP));
    XMStoreFloat4x4(&constants.view,              XMMatrixTranspose(viewMat));
    XMStoreFloat4x4(&constants.invProjection,     XMMatrixTranspose(invProj));
    constants.cameraPosition = camera.GetPosition();
    constants.maxDistance     = m_maxDistance;
    constants.screenWidth    = static_cast<float>(m_width);
    constants.screenHeight   = static_cast<float>(m_height);
    constants.halfWidth      = static_cast<float>(m_halfWidth);
    constants.halfHeight     = static_cast<float>(m_halfHeight);
    constants.skyTopColor    = m_skyTopColor;
    constants.frameIndex     = static_cast<float>(m_frameCount);
    constants.skyBottomColor = m_skyBottomColor;

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &constants, sizeof(constants));
        m_cb.Unmap(frameIndex);
    }

    // ライトCB
    void* lp = m_lightCB.Map(frameIndex);
    if (lp)
    {
        memcpy(lp, &m_lightConstants, sizeof(m_lightConstants));
        m_lightCB.Unmap(frameIndex);
    }

    // インスタンスデータCB
    uint32_t instanceCount = static_cast<uint32_t>(m_instanceData.size());
    if (instanceCount > 0)
    {
        void* instData = m_instanceDataCB.Map(frameIndex);
        if (instData)
        {
            auto* dst = static_cast<XMFLOAT4*>(instData);
            for (uint32_t i = 0; i < instanceCount; ++i)
                dst[i] = m_instanceData[i].albedoMetallic;
            for (uint32_t i = 0; i < instanceCount; ++i)
                dst[k_MaxInstances + i] = m_instanceData[i].roughnessGeom;
            for (uint32_t i = 0; i < instanceCount; ++i)
                dst[k_MaxInstances * 2 + i] = m_instanceData[i].extraData;
            m_instanceDataCB.Unmap(frameIndex);
        }
    }

    // DispatchRays
    ID3D12DescriptorHeap* heaps[] = { m_dispatchHeap.GetHeap() };
    cmdList4->SetDescriptorHeaps(1, heaps);
    cmdList4->SetComputeRootSignature(m_giPipeline.GetGlobalRootSignature());
    cmdList4->SetComputeRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList4->SetComputeRootShaderResourceView(1, m_accelStruct->GetTLASAddress());
    cmdList4->SetComputeRootDescriptorTable(2, m_dispatchHeap.GetGPUHandle(heapBase + 0));
    cmdList4->SetComputeRootDescriptorTable(3, m_dispatchHeap.GetGPUHandle(heapBase + 3));
    cmdList4->SetComputeRootConstantBufferView(4, m_instanceDataCB.GetGPUVirtualAddress(frameIndex));
    cmdList4->SetComputeRootDescriptorTable(5, m_dispatchHeap.GetGPUHandle(k_GeomSlotsBase));
    cmdList4->SetComputeRootDescriptorTable(6, m_dispatchHeap.GetGPUHandle(k_TextureSlotsBase));
    cmdList4->SetComputeRootConstantBufferView(7, m_lightCB.GetGPUVirtualAddress(frameIndex));

    m_giPipeline.DispatchRays(cmdList4, m_halfWidth, m_halfHeight);

    // UAVバリア
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_giUAV.Get();
        cmdList->ResourceBarrier(1, &barrier);
    }

    // テクスチャを戻す
    if (!m_textureResources.empty())
    {
        std::vector<D3D12_RESOURCE_BARRIER> texBarriers(m_textureResources.size());
        for (size_t i = 0; i < m_textureResources.size(); ++i)
        {
            texBarriers[i] = {};
            texBarriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            texBarriers[i].Transition.pResource   = m_textureResources[i].Get();
            texBarriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            texBarriers[i].Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            texBarriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        }
        cmdList->ResourceBarrier(static_cast<UINT>(texBarriers.size()), texBarriers.data());
    }

    // GI UAV → SRV遷移 (テンポラルパスで読む)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_giUAV.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        m_giUAVState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // SRCとDepthをPIXEL_SHADER_RESOURCEに戻す (テンポラル/コンポジットで使う)
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (m_normalRT)
        m_normalRT->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // === Pass 2: テンポラル蓄積 (フル解像度) ===
    // 半解像度GIをバイリニアでアップスケールし、前フレーム結果と指数移動平均で蓄積。
    // 前フレームVP行列でリプロジェクション、深度差で棄却判定する
    {
        uint32_t readIdx  = m_temporalWriteIdx;
        uint32_t writeIdx = 1 - m_temporalWriteIdx;

        m_temporalHistory[writeIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_temporalHistory[readIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // ヒープ更新: [0]=currentGI, [1]=history, [2]=depth, [3]=prevDepth
        uint32_t tBase = frameIndex * 4;
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = 1;

            // CurrentGI (半解像度, バイリニアアップスケール)
            m_device5->CreateShaderResourceView(m_giUAV.Get(), &srvDesc,
                m_temporalHeap.GetCPUHandle(tBase + 0));
            // History
            m_device5->CreateShaderResourceView(m_temporalHistory[readIdx].GetResource(), &srvDesc,
                m_temporalHeap.GetCPUHandle(tBase + 1));
            // Depth
            srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
            m_device5->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                m_temporalHeap.GetCPUHandle(tBase + 2));
            // PrevDepth
            m_device5->CreateShaderResourceView(m_prevDepthCopy.Get(), &srvDesc,
                m_temporalHeap.GetCPUHandle(tBase + 3));
        }

        // テンポラル定数
        RTGITemporalConstants tc = {};
        XMStoreFloat4x4(&tc.prevViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&m_previousVP)));
        XMStoreFloat4x4(&tc.invViewProjection,  XMMatrixTranspose(invVP));
        tc.alpha      = m_temporalAlpha;
        tc.frameCount = static_cast<float>(m_frameCount);
        tc.fullWidth  = static_cast<float>(m_width);
        tc.fullHeight = static_cast<float>(m_height);

        void* tcp = m_temporalCB.Map(frameIndex);
        if (tcp) { memcpy(tcp, &tc, sizeof(tc)); m_temporalCB.Unmap(frameIndex); }

        // 描画
        auto rtv = m_temporalHistory[writeIdx].GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

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

        ID3D12DescriptorHeap* tHeaps[] = { m_temporalHeap.GetHeap() };
        cmdList->SetDescriptorHeaps(1, tHeaps);
        cmdList->SetGraphicsRootConstantBufferView(0, m_temporalCB.GetGPUVirtualAddress(frameIndex));
        cmdList->SetGraphicsRootDescriptorTable(1, m_temporalHeap.GetGPUHandle(tBase));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);

        m_temporalWriteIdx = writeIdx;
    }

    // === Pass 3: A-Trous 空間フィルタ (フル解像度, 複数イテレーション) ===
    // Edge-avoiding (depth/normal/color) ウェイトで空間ノイズ除去。
    // ステップ幅を2^iter倍に拡大して広範囲をカバーする (a trous = "with holes")。
    // ping-pong RTで入出力を交互に切替え、最終出力は可変のRT
    {
        m_temporalHistory[m_temporalWriteIdx].TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // 全イテレーションのCBデータを一括書き込み (GPU実行時にオーバーライトされない)
        {
            void* scp = m_spatialCB.Map(frameIndex);
            if (scp)
            {
                for (int iter = 0; iter < m_spatialIterations; ++iter)
                {
                    RTGISpatialConstants sc = {};
                    sc.fullWidth   = static_cast<float>(m_width);
                    sc.fullHeight  = static_cast<float>(m_height);
                    sc.stepWidth   = static_cast<float>(1 << iter);
                    sc.sigmaDepth  = 0.01f;
                    sc.sigmaNormal = 128.0f;
                    sc.sigmaColor  = 4.0f;
                    memcpy(static_cast<uint8_t*>(scp) + iter * 256, &sc, sizeof(sc));
                }
                m_spatialCB.Unmap(frameIndex);
            }
        }

        // 初回入力はテンポラル出力
        RenderTarget* input = &m_temporalHistory[m_temporalWriteIdx];
        int spatialIdx = 0;

        for (int iter = 0; iter < m_spatialIterations; ++iter)
        {
            RenderTarget* output = &m_spatialPingPong[spatialIdx];
            output->TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
            if (input != &m_temporalHistory[m_temporalWriteIdx])
                input->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            // ヒープ: 各イテレーション固有スロット [iter*3+0]=input, [iter*3+1]=depth, [iter*3+2]=normal
            uint32_t sBase = iter * 3;
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MipLevels = 1;

                m_device5->CreateShaderResourceView(input->GetResource(), &srvDesc,
                    m_spatialHeap.GetCPUHandle(sBase + 0));

                srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                m_device5->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                    m_spatialHeap.GetCPUHandle(sBase + 1));

                srvDesc.Format = m_normalRT ? m_normalRT->GetFormat() : DXGI_FORMAT_R16G16B16A16_FLOAT;
                m_device5->CreateShaderResourceView(
                    m_normalRT ? m_normalRT->GetResource() : nullptr, &srvDesc,
                    m_spatialHeap.GetCPUHandle(sBase + 2));
            }

            auto rtv = output->GetRTVHandle();
            cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

            D3D12_VIEWPORT vp = {};
            vp.Width  = static_cast<float>(m_width);
            vp.Height = static_cast<float>(m_height);
            vp.MaxDepth = 1.0f;
            cmdList->RSSetViewports(1, &vp);

            D3D12_RECT rect = {};
            rect.right  = static_cast<LONG>(m_width);
            rect.bottom = static_cast<LONG>(m_height);
            cmdList->RSSetScissorRects(1, &rect);

            cmdList->SetPipelineState(m_spatialPSO.Get());
            cmdList->SetGraphicsRootSignature(m_spatialRS.Get());

            ID3D12DescriptorHeap* sHeaps[] = { m_spatialHeap.GetHeap() };
            cmdList->SetDescriptorHeaps(1, sHeaps);
            cmdList->SetGraphicsRootConstantBufferView(0,
                m_spatialCB.GetGPUVirtualAddress(frameIndex) + iter * 256);
            cmdList->SetGraphicsRootDescriptorTable(1, m_spatialHeap.GetGPUHandle(sBase));

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdList->DrawInstanced(3, 1, 0, 0);

            input = output;
            spatialIdx = 1 - spatialIdx;
        }

        // 最終デノイズ結果は input が指すRT
        RenderTarget* denoisedGI = input;

        // === Pass 4: コンポジット ===
        denoisedGI->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        albedoRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // コンポジットヒープ: [0]=scene, [1]=denoisedGI, [2]=depth, [3]=albedo
        uint32_t cBase = frameIndex * 4;
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = srcHDR.GetFormat();
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = 1;

            m_device5->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                m_compositeHeap.GetCPUHandle(cBase + 0));

            srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            m_device5->CreateShaderResourceView(denoisedGI->GetResource(), &srvDesc,
                m_compositeHeap.GetCPUHandle(cBase + 1));

            srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
            m_device5->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                m_compositeHeap.GetCPUHandle(cBase + 2));

            srvDesc.Format = albedoRT.GetFormat();
            m_device5->CreateShaderResourceView(albedoRT.GetResource(), &srvDesc,
                m_compositeHeap.GetCPUHandle(cBase + 3));
        }

        RTGICompositeConstants cc = {};
        cc.intensity  = m_intensity;
        cc.debugMode  = static_cast<float>(m_debugMode);
        cc.fullWidth  = static_cast<float>(m_width);
        cc.fullHeight = static_cast<float>(m_height);

        void* ccp = m_compositeCB.Map(frameIndex);
        if (ccp) { memcpy(ccp, &cc, sizeof(cc)); m_compositeCB.Unmap(frameIndex); }

        auto destRTV = destHDR.GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &destRTV, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width  = static_cast<float>(m_width);
        vp.Height = static_cast<float>(m_height);
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT rect = {};
        rect.right  = static_cast<LONG>(m_width);
        rect.bottom = static_cast<LONG>(m_height);
        cmdList->RSSetScissorRects(1, &rect);

        cmdList->SetPipelineState(m_compositePSO.Get());
        cmdList->SetGraphicsRootSignature(m_compositeRS.Get());

        ID3D12DescriptorHeap* cHeaps[] = { m_compositeHeap.GetHeap() };
        cmdList->SetDescriptorHeaps(1, cHeaps);
        cmdList->SetGraphicsRootConstantBufferView(0, m_compositeCB.GetGPUVirtualAddress(frameIndex));
        cmdList->SetGraphicsRootDescriptorTable(1, m_compositeHeap.GetGPUHandle(cBase));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // 深度バッファを前フレームコピーに保存 (次フレームのテンポラルリプロジェクション用)
    // D32_FLOAT → R32_FLOAT にCopyTextureRegionでフォーマット互換コピー
    {
        // depth → COPY_SOURCE
        depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

        // prevDepthCopy → COPY_DEST
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_prevDepthCopy.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        // 深度バッファはD32_FLOAT (typeless copy)
        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource = depth.GetResource();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource = m_prevDepthCopy.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        // 戻す
        barrier.Transition.pResource   = m_prevDepthCopy.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        cmdList->ResourceBarrier(1, &barrier);

        depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    // VP行列を保存 (次フレームのテンポラル用)
    XMStoreFloat4x4(&m_previousVP, vpMat);
    m_frameCount++;
}

void RTGI::OnResize(ID3D12Device* device, uint32_t w, uint32_t h)
{
    m_width      = w;
    m_height     = h;
    m_halfWidth  = (std::max)(w / 2, 1u);
    m_halfHeight = (std::max)(h / 2, 1u);

    if (m_device5)
    {
        CreateHalfResUAV();
        CreateTemporalResources();
        CreateSpatialResources();
        m_frameCount = 0;
    }
}

} // namespace GX
