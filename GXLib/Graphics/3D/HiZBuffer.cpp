/// @file HiZBuffer.cpp
/// @brief Hi-Z オクルージョンカリング実装
#include "pch_graphics.h"
#include "Graphics/3D/HiZBuffer.h"
#include "Graphics/Pipeline/RootSignature.h"
#include "Graphics/Device/BarrierBatch.h"
#include "Core/Logger.h"

namespace gx
{

bool HiZBuffer::Initialize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_device = device;
    m_width  = width;
    m_height = height;

    // ミップレベル数を計算
    uint32_t maxDim = (std::max)(width, height);
    m_mipLevels = 1;
    while ((1u << m_mipLevels) < maxDim)
        ++m_mipLevels;
    m_mipLevels = (std::min)(m_mipLevels, 12u);  // 最大12レベル

    // シェーダーコンパイラ
    if (!m_shader.Initialize())
        return false;

    // ディスクリプタヒープ（SRV + UAV: 各ミップ×2 + 深度SRV + カリング用SRV/UAV×3）
    uint32_t descriptorCount = m_mipLevels * 2 + 1 + 3;
    if (!m_srvUavHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                  descriptorCount, true))
    {
        GX_LOG_ERROR("HiZBuffer: Failed to create descriptor heap");
        return false;
    }

    // Hi-Zテクスチャ作成
    if (!CreateHiZTexture(device))
        return false;

    // パイプライン作成
    if (!CreatePipelines(device))
        return false;

    // 定数バッファ
    if (!m_generateCB.Initialize(device, 256, 256))
        return false;
    if (!m_cullCB.Initialize(device, 256, 256))
        return false;

    m_maxObjects = k_MaxCullObjects;
    m_ready = true;
    GX_LOG_INFO("HiZBuffer initialized (%dx%d, %u mip levels)", width, height, m_mipLevels);
    return true;
}

bool HiZBuffer::CreateHiZTexture(ID3D12Device* device)
{
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = m_width / 2;   // ミップ0 = 半解像度
    texDesc.Height = m_height / 2;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = static_cast<UINT16>(m_mipLevels);
    texDesc.Format = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&m_hiZTexture));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("HiZBuffer: Failed to create Hi-Z texture");
        return false;
    }

    // 各ミップのSRV/UAVを作成
    m_mipSRVSlots.resize(m_mipLevels);
    m_mipUAVSlots.resize(m_mipLevels);

    for (uint32_t i = 0; i < m_mipLevels; ++i)
    {
        m_mipSRVSlots[i] = m_srvUavHeap.AllocateIndex();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = i;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_hiZTexture.Get(), &srvDesc,
                                          m_srvUavHeap.GetCPUHandle(m_mipSRVSlots[i]));

        m_mipUAVSlots[i] = m_srvUavHeap.AllocateIndex();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = i;
        device->CreateUnorderedAccessView(m_hiZTexture.Get(), nullptr, &uavDesc,
                                           m_srvUavHeap.GetCPUHandle(m_mipUAVSlots[i]));
    }

    // 入力深度SRV用スロット（CopyDescriptors先）
    m_depthSRVSlot = m_srvUavHeap.AllocateIndex();

    // カリング用ディスクリプタスロット（固定確保）
    m_boundsSRVSlot = m_srvUavHeap.AllocateIndex();
    m_flagsUAVSlot  = m_srvUavHeap.AllocateIndex();

    return true;
}

bool HiZBuffer::CreatePipelines(ID3D12Device* device)
{
    // --- Hi-Z生成パイプライン ---
    // RS: [0] CBV b0, [1] SRV t0 (ソースミップ), [2] UAV u0 (出力ミップ)
    m_generateRS = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1)
        .Build(device);
    if (!m_generateRS)
        return false;

    auto generateCS = m_shader.CompileFromFile(
        L"Shaders/HiZGenerate.hlsl", L"CSMain", L"cs_6_0");
    if (!generateCS.valid)
    {
        GX_LOG_ERROR("HiZBuffer: Failed to compile HiZGenerate shader");
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC generateDesc = {};
    generateDesc.pRootSignature = m_generateRS.Get();
    generateDesc.CS = generateCS.GetBytecode();
    if (FAILED(device->CreateComputePipelineState(&generateDesc, IID_PPV_ARGS(&m_generatePSO))))
    {
        GX_LOG_ERROR("HiZBuffer: Failed to create generate PSO");
        return false;
    }

    // --- カリングパイプライン ---
    // RS: [0] CBV b0, [1] SRV t0 (バウンド), [2] SRV t1 (Hi-Z), [3] UAV u0 (可視フラグ)
    m_cullRS = RootSignatureBuilder()
        .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
        .AddCBV(0)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1)
        .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1)
        .Build(device);
    if (!m_cullRS)
        return false;

    auto cullCS = m_shader.CompileFromFile(
        L"Shaders/HiZCull.hlsl", L"CSMain", L"cs_6_0");
    if (!cullCS.valid)
    {
        GX_LOG_ERROR("HiZBuffer: Failed to compile HiZCull shader");
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC cullDesc = {};
    cullDesc.pRootSignature = m_cullRS.Get();
    cullDesc.CS = cullCS.GetBytecode();
    if (FAILED(device->CreateComputePipelineState(&cullDesc, IID_PPV_ARGS(&m_cullPSO))))
    {
        GX_LOG_ERROR("HiZBuffer: Failed to create cull PSO");
        return false;
    }

    return true;
}

