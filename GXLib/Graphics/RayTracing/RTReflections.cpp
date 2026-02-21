/// @file RTReflections.cpp
/// @brief DXR レイトレーシング反射エフェクトの実装
#include "pch.h"
#include "Graphics/RayTracing/RTReflections.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Device/BarrierBatch.h"
#include "Core/Logger.h"

namespace GX
{

bool RTReflections::Initialize(ID3D12Device5* device, uint32_t width, uint32_t height)
{
    m_device5    = device;
    m_width      = width;
    m_height     = height;

    // アクセラレーション構造
    if (!m_accelStruct.Initialize(device))
        return false;

    // RTパイプライン
    if (!m_rtPipeline.Initialize(device))
        return false;

    // フル解像度UAVテクスチャ作成
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width            = m_width;
        texDesc.Height           = m_height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels        = 1;
        texDesc.Format           = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr, IID_PPV_ARGS(&m_reflectionUAV));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("RTReflections: Failed to create UAV texture");
            return false;
        }
        m_reflectionState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    // ディスパッチ用 shader-visible SRVヒープ (1つのヒープに全リソースを集約)
    // レイアウト: [8..39]=geometry VB/IB(ByteAddressBuffer), [40..71]=albedo textures,
    //            [72..79]=per-frame SRV/UAV (Scene,Depth,Normal,Output × 2フレーム)
    if (!m_dispatchHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, k_DispatchHeapSize, true))
        return false;

    // 定数バッファ (256B for RTReflectionConstants)
    if (!m_cb.Initialize(device, 256, 256))
        return false;

    // ライト定数バッファ (sizeof(LightConstants) ≈ 1040, 256 align → 1280)
    if (!m_lightCB.Initialize(device, 1280, 1280))
        return false;

    // コンポジットパス
    if (!m_compositeShader.Initialize())
        return false;

    if (!m_compositeCB.Initialize(device, 256, 256))
        return false;

    // コンポジット用SRVヒープ (4テクスチャ × 2フレーム = 8)
    if (!m_compositeHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true))
        return false;

    // per-instance PBRデータ CBV
    // GPU側は3つの配列を連続配置: albedoMetallic[512] + roughnessGeom[512] + extraData[512]
    // StructuredBufferではなくCBVとして渡す (24576B = 512 * sizeof(float4) * 3)
    if (!m_instanceDataCB.Initialize(device, 24576, 24576))
        return false;
    m_instanceData.reserve(k_MaxInstances);

    if (!CreateCompositePipeline(device))
        return false;

    GX_LOG_INFO("RTReflections initialized (%dx%d, full-res dispatch)", width, height);
    return true;
}

bool RTReflections::CreateCompositePipeline(ID3D12Device* device)
{
    // ルートシグネチャ: [0]=CBV(b0) + [1]=DescTable(t0,t1,t2,t3: 4SRV) + s0(linear) + s1(point)
    {
        RootSignatureBuilder rsb;
        m_compositeRS = rsb
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
        if (!m_compositeRS) return false;
    }

    // シェーダーコンパイル
    auto vs = m_compositeShader.CompileFromFile(L"Shaders/RTReflectionComposite.hlsl", L"FullscreenVS", L"vs_6_0");
    if (!vs.valid) return false;

    auto ps = m_compositeShader.CompileFromFile(L"Shaders/RTReflectionComposite.hlsl", L"PSMain", L"ps_6_0");
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

int RTReflections::BuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                              ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                              ID3D12Resource* ib, uint32_t indexCount,
                              DXGI_FORMAT indexFormat)
{
    if (indexFormat != DXGI_FORMAT_R32_UINT)
    {
        GX_LOG_WARN("RTReflections: Only R32_UINT index format supported, got %d", indexFormat);
        return -1;
    }

    int idx = m_accelStruct.BuildBLAS(cmdList, vb, vertexCount, vertexStride,
                                       ib, indexCount, indexFormat);
    if (idx >= 0)
    {
        m_blasLookup[vb] = idx;

        // VB/IBリソースを保存 (ClosestHitでByteAddressBufferとしてアクセス, ComPtrで寿命管理)
        if (static_cast<int>(m_blasGeometry.size()) <= idx)
            m_blasGeometry.resize(idx + 1);
        m_blasGeometry[idx].vb = vb;
        m_blasGeometry[idx].ib = ib;
        m_blasGeometry[idx].vertexStride = vertexStride;
    }
    return idx;
}

int RTReflections::FindOrBuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                                    ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                                    ID3D12Resource* ib, uint32_t indexCount,
                                    DXGI_FORMAT indexFormat)
{
    auto it = m_blasLookup.find(vb);
    if (it != m_blasLookup.end())
        return it->second;
    return BuildBLAS(cmdList, vb, vertexCount, vertexStride, ib, indexCount, indexFormat);
}

