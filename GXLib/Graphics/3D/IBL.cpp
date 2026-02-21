/// @file IBL.cpp
/// @brief IBL（イメージベースドライティング）リソース生成の実装
#include "pch.h"
#include "Graphics/3D/IBL.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

bool IBL::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                     DescriptorHeap& srvHeap)
{
    m_device = device;
    m_cmdQueue = cmdQueue;
    m_srvHeap = &srvHeap;

    // シェーダーコンパイラ
    if (!m_shaderCompiler.Initialize())
    {
        GX_LOG_ERROR("IBL: Failed to initialize shader compiler");
        return false;
    }

    // コマンドアロケータ・リスト・フェンス（IBL生成専用、非同期不要）
    HRESULT hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator));
    if (FAILED(hr)) return false;

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_cmdList));
    if (FAILED(hr)) return false;
    m_cmdList->Close();

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr)) return false;
    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    // SRVスロット割り当て（irradiance, prefiltered, brdfLUT の3スロット）
    m_irradianceSRVSlot = srvHeap.AllocateIndex();
    m_prefilteredSRVSlot = srvHeap.AllocateIndex();
    m_brdfLUTSRVSlot = srvHeap.AllocateIndex();

    // 環境キューブマップ用SRVスロット（生成パイプラインの入力用）
    m_envCubemapSRVSlot = srvHeap.AllocateIndex();

    // BRDF LUTを生成（一度だけ、環境マップに依存しない）
    if (!GenerateBRDFLUT(device, cmdQueue))
    {
        GX_LOG_ERROR("IBL: Failed to generate BRDF LUT");
        return false;
    }

    GX_LOG_INFO("IBL: Initialized (irradiance=%u, prefiltered=%u, brdfLUT=%u)",
                m_irradianceSRVSlot, m_prefilteredSRVSlot, m_brdfLUTSRVSlot);
    return true;
}

void IBL::UpdateFromSkybox(const XMFLOAT3& topColor, const XMFLOAT3& bottomColor,
                           const XMFLOAT3& sunDirection, float sunIntensity)
{
    m_topColor = topColor;
    m_bottomColor = bottomColor;
    m_sunDirection = sunDirection;
    m_sunIntensity = sunIntensity;

    // 環境キューブマップを生成してからIBLテクスチャを更新する
    if (!GenerateEnvironmentCubemap(m_device, m_cmdQueue))
    {
        GX_LOG_ERROR("IBL: Failed to generate environment cubemap");
        return;
    }

    if (!GenerateIrradianceMap(m_device, m_cmdQueue))
    {
        GX_LOG_ERROR("IBL: Failed to generate irradiance map");
        return;
    }

    if (!GeneratePrefilteredMap(m_device, m_cmdQueue))
    {
        GX_LOG_ERROR("IBL: Failed to generate prefiltered map");
        return;
    }

    m_ready = true;
    GX_LOG_INFO("IBL: Updated from Skybox parameters");
}

D3D12_GPU_DESCRIPTOR_HANDLE IBL::GetIrradianceSRV() const
{
    return m_srvHeap->GetGPUHandle(m_irradianceSRVSlot);
}

D3D12_GPU_DESCRIPTOR_HANDLE IBL::GetPrefilteredSRV() const
{
    return m_srvHeap->GetGPUHandle(m_prefilteredSRVSlot);
}

D3D12_GPU_DESCRIPTOR_HANDLE IBL::GetBRDFLUTSRV() const
{
    return m_srvHeap->GetGPUHandle(m_brdfLUTSRVSlot);
}

void IBL::Shutdown()
{
    if (m_fenceEvent)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
    m_envCubemap.Reset();
    m_irradianceMap.Reset();
    m_prefilteredMap.Reset();
    m_brdfLUT.Reset();
    m_ready = false;
}