void HiZBuffer::GenerateHiZ(ID3D12GraphicsCommandList* cmdList,
                              D3D12_GPU_DESCRIPTOR_HANDLE depthSRV,
                              uint32_t frameIndex)
{
    if (!m_ready) return;

    ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(m_generateRS.Get());
    cmdList->SetPipelineState(m_generatePSO.Get());

    uint32_t srcW = m_width;
    uint32_t srcH = m_height;

    for (uint32_t mip = 0; mip < m_mipLevels; ++mip)
    {
        uint32_t dstW = (std::max)(srcW / 2, 1u);
        uint32_t dstH = (std::max)(srcH / 2, 1u);

        // 定数バッファ
        struct HiZConstants { uint32_t srcW, srcH, dstW, dstH; };
        HiZConstants cb = { srcW, srcH, dstW, dstH };

        void* cbData = m_generateCB.Map(frameIndex);
        if (cbData)
        {
            memcpy(cbData, &cb, sizeof(cb));
            m_generateCB.Unmap(frameIndex);
        }
        cmdList->SetComputeRootConstantBufferView(0,
            m_generateCB.GetGPUVirtualAddress(frameIndex));

        // SRV: ソースミップ（最初のパスは深度バッファ、以降は前のミップ）
        if (mip == 0)
        {
            cmdList->SetComputeRootDescriptorTable(1, depthSRV);
        }
        else
        {
            cmdList->SetComputeRootDescriptorTable(1,
                m_srvUavHeap.GetGPUHandle(m_mipSRVSlots[mip - 1]));
        }

        // UAV: 出力ミップ
        cmdList->SetComputeRootDescriptorTable(2,
            m_srvUavHeap.GetGPUHandle(m_mipUAVSlots[mip]));

        // Dispatch
        uint32_t groupX = (dstW + 7) / 8;
        uint32_t groupY = (dstH + 7) / 8;
        cmdList->Dispatch(groupX, groupY, 1);

        // UAVバリア（次のミップが前のミップを読む前に完了を保証）
        D3D12_RESOURCE_BARRIER uavBarrier = {};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = m_hiZTexture.Get();
        cmdList->ResourceBarrier(1, &uavBarrier);

        srcW = dstW;
        srcH = dstH;
    }
}

