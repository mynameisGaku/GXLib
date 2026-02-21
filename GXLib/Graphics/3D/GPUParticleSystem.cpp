/// @file GPUParticleSystem.cpp
/// @brief Compute Shader駆動GPUパーティクルシステムの実装
///
/// GPU上でパーティクルの生成・物理更新・描画を行う。
/// 3つのCompute Shader（Init/Emit/Update）と1つのGraphics PSO（Draw）を使用。
///
/// バッファ構成:
///   - m_particleBuffer: RWStructuredBuffer<GPUParticle> (DEFAULT heap, UAV)
///     全maxParticlesスロットを常に保持するリングバッファ
///   - m_counterBuffer: uint[1] (DEFAULT heap, UAV)
///     リングバッファの現在書き込み位置
///   - m_counterUpload: カウンターリセット用UPLOAD buffer
///   - m_counterReadback: CPU読み取り用READBACK buffer（未使用、将来拡張用）
///
/// ディスパッチ戦略:
///   - Init: ceil(maxParticles/256) — 全スロットをlife=0に
///   - Emit: ceil(emitCount/256) — リングバッファ位置から新規パーティクル書き込み
///   - Update: ceil(maxParticles/256) — 全スロットを走査、life>0のみ物理更新
///   - Draw: maxParticles*6頂点 — 全スロット走査、life<=0は退化三角形
#include "pch.h"
#include "Graphics/3D/GPUParticleSystem.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Pipeline/PipelineState.h"
#include "Core/Logger.h"

namespace GX
{

// ============================================================================
// 初期化
// ============================================================================

bool GPUParticleSystem::Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                                   uint32_t maxParticles)
{
    m_maxParticles = maxParticles;

    // シェーダーコンパイラ初期化
    if (!m_shader.Initialize())
    {
        GX_LOG_ERROR("GPUParticleSystem: shader compiler init failed");
        return false;
    }

    // バッファ作成
    if (!CreateBuffers(device))
        return false;

    // PSO作成
    if (!CreatePSOs(device))
        return false;

    // ディスクリプタ作成
    CreateDescriptors(device);

    // DynamicBuffer（定数バッファ）の初期化
    // 256バイトアラインメント（D3D12定数バッファ要件）
    uint32_t updateCBSize = (sizeof(UpdateCB) + 255) & ~255;
    uint32_t emitCBSize = (sizeof(EmitCB) + 255) & ~255;
    uint32_t drawCBSize = (sizeof(DrawCB) + 255) & ~255;

    if (!m_updateCBBuffer.Initialize(device, updateCBSize, updateCBSize))
    {
        GX_LOG_ERROR("GPUParticleSystem: UpdateCB init failed");
        return false;
    }
    if (!m_emitCBBuffer.Initialize(device, emitCBSize, emitCBSize))
    {
        GX_LOG_ERROR("GPUParticleSystem: EmitCB init failed");
        return false;
    }
    if (!m_drawCBBuffer.Initialize(device, drawCBSize, drawCBSize))
    {
        GX_LOG_ERROR("GPUParticleSystem: DrawCB init failed");
        return false;
    }

    // Init CSでパーティクルプールを初期化
    InitializeParticlePool(device, cmdQueue);

    m_initialized = true;
    GX_LOG_INFO("GPUParticleSystem initialized (max: %u particles)", m_maxParticles);
    return true;
}

// ============================================================================
// バッファ作成
// ============================================================================

