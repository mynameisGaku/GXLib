/// @file IndirectCommandBuffer.cpp
/// @brief GPU間接描画用コマンドバッファの実装
#include "pch.h"
#include "Graphics/Device/IndirectCommandBuffer.h"
#include "Core/Logger.h"

namespace GX
{

bool IndirectCommandBuffer::Initialize(ID3D12Device* device, uint32_t maxCommands,
                                        IndirectCommandType type)
{
    m_type = type;
    m_maxCommands = maxCommands;
    m_commandCount = 0;

    // コマンド引数のストライドを決定
    if (type == IndirectCommandType::DrawIndexed)
        m_stride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    else
        m_stride = sizeof(D3D12_DRAW_ARGUMENTS);

    // コマンドシグネチャを作成
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    if (type == IndirectCommandType::DrawIndexed)
        argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
    else
        argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

    D3D12_COMMAND_SIGNATURE_DESC sigDesc = {};
    sigDesc.ByteStride = m_stride;
    sigDesc.NumArgumentDescs = 1;
    sigDesc.pArgumentDescs = &argDesc;
    sigDesc.NodeMask = 0;

    HRESULT hr = device->CreateCommandSignature(&sigDesc, nullptr,
                                                 IID_PPV_ARGS(&m_commandSignature));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("IndirectCommandBuffer: CreateCommandSignature failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }

    // UPLOADヒープにコマンド引数バッファを作成（CPU→GPU毎フレーム書き込み）
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = static_cast<uint64_t>(m_stride) * m_maxCommands;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
                                          &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                          nullptr, IID_PPV_ARGS(&m_argumentBuffer));
    if (FAILED(hr))
    {
        GX_LOG_ERROR("IndirectCommandBuffer: CreateCommittedResource failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }

    // 永続マップ（UPLOADヒープなので安全）
    hr = m_argumentBuffer->Map(0, nullptr, &m_mappedData);
    if (FAILED(hr))
    {
        GX_LOG_ERROR("IndirectCommandBuffer: Map failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }

    return true;
}

void IndirectCommandBuffer::Reset()
{
    m_commandCount = 0;
}

void IndirectCommandBuffer::AddDrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                            uint32_t startIndex, int32_t baseVertex,
                                            uint32_t startInstance)
{
    if (m_commandCount >= m_maxCommands || m_type != IndirectCommandType::DrawIndexed)
        return;

    auto* args = static_cast<D3D12_DRAW_INDEXED_ARGUMENTS*>(m_mappedData);
    args[m_commandCount].IndexCountPerInstance = indexCount;
    args[m_commandCount].InstanceCount = instanceCount;
    args[m_commandCount].StartIndexLocation = startIndex;
    args[m_commandCount].BaseVertexLocation = baseVertex;
    args[m_commandCount].StartInstanceLocation = startInstance;
    ++m_commandCount;
}

void IndirectCommandBuffer::AddDraw(uint32_t vertexCount, uint32_t instanceCount,
                                     uint32_t startVertex, uint32_t startInstance)
{
    if (m_commandCount >= m_maxCommands || m_type != IndirectCommandType::Draw)
        return;

    auto* args = static_cast<D3D12_DRAW_ARGUMENTS*>(m_mappedData);
    args[m_commandCount].VertexCountPerInstance = vertexCount;
    args[m_commandCount].InstanceCount = instanceCount;
    args[m_commandCount].StartVertexLocation = startVertex;
    args[m_commandCount].StartInstanceLocation = startInstance;
    ++m_commandCount;
}

void IndirectCommandBuffer::Execute(ID3D12GraphicsCommandList* cmdList)
{
    if (m_commandCount == 0 || !m_commandSignature || !m_argumentBuffer)
        return;

    cmdList->ExecuteIndirect(m_commandSignature.Get(), m_commandCount,
                             m_argumentBuffer.Get(), 0, nullptr, 0);
}

} // namespace GX
