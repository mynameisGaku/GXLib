/// @file LensFlare.cpp
/// @brief 物理ベースレンズフレア — 光束追跡（Ray Bundle Tracing）の実装
///
/// Pass 1 [CS]: レンズ系をレイトレースし GhostVertex バッファを生成
/// Pass 2 [VS/PS]: ゴーストクアッドを加算ブレンドでフレアRTに描画
/// Pass 3 [Fullscreen PS]: HDRシーン + フレアRT を合成
#include "pch_graphics.h"
#include "Graphics/PostEffect/LensFlare.h"
#include "Math/MathConvert.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Graphics/Pipeline/ShaderLibrary.h"
#include "Core/Logger.h"

namespace gx
{

// ============================================================================
// レンズ系 / ゴースト列挙 (CPU)
// ============================================================================

void LensFlare::InitializeLensSystem()
{
    // 7インターフェースのレンズ系（センサーからシーン方向に定義）
    // idx0 が最もセンサーに近く、idx6 が最もシーン側
    m_lensInterfaces.clear();

    // センサー座標系: Z=0 がセンサー面、Z>0 がシーン方向
    // centerZ はレンズ中心のZ座標（mm）
    auto addIface = [&](float z, float r, float aR, float n1, float n2, int apt) {
        LensInterfaceData d;
        d.centerZ        = z;
        d.radius         = r;
        d.apertureRadius = aR;
        d.n1             = n1;
        d.n2             = n2;
        d.coatingN       = 0.0f;
        d.coatingD       = 0.0f;
        d.isAperture     = apt;
        m_lensInterfaces.push_back(d);
    };

    //  idx  Z(mm)  radius  aperR  n1→n2    isAperture  Description
    addIface( 5.0f, -120.0f, 25.0f, 1.52f, 1.0f,  0);  // 0: 後玉 裏面
    addIface(12.0f,   45.0f, 24.0f, 1.0f,  1.67f, 0);   // 1: 後群前面
    addIface(20.0f,  -60.0f, 22.0f, 1.67f, 1.0f,  0);   // 2: 中群後面
    addIface(28.0f,    0.0f, 18.0f, 1.0f,  1.0f,  1);    // 3: 絞り (Aperture Stop)
    addIface(36.0f,   50.0f, 22.0f, 1.0f,  1.67f, 0);   // 4: 中群前面
    addIface(44.0f, -200.0f, 24.0f, 1.52f, 1.0f,  0);   // 5: 前群後面
    addIface(52.0f,   80.0f, 25.0f, 1.0f,  1.52f, 0);   // 6: 前玉

    m_numInterfaces = static_cast<int>(m_lensInterfaces.size());
}

void LensFlare::EnumerateGhosts()
{
    m_ghostPairsA.clear();
    m_ghostPairsB.clear();

    // 全 (a, b) ペアを列挙: a > b, 両方が aperture でない
    // a = シーン側の反射面（Z大）, b = センサー側の反射面（Z小）
    for (int a = 0; a < m_numInterfaces; ++a)
    {
        if (m_lensInterfaces[a].isAperture) continue;
        for (int b = 0; b < a; ++b)
        {
            if (m_lensInterfaces[b].isAperture) continue;
            m_ghostPairsA.push_back(static_cast<uint32_t>(a));
            m_ghostPairsB.push_back(static_cast<uint32_t>(b));
        }
    }

    m_numGhosts = static_cast<int>(m_ghostPairsA.size());
}

// ============================================================================
// 初期化 / 終了
// ============================================================================

bool LensFlare::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // レンズ系データ初期化 (CPU)
    InitializeLensSystem();
    EnumerateGhosts();

    // CPU-only 初期化 (テスト用)
    if (!device)
    {
        m_initialized = true;
        return true;
    }

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // 定数バッファ (256B align × 2フレーム)
    if (!m_constantBuffer.Initialize(device, 256, 256))
        return false;