bool GPUParticleSystem::CreateBuffers(ID3D12Device* device)
{
    // パーティクルプール（DEFAULT heap, UAV）
    uint64_t particleSize = static_cast<uint64_t>(m_maxParticles) * sizeof(GPUParticle);
    if (!m_particleBuffer.CreateDefaultBuffer(device, particleSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
    {
        GX_LOG_ERROR("GPUParticleSystem: particle buffer creation failed");
        return false;
    }

    // カウンターバッファ（DEFAULT heap, UAV）— 1 uint
    if (!m_counterBuffer.CreateDefaultBuffer(device, sizeof(uint32_t),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
    {
        GX_LOG_ERROR("GPUParticleSystem: counter buffer creation failed");
        return false;
    }

    // カウンターリセット用UPLOAD buffer
    if (!m_counterUpload.CreateUploadBufferEmpty(device, sizeof(uint32_t)))
    {
        GX_LOG_ERROR("GPUParticleSystem: counter upload buffer creation failed");
        return false;
    }

    // READBACK buffer（将来拡張用: 生存カウント読み取り）
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_READBACK;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(uint32_t);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ComPtr<ID3D12Resource> readbackResource;
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&readbackResource)
        );
        if (FAILED(hr))
        {
            GX_LOG_ERROR("GPUParticleSystem: readback buffer creation failed (0x%08X)", hr);
            return false;
        }
        // Note: m_counterReadback is a Buffer but we won't use its helpers for readback.
        // The readback resource is stored separately if needed in the future.
    }

    return true;
}

// ============================================================================
// PSO作成
// ============================================================================

bool GPUParticleSystem::CreatePSOs(ID3D12Device* device)
{
    // --- Compute Root Signature ---
    // [0] CBV b0 = UpdateCB or EmitCB
    // [1] DescriptorTable: UAV u0..u1 = particles + counter
    m_computeRS = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2)
        .Build(device);

    if (!m_computeRS)
    {
        GX_LOG_ERROR("GPUParticleSystem: compute root signature failed");
        return false;
    }

    // --- Compute PSOs ---
    // Init CS
    ShaderBlob initCS = m_shader.CompileFromFile(
        L"Shaders/GPUParticleInit.hlsl", L"CSMain", L"cs_6_0");
    if (!initCS.valid)
    {
        GX_LOG_ERROR("GPUParticleSystem: Init CS compile failed: %s",
                     m_shader.GetLastError().c_str());
        return false;
    }

    // Emit CS
    ShaderBlob emitCS = m_shader.CompileFromFile(
        L"Shaders/GPUParticleEmit.hlsl", L"CSMain", L"cs_6_0");
    if (!emitCS.valid)
    {
        GX_LOG_ERROR("GPUParticleSystem: Emit CS compile failed: %s",
                     m_shader.GetLastError().c_str());
        return false;
    }

    // Update CS
    ShaderBlob updateCS = m_shader.CompileFromFile(
        L"Shaders/GPUParticleUpdate.hlsl", L"CSMain", L"cs_6_0");
    if (!updateCS.valid)
    {
        GX_LOG_ERROR("GPUParticleSystem: Update CS compile failed: %s",
                     m_shader.GetLastError().c_str());
        return false;
    }

    // Compute PSOはD3D12_COMPUTE_PIPELINE_STATE_DESCで直接作成
    auto createComputePSO = [&](const ShaderBlob& cs, ComPtr<ID3D12PipelineState>& outPSO)
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_computeRS.Get();
        desc.CS = cs.GetBytecode();

        HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&outPSO));
        return SUCCEEDED(hr);
    };

    if (!createComputePSO(initCS, m_initPSO))
    {
        GX_LOG_ERROR("GPUParticleSystem: Init PSO creation failed");
        return false;
    }
    if (!createComputePSO(emitCS, m_emitPSO))
    {
        GX_LOG_ERROR("GPUParticleSystem: Emit PSO creation failed");
        return false;
    }
    if (!createComputePSO(updateCS, m_updatePSO))
    {
        GX_LOG_ERROR("GPUParticleSystem: Update PSO creation failed");
        return false;
    }

    // --- Draw Root Signature ---
    // [0] CBV b0 = DrawCB（カメラ行列）
    // [1] DescriptorTable: SRV t0 = StructuredBuffer<GPUParticle>（読み取り専用）
    m_drawRS = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1)
        .Build(device);

    if (!m_drawRS)
    {
        GX_LOG_ERROR("GPUParticleSystem: draw root signature failed");
        return false;
    }

    // --- Draw PSO ---
    ShaderBlob drawVS = m_shader.CompileFromFile(
        L"Shaders/GPUParticle.hlsl", L"VSMain", L"vs_6_0");
    ShaderBlob drawPS = m_shader.CompileFromFile(
        L"Shaders/GPUParticle.hlsl", L"PSMain", L"ps_6_0");
    if (!drawVS.valid || !drawPS.valid)
    {
        GX_LOG_ERROR("GPUParticleSystem: Draw shader compile failed: %s",
                     m_shader.GetLastError().c_str());
        return false;
    }

    // Graphics PSOをraw struct で作成（ParticleSystem3Dと同パターン）
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_drawRS.Get();
    psoDesc.VS = drawVS.GetBytecode();
    psoDesc.PS = drawPS.GetBytecode();
    psoDesc.InputLayout = { nullptr, 0 };  // VBなし（SV_VertexIDベース）

    // アルファブレンド（加算寄り: SrcAlpha + One で光の粒子感を出す）
    auto& rt = psoDesc.BlendState.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D12_BLEND_ONE;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // 深度テストあり、書き込みなし（パーティクルは半透明）
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;  // HDRパイプライン
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_drawPSO));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUParticleSystem: Draw PSO creation failed (0x%08X)", hr);
        return false;
    }

    return true;
}

