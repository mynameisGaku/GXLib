/// @file InstanceBuffer.cpp
/// @brief インスタンシング描画用データバッファの実装
#include "pch.h"
#include "Graphics/3D/InstanceBuffer.h"
#include "Core/Logger.h"

namespace GX
{

bool InstanceBuffer::Initialize(ID3D12Device* device, uint32_t maxInstances)
{
    m_maxInstances = maxInstances;
    uint32_t bufferSize = maxInstances * sizeof(InstanceData);

    if (!m_buffer.Initialize(device, bufferSize, sizeof(InstanceData)))
    {
        GX_LOG_ERROR("InstanceBuffer: failed to initialize (max=%u)", maxInstances);
        return false;
    }

    m_instances.reserve(maxInstances);
    return true;
}

void InstanceBuffer::Reset()
{
    m_instances.clear();
}

void InstanceBuffer::AddInstance(const Transform3D& transform)
{
    if (m_instances.size() >= m_maxInstances)
    {
        GX_LOG_WARN("InstanceBuffer: max instances (%u) exceeded", m_maxInstances);
        return;
    }

    InstanceData data;
    XMStoreFloat4x4(&data.world, XMMatrixTranspose(transform.GetWorldMatrix()));
    XMStoreFloat4x4(&data.worldInvTranspose, XMMatrixTranspose(transform.GetWorldInverseTranspose()));
    m_instances.push_back(data);
}

void InstanceBuffer::AddInstance(const XMMATRIX& worldMatrix)
{
    if (m_instances.size() >= m_maxInstances)
    {
        GX_LOG_WARN("InstanceBuffer: max instances (%u) exceeded", m_maxInstances);
        return;
    }

    InstanceData data;
    XMStoreFloat4x4(&data.world, XMMatrixTranspose(worldMatrix));
    XMMATRIX invTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, worldMatrix));
    XMStoreFloat4x4(&data.worldInvTranspose, XMMatrixTranspose(invTranspose));
    m_instances.push_back(data);
}

void InstanceBuffer::Upload(uint32_t frameIndex)
{
    if (m_instances.empty()) return;

    void* mapped = m_buffer.Map(frameIndex);
    if (!mapped)
    {
        GX_LOG_ERROR("InstanceBuffer: failed to map buffer");
        return;
    }

    uint32_t copySize = static_cast<uint32_t>(m_instances.size() * sizeof(InstanceData));
    memcpy(mapped, m_instances.data(), copySize);
    m_buffer.Unmap(frameIndex);
}

D3D12_GPU_VIRTUAL_ADDRESS InstanceBuffer::GetGPUVirtualAddress(uint32_t frameIndex) const
{
    return m_buffer.GetGPUVirtualAddress(frameIndex);
}

} // namespace GX
