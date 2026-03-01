#pragma once
/// @file Tween.h
/// @brief Tweenアニメーション（イージング関数 + Tweenマネージャー）
#include <cmath>
#include <vector>
#include <functional>

namespace gx {
/// @addtogroup grp_math
/// @{

// ============================================================================
// イージング関数 (t: 0.0 〜 1.0 → 0.0 〜 1.0)
// ============================================================================
namespace Ease {

constexpr float PI = 3.14159265358979323846f;

// --- Linear ---
inline float Linear(float t) { return t; }

// --- Quad ---
inline float InQuad(float t)    { return t * t; }
inline float OutQuad(float t)   { return t * (2.0f - t); }
inline float InOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }

// --- Cubic ---
inline float InCubic(float t)    { return t * t * t; }
inline float OutCubic(float t)   { float u = t - 1.0f; return u * u * u + 1.0f; }
inline float InOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : 1.0f + (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f); }

// --- Quart ---
inline float InQuart(float t)    { return t * t * t * t; }
inline float OutQuart(float t)   { float u = t - 1.0f; return 1.0f - u * u * u * u; }
inline float InOutQuart(float t) { return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) * (t - 1.0f); }

// --- Quint ---
inline float InQuint(float t)    { return t * t * t * t * t; }
inline float OutQuint(float t)   { float u = t - 1.0f; return 1.0f + u * u * u * u * u; }
inline float InOutQuint(float t) { return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f + 16.0f * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) * (t - 1.0f) * (t - 1.0f); }

// --- Sine ---
inline float InSine(float t)    { return 1.0f - std::cos(t * PI * 0.5f); }
inline float OutSine(float t)   { return std::sin(t * PI * 0.5f); }
inline float InOutSine(float t) { return 0.5f * (1.0f - std::cos(t * PI)); }

// --- Expo ---
inline float InExpo(float t)    { return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
inline float OutExpo(float t)   { return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
inline float InOutExpo(float t) {
    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;
    return t < 0.5f ? 0.5f * std::pow(2.0f, 20.0f * t - 10.0f)
                    : 1.0f - 0.5f * std::pow(2.0f, -20.0f * t + 10.0f);
}

// --- Circ ---
inline float InCirc(float t)    { return 1.0f - std::sqrt(1.0f - t * t); }
inline float OutCirc(float t)   { float u = t - 1.0f; return std::sqrt(1.0f - u * u); }
inline float InOutCirc(float t) {
    return t < 0.5f ? 0.5f * (1.0f - std::sqrt(1.0f - 4.0f * t * t))
                    : 0.5f * (std::sqrt(1.0f - (2.0f * t - 2.0f) * (2.0f * t - 2.0f)) + 1.0f);
}

// --- Back ---
inline float InBack(float t)    { constexpr float s = 1.70158f; return t * t * ((s + 1.0f) * t - s); }
inline float OutBack(float t)   { constexpr float s = 1.70158f; float u = t - 1.0f; return u * u * ((s + 1.0f) * u + s) + 1.0f; }
inline float InOutBack(float t) {
    constexpr float s = 1.70158f * 1.525f;
    if (t < 0.5f) { float u = 2.0f * t; return 0.5f * (u * u * ((s + 1.0f) * u - s)); }
    float u = 2.0f * t - 2.0f; return 0.5f * (u * u * ((s + 1.0f) * u + s) + 2.0f);
}

// --- Elastic ---
inline float InElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * (2.0f * PI / 3.0f));
}
inline float OutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * (2.0f * PI / 3.0f)) + 1.0f;
}
inline float InOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    constexpr float c = 2.0f * PI / 4.5f;
    return t < 0.5f ? -0.5f * std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c)
                    :  0.5f * std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c) + 1.0f;
}

// --- Bounce ---
inline float OutBounce(float t) {
    if (t < 1.0f / 2.75f)        return 7.5625f * t * t;
    if (t < 2.0f / 2.75f)        { float u = t - 1.5f / 2.75f;   return 7.5625f * u * u + 0.75f; }
    if (t < 2.5f / 2.75f)        { float u = t - 2.25f / 2.75f;  return 7.5625f * u * u + 0.9375f; }
    float u = t - 2.625f / 2.75f; return 7.5625f * u * u + 0.984375f;
}
inline float InBounce(float t)    { return 1.0f - OutBounce(1.0f - t); }
inline float InOutBounce(float t) { return t < 0.5f ? 0.5f * InBounce(2.0f * t) : 0.5f * OutBounce(2.0f * t - 1.0f) + 0.5f; }

} // namespace Ease

