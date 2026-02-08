/// @file ShadowMap.cpp
/// @brief シャドウマップリソースの実装
#include "pch.h"
#include "Graphics/3D/ShadowMap.h"
#include "Core/Logger.h"

namespace GX
{

bool ShadowMap::Create(ID3D12Device* device, uint32_t size,
                        DescriptorHeap* srvHeap, uint32_t srvIndex)
{
    m_size = size;

    if (!m_depthBuffer.CreateWithSRV(device, size, size, srvHeap, srvIndex))
    {
        GX_LOG_ERROR("ShadowMap: Failed to create depth buffer (%dx%d)", size, size);
        return false;
    }

    GX_LOG_INFO("ShadowMap created (%dx%d)", size, size);
    return true;
}

} // namespace GX