// ============================================================================
// ディスクリプタ作成
// ============================================================================

void GPUParticleSystem::CreateDescriptors(ID3D12Device* device)
{
    // Shader-visible SRV/UAV heap（Compute + Draw 両方で使う）
    // スロット:
    //   0: particles UAV (u0)
    //   1: counter UAV (u1)
    //   2: particles SRV (t0, 描画用)
    m_srvUavHeap.Initialize(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        8, true);

    m_particleUAVSlot = m_srvUavHeap.AllocateIndex();
    m_counterUAVSlot = m_srvUavHeap.AllocateIndex();
    m_particleSRVSlot = m_srvUavHeap.AllocateIndex();

    // パーティクルバッファ UAV (RWStructuredBuffer<GPUParticle>)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = m_maxParticles;
        uavDesc.Buffer.StructureByteStride = sizeof(GPUParticle);

        device->CreateUnorderedAccessView(
            m_particleBuffer.GetResource(), nullptr, &uavDesc,
            m_srvUavHeap.GetCPUHandle(m_particleUAVSlot));
    }

    // カウンターバッファ UAV (RWStructuredBuffer<uint>)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = 1;
        uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);

        device->CreateUnorderedAccessView(
            m_counterBuffer.GetResource(), nullptr, &uavDesc,
            m_srvUavHeap.GetCPUHandle(m_counterUAVSlot));
    }

    // パーティクルバッファ SRV (StructuredBuffer<GPUParticle>, 描画用)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_maxParticles;
        srvDesc.Buffer.StructureByteStride = sizeof(GPUParticle);

        device->CreateShaderResourceView(
            m_particleBuffer.GetResource(), &srvDesc,
            m_srvUavHeap.GetCPUHandle(m_particleSRVSlot));
    }
}

// ============================================================================
// パーティクルプール初期化（Init CS）
// ============================================================================