void RTReflections::BeginFrame()
{
    m_accelStruct.BeginFrame();
    m_instanceData.clear();
    m_textureLookup.clear();
    m_textureResources.clear();
    m_nextTextureSlot = 0;
}

void RTReflections::AddInstance(int blasIndex, const XMMATRIX& worldMatrix,
                                 const XMFLOAT3& albedo, float metallic, float roughness,
                                 ID3D12Resource* albedoTex, uint32_t instanceFlags)
{
    if (m_instanceData.size() >= k_MaxInstances)
    {
        GX_LOG_WARN("RTReflections: exceeded max instances (%u)", k_MaxInstances);
        return;
    }

    m_accelStruct.AddInstance(blasIndex, worldMatrix, 0, 0xFF, instanceFlags);

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

    // BLAS から頂点ストライドを取得
    float vertexStride = 0.0f;
    if (blasIndex >= 0 && blasIndex < static_cast<int>(m_blasGeometry.size()))
        vertexStride = static_cast<float>(m_blasGeometry[blasIndex].vertexStride);

    InstancePBR inst;
    inst.albedoMetallic = { albedo.x, albedo.y, albedo.z, metallic };
    inst.roughnessGeom  = { roughness, static_cast<float>(blasIndex), texIdx, hasTexture };
    inst.extraData      = { vertexStride, 0.0f, 0.0f, 0.0f };
    m_instanceData.push_back(inst);
}

