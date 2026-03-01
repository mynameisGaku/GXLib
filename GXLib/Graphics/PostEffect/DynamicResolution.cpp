/// @file DynamicResolution.cpp
/// @brief 動的解像度スケーリングの実装
#include "pch_graphics.h"
#include "Graphics/PostEffect/DynamicResolution.h"
#include "Core/Logger.h"
#include <algorithm>
#include <cmath>

namespace gx
{

bool DynamicResolution::Initialize(uint32_t nativeWidth, uint32_t nativeHeight,
                                    const DynamicResolutionConfig& config)
{
    if (nativeWidth == 0 || nativeHeight == 0)
    {
        GX_LOG_ERROR("DynamicResolution::Initialize: invalid dimensions");
        return false;
    }

    m_nativeWidth = nativeWidth;
    m_nativeHeight = nativeHeight;
    m_config = config;
    m_currentScale = config.maxScale;
    m_initialized = true;

    GX_LOG_INFO("DynamicResolution initialized: %ux%u, target %.2f ms",
                nativeWidth, nativeHeight, config.targetFrameTimeMs);
    return true;
}

void DynamicResolution::UpdateScale(float gpuTimeMs)
{
    if (!m_initialized || !m_config.enabled || gpuTimeMs <= 0.0f)
        return;

    float ratio = m_config.targetFrameTimeMs / gpuTimeMs;
    float targetScale = m_currentScale * ratio;

    targetScale = std::clamp(targetScale, m_config.minScale, m_config.maxScale);

    // Exponential smoothing
    m_currentScale += (targetScale - m_currentScale) * m_config.adaptationSpeed;
    m_currentScale = std::clamp(m_currentScale, m_config.minScale, m_config.maxScale);
}

uint32_t DynamicResolution::GetRenderWidth() const
{
    return static_cast<uint32_t>(m_nativeWidth * m_currentScale);
}

uint32_t DynamicResolution::GetRenderHeight() const
{
    return static_cast<uint32_t>(m_nativeHeight * m_currentScale);
}

void DynamicResolution::ApplyUpscale(ID3D12GraphicsCommandList* cmdList)
{
    if (!cmdList || !m_initialized) return;
    // In production: dispatch CAS upscale compute shader
    // For now, the post-effect pipeline handles bilinear upscale
}

} // namespace gx