    // SRV/UAV ヒープ (shader-visible)
    // [0] LensInterface SRV (t0, CS用)
    // [1] GhostPair SRV (t1, CS用)
    // [2] GhostVertex UAV (u0, CS用)
    // [3] GhostVertex SRV (t0, VS用)
    // [4] Scene SRV (t0, Composite用)
    // [5] Flare SRV (t1, Composite用)
    if (!m_srvUavHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, true))
        return false;

    // GPU バッファ作成
    if (!CreateGPUBuffers(device))
        return false;

    // フレア RT (中間レンダーターゲット)
    if (!m_flareRT.Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT))
        return false;

    // 各パイプライン作成
    if (!CreateComputePipeline(device))
        return false;
    if (!CreateGhostRenderPipeline(device))
        return false;
    if (!CreateCompositePipeline(device))
        return false;

    m_initialized = true;
    GX_LOG_INFO("LensFlare (physics-based) initialized (%ux%u, %d interfaces, %d ghosts)",
                width, height, m_numInterfaces, m_numGhosts);
    return true;
}

void LensFlare::Shutdown()
{
    m_computePSO.Reset();
    m_computeRS.Reset();
    m_ghostPSO.Reset();
    m_ghostRS.Reset();
    m_compositePSO.Reset();
    m_compositeRS.Reset();
    m_initialized = false;
}

// ============================================================================
// GPU バッファ作成
// ============================================================================

bool LensFlare::CreateGPUBuffers(ID3D12Device* device)
{
    // --- LensInterface Upload Buffer ---
    {
        uint64_t size = sizeof(LensInterfaceData) * m_numInterfaces;
        if (!m_lensInterfaceBuffer.CreateUploadBufferEmpty(device, size))
            return false;

        void* p = m_lensInterfaceBuffer.Map();
        if (p)
        {
            memcpy(p, m_lensInterfaces.data(), size);
            m_lensInterfaceBuffer.Unmap();
        }

        // SRV: StructuredBuffer<LensInterface>
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                     = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement        = 0;
        srvDesc.Buffer.NumElements         = m_numInterfaces;
        srvDesc.Buffer.StructureByteStride = sizeof(LensInterfaceData);

        device->CreateShaderResourceView(
            m_lensInterfaceBuffer.GetResource(), &srvDesc,
            m_srvUavHeap.GetCPUHandle(0));
    }

    // --- GhostPair Upload Buffer ---
    {
        // uint2 (8 bytes) per ghost
        struct GhostPair { uint32_t a, b; };
        uint64_t size = sizeof(GhostPair) * m_numGhosts;
        if (!m_ghostPairBuffer.CreateUploadBufferEmpty(device, size))
            return false;

        void* p = m_ghostPairBuffer.Map();
        if (p)
        {
            auto* pairs = reinterpret_cast<GhostPair*>(p);
            for (int i = 0; i < m_numGhosts; ++i)
            {
                pairs[i].a = m_ghostPairsA[i];
                pairs[i].b = m_ghostPairsB[i];
            }
            m_ghostPairBuffer.Unmap();
        }

        // SRV: StructuredBuffer<uint2>
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                     = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement        = 0;
        srvDesc.Buffer.NumElements         = m_numGhosts;
        srvDesc.Buffer.StructureByteStride = sizeof(GhostPair);

        device->CreateShaderResourceView(
            m_ghostPairBuffer.GetResource(), &srvDesc,
            m_srvUavHeap.GetCPUHandle(1));
    }

    // --- GhostVertex Default Buffer (UAV) ---
    {
        // GhostVertex: 2*3 floats (posR/G/B) + 3 floats (intensityR/G/B) + 1 float (valid) = 10 floats = 40 bytes
        static constexpr uint32_t k_GhostVertexStride = 40;
        int gridSize = m_settings.gridSize;
        uint64_t totalVertices = static_cast<uint64_t>(m_numGhosts) * gridSize * gridSize;
        uint64_t size = totalVertices * k_GhostVertexStride;

        if (!m_ghostVertexBuffer.CreateDefaultBuffer(device, size,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
            return false;

        // UAV: RWStructuredBuffer<GhostVertex>
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension              = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format                     = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.FirstElement        = 0;
        uavDesc.Buffer.NumElements         = static_cast<UINT>(totalVertices);
        uavDesc.Buffer.StructureByteStride = k_GhostVertexStride;

        device->CreateUnorderedAccessView(
            m_ghostVertexBuffer.GetResource(), nullptr, &uavDesc,
            m_srvUavHeap.GetCPUHandle(2));

        // SRV: StructuredBuffer<GhostVertex> (VS用)
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                     = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement        = 0;
        srvDesc.Buffer.NumElements         = static_cast<UINT>(totalVertices);
        srvDesc.Buffer.StructureByteStride = k_GhostVertexStride;

        device->CreateShaderResourceView(
            m_ghostVertexBuffer.GetResource(), &srvDesc,
            m_srvUavHeap.GetCPUHandle(3));
    }

    return true;
}

// ============================================================================
// パイプライン作成
// ============================================================================

bool LensFlare::CreateComputePipeline(ID3D12Device* device)
{
    // Compute Root Signature: [0]=CBV(b0), [1]=DescTable(t0..t1 SRV), [2]=DescTable(u0 UAV)
    m_computeRS = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, 0,
                            D3D12_SHADER_VISIBILITY_ALL,
                            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0,
                            D3D12_SHADER_VISIBILITY_ALL,
                            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
        .Build(device);

    if (!m_computeRS)
    {
        GX_LOG_ERROR("LensFlare: compute root signature failed");
        return false;
    }

    auto csBlob = m_shader.CompileFromFile(L"Shaders/LensFlare.hlsl", L"CSTraceGhosts", L"cs_6_0");
    if (!csBlob.valid)
    {
        GX_LOG_ERROR("LensFlare: CS compile failed");
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_computeRS.Get();
    desc.CS = csBlob.GetBytecode();

    HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_computePSO));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("LensFlare: compute PSO failed");
        return false;
    }

    return true;
}

