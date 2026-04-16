/// @file TransitionEffect.cpp
/// @brief TransitionEffect の実装
#include "pch_common.h"
#include "Core/Scene/TransitionEffect.h"

namespace gx
{

// -------------------------------------------------------------------------
// Helper: smoothstep 補間 (エッジの柔らかさ用)
// -------------------------------------------------------------------------
static float Smoothstep(float edge0, float edge1, float x)
{
    if (edge1 <= edge0) return x >= edge0 ? 1.0f : 0.0f;
    float t = (x - edge0) / (edge1 - edge0);
    t = (std::max)(0.0f, (std::min)(1.0f, t));
    return t * t * (3.0f - 2.0f * t);
}

// -------------------------------------------------------------------------
// Helper: 簡易ハッシュ (Dissolve 用)
// -------------------------------------------------------------------------
static float HashNoise(float x, float y)
{
    // 整数化してビット操作ベースの疑似乱数を生成
    uint32_t ix = static_cast<uint32_t>(x * 1024.0f);
    uint32_t iy = static_cast<uint32_t>(y * 1024.0f);
    uint32_t seed = ix * 73856093u ^ iy * 19349663u;
    seed = (seed ^ (seed >> 16)) * 0x45d9f3bu;
    seed = (seed ^ (seed >> 16)) * 0x45d9f3bu;
    seed = seed ^ (seed >> 16);
    return static_cast<float>(seed & 0xFFFFu) / 65535.0f;
}

// -------------------------------------------------------------------------
// BuiltinTransition
// -------------------------------------------------------------------------

BuiltinTransition::BuiltinTransition(TransitionType type, const TransitionParams& params)
    : m_type(type)
    , m_params(params)
{
}

float BuiltinTransition::GetMaskValue(float progress, float screenX, float screenY) const
{
    // progress を 0-1 にクランプ
    progress = (std::max)(0.0f, (std::min)(1.0f, progress));
    float soft = (std::max)(0.001f, m_params.softEdge);

    switch (m_type)
    {
    case TransitionType::Fade:
    {
        // 単純にprogressをマスク値として返す
        return progress;
    }

    case TransitionType::Wipe:
    {
        // 横方向ワイプ: screenX < progress の領域が不透明（左から右へ）
        float edge = progress;
        return 1.0f - Smoothstep(edge - soft, edge + soft, screenX);
    }

    case TransitionType::WipeVertical:
    {
        // 縦方向ワイプ: screenY < progress の領域が不透明（上から下へ）
        float edge = progress;
        return 1.0f - Smoothstep(edge - soft, edge + soft, screenY);
    }

    case TransitionType::CircleOpen:
    {
        // 円形マスク(開く): 中心から外側へ広がる
        float dx = screenX - m_params.centerX;
        float dy = screenY - m_params.centerY;
        float dist = sqrtf(dx * dx + dy * dy);
        // 画面四隅までの距離の最大値を使う
        float d1 = sqrtf(m_params.centerX * m_params.centerX + m_params.centerY * m_params.centerY);
        float d2 = sqrtf((1.0f - m_params.centerX) * (1.0f - m_params.centerX) + m_params.centerY * m_params.centerY);
        float d3 = sqrtf(m_params.centerX * m_params.centerX + (1.0f - m_params.centerY) * (1.0f - m_params.centerY));
        float d4 = sqrtf((1.0f - m_params.centerX) * (1.0f - m_params.centerX) +
                         (1.0f - m_params.centerY) * (1.0f - m_params.centerY));
        float maxRadius = (std::max)((std::max)(d1, d2), (std::max)(d3, d4));
        if (maxRadius < 1e-6f) maxRadius = 1.0f;

        float normalizedDist = dist / maxRadius;
        // progress範囲をソフトエッジ分拡張: progress=0で完全透明、progress=1で完全不透明
        float p = progress * (1.0f + 2.0f * soft) - soft;
        return 1.0f - Smoothstep(p - soft, p + soft, normalizedDist);
    }

    case TransitionType::CircleClose:
    {
        // 円形マスク(閉じる): 外側から中心へ縮小（CircleOpenの逆進行）
        float dx = screenX - m_params.centerX;
        float dy = screenY - m_params.centerY;
        float dist = sqrtf(dx * dx + dy * dy);
        float d1 = sqrtf(m_params.centerX * m_params.centerX + m_params.centerY * m_params.centerY);
        float d2 = sqrtf((1.0f - m_params.centerX) * (1.0f - m_params.centerX) + m_params.centerY * m_params.centerY);
        float d3 = sqrtf(m_params.centerX * m_params.centerX + (1.0f - m_params.centerY) * (1.0f - m_params.centerY));
        float d4 = sqrtf((1.0f - m_params.centerX) * (1.0f - m_params.centerX) +
                         (1.0f - m_params.centerY) * (1.0f - m_params.centerY));
        float maxRadius = (std::max)((std::max)(d1, d2), (std::max)(d3, d4));
        if (maxRadius < 1e-6f) maxRadius = 1.0f;

        float normalizedDist = dist / maxRadius;
        // CircleOpenの逆: (1-progress)で同じロジック
        float ep = 1.0f - progress;
        float p = ep * (1.0f + 2.0f * soft) - soft;
        return 1.0f - Smoothstep(p - soft, p + soft, normalizedDist);
    }

    case TransitionType::Dissolve:
    {
        // ランダムディゾルブ: progressがノイズ値を超えた部分が不透明
        float noise = HashNoise(screenX, screenY);
        return Smoothstep(noise - soft, noise + soft, progress);
    }

    case TransitionType::SlideLeft:
    {
        // スライド左: 画面が左にスライドして覆う
        float edge = 1.0f - progress;
        return Smoothstep(edge - soft, edge + soft, screenX);
    }

    case TransitionType::SlideRight:
    {
        // スライド右: 画面が右にスライドして覆う
        float edge = progress;
        return 1.0f - Smoothstep(edge - soft, edge + soft, screenX);
    }

    default:
        return progress;
    }
}

// -------------------------------------------------------------------------
// TransitionEffectManager
// -------------------------------------------------------------------------

void TransitionEffectManager::SetEffect(std::unique_ptr<ITransitionEffect> effect)
{
    m_effect = std::move(effect);
}

void TransitionEffectManager::SetEffect(TransitionType type, const TransitionParams& params)
{
    m_effect = std::make_unique<BuiltinTransition>(type, params);
}

float TransitionEffectManager::SampleMask(float normalizedX, float normalizedY) const
{
    if (!m_effect)
        return m_progress; // フォールバック: 単純フェード
    return m_effect->GetMaskValue(m_progress, normalizedX, normalizedY);
}

} // namespace gx