// ============================================================================
// Tweenマネージャー
// ============================================================================

/// @brief イージング関数の型
using EaseFunc = float(*)(float);

/// @brief 1つのTweenアニメーション
struct TweenEntry
{
    float* target     = nullptr;     ///< 変更対象のfloat変数
    float  from       = 0.0f;        ///< 開始値
    float  to         = 0.0f;        ///< 終了値
    float  duration   = 0.0f;        ///< 所要時間（秒）
    float  elapsed    = 0.0f;        ///< 経過時間
    float  delay      = 0.0f;        ///< 開始遅延（秒）
    EaseFunc ease     = Ease::Linear;
    std::function<void()> onComplete;
    bool   alive      = true;
};

/// @brief Tweenを管理するクラス
class TweenManager
{
public:
    /// @brief Tweenを登録する
    /// @param target 変化させるfloat変数のポインタ
    /// @param from 開始値
    /// @param to 終了値
    /// @param duration 所要時間（秒）
    /// @param ease イージング関数（デフォルト: Linear）
    /// @return Tween ID（Cancel用）
    int To(float* target, float from, float to, float duration, EaseFunc ease = Ease::Linear)
    {
        int id = m_nextId++;
        m_tweens.push_back({ target, from, to, duration, 0.0f, 0.0f, ease, nullptr, true });
        m_ids.push_back(id);
        return id;
    }

    /// @brief 遅延付きTweenを登録する
    int ToDelayed(float* target, float from, float to, float duration, float delay, EaseFunc ease = Ease::Linear)
    {
        int id = m_nextId++;
        m_tweens.push_back({ target, from, to, duration, 0.0f, delay, ease, nullptr, true });
        m_ids.push_back(id);
        return id;
    }

    /// @brief 完了コールバック付きTweenを登録する
    int ToWithCallback(float* target, float from, float to, float duration, EaseFunc ease, std::function<void()> onComplete)
    {
        int id = m_nextId++;
        m_tweens.push_back({ target, from, to, duration, 0.0f, 0.0f, ease, std::move(onComplete), true });
        m_ids.push_back(id);
        return id;
    }

    /// @brief 全Tweenを更新する（毎フレーム呼ぶ）
    /// @param dt デルタタイム（秒）
    void Update(float dt)
    {
        for (size_t i = 0; i < m_tweens.size(); ++i)
        {
            auto& tw = m_tweens[i];
            if (!tw.alive) continue;

            if (tw.delay > 0.0f)
            {
                tw.delay -= dt;
                if (tw.delay > 0.0f) continue;
                dt = -tw.delay;
                tw.delay = 0.0f;
            }

            tw.elapsed += dt;
            float t = tw.elapsed / tw.duration;
            if (t >= 1.0f)
            {
                t = 1.0f;
                tw.alive = false;
            }

            float eased = tw.ease(t);
            if (tw.target)
                *tw.target = tw.from + (tw.to - tw.from) * eased;

            if (!tw.alive && tw.onComplete)
                tw.onComplete();
        }

        // 完了したものを除去
        for (int i = static_cast<int>(m_tweens.size()) - 1; i >= 0; --i)
        {
            if (!m_tweens[i].alive)
            {
                m_tweens.erase(m_tweens.begin() + i);
                m_ids.erase(m_ids.begin() + i);
            }
        }
    }

    /// @brief 指定IDのTweenをキャンセルする
    void Cancel(int id)
    {
        for (size_t i = 0; i < m_ids.size(); ++i)
        {
            if (m_ids[i] == id) { m_tweens[i].alive = false; break; }
        }
    }

    /// @brief 全Tweenをキャンセルする
    void CancelAll()
    {
        m_tweens.clear();
        m_ids.clear();
    }

    /// @brief アクティブなTween数を取得する
    int Count() const { return static_cast<int>(m_tweens.size()); }

private:
    std::vector<TweenEntry> m_tweens;
    std::vector<int>        m_ids;
    int                     m_nextId = 0;
};

/// @}
} // namespace gx