void IBL::FlushGPU()
{
    m_fenceValue++;
    m_cmdQueue->Signal(m_fence.Get(), m_fenceValue);
    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

// ============================================================================
// BRDF LUT 生成
// ============================================================================

bool IBL::GenerateBRDFLUT(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // シェーダーコンパイル
    auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/BRDF_LUT.hlsl", L"FullscreenVS", L"vs_6_0");
    auto psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/BRDF_LUT.hlsl", L"PSMain", L"ps_6_0");
    if (!vsBlob.valid || !psBlob.valid)
    {
        GX_LOG_ERROR("IBL: Failed to compile BRDF_LUT shader");
        return false;
    }

    // ルートシグネチャ（パラメータなし、描画のみ）
    RootSignatureBuilder rsBuilder;
    auto rootSig = rsBuilder
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .Build(device);
    if (!rootSig) return false;

    // PSO（入力なし、R16G16_FLOAT出力）
    PipelineStateBuilder psoBuilder;
    auto pso = psoBuilder
        .SetRootSignature(rootSig.Get())
        .SetVertexShader(vsBlob.GetBytecode())
        .SetPixelShader(psBlob.GetBytecode())
        .SetRenderTargetFormat(DXGI_FORMAT_R16G16_FLOAT, 0)
        .SetDepthEnable(false)
        .SetCullMode(D3D12_CULL_MODE_NONE)
        .Build(device);
    if (!pso) return false;

    // BRDF LUTリソース作成（RT + SRV）
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = k_BRDFLUTSize;
    texDesc.Height = k_BRDFLUTSize;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearVal = {};
    clearVal.Format = DXGI_FORMAT_R16G16_FLOAT;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal,
        IID_PPV_ARGS(&m_brdfLUT));
    if (FAILED(hr)) return false;

    // RTV用ディスクリプタヒープ（一時的）
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(m_brdfLUT.Get(), nullptr, rtvHandle);

    // コマンド記録
    m_cmdAllocator->Reset();
    m_cmdList->Reset(m_cmdAllocator.Get(), pso.Get());

    D3D12_VIEWPORT viewport = { 0, 0, (float)k_BRDFLUTSize, (float)k_BRDFLUTSize, 0, 1 };
    D3D12_RECT scissor = { 0, 0, (LONG)k_BRDFLUTSize, (LONG)k_BRDFLUTSize };
    m_cmdList->RSSetViewports(1, &viewport);
    m_cmdList->RSSetScissorRects(1, &scissor);
    m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    m_cmdList->SetGraphicsRootSignature(rootSig.Get());
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_cmdList->DrawInstanced(3, 1, 0, 0);

    // RT → SRV状態遷移
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_brdfLUT.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);

    m_cmdList->Close();

    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);
    FlushGPU();

    // SRV作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(m_brdfLUT.Get(), &srvDesc,
        m_srvHeap->GetCPUHandle(m_brdfLUTSRVSlot));

    return true;
}

// ============================================================================
// 環境キューブマップ キャプチャ
// ============================================================================