void GPUParticleSystem::InitializeParticlePool(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
    // 一時的なコマンドアロケータとコマンドリストを作成してInit CSをディスパッチ
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> cmdList;

    HRESULT hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&allocator));
    if (FAILED(hr)) return;

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    if (FAILED(hr)) return;

    // Init CSをディスパッチ
    ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(m_computeRS.Get());
    cmdList->SetPipelineState(m_initPSO.Get());

    // CBV: UpdateCBを使い回す（deltaTime=0, maxParticles設定）
    // Init CSはmaxParticlesだけ参照するので、一時的にUploadバッファにマッピング
    UpdateCB initCB = {};
    initCB.maxParticles = m_maxParticles;

    // counter upload にゼロを書いてリセット
    {
        void* mapped = m_counterUpload.Map();
        if (mapped)
        {
            uint32_t zero = 0;
            memcpy(mapped, &zero, sizeof(uint32_t));
            m_counterUpload.Unmap();
        }
    }

    // カウンターバッファを一旦COPY_DESTにしてリセット
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_counterBuffer.GetResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    cmdList->CopyBufferRegion(m_counterBuffer.GetResource(), 0,
                               m_counterUpload.GetResource(), 0, sizeof(uint32_t));

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_counterBuffer.GetResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // Init CSはCBVからmaxParticlesを読む。一時Upload bufferでCBVバインド
    // DynamicBufferはまだ初期化前なので、counterUploadを流用してCBを作る
    // （サイズが足りないので、代わにルートCBVにGPU仮想アドレスは渡せない）
    // → Init CSをシンプルに: maxParticlesをpush constantにする代わりに
    //   UpdateCBと同じレイアウトでUploadバッファを作る
    Buffer initCBUpload;
    initCBUpload.CreateUploadBufferEmpty(device, 256);  // 256B aligned
    {
        void* mapped = initCBUpload.Map();
        if (mapped)
        {
            memcpy(mapped, &initCB, sizeof(UpdateCB));
            initCBUpload.Unmap();
        }
    }

    cmdList->SetComputeRootConstantBufferView(0, initCBUpload.GetGPUVirtualAddress());
    cmdList->SetComputeRootDescriptorTable(1,
        m_srvUavHeap.GetGPUHandle(m_particleUAVSlot));

    uint32_t groupCount = (m_maxParticles + 255) / 256;
    cmdList->Dispatch(groupCount, 1, 1);

    // UAVバリア
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_particleBuffer.GetResource();
        cmdList->ResourceBarrier(1, &barrier);
    }

    cmdList->Close();

    // 実行して完了を待つ
    ID3D12CommandList* lists[] = { cmdList.Get() };
    cmdQueue->ExecuteCommandLists(1, lists);

    // Fence で GPU 完了を待つ
    ComPtr<ID3D12Fence> fence;
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    cmdQueue->Signal(fence.Get(), 1);
    if (fence->GetCompletedValue() < 1)
    {
        fence->SetEventOnCompletion(1, event);
        WaitForSingleObject(event, INFINITE);
    }
    CloseHandle(event);

    m_poolInitialized = true;
}

// ============================================================================
// Emit（バースト放出）
// ============================================================================

void GPUParticleSystem::Emit(uint32_t count)
{
    m_pendingEmitCount += count;
}

// ============================================================================
// Update（Compute Shaderで物理更新）
// ============================================================================