void HiZBuffer::CullObjects(ID3D12GraphicsCommandList* cmdList,
                              const HiZObjectBounds* bounds, uint32_t objectCount,
                              const XMMATRIX& viewProjection, uint32_t frameIndex)
{
    if (!m_ready || objectCount == 0) return;
    objectCount = (std::min)(objectCount, k_MaxCullObjects);

    // バウンドバッファを作成/更新（UPLOAD ヒープ）
    uint32_t boundsSize = objectCount * sizeof(HiZObjectBounds);
    if (!m_boundsBuffer || m_maxObjects < objectCount)
    {
        uint32_t allocSize = (std::max)(objectCount, k_MaxCullObjects) * sizeof(HiZObjectBounds);
        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = allocSize;
        bufDesc.Height = 1;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1;
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        D3D12_HEAP_PROPERTIES uploadHeap = {};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
                                           &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                           nullptr, IID_PPV_ARGS(&m_boundsBuffer));
        m_maxObjects = objectCount;
    }

    // バウンドデータをアップロード
    void* mapped = nullptr;
    m_boundsBuffer->Map(0, nullptr, &mapped);
    if (mapped)
    {
        memcpy(mapped, bounds, boundsSize);
        m_boundsBuffer->Unmap(0, nullptr);
    }

    // 可視フラグバッファ（UAV）+ リードバック
    uint32_t flagsSize = objectCount * sizeof(uint32_t);
    if (!m_visibilityBuffer)
    {
        D3D12_RESOURCE_DESC bufDesc = {};
        bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufDesc.Width = k_MaxCullObjects * sizeof(uint32_t);
        bufDesc.Height = 1;
        bufDesc.DepthOrArraySize = 1;
        bufDesc.MipLevels = 1;
        bufDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufDesc.SampleDesc.Count = 1;
        bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        D3D12_HEAP_PROPERTIES defaultHeap = {};
        defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;
        m_device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
                                           &bufDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                           nullptr, IID_PPV_ARGS(&m_visibilityBuffer));

        // リードバック用
        D3D12_HEAP_PROPERTIES readbackHeap = {};
        readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;
        bufDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        m_device->CreateCommittedResource(&readbackHeap, D3D12_HEAP_FLAG_NONE,
                                           &bufDesc, D3D12_RESOURCE_STATE_COPY_DEST,
                                           nullptr, IID_PPV_ARGS(&m_readbackBuffer));
    }

    // バウンドSRV作成（固定スロットを再利用）
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = objectCount;
    srvDesc.Buffer.StructureByteStride = sizeof(HiZObjectBounds);
    m_device->CreateShaderResourceView(m_boundsBuffer.Get(), &srvDesc,
                                        m_srvUavHeap.GetCPUHandle(m_boundsSRVSlot));

    // 可視フラグUAV作成（固定スロットを再利用）
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = objectCount;
    uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);
    m_device->CreateUnorderedAccessView(m_visibilityBuffer.Get(), nullptr,
                                         &uavDesc, m_srvUavHeap.GetCPUHandle(m_flagsUAVSlot));

    // 定数バッファ
    struct CullConstants
    {
        XMFLOAT4X4 viewProjection;
        uint32_t objectCount;
        uint32_t hiZW, hiZH;
        uint32_t hiZMipLevels;
    };
    CullConstants cb;
    XMStoreFloat4x4(&cb.viewProjection, XMMatrixTranspose(viewProjection));
    cb.objectCount = objectCount;
    cb.hiZW = m_width / 2;
    cb.hiZH = m_height / 2;
    cb.hiZMipLevels = m_mipLevels;

    void* cbData = m_cullCB.Map(frameIndex);
    if (cbData)
    {
        memcpy(cbData, &cb, sizeof(cb));
        m_cullCB.Unmap(frameIndex);
    }

    // Dispatch
    ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(m_cullRS.Get());
    cmdList->SetPipelineState(m_cullPSO.Get());

    cmdList->SetComputeRootConstantBufferView(0,
        m_cullCB.GetGPUVirtualAddress(frameIndex));
    cmdList->SetComputeRootDescriptorTable(1,
        m_srvUavHeap.GetGPUHandle(m_boundsSRVSlot));
    cmdList->SetComputeRootDescriptorTable(2,
        m_srvUavHeap.GetGPUHandle(m_mipSRVSlots[0]));  // Hi-Z全ミップ
    cmdList->SetComputeRootDescriptorTable(3,
        m_srvUavHeap.GetGPUHandle(m_flagsUAVSlot));

    uint32_t groupCount = (objectCount + 63) / 64;
    cmdList->Dispatch(groupCount, 1, 1);

    // コピーをリードバックバッファへ
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_visibilityBuffer.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->CopyBufferRegion(m_readbackBuffer.Get(), 0,
                                   m_visibilityBuffer.Get(), 0, flagsSize);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        cmdList->ResourceBarrier(1, &barrier);
    }
}

bool HiZBuffer::ReadVisibilityFlags(uint32_t* outFlags, uint32_t objectCount) const
{
    if (!m_readbackBuffer || !outFlags || objectCount == 0)
        return false;

    void* mapped = nullptr;
    D3D12_RANGE readRange = { 0, objectCount * sizeof(uint32_t) };
    HRESULT hr = m_readbackBuffer->Map(0, &readRange, &mapped);
    if (FAILED(hr))
        return false;

    memcpy(outFlags, mapped, objectCount * sizeof(uint32_t));

    D3D12_RANGE writeRange = { 0, 0 };
    m_readbackBuffer->Unmap(0, &writeRange);
    return true;
}

void HiZBuffer::OnResize(ID3D12Device* device, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    uint32_t maxDim = (std::max)(width, height);
    m_mipLevels = 1;
    while ((1u << m_mipLevels) < maxDim)
        ++m_mipLevels;
    m_mipLevels = (std::min)(m_mipLevels, 12u);

    CreateHiZTexture(device);
}

} // namespace gx