bool LensFlare::CreateGhostRenderPipeline(ID3D12Device* device)
{
    // Ghost Render Root Signature: [0]=CBV(b0), [1]=DescTable(t0 SRV, vertex visibility)
    m_ghostRS = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1,
                            0, D3D12_SHADER_VISIBILITY_VERTEX,
                            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
        .Build(device);

    if (!m_ghostRS)
    {
        GX_LOG_ERROR("LensFlare: ghost root signature failed");
        return false;
    }

    auto vsBlob = m_shader.CompileFromFile(L"Shaders/LensFlare.hlsl", L"VSGhost", L"vs_6_0");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/LensFlare.hlsl", L"PSGhost", L"ps_6_0");

    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("LensFlare: ghost VS/PS compile failed");
        return false;
    }

    // 加算ブレンド PSO (VB不要、SV_VertexID で読み取り)
    PipelineStateBuilder b;
    m_ghostPSO = b.SetRootSignature(m_ghostRS.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .SetAdditiveBlend()
        .Build(device);

    if (!m_ghostPSO)
    {
        GX_LOG_ERROR("LensFlare: ghost PSO failed");
        return false;
    }

    return true;
}

bool LensFlare::CreateCompositePipeline(ID3D12Device* device)
{
    // Composite Root Signature: [0]=CBV(b0), [1]=DescTable(t0..t1 SRV, pixel vis) + s0(linear)
    m_compositeRS = RootSignatureBuilder()
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2, 0,
                            D3D12_SHADER_VISIBILITY_PIXEL,
                            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE)
        .AddStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                          D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
                          D3D12_COMPARISON_FUNC_NEVER)
        .Build(device);

    if (!m_compositeRS)
    {
        GX_LOG_ERROR("LensFlare: composite root signature failed");
        return false;
    }

    auto vsBlob = m_shader.CompileFromFile(L"Shaders/LensFlare.hlsl", L"FullscreenVS", L"vs_6_0");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/LensFlare.hlsl", L"PSComposite",  L"ps_6_0");

    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("LensFlare: composite VS/PS compile failed");
        return false;
    }

    PipelineStateBuilder b;
    m_compositePSO = b.SetRootSignature(m_compositeRS.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .Build(device);

    if (!m_compositePSO)
    {
        GX_LOG_ERROR("LensFlare: composite PSO failed");
        return false;
    }

    return true;
}

