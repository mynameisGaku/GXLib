/// @file GPUDrivenRenderer.cpp
/// @brief GPUドリブンレンダリングの実装
#include "pch_graphics.h"
#include "Graphics/3D/GPUDrivenRenderer.h"
#include "Core/Logger.h"
#include <cstring>

namespace gx
{

bool GPUDrivenRenderer::Initialize(ID3D12Device* device, ID3D12Device2* device2)
{
    if (!device)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::Initialize: device is null");
        return false;
    }

    m_available = false;
    m_tier = MeshShaderTier::None;
    m_meshletCount = 0;

    // メッシュシェーダの対応レベルを検出
    m_tier = MeshPipeline::DetectTier(device);

    if (m_tier == MeshShaderTier::None)
    {
        GX_LOG_INFO("GPUDrivenRenderer: Mesh Shader not available -- GPU-driven rendering disabled");
        return true; // 非対応でも初期化自体は成功
    }

    if (!device2)
    {
        GX_LOG_WARN("GPUDrivenRenderer: ID3D12Device2 not available -- cannot create stream PSO");
        return true;
    }

    m_available = true;
    m_device2 = device2;
    GX_LOG_INFO("GPUDrivenRenderer initialized -- Mesh Shader GPU-driven rendering available");

    // --- シェーダコンパイル + PSO作成 ---
    if (!m_shader.Initialize())
    {
        GX_LOG_WARN("GPUDrivenRenderer: DXC compiler initialization failed -- PSO not created");
        return true;
    }

    auto asBlob = m_shader.CompileFromFile(L"Shaders/MeshCull.hlsl", L"ASMain", L"as_6_5");
    auto msBlob = m_shader.CompileFromFile(L"Shaders/MeshCull.hlsl", L"MSMain", L"ms_6_5");
    auto psBlob = m_shader.CompileFromFile(L"Shaders/MeshCull.hlsl", L"PSMain", L"ps_6_0");

    if (!asBlob.valid || !msBlob.valid || !psBlob.valid)
    {
        GX_LOG_WARN("GPUDrivenRenderer: Shader compilation failed (shader may not exist yet)");
        return true; // シェーダがなくても初期化は成功
    }

    // ルートシグネチャ: b0 = 32bit定数(20 DWORDs: viewProj 16 + meshletCount 1 + padding 3), t0 = SRVテーブル
    D3D12_ROOT_PARAMETER rootParams[2] = {};

    // param[0]: 32ビット定数 (b0) — 20 DWORD
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[0].Constants.ShaderRegister = 0;
    rootParams[0].Constants.RegisterSpace  = 0;
    rootParams[0].Constants.Num32BitValues = 20;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // param[1]: SRVディスクリプタテーブル (t0) — メッシュレットバッファ
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType          = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors     = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace      = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges   = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
    rsDesc.NumParameters = 2;
    rsDesc.pParameters   = rootParams;
    rsDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                              &sigBlob, &errBlob);
    if (FAILED(hr))
    {
        GX_LOG_WARN("GPUDrivenRenderer: Failed to serialize root signature (0x%08X)", hr);
        return true;
    }

    hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        GX_LOG_WARN("GPUDrivenRenderer: Failed to create root signature (0x%08X)", hr);
        return true;
    }

    // MeshPipelineDescを構築してPSOを作成
    auto asBytecode = asBlob.GetBytecode();
    auto msBytecode = msBlob.GetBytecode();
    auto psBytecode = psBlob.GetBytecode();

    MeshPipelineDesc meshDesc;
    meshDesc.asBlob         = asBytecode.pShaderBytecode;
    meshDesc.asBlobSize     = static_cast<uint32_t>(asBytecode.BytecodeLength);
    meshDesc.msBlob         = msBytecode.pShaderBytecode;
    meshDesc.msBlobSize     = static_cast<uint32_t>(msBytecode.BytecodeLength);
    meshDesc.psBlob         = psBytecode.pShaderBytecode;
    meshDesc.psBlobSize     = static_cast<uint32_t>(psBytecode.BytecodeLength);
    meshDesc.rootSignature  = m_rootSignature.Get();

    m_meshPSO = m_meshPipeline.Build(device2, meshDesc);
    if (!m_meshPSO)
    {
        GX_LOG_WARN("GPUDrivenRenderer: Failed to build mesh shader PSO");
        return true;
    }

    m_psoReady = true;
    GX_LOG_INFO("GPUDrivenRenderer: Mesh shader PSO created successfully");

    return true;
}

