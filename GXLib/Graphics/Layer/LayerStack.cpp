/// @file LayerStack.cpp
/// @brief レイヤースタック管理の実装
#include "pch.h"
#include "Graphics/Layer/LayerStack.h"
#include "Core/Logger.h"

namespace GX
{

RenderLayer* LayerStack::CreateLayer(ID3D12Device* device, const std::string& name,
                                      int32_t zOrder, uint32_t w, uint32_t h)
{
    auto layer = std::make_unique<RenderLayer>();
    if (!layer->Create(device, name, zOrder, w, h))
        return nullptr;

    RenderLayer* ptr = layer.get();
    m_layers.push_back(std::move(layer));
    m_needsSort = true;

    GX_LOG_INFO("LayerStack: Added layer '%s' (Z:%d), total: %zu", name.c_str(), zOrder, m_layers.size());
    return ptr;
}

RenderLayer* LayerStack::GetLayer(const std::string& name)
{
    for (auto& layer : m_layers)
    {
        if (layer->GetName() == name)
            return layer.get();
    }
    return nullptr;
}

bool LayerStack::RemoveLayer(const std::string& name)
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
    {
        if ((*it)->GetName() == name)
        {
            m_layers.erase(it);
            m_needsSort = true;
            GX_LOG_INFO("LayerStack: Removed layer '%s'", name.c_str());
            return true;
        }
    }
    return false;
}

const std::vector<RenderLayer*>& LayerStack::GetSortedLayers()
{
    if (m_needsSort)
        SortLayers();
    return m_sortedLayers;
}

void LayerStack::OnResize(ID3D12Device* device, uint32_t w, uint32_t h)
{
    for (auto& layer : m_layers)
        layer->OnResize(device, w, h);
}

void LayerStack::SortLayers()
{
    m_sortedLayers.clear();
    m_sortedLayers.reserve(m_layers.size());
    for (auto& layer : m_layers)
        m_sortedLayers.push_back(layer.get());

    std::sort(m_sortedLayers.begin(), m_sortedLayers.end(),
              [](const RenderLayer* a, const RenderLayer* b)
              { return a->GetZOrder() < b->GetZOrder(); });

    m_needsSort = false;
}

} // namespace GX