bool IBL::GenerateEnvironmentCubemap(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // シェーダーコンパイル（初回のみ）
    if (!m_envCapturePSO)
    {
        auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/IBLEnvCapture.hlsl", L"FullscreenVS", L"vs_6_0");
        auto psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/IBLEnvCapture.hlsl", L"PSMain", L"ps_6_0");
        if (!vsBlob.valid || !psBlob.valid) return false;

        // ルートシグネチャ: [0] CBV b0 (EnvConstants)
        RootSignatureBuilder rsBuilder;
        m_genRootSig = rsBuilder
            .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
            .AddCBV(0)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0,
                                D3D12_SHADER_VISIBILITY_PIXEL)
            .AddStaticSampler(0)
            .Build(device);
        if (!m_genRootSig) return false;

        PipelineStateBuilder psoBuilder;
        m_envCapturePSO = psoBuilder
            .SetRootSignature(m_genRootSig.Get())
            .SetVertexShader(vsBlob.GetBytecode())
            .SetPixelShader(psBlob.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_envCapturePSO) return false;
    }

    // キューブマップリソース作成
    D3D12_RESOURCE_DESC cubeDesc = {};
    cubeDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    cubeDesc.Width = k_EnvMapSize;
    cubeDesc.Height = k_EnvMapSize;
    cubeDesc.DepthOrArraySize = 6;
    cubeDesc.MipLevels = 1;
    cubeDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    cubeDesc.SampleDesc.Count = 1;
    cubeDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearVal = {};
    clearVal.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

    m_envCubemap.Reset();
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &cubeDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal,
        IID_PPV_ARGS(&m_envCubemap));
    if (FAILED(hr)) return false;

    // RTV ヒープ（6面分）
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 6;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    uint32_t rtvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (uint32_t face = 0; face < 6; ++face)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.FirstArraySlice = face;
        rtvDesc.Texture2DArray.ArraySize = 1;
        rtvDesc.Texture2DArray.MipSlice = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += face * rtvIncSize;
        device->CreateRenderTargetView(m_envCubemap.Get(), &rtvDesc, rtvHandle);
    }

    // 定数バッファ (アップロードヒープ)
    struct EnvConstants
    {
        uint32_t faceIndex;
        float sunIntensity;
        float _pad[2];
        XMFLOAT3 topColor;
        float _pad2;
        XMFLOAT3 bottomColor;
        float _pad3;
        XMFLOAT3 sunDirection;
        float _pad4;
    };
    static_assert(sizeof(EnvConstants) == 64, "EnvConstants size mismatch");

    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = 256; // 256Bアライン
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.Format = DXGI_FORMAT_UNKNOWN;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES uploadProps = {};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    ComPtr<ID3D12Resource> cbResource;
    device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbResource));

    // 6面レンダリング
    m_cmdAllocator->Reset();
    m_cmdList->Reset(m_cmdAllocator.Get(), m_envCapturePSO.Get());

    m_cmdList->SetGraphicsRootSignature(m_genRootSig.Get());
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VIEWPORT viewport = { 0, 0, (float)k_EnvMapSize, (float)k_EnvMapSize, 0, 1 };
    D3D12_RECT scissor = { 0, 0, (LONG)k_EnvMapSize, (LONG)k_EnvMapSize };
    m_cmdList->RSSetViewports(1, &viewport);
    m_cmdList->RSSetScissorRects(1, &scissor);

    for (uint32_t face = 0; face < 6; ++face)
    {
        // CB更新
        EnvConstants* cbData = nullptr;
        cbResource->Map(0, nullptr, reinterpret_cast<void**>(&cbData));
        if (cbData)
        {
            cbData->faceIndex = face;
            cbData->sunIntensity = m_sunIntensity;
            cbData->topColor = m_topColor;
            cbData->bottomColor = m_bottomColor;
            cbData->sunDirection = m_sunDirection;
            cbResource->Unmap(0, nullptr);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += face * rtvIncSize;
        m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        m_cmdList->SetGraphicsRootConstantBufferView(0, cbResource->GetGPUVirtualAddress());
        m_cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // RT → SRV
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_envCubemap.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);

    m_cmdList->Close();

    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);
    FlushGPU();

    // SRV作成（キューブマップとして）
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MipLevels = 1;
    device->CreateShaderResourceView(m_envCubemap.Get(), &srvDesc,
        m_srvHeap->GetCPUHandle(m_envCubemapSRVSlot));

    return true;
}

// ============================================================================
// 拡散照射マップ生成
// ============================================================================