void GPUDrivenRenderer::UploadMeshlets(ID3D12Device* device,
                                        const MeshletData* data, uint32_t count)
{
    if (!m_available)
    {
        GX_LOG_WARN("GPUDrivenRenderer::UploadMeshlets: GPU-driven rendering not available");
        return;
    }

    if (!device || !data || count == 0)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::UploadMeshlets: invalid parameters");
        return;
    }

    // メッシュレットデータを構造化バッファとしてアップロード
    uint32_t bufferSize = count * static_cast<uint32_t>(sizeof(MeshletData));

    // UPLOADヒープに頂点バッファとして作成（構造化バッファとして使う）
    // 本来はDEFAULTヒープ + コピーが望ましいが、簡略化のためUPLOADを使用
    bool result = m_meshletBuffer.CreateVertexBuffer(
        device, data, bufferSize, static_cast<uint32_t>(sizeof(MeshletData)));

    if (!result)
    {
        GX_LOG_ERROR("Failed to create meshlet buffer");
        return;
    }

    m_meshletCount = count;
    GX_LOG_INFO("Uploaded %u meshlets (%u bytes)", count, bufferSize);
}

void GPUDrivenRenderer::Render(ID3D12GraphicsCommandList6* cmdList, uint32_t frameIndex,
                                const Matrix4x4& viewProj, uint32_t meshletCount)
{
    if (!m_available || !cmdList)
        return;

    if (meshletCount == 0 || m_meshletCount == 0)
        return;

    // メッシュレット数を実際のアップロード済み数にクランプ
    uint32_t dispatchCount = (meshletCount < m_meshletCount) ? meshletCount : m_meshletCount;

    (void)frameIndex;

    if (!m_psoReady)
    {
        // PSO未構築の場合はスタブログのみ
        GX_LOG_INFO("GPUDrivenRenderer: dispatch %u meshlets (stub -- PSO not ready)", dispatchCount);
        return;
    }

    // 1. PSO + ルートシグネチャをセット
    cmdList->SetPipelineState(m_meshPSO.Get());
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

    // 2. 32bit定数をセット: viewProj行列(16 floats) + meshletCount(1 uint) + padding(3)
    struct
    {
        float viewProj[16];
        uint32_t meshletCount;
        uint32_t pad[3];
    } constants = {};

    // Matrix4x4は行優先4x4 = 16 floats
    std::memcpy(constants.viewProj, &viewProj, sizeof(float) * 16);
    constants.meshletCount = dispatchCount;

    cmdList->SetGraphicsRoot32BitConstants(0, 20, &constants, 0);

    // 3. メッシュレット1つにつき1スレッドグループとしてDispatchMesh
    MeshPipeline::DispatchMesh(cmdList, dispatchCount, 1, 1);
}