void GPUParticleSystem::Update(ID3D12GraphicsCommandList* cmdList,
                                float deltaTime, uint32_t frameIndex)
{
    if (!m_initialized || !m_poolInitialized) return;

    // Shader-visibleヒープをセット
    ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(m_computeRS.Get());

    // UAVテーブルをバインド（u0=particles, u1=counter）
    cmdList->SetComputeRootDescriptorTable(1,
        m_srvUavHeap.GetGPUHandle(m_particleUAVSlot));

    // --- Emit ---
    if (m_pendingEmitCount > 0)
    {
        uint32_t emitCount = m_pendingEmitCount;
        if (emitCount > m_maxParticles) emitCount = m_maxParticles;

        // Emit CB を書き込む
        EmitCB emitCB = {};
        emitCB.emitCount = emitCount;
        emitCB.emitPosition = m_emitPosition;
        emitCB.velocityMin = m_velocityMin;
        emitCB.velocityMax = m_velocityMax;
        emitCB.lifeMin = m_lifeMin;
        emitCB.lifeMax = m_lifeMax;
        emitCB.sizeStart = m_sizeStart;
        emitCB.sizeEnd = m_sizeEnd;
        emitCB.colorStart = m_colorStart;
        emitCB.colorEnd = m_colorEnd;
        emitCB.randomSeed = m_frameCounter;
        emitCB.emitOffset = m_emitRingIndex;

        void* mapped = m_emitCBBuffer.Map(frameIndex);
        if (mapped)
        {
            memcpy(mapped, &emitCB, sizeof(EmitCB));
            m_emitCBBuffer.Unmap(frameIndex);
        }

        cmdList->SetPipelineState(m_emitPSO.Get());
        cmdList->SetComputeRootConstantBufferView(0,
            m_emitCBBuffer.GetGPUVirtualAddress(frameIndex));

        uint32_t emitGroups = (emitCount + 255) / 256;
        cmdList->Dispatch(emitGroups, 1, 1);

        // リングバッファインデックスを進める
        m_emitRingIndex = (m_emitRingIndex + emitCount) % m_maxParticles;
        m_pendingEmitCount = 0;

        // UAVバリア（Emitの書き込みが完了してからUpdateを実行）
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_particleBuffer.GetResource();
        cmdList->ResourceBarrier(1, &barrier);
    }

    // --- Update ---
    {
        UpdateCB updateCB = {};
        updateCB.deltaTime = deltaTime;
        updateCB.gravity = m_gravity;
        updateCB.drag = m_drag;
        updateCB.maxParticles = m_maxParticles;

        void* mapped = m_updateCBBuffer.Map(frameIndex);
        if (mapped)
        {
            memcpy(mapped, &updateCB, sizeof(UpdateCB));
            m_updateCBBuffer.Unmap(frameIndex);
        }

        cmdList->SetPipelineState(m_updatePSO.Get());
        cmdList->SetComputeRootConstantBufferView(0,
            m_updateCBBuffer.GetGPUVirtualAddress(frameIndex));

        uint32_t updateGroups = (m_maxParticles + 255) / 256;
        cmdList->Dispatch(updateGroups, 1, 1);

        // UAVバリア（Updateの書き込みが完了してからDrawで読み取り）
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = m_particleBuffer.GetResource();
        cmdList->ResourceBarrier(1, &barrier);
    }

    m_frameCounter++;
}

// ============================================================================
// Draw（ビルボード描画）
// ============================================================================

void GPUParticleSystem::Draw(ID3D12GraphicsCommandList* cmdList,
                              const Camera3D& camera, uint32_t frameIndex)
{
    if (!m_initialized || !m_poolInitialized) return;

    // パーティクルバッファをUAV→SRVに遷移
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_particleBuffer.GetResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                                        | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // 定数バッファ更新
    DrawCB drawCB = {};
    XMMATRIX vp = camera.GetViewProjectionMatrix();
    XMStoreFloat4x4(&drawCB.viewProj, XMMatrixTranspose(vp));
    drawCB.cameraRight = camera.GetRight();
    drawCB.cameraUp = camera.GetUp();

    void* mapped = m_drawCBBuffer.Map(frameIndex);
    if (mapped)
    {
        memcpy(mapped, &drawCB, sizeof(DrawCB));
        m_drawCBBuffer.Unmap(frameIndex);
    }

    // 描画コマンド
    ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetGraphicsRootSignature(m_drawRS.Get());
    cmdList->SetGraphicsRootConstantBufferView(0,
        m_drawCBBuffer.GetGPUVirtualAddress(frameIndex));
    cmdList->SetGraphicsRootDescriptorTable(1,
        m_srvUavHeap.GetGPUHandle(m_particleSRVSlot));

    cmdList->SetPipelineState(m_drawPSO.Get());
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetVertexBuffers(0, 0, nullptr);   // VBなし
    cmdList->IASetIndexBuffer(nullptr);            // IBなし

    // 全スロットを描画（life<=0の粒子はVSで退化三角形にして不可視化）
    cmdList->DrawInstanced(m_maxParticles * 6, 1, 0, 0);

    // SRV→UAVに遷移（次フレームのCompute用）
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_particleBuffer.GetResource();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
                                          | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }
}

// ============================================================================
// シャットダウン
// ============================================================================

void GPUParticleSystem::Shutdown()
{
    m_initPSO.Reset();
    m_emitPSO.Reset();
    m_updatePSO.Reset();
    m_computeRS.Reset();
    m_drawPSO.Reset();
    m_drawRS.Reset();
    m_initialized = false;
    m_poolInitialized = false;
}

} // namespace GX