void RTReflections::CreateGeometrySRVs()
{
    // 各BLASのVB/IBを ByteAddressBuffer SRVとしてヒープ[k_GeomSlotsBase+i*2]に作成

    // まず全32スロットをnull SRVで初期化 (STATICディスクリプタレンジは全要素初期化必須)
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

    // 実BLASのVB/IBで上書き
    for (size_t i = 0; i < m_blasGeometry.size(); ++i)
    {
        auto& geo = m_blasGeometry[i];

        // VB SRV (ByteAddressBuffer)
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

        // IB SRV (ByteAddressBuffer)
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

    // テクスチャスロット[40..71]をnull Texture2D SRVで初期化
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

    GX_LOG_INFO("RTReflections: Created geometry SRVs for %zu BLASes", m_blasGeometry.size());
}

void RTReflections::SetLights(const LightData* lights, uint32_t count, const XMFLOAT3& ambient)
{
    m_lightConstants = {};
    uint32_t n = (std::min)(count, LightConstants::k_MaxLights);
    for (uint32_t i = 0; i < n; ++i)
        m_lightConstants.lights[i] = lights[i];
    m_lightConstants.numLights = n;
    m_lightConstants.ambientColor = ambient;
}

void RTReflections::SetSkyColors(const XMFLOAT3& top, const XMFLOAT3& bottom)
{
    m_skyTopColor    = top;
    m_skyBottomColor = bottom;
}

void RTReflections::Execute(ID3D12GraphicsCommandList4* cmdList4, uint32_t frameIndex,
                             RenderTarget& srcHDR, RenderTarget& destHDR,
                             DepthBuffer& depth, const Camera3D& camera)
{
    // TLAS構築
    m_accelStruct.BuildTLAS(cmdList4, frameIndex);

    auto* cmdList = static_cast<ID3D12GraphicsCommandList*>(cmdList4);

    // DXRのDispatchRaysはコンピュートパイプラインで実行されるため、
    // 入力SRVはNON_PIXEL_SHADER_RESOURCE状態でなければならない
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    if (m_normalRT)
        m_normalRT->TransitionTo(cmdList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    // テクスチャリソースを NON_PIXEL_SHADER_RESOURCE に遷移 (DXR はコンピュートパイプライン)
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

    // 反射UAVをUAV状態にする
    if (m_reflectionState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_reflectionUAV.Get();
        barrier.Transition.StateBefore = m_reflectionState;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        m_reflectionState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    // ディスパッチ用ヒープ更新 (4スロット/フレーム, ジオメトリ/テクスチャの後に配置)
    uint32_t heapBase = k_PerFrameBase + frameIndex * k_PerFrameCount;
    {
        // [base+0] = Scene SRV (t1)
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = srcHDR.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        m_device5->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                                             m_dispatchHeap.GetCPUHandle(heapBase + 0));

        // [base+1] = Depth SRV (t2) — フォーマットを深度バッファ形式から動的に決定
        {
            DXGI_FORMAT depthFormat = depth.GetFormat();
            DXGI_FORMAT depthSrvFormat = DXGI_FORMAT_R32_FLOAT; // デフォルト (D32_FLOAT)
            if (depthFormat == DXGI_FORMAT_D24_UNORM_S8_UINT)
                depthSrvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            srvDesc.Format = depthSrvFormat;
        }
        m_device5->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                             m_dispatchHeap.GetCPUHandle(heapBase + 1));

        // [base+2] = Normal SRV (t3)
        srvDesc.Format = m_normalRT ? m_normalRT->GetFormat() : DXGI_FORMAT_R16G16B16A16_FLOAT;
        m_device5->CreateShaderResourceView(
            m_normalRT ? m_normalRT->GetResource() : nullptr, &srvDesc,
            m_dispatchHeap.GetCPUHandle(heapBase + 2));

        // [base+3] = Output UAV (u0)
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
        uavDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        m_device5->CreateUnorderedAccessView(m_reflectionUAV.Get(), nullptr, &uavDesc,
                                              m_dispatchHeap.GetCPUHandle(heapBase + 3));
    }

    // 定数バッファ更新
    XMMATRIX viewMat = camera.GetViewMatrix();
    XMMATRIX projMat = camera.GetProjectionMatrix();
    XMMATRIX vpMat   = viewMat * projMat;
    XMMATRIX invVP   = XMMatrixInverse(nullptr, vpMat);
    XMMATRIX invProj = XMMatrixInverse(nullptr, projMat);

    RTReflectionConstants constants = {};
    XMStoreFloat4x4(&constants.invViewProjection, XMMatrixTranspose(invVP));
    XMStoreFloat4x4(&constants.view,              XMMatrixTranspose(viewMat));
    XMStoreFloat4x4(&constants.invProjection,     XMMatrixTranspose(invProj));
    XMFLOAT3 camPos = camera.GetPosition();
    constants.cameraPosition = camPos;
    constants.maxDistance     = m_maxDistance;
    constants.screenWidth    = static_cast<float>(m_width);
    constants.screenHeight   = static_cast<float>(m_height);
    constants.debugMode      = static_cast<float>(m_debugMode);
    constants.intensity      = m_intensity;
    constants.skyTopColor    = m_skyTopColor;
    constants.skyBottomColor = m_skyBottomColor;

    void* p = m_cb.Map(frameIndex);
    if (p)
    {
        memcpy(p, &constants, sizeof(constants));
        m_cb.Unmap(frameIndex);
    }

    // ライト定数バッファアップロード
    void* lp = m_lightCB.Map(frameIndex);
    if (lp)
    {
        memcpy(lp, &m_lightConstants, sizeof(m_lightConstants));
        m_lightCB.Unmap(frameIndex);
    }

    // per-instance PBRデータをアップロード
    // GPU側レイアウト: float4 albedoMetallic[512] + float4 roughnessGeom[512] + float4 extraData[512]
    uint32_t instanceCount = static_cast<uint32_t>(m_instanceData.size());
    if (instanceCount > 0)
    {
        void* instData = m_instanceDataCB.Map(frameIndex);
        if (instData)
        {
            auto* dst = static_cast<XMFLOAT4*>(instData);
            // [0..511]: albedoMetallic
            for (uint32_t i = 0; i < instanceCount; ++i)
                dst[i] = m_instanceData[i].albedoMetallic;
            // [512..1023]: roughnessGeom
            for (uint32_t i = 0; i < instanceCount; ++i)
                dst[k_MaxInstances + i] = m_instanceData[i].roughnessGeom;
            // [1024..1535]: extraData (vertexStride etc.)
            for (uint32_t i = 0; i < instanceCount; ++i)
                dst[k_MaxInstances * 2 + i] = m_instanceData[i].extraData;
            m_instanceDataCB.Unmap(frameIndex);
        }
    }

    // DispatchRays
    ID3D12DescriptorHeap* heaps[] = { m_dispatchHeap.GetHeap() };
    cmdList4->SetDescriptorHeaps(1, heaps);
    cmdList4->SetComputeRootSignature(m_rtPipeline.GetGlobalRootSignature());
    cmdList4->SetComputeRootConstantBufferView(0, m_cb.GetGPUVirtualAddress(frameIndex));
    cmdList4->SetComputeRootShaderResourceView(1, m_accelStruct.GetTLASAddress());
    cmdList4->SetComputeRootDescriptorTable(2, m_dispatchHeap.GetGPUHandle(heapBase + 0)); // t1,t2,t3
    cmdList4->SetComputeRootDescriptorTable(3, m_dispatchHeap.GetGPUHandle(heapBase + 3)); // u0
    cmdList4->SetComputeRootConstantBufferView(4, m_instanceDataCB.GetGPUVirtualAddress(frameIndex)); // b1: PBR data
    cmdList4->SetComputeRootDescriptorTable(5, m_dispatchHeap.GetGPUHandle(k_GeomSlotsBase)); // geometry VB/IB SRVs (space1)
    cmdList4->SetComputeRootDescriptorTable(6, m_dispatchHeap.GetGPUHandle(k_TextureSlotsBase)); // albedo textures (space2)
    cmdList4->SetComputeRootConstantBufferView(7, m_lightCB.GetGPUVirtualAddress(frameIndex)); // b2: LightConstants

    m_rtPipeline.DispatchRays(cmdList4, m_width, m_height);

    // UAVバリア
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_reflectionUAV.Get();
        cmdList->ResourceBarrier(1, &barrier);
    }

    // テクスチャリソースを PIXEL_SHADER_RESOURCE に戻す
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

    // === コンポジットパス (ピクセルシェーダーベース) ===
    // DispatchRaysで書いた反射UAVをSRVとして読み、srcHDRにFresnel反射率で加算合成
    // srcHDR / depth / normal をピクセルシェーダー用に遷移
    srcHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (m_normalRT)
        m_normalRT->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // 反射UAVをSRV状態に遷移
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = m_reflectionUAV.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
        m_reflectionState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }

    // destHDR を RT 状態に
    destHDR.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // コンポジット用ヒープ更新 (4 SRV/フレーム)
    uint32_t compBase = frameIndex * 4;
    {
        // [0] = Scene SRV
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                    = srcHDR.GetFormat();
        srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels      = 1;
        m_device5->CreateShaderResourceView(srcHDR.GetResource(), &srvDesc,
                                             m_compositeHeap.GetCPUHandle(compBase + 0));

        // [1] = Depth SRV — フォーマットを深度バッファ形式から動的に決定
        {
            DXGI_FORMAT depthFormat = depth.GetFormat();
            DXGI_FORMAT depthSrvFormat = DXGI_FORMAT_R32_FLOAT; // デフォルト (D32_FLOAT)
            if (depthFormat == DXGI_FORMAT_D24_UNORM_S8_UINT)
                depthSrvFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            srvDesc.Format = depthSrvFormat;
        }
        m_device5->CreateShaderResourceView(depth.GetResource(), &srvDesc,
                                             m_compositeHeap.GetCPUHandle(compBase + 1));

        // [2] = Reflection SRV
        srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        m_device5->CreateShaderResourceView(m_reflectionUAV.Get(), &srvDesc,
                                             m_compositeHeap.GetCPUHandle(compBase + 2));

        // [3] = Normal SRV
        srvDesc.Format = m_normalRT ? m_normalRT->GetFormat() : DXGI_FORMAT_R16G16B16A16_FLOAT;
        m_device5->CreateShaderResourceView(
            m_normalRT ? m_normalRT->GetResource() : nullptr, &srvDesc,
            m_compositeHeap.GetCPUHandle(compBase + 3));
    }

    // コンポジット描画
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

    cmdList->SetPipelineState(m_compositePSO.Get());
    cmdList->SetGraphicsRootSignature(m_compositeRS.Get());

    ID3D12DescriptorHeap* compHeaps[] = { m_compositeHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, compHeaps);

    // コンポジット定数バッファ更新 (Fresnel計算に必要なカメラ/行列情報を含む)
    RTCompositeConstants compConstants = {};
    compConstants.intensity = m_intensity;
    compConstants.debugMode = static_cast<float>(m_debugMode);
    compConstants.screenWidth  = static_cast<float>(m_width);
    compConstants.screenHeight = static_cast<float>(m_height);
    compConstants.cameraPosition = camPos;
    XMStoreFloat4x4(&compConstants.invViewProjection, XMMatrixTranspose(invVP));

    void* cp = m_compositeCB.Map(frameIndex);
    if (cp)
    {
        memcpy(cp, &compConstants, sizeof(compConstants));
        m_compositeCB.Unmap(frameIndex);
    }

    cmdList->SetGraphicsRootConstantBufferView(0, m_compositeCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1, m_compositeHeap.GetGPUHandle(compBase));

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    // 深度バッファを DEPTH_WRITE に戻す
    depth.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void RTReflections::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width      = width;
    m_height     = height;

    // フル解像度UAVテクスチャを再作成
    if (m_device5)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width            = m_width;
        texDesc.Height           = m_height;
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels        = 1;
        texDesc.Format           = DXGI_FORMAT_R16G16B16A16_FLOAT;
        texDesc.SampleDesc.Count = 1;
        texDesc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        m_reflectionUAV.Reset();
        HRESULT hr = m_device5->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr, IID_PPV_ARGS(&m_reflectionUAV));
        if (FAILED(hr))
        {
            GX_LOG_ERROR("RTReflections: Failed to recreate UAV on resize");
            return;
        }
        m_reflectionState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
}

} // namespace GX