void GPUDrivenRenderer::CreateIndirectPipeline(ID3D12Device* device)
{
    if (!device)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::CreateIndirectPipeline: device is null");
        return;
    }

    static constexpr uint32_t k_MaxIndirectObjects = 65536;

    // =========================================================================
    // 1. Command signature for ExecuteIndirect (DrawIndexed)
    // =========================================================================
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
    sigDesc.ByteStride = sizeof(GPUDrawCommand);
    sigDesc.NumArgumentDescs = 1;
    sigDesc.pArgumentDescs = &argDesc;

    HRESULT hr = device->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(&m_commandSignature));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUDrivenRenderer: Failed to create command signature (0x%08X)", hr);
        return;
    }

    // =========================================================================
    // 2. Cull compute root signature: b0 constants, t0 SRV table, u0+u1 UAV table
    // =========================================================================
    D3D12_ROOT_PARAMETER cullParams[3] = {};

    // [0] 32-bit constants (b0): viewProj(16) + objectCount(1) + pad(3) = 20 DWORDs
    cullParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    cullParams[0].Constants.ShaderRegister = 0;
    cullParams[0].Constants.Num32BitValues = 20;
    cullParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // [1] SRV descriptor table (t0): bounds buffer
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    cullParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    cullParams[1].DescriptorTable.NumDescriptorRanges = 1;
    cullParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    cullParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // [2] UAV descriptor table (u0, u1): draw commands + counter
    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 2;
    uavRange.BaseShaderRegister = 0;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    cullParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    cullParams[2].DescriptorTable.NumDescriptorRanges = 1;
    cullParams[2].DescriptorTable.pDescriptorRanges = &uavRange;
    cullParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC cullRsDesc = {};
    cullRsDesc.NumParameters = 3;
    cullRsDesc.pParameters = cullParams;

    ComPtr<ID3DBlob> sigBlob, errBlob;
    hr = D3D12SerializeRootSignature(&cullRsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUDrivenRenderer: Failed to serialize cull root signature (0x%08X)", hr);
        return;
    }

    hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
                                      IID_PPV_ARGS(&m_cullRootSignature));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUDrivenRenderer: Failed to create cull root signature (0x%08X)", hr);
        return;
    }

    // =========================================================================
    // 3. Compile GPUCull.hlsl and create compute PSO
    // =========================================================================
    if (!m_cullShader.Initialize())
    {
        GX_LOG_WARN("GPUDrivenRenderer: DXC compiler init failed for cull shader");
        m_indirectReady = (m_commandSignature != nullptr);
        return;
    }

    ShaderBlob csBlob = m_cullShader.CompileFromFile(L"Shaders/GPUCull.hlsl", L"CSMain", L"cs_6_0");
    if (!csBlob.valid)
    {
        GX_LOG_WARN("GPUDrivenRenderer: Cull shader compilation failed (shader may not exist yet)");
        m_indirectReady = (m_commandSignature != nullptr);
        return;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_cullRootSignature.Get();
    psoDesc.CS = csBlob.GetBytecode();

    hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_cullPSO));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("GPUDrivenRenderer: Failed to create cull compute PSO (0x%08X)", hr);
        m_indirectReady = (m_commandSignature != nullptr);
        return;
    }

    // =========================================================================
    // 4. Create indirect argument buffer (DEFAULT heap, UAV)
    //    Sized for k_MaxIndirectObjects draw commands
    // =========================================================================
    m_maxIndirectObjects = k_MaxIndirectObjects;
    uint64_t argBufferSize = static_cast<uint64_t>(m_maxIndirectObjects) * sizeof(GPUDrawCommand);

    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = argBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
            IID_PPV_ARGS(&m_indirectArgBuffer));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("GPUDrivenRenderer: Failed to create indirect arg buffer (0x%08X)", hr);
            return;
        }
    }

    // =========================================================================
    // 5. Create counter buffer (4 bytes, DEFAULT heap, UAV)
    // =========================================================================
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeof(uint32_t);
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
            IID_PPV_ARGS(&m_counterBuffer));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("GPUDrivenRenderer: Failed to create counter buffer (0x%08X)", hr);
            return;
        }
    }

    // =========================================================================
    // 6. Create counter readback buffer (READBACK heap)
    // =========================================================================
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

        hr = device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_counterReadback));

        if (FAILED(hr))
        {
            GX_LOG_ERROR("GPUDrivenRenderer: Failed to create counter readback buffer (0x%08X)", hr);
            return;
        }
    }

    // =========================================================================
    // 7. Create counter upload buffer (for clearing to zero)
    // =========================================================================
    if (!m_counterUpload.CreateUploadBufferEmpty(device, sizeof(uint32_t)))
    {
        GX_LOG_ERROR("GPUDrivenRenderer: Failed to create counter upload buffer");
        return;
    }
    // Pre-write zero into the upload buffer
    {
        void* mapped = m_counterUpload.Map();
        if (mapped)
        {
            uint32_t zero = 0;
            std::memcpy(mapped, &zero, sizeof(uint32_t));
            m_counterUpload.Unmap();
        }
    }

    // =========================================================================
    // 8. Create shader-visible descriptor heap for cull pass
    //    Layout: [0] = t0 bounds SRV, [1] = u0 draw commands UAV, [2] = u1 counter UAV
    // =========================================================================
    if (!m_cullDescHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3, true))
    {
        GX_LOG_ERROR("GPUDrivenRenderer: Failed to create cull descriptor heap");
        return;
    }

    // [0] Bounds SRV (t0) — StructuredBuffer<MeshletBounds>
    // Note: Descriptor is created here as a placeholder; it will be updated
    // when UploadMeshletBounds provides actual data. For now we fill the slot
    // so the heap layout is correct.
    uint32_t boundsSrvSlot = m_cullDescHeap.AllocateIndex();
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = 1;
        srvDesc.Buffer.StructureByteStride = sizeof(MeshletBounds);

        // Create a dummy SRV pointing to the indirect arg buffer just to fill the slot.
        // The real SRV will be recreated when bounds are uploaded.
        device->CreateShaderResourceView(
            m_indirectArgBuffer.Get(), &srvDesc,
            m_cullDescHeap.GetCPUHandle(boundsSrvSlot));
    }

    // [1] Draw commands UAV (u0) — RWStructuredBuffer<DrawCommand>
    uint32_t drawCmdUavSlot = m_cullDescHeap.AllocateIndex();
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = m_maxIndirectObjects;
        uavDesc.Buffer.StructureByteStride = sizeof(GPUDrawCommand);

        device->CreateUnorderedAccessView(
            m_indirectArgBuffer.Get(), nullptr, &uavDesc,
            m_cullDescHeap.GetCPUHandle(drawCmdUavSlot));
    }

    // [2] Counter UAV (u1) — RWByteAddressBuffer
    uint32_t counterUavSlot = m_cullDescHeap.AllocateIndex();
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = 1;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

        device->CreateUnorderedAccessView(
            m_counterBuffer.Get(), nullptr, &uavDesc,
            m_cullDescHeap.GetCPUHandle(counterUavSlot));
    }

    m_indirectReady = true;
    GX_LOG_INFO("GPUDrivenRenderer: Indirect pipeline created successfully (max %u objects)", m_maxIndirectObjects);
}