// ============================================================================
// 太陽スクリーン座標の更新
// ============================================================================

void LensFlare::UpdateSunScreenPos(const Camera3D& camera)
{
    // VolumetricLight::UpdateSunInfo() と同じパターン
    XMVECTOR sunDir = XMVector3Normalize(XMLoadFloat3(XM(&m_lightDirection)));
    XMVECTOR camPos = XMLoadFloat3(XM(&camera.GetPosition()));

    // 太陽は光源方向の逆向きに十分遠い仮想位置
    XMVECTOR sunWorld = XMVectorSubtract(camPos, XMVectorScale(sunDir, 1000.0f));

    XMMATRIX viewProj = ToXMMATRIX(camera.GetViewProjectionMatrix());
    XMVECTOR sunClip = XMVector3TransformCoord(sunWorld, viewProj);

    Vector3 sunNDC = {0.0f, 0.0f, 0.0f};
    XMStoreFloat3(XM(&sunNDC), sunClip);
    float sunU = sunNDC.x * 0.5f + 0.5f;
    float sunV = -sunNDC.y * 0.5f + 0.5f;

    // ビュー空間Zで前方チェック
    XMMATRIX viewMat = ToXMMATRIX(camera.GetViewMatrix());
    XMVECTOR sunView = XMVector3TransformCoord(sunWorld, viewMat);
    Vector3 sunViewF;
    XMStoreFloat3(XM(&sunViewF), sunView);

    float sunVisible = 1.0f;

    if (sunViewF.z <= 0.0f)
    {
        sunVisible = 0.0f;
    }
    else
    {
        // 画面端に近づくにつれフレアを滑らかにフェードアウト
        float distFromCenter = sqrtf((sunU - 0.5f) * (sunU - 0.5f) + (sunV - 0.5f) * (sunV - 0.5f));
        float fadeT = (distFromCenter - 0.7f) / 1.3f;
        fadeT = (std::max)(0.0f, (std::min)(1.0f, fadeT));
        sunVisible = 1.0f - fadeT;
    }

    m_sunVisible = sunVisible;
    m_sunScreenUV = { sunU, sunV };
}

// ============================================================================
// Execute — 3パス描画
// ============================================================================

