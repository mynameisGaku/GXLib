/// @file AudioOcclusion.cpp
/// @brief オーディオオクルージョン実装
#include "pch_audio.h"
#include "Audio/AudioOcclusion.h"
#include <algorithm>
#include <cmath>

namespace gx
{

OcclusionParams AudioOcclusion::Calculate(const Vector3& emitterPos) const
{
    OcclusionParams params;

    if (!m_enabled || !m_raycastCallback)
        return params; // デフォルト（遮蔽なし）

    // コールバックで遮蔽度を取得 (0..1)
    float occlusion = m_raycastCallback(m_listenerPos, emitterPos);
    occlusion = (std::max)(0.0f, (std::min)(1.0f, occlusion));

    // 遮蔽度に基づいてパラメータを計算
    params.directLevel   = 1.0f - occlusion * 0.8f;  // 最大80%減衰
    params.reverbLevel   = 1.0f - occlusion * 0.5f;  // リバーブは緩やかに減衰
    params.lowPassCutoff = 1.0f - occlusion * 0.7f;  // 壁越しはこもった音に

    return params;
}

} // namespace gx