void GPUDrivenRenderer::UploadMeshletBounds(ID3D12Device* device, const MeshletBounds* bounds, uint32_t count)
{
    if (!device || !bounds || count == 0)
    {
        GX_LOG_ERROR("GPUDrivenRenderer::UploadMeshletBounds: invalid parameters");
        return;
    }

    uint32_t bufferSize = count * static_cast<uint32_t>(sizeof(MeshletBounds));
    m_boundsBuffer.CreateVertexBuffer(device, bounds, bufferSize, static_cast<uint32_t>(sizeof(MeshletBounds)));
    m_boundsCount = count;

    // Update the bounds SRV in the cull descriptor heap (slot 0)
    if (m_cullDescHeap.GetHeap() && m_boundsBuffer.GetResource())
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = count;
        srvDesc.Buffer.StructureByteStride = sizeof(MeshletBounds);

        device->CreateShaderResourceView(
            m_boundsBuffer.GetResource(), &srvDesc,
            m_cullDescHeap.GetCPUHandle(0));
    }

    GX_LOG_INFO("Uploaded %u meshlet bounds", count);
}

void GPUDrivenRenderer::CullAndDraw(ID3D12GraphicsCommandList* cmdList,
                                      const Matrix4x4& viewProj, uint32_t objectCount)
{
    if (!cmdList || !m_indirectReady || objectCount == 0)
        return;

    // Fall back to stub if GPU resources were not fully created
    if (!m_cullPSO || !m_cullRootSignature || !m_indirectArgBuffer ||
        !m_counterBuffer || !m_commandSignature || !m_cullDescHeap.GetHeap())
    {
        m_visibleCount = objectCount;
        GX_LOG_INFO("GPUDrivenRenderer::CullAndDraw: %u objects (stub - GPU resources incomplete)", objectCount);
        return;
    }

    // Clamp to max supported object count
    uint32_t dispatchCount = (objectCount < m_maxIndirectObjects) ? objectCount : m_maxIndirectObjects;

    // =========================================================================
    // 1. Clear counter buffer to zero
    //    Transition counter: UAV -> COPY_DEST, copy zero, COPY_DEST -> UAV
    // =========================================================================
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_counterBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    cmdList->CopyBufferRegion(m_counterBuffer.Get(), 0,
                               m_counterUpload.GetResource(), 0, sizeof(uint32_t));

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_counterBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // =========================================================================
    // 2. Set compute pipeline for frustum culling
    // =========================================================================
    cmdList->SetComputeRootSignature(m_cullRootSignature.Get());
    cmdList->SetPipelineState(m_cullPSO.Get());

    // =========================================================================
    // 3. Set root constants (b0): viewProj + objectCount
    // =========================================================================
    struct
    {
        float viewProj[16];
        uint32_t objectCount;
        uint32_t pad[3];
    } cullConstants = {};

    std::memcpy(cullConstants.viewProj, &viewProj, sizeof(float) * 16);
    cullConstants.objectCount = dispatchCount;

    cmdList->SetComputeRoot32BitConstants(0, 20, &cullConstants, 0);

    // =========================================================================
    // 4. Bind descriptor tables (t0 SRV, u0+u1 UAVs)
    // =========================================================================
    ID3D12DescriptorHeap* heaps[] = { m_cullDescHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    // Root param [1] = SRV table starting at heap slot 0 (bounds SRV)
    cmdList->SetComputeRootDescriptorTable(1, m_cullDescHeap.GetGPUHandle(0));

    // Root param [2] = UAV table starting at heap slot 1 (draw commands UAV + counter UAV)
    cmdList->SetComputeRootDescriptorTable(2, m_cullDescHeap.GetGPUHandle(1));

    // =========================================================================
    // 5. Dispatch compute: 64 threads per group
    // =========================================================================
    uint32_t groupCount = (dispatchCount + 63) / 64;
    cmdList->Dispatch(groupCount, 1, 1);

    // =========================================================================
    // 6. UAV barrier to ensure cull writes are complete before ExecuteIndirect
    // =========================================================================
    {
        D3D12_RESOURCE_BARRIER barriers[2] = {};

        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[0].UAV.pResource = m_indirectArgBuffer.Get();

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[1].UAV.pResource = m_counterBuffer.Get();

        cmdList->ResourceBarrier(2, barriers);
    }

    // =========================================================================
    // 7. Transition indirect arg buffer for ExecuteIndirect
    //    UAV -> INDIRECT_ARGUMENT
    // =========================================================================
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_indirectArgBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // =========================================================================
    // 8. ExecuteIndirect with the visible count from the counter buffer
    //    We use the counter buffer as the count buffer argument to
    //    ExecuteIndirect, which reads the UINT32 at offset 0 to determine
    //    how many commands to execute.
    // =========================================================================
    cmdList->ExecuteIndirect(m_commandSignature.Get(), dispatchCount,
                              m_indirectArgBuffer.Get(), 0,
                              m_counterBuffer.Get(), 0);

    // =========================================================================
    // 9. Transition indirect arg buffer back to UAV for next frame
    //    INDIRECT_ARGUMENT -> UAV
    // =========================================================================
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_indirectArgBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // =========================================================================
    // 10. Copy counter to readback buffer for CPU-side visible count query
    //     Counter: UAV -> COPY_SOURCE, copy, COPY_SOURCE -> UAV
    // =========================================================================
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_counterBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    cmdList->CopyBufferRegion(m_counterReadback.Get(), 0,
                               m_counterBuffer.Get(), 0, sizeof(uint32_t));

    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_counterBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);
    }

    // =========================================================================
    // 11. Read back visible count from readback buffer
    //     Note: This reads the result from the *previous* frame's copy because
    //     the current frame's GPU work has not completed yet. For the first
    //     frame this will read zero, which is acceptable. A more robust
    //     approach would use a fence, but for m_visibleCount (used only for
    //     statistics/diagnostics) a one-frame delay is sufficient.
    // =========================================================================
    {
        D3D12_RANGE readRange = { 0, sizeof(uint32_t) };
        void* mapped = nullptr;
        HRESULT mapHr = m_counterReadback->Map(0, &readRange, &mapped);
        if (SUCCEEDED(mapHr) && mapped)
        {
            std::memcpy(&m_visibleCount, mapped, sizeof(uint32_t));
            D3D12_RANGE writeRange = { 0, 0 };
            m_counterReadback->Unmap(0, &writeRange);
        }
        else
        {
            m_visibleCount = dispatchCount;
        }
    }
}

} // namespace gx