bool IBL::GenerateIrradianceMap(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // PSO（初回のみコンパイル）
    if (!m_irradiancePSO)
    {
        auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/IBLIrradiance.hlsl", L"FullscreenVS", L"vs_6_0");
        auto psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/IBLIrradiance.hlsl", L"PSMain", L"ps_6_0");
        if (!vsBlob.valid || !psBlob.valid) return false;

        // genRootSig は環境キューブマップ生成時に作成済み（[0]=CBV b0, [1]=SRV t0, s0サンプラ）
        PipelineStateBuilder psoBuilder;
        m_irradiancePSO = psoBuilder
            .SetRootSignature(m_genRootSig.Get())
            .SetVertexShader(vsBlob.GetBytecode())
            .SetPixelShader(psBlob.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_irradiancePSO) return false;
    }

    // 照射マップリソース作成
    D3D12_RESOURCE_DESC cubeDesc = {};
    cubeDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    cubeDesc.Width = k_IrradianceSize;
    cubeDesc.Height = k_IrradianceSize;
    cubeDesc.DepthOrArraySize = 6;
    cubeDesc.MipLevels = 1;
    cubeDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    cubeDesc.SampleDesc.Count = 1;
    cubeDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearVal = {};
    clearVal.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

    m_irradianceMap.Reset();
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &cubeDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal,
        IID_PPV_ARGS(&m_irradianceMap));
    if (FAILED(hr)) return false;

    // RTV + CB
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 6;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    uint32_t rtvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (uint32_t face = 0; face < 6; ++face)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.FirstArraySlice = face;
        rtvDesc.Texture2DArray.ArraySize = 1;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += face * rtvIncSize;
        device->CreateRenderTargetView(m_irradianceMap.Get(), &rtvDesc, rtvHandle);
    }

    // CB用
    struct GenConstants
    {
        uint32_t faceIndex;
        float roughness;
        float _pad[2];
    };

    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = 256;
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.Format = DXGI_FORMAT_UNKNOWN;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES uploadProps = {};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    ComPtr<ID3D12Resource> cbResource;
    device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbResource));

    // SRVヒープをバインドして環境キューブマップを入力する
    m_cmdAllocator->Reset();
    m_cmdList->Reset(m_cmdAllocator.Get(), m_irradiancePSO.Get());

    m_cmdList->SetGraphicsRootSignature(m_genRootSig.Get());
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap->GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VIEWPORT viewport = { 0, 0, (float)k_IrradianceSize, (float)k_IrradianceSize, 0, 1 };
    D3D12_RECT scissor = { 0, 0, (LONG)k_IrradianceSize, (LONG)k_IrradianceSize };
    m_cmdList->RSSetViewports(1, &viewport);
    m_cmdList->RSSetScissorRects(1, &scissor);

    // 環境キューブマップSRVバインド
    m_cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGPUHandle(m_envCubemapSRVSlot));

    for (uint32_t face = 0; face < 6; ++face)
    {
        GenConstants* cbData = nullptr;
        cbResource->Map(0, nullptr, reinterpret_cast<void**>(&cbData));
        if (cbData)
        {
            cbData->faceIndex = face;
            cbData->roughness = 0.0f;
            cbResource->Unmap(0, nullptr);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += face * rtvIncSize;
        m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        m_cmdList->SetGraphicsRootConstantBufferView(0, cbResource->GetGPUVirtualAddress());
        m_cmdList->DrawInstanced(3, 1, 0, 0);
    }

    // RT → SRV
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_irradianceMap.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);

    m_cmdList->Close();

    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);
    FlushGPU();

    // SRV作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MipLevels = 1;
    device->CreateShaderResourceView(m_irradianceMap.Get(), &srvDesc,
        m_srvHeap->GetCPUHandle(m_irradianceSRVSlot));

    return true;
}

// ============================================================================
// 鏡面プリフィルタマップ生成
// ============================================================================

