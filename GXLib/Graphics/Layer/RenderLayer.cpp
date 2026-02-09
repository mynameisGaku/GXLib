/// @file RenderLayer.cpp
/// @brief 描画レイヤーの実装
#include "pch.h"
#include "Graphics/Layer/RenderLayer.h"
#include "Core/Logger.h"

namespace GX
{

bool RenderLayer::Create(ID3D12Device* device, const std::string& name,
                          int32_t zOrder, uint32_t width, uint32_t height)
{
    m_name    = name;
    m_zOrder  = zOrder;
    m_width   = width;
    m_height  = height;

    if (!m_renderTarget.Create(device, width, height, DXGI_FORMAT_R8G8B8A8_UNORM))
    {
        GX_LOG_ERROR("RenderLayer '%s': Failed to create RT (%dx%d)", name.c_str(), width, height);
        return false;
    }

    GX_LOG_INFO("RenderLayer '%s' created (Z:%d, %dx%d)", name.c_str(), zOrder, width, height);
    return true;
}

void RenderLayer::Begin(ID3D12GraphicsCommandList* cmdList)
{
    m_cmdList = cmdList;

    m_renderTarget.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto rtvHandle = m_renderTarget.GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT vp = {};
    vp.Width    = static_cast<float>(m_width);
    vp.Height   = static_cast<float>(m_height);
    vp.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &vp);

    D3D12_RECT sc = {};
    sc.right  = static_cast<LONG>(m_width);
    sc.bottom = static_cast<LONG>(m_height);
    cmdList->RSSetScissorRects(1, &sc);
}

void RenderLayer::End()
{
    m_renderTarget.TransitionTo(m_cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cmdList = nullptr;
}

void RenderLayer::Clear(ID3D12GraphicsCommandList* cmdList,
                          float r, float g, float b, float a)
{
    m_renderTarget.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    const float clearColor[] = { r, g, b, a };
    cmdList->ClearRenderTargetView(m_renderTarget.GetRTVHandle(), clearColor, 0, nullptr);
}

void RenderLayer::OnResize(ID3D12Device* device, uint32_t w, uint32_t h)
{
    m_width  = w;
    m_height = h;
    m_renderTarget.Create(device, w, h, DXGI_FORMAT_R8G8B8A8_UNORM);
}

} // namespace GX