void LensFlare::Execute(ID3D12GraphicsCommandList* cmdList,
                        RenderTarget& hdrInput, RenderTarget& outputRT,
                        uint32_t frameIndex, const Camera3D& camera)
{
    if (!m_settings.enabled || !m_initialized || !cmdList)
        return;

    // 太陽スクリーン座標を更新
    UpdateSunScreenPos(camera);

    // 太陽が見えない → シーンをそのまま出力
    if (m_sunVisible <= 0.001f)
    {
        // パススルー: hdrInput → outputRT にコピー
        hdrInput.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        outputRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // Compositeパスで intensity=0 として実行
        auto rtv = outputRT.GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width    = static_cast<float>(m_width);
        vp.Height   = static_cast<float>(m_height);
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT sc = {};
        sc.right  = static_cast<LONG>(m_width);
        sc.bottom = static_cast<LONG>(m_height);
        cmdList->RSSetScissorRects(1, &sc);

        // Scene SRV を [4] に動的更新
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                    = hdrInput.GetFormat();
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels      = 1;
            m_device->CreateShaderResourceView(hdrInput.GetResource(), &srvDesc,
                                               m_srvUavHeap.GetCPUHandle(4));
        }
        // Flare SRV を [5] に (m_flareRT)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                    = m_flareRT.GetFormat();
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels      = 1;
            m_device->CreateShaderResourceView(m_flareRT.GetResource(), &srvDesc,
                                               m_srvUavHeap.GetCPUHandle(5));
        }

        // Composite (intensity=0 でフレア無し)
        struct CompositeCB { float intensity; float pad[3]; } ccb = { 0.0f, {0,0,0} };
        void* p = m_constantBuffer.Map(frameIndex);
        if (p) { memcpy(p, &ccb, sizeof(ccb)); m_constantBuffer.Unmap(frameIndex); }

        cmdList->SetGraphicsRootSignature(m_compositeRS.Get());
        cmdList->SetPipelineState(m_compositePSO.Get());
        ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);
        cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));
        cmdList->SetGraphicsRootDescriptorTable(1, m_srvUavHeap.GetGPUHandle(4));
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
        return;
    }

    int gridSize = m_settings.gridSize;
    uint32_t totalVertices = static_cast<uint32_t>(m_numGhosts) * gridSize * gridSize;

    // ========================================================================
    // Pass 1: Compute Shader — 光束追跡
    // ========================================================================
    {
        // 定数バッファ更新
        struct LensFlareCB
        {
            float lightScreenUV[2];
            float lightBrightness;
            float globalIntensity;
            uint32_t numInterfaces;
            uint32_t gridSize;
            uint32_t numGhosts;
            float sensorSize;
            float apertureBlades;
            float starburstStrength;
            float screenSize[2];
        };

        LensFlareCB cb = {};
        cb.lightScreenUV[0] = m_sunScreenUV.x;
        cb.lightScreenUV[1] = m_sunScreenUV.y;
        cb.lightBrightness  = m_sunVisible * 5.0f;  // 輝度（可視性でスケール）
        cb.globalIntensity  = m_settings.intensity;
        cb.numInterfaces    = static_cast<uint32_t>(m_numInterfaces);
        cb.gridSize         = static_cast<uint32_t>(gridSize);
        cb.numGhosts        = static_cast<uint32_t>(m_numGhosts);
        cb.sensorSize       = 36.0f;  // 35mmフルサイズ相当 (mm)
        cb.apertureBlades   = m_settings.apertureBlades;
        cb.starburstStrength = m_settings.starburstStrength;
        cb.screenSize[0]    = static_cast<float>(m_width);
        cb.screenSize[1]    = static_cast<float>(m_height);

        void* p = m_constantBuffer.Map(frameIndex);
        if (p)
        {
            memcpy(p, &cb, sizeof(cb));
            m_constantBuffer.Unmap(frameIndex);
        }

        // ヒープバインド
        ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        cmdList->SetComputeRootSignature(m_computeRS.Get());
        cmdList->SetPipelineState(m_computePSO.Get());

        cmdList->SetComputeRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));
        cmdList->SetComputeRootDescriptorTable(1, m_srvUavHeap.GetGPUHandle(0)); // t0,t1 SRV
        cmdList->SetComputeRootDescriptorTable(2, m_srvUavHeap.GetGPUHandle(2)); // u0 UAV

        uint32_t dispatchX = (totalVertices + 255) / 256;
        cmdList->Dispatch(dispatchX, 1, 1);

        // UAV バリア
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_ghostVertexBuffer.GetResource();
        cmdList->ResourceBarrier(1, &barrier);
    }

    // ========================================================================
    // Pass 2: Ghost Quad Rendering — 加算ブレンドでフレアRTに描画
    // ========================================================================
    {
        m_flareRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // フレアRTをクリア（黒）
        auto rtv = m_flareRT.GetRTVHandle();
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width    = static_cast<float>(m_width);
        vp.Height   = static_cast<float>(m_height);
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT sc = {};
        sc.right  = static_cast<LONG>(m_width);
        sc.bottom = static_cast<LONG>(m_height);
        cmdList->RSSetScissorRects(1, &sc);

        // ヒープ再バインド（グラフィックスパイプラインに切り替え）
        ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        cmdList->SetGraphicsRootSignature(m_ghostRS.Get());
        cmdList->SetPipelineState(m_ghostPSO.Get());

        // GhostVertex SRV はスロット[3]
        cmdList->SetGraphicsRootDescriptorTable(1, m_srvUavHeap.GetGPUHandle(3));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 各チャンネル (R, G, B) を別インスタンスバッチで描画
        uint32_t quadsPerGhost = (gridSize - 1) * (gridSize - 1);
        uint32_t verticesPerGhost = quadsPerGhost * 6; // 2 triangles per quad

        for (uint32_t ch = 0; ch < 3; ++ch)
        {
            // Ghost Render CB を更新
            struct GhostRenderCB
            {
                uint32_t gridSizeRender;
                uint32_t numGhostsRender;
                uint32_t channelIndex;
                float    globalIntensity;
                float    starburstStrength;
                float    apertureBlades;
                float    screenSize[2];
                float    lightScreenUV[2];
                float    padding[2];
            };

            GhostRenderCB gcb = {};
            gcb.gridSizeRender    = static_cast<uint32_t>(gridSize);
            gcb.numGhostsRender   = static_cast<uint32_t>(m_numGhosts);
            gcb.channelIndex      = ch;
            gcb.globalIntensity   = m_settings.intensity * m_sunVisible;
            gcb.starburstStrength = m_settings.starburstStrength;
            gcb.apertureBlades    = m_settings.apertureBlades;
            gcb.screenSize[0]     = static_cast<float>(m_width);
            gcb.screenSize[1]     = static_cast<float>(m_height);
            gcb.lightScreenUV[0]  = m_sunScreenUV.x;
            gcb.lightScreenUV[1]  = m_sunScreenUV.y;

            void* p = m_constantBuffer.Map(frameIndex);
            if (p)
            {
                memcpy(p, &gcb, sizeof(gcb));
                m_constantBuffer.Unmap(frameIndex);
            }

            cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));

            // DrawInstanced: verticesPerGhost 頂点 × numGhosts インスタンス
            cmdList->DrawInstanced(verticesPerGhost, m_numGhosts, 0, 0);
        }
    }

    // ========================================================================
    // Pass 3: Composite — シーン + フレアRT 合成
    // ========================================================================
    {
        hdrInput.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_flareRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        outputRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = outputRT.GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

        D3D12_VIEWPORT vp = {};
        vp.Width    = static_cast<float>(m_width);
        vp.Height   = static_cast<float>(m_height);
        vp.MaxDepth = 1.0f;
        cmdList->RSSetViewports(1, &vp);

        D3D12_RECT sc = {};
        sc.right  = static_cast<LONG>(m_width);
        sc.bottom = static_cast<LONG>(m_height);
        cmdList->RSSetScissorRects(1, &sc);

        // Scene SRV → [4], Flare SRV → [5]
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                    = hdrInput.GetFormat();
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels      = 1;
            m_device->CreateShaderResourceView(hdrInput.GetResource(), &srvDesc,
                                               m_srvUavHeap.GetCPUHandle(4));
        }
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format                    = m_flareRT.GetFormat();
            srvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels      = 1;
            m_device->CreateShaderResourceView(m_flareRT.GetResource(), &srvDesc,
                                               m_srvUavHeap.GetCPUHandle(5));
        }

        // Composite CB
        struct CompositeCB { float intensity; float pad[3]; };
        CompositeCB ccb = { m_settings.intensity, {0,0,0} };

        void* p = m_constantBuffer.Map(frameIndex);
        if (p) { memcpy(p, &ccb, sizeof(ccb)); m_constantBuffer.Unmap(frameIndex); }

        ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
        cmdList->SetDescriptorHeaps(1, heaps);

        cmdList->SetGraphicsRootSignature(m_compositeRS.Get());
        cmdList->SetPipelineState(m_compositePSO.Get());
        cmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer.GetGPUVirtualAddress(frameIndex));
        cmdList->SetGraphicsRootDescriptorTable(1, m_srvUavHeap.GetGPUHandle(4));

        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
}

// ============================================================================
// リサイズ
// ============================================================================

void LensFlare::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    // フレアRTを再作成
    if (device && m_initialized)
    {
        m_flareRT.Create(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    }
}

} // namespace gx