bool IBL::GeneratePrefilteredMap(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // PSO（初回のみ）
    if (!m_prefilteredPSO)
    {
        auto vsBlob = m_shaderCompiler.CompileFromFile(L"Shaders/IBLPrefilter.hlsl", L"FullscreenVS", L"vs_6_0");
        auto psBlob = m_shaderCompiler.CompileFromFile(L"Shaders/IBLPrefilter.hlsl", L"PSMain", L"ps_6_0");
        if (!vsBlob.valid || !psBlob.valid) return false;

        PipelineStateBuilder psoBuilder;
        m_prefilteredPSO = psoBuilder
            .SetRootSignature(m_genRootSig.Get())
            .SetVertexShader(vsBlob.GetBytecode())
            .SetPixelShader(psBlob.GetBytecode())
            .SetRenderTargetFormat(DXGI_FORMAT_R16G16B16A16_FLOAT, 0)
            .SetDepthEnable(false)
            .SetCullMode(D3D12_CULL_MODE_NONE)
            .Build(device);
        if (!m_prefilteredPSO) return false;
    }

    // プリフィルタキューブマップ（ミップ付き）
    D3D12_RESOURCE_DESC cubeDesc = {};
    cubeDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    cubeDesc.Width = k_PrefilteredSize;
    cubeDesc.Height = k_PrefilteredSize;
    cubeDesc.DepthOrArraySize = 6;
    cubeDesc.MipLevels = k_PrefilteredMipLevels;
    cubeDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    cubeDesc.SampleDesc.Count = 1;
    cubeDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearVal = {};
    clearVal.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

    m_prefilteredMap.Reset();
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &cubeDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearVal,
        IID_PPV_ARGS(&m_prefilteredMap));
    if (FAILED(hr)) return false;

    // RTV（6面×5ミップ=30個）
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 6 * k_PrefilteredMipLevels;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));

    uint32_t rtvIncSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (uint32_t mip = 0; mip < k_PrefilteredMipLevels; ++mip)
    {
        for (uint32_t face = 0; face < 6; ++face)
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.FirstArraySlice = face;
            rtvDesc.Texture2DArray.ArraySize = 1;
            rtvDesc.Texture2DArray.MipSlice = mip;

            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += (mip * 6 + face) * rtvIncSize;
            device->CreateRenderTargetView(m_prefilteredMap.Get(), &rtvDesc, rtvHandle);
        }
    }

    // CB用
    struct GenConstants
    {
        uint32_t faceIndex;
        float roughness;
        float _pad[2];
    };

    D3D12_RESOURCE_DESC cbDesc = {};
    cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbDesc.Width = 256;
    cbDesc.Height = 1;
    cbDesc.DepthOrArraySize = 1;
    cbDesc.MipLevels = 1;
    cbDesc.Format = DXGI_FORMAT_UNKNOWN;
    cbDesc.SampleDesc.Count = 1;
    cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES uploadProps = {};
    uploadProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    ComPtr<ID3D12Resource> cbResource;
    device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbResource));

    // 描画
    m_cmdAllocator->Reset();
    m_cmdList->Reset(m_cmdAllocator.Get(), m_prefilteredPSO.Get());

    m_cmdList->SetGraphicsRootSignature(m_genRootSig.Get());
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap->GetHeap() };
    m_cmdList->SetDescriptorHeaps(1, heaps);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_cmdList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGPUHandle(m_envCubemapSRVSlot));

    for (uint32_t mip = 0; mip < k_PrefilteredMipLevels; ++mip)
    {
        uint32_t mipSize = k_PrefilteredSize >> mip;
        if (mipSize < 1) mipSize = 1;

        float roughness = static_cast<float>(mip) / static_cast<float>(k_PrefilteredMipLevels - 1);

        D3D12_VIEWPORT viewport = { 0, 0, (float)mipSize, (float)mipSize, 0, 1 };
        D3D12_RECT scissor = { 0, 0, (LONG)mipSize, (LONG)mipSize };
        m_cmdList->RSSetViewports(1, &viewport);
        m_cmdList->RSSetScissorRects(1, &scissor);

        for (uint32_t face = 0; face < 6; ++face)
        {
            GenConstants* cbData = nullptr;
            cbResource->Map(0, nullptr, reinterpret_cast<void**>(&cbData));
            if (cbData)
            {
                cbData->faceIndex = face;
                cbData->roughness = roughness;
                cbResource->Unmap(0, nullptr);
            }

            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
            rtvHandle.ptr += (mip * 6 + face) * rtvIncSize;
            m_cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
            m_cmdList->SetGraphicsRootConstantBufferView(0, cbResource->GetGPUVirtualAddress());
            m_cmdList->DrawInstanced(3, 1, 0, 0);
        }
    }

    // RT → SRV
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_prefilteredMap.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);

    m_cmdList->Close();

    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);
    FlushGPU();

    // SRV作成（全ミップレベル）
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MipLevels = k_PrefilteredMipLevels;
    device->CreateShaderResourceView(m_prefilteredMap.Get(), &srvDesc,
        m_srvHeap->GetCPUHandle(m_prefilteredSRVSlot));

    return true;
}

} // namespace GX
