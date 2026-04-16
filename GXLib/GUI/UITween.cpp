/// @file UITween.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"

#include "GUI/UITween.h"
#include "GUI/Widget.h"
#include <cmath>
#include <algorithm>

namespace gx { namespace GUI {

// ============================================================================
// アニメーション作成
// ============================================================================

UIAnimHandle UITweenManager::Animate(Widget* widget, UIAnimProperty prop,
                                      float from, float to, float duration,
                                      UIEaseType ease)
{
    return AnimateDelayed(widget, prop, from, to, duration, 0.0f, ease);
}

UIAnimHandle UITweenManager::AnimateDelayed(Widget* widget, UIAnimProperty prop,
                                             float from, float to, float duration,
                                             float delay, UIEaseType ease)
{
    if (!widget || duration <= 0.0f) return k_InvalidAnimHandle;

    Animation anim;
    anim.handle = m_nextHandle++;
    anim.widget = widget;
    anim.property = prop;
    anim.from = from;
    anim.to = to;
    anim.duration = duration;
    anim.delay = delay;
    anim.elapsed = 0.0f;
    anim.ease = ease;
    anim.started = false;

    // delay が 0 の場合は即座に開始値を適用
    if (delay <= 0.0f)
    {
        ApplyValue(widget, prop, from);
        anim.started = true;
    }

    m_animations.push_back(std::move(anim));
    return m_animations.back().handle;
}

void UITweenManager::StaggerChildren(Widget* parent, UIAnimProperty prop,
                                      float from, float to, float duration,
                                      float staggerDelay, UIEaseType ease)
{
    if (!parent) return;

    const auto& children = parent->GetChildren();
    float currentDelay = 0.0f;

    for (const auto& child : children)
    {
        if (!child->visible) continue;
        AnimateDelayed(child.get(), prop, from, to, duration, currentDelay, ease);
        currentDelay += staggerDelay;
    }
}

// ============================================================================
// キャンセル
// ============================================================================

void UITweenManager::Cancel(UIAnimHandle handle)
{
    m_animations.erase(
        std::remove_if(m_animations.begin(), m_animations.end(),
            [handle](const Animation& a) { return a.handle == handle; }),
        m_animations.end());
}

void UITweenManager::CancelAll(Widget* widget)
{
    m_animations.erase(
        std::remove_if(m_animations.begin(), m_animations.end(),
            [widget](const Animation& a) { return a.widget == widget; }),
        m_animations.end());
}

// ============================================================================
// 更新
// ============================================================================

void UITweenManager::Update(float dt)
{
    if (m_animations.empty()) return;

    // 完了したアニメーションのコールバックを後から呼ぶ
    gx::Vector<std::function<void()>> completedCallbacks;

    auto it = m_animations.begin();
    while (it != m_animations.end())
    {
        auto& anim = *it;

        anim.elapsed += dt;

        // 遅延中
        if (!anim.started)
        {
            if (anim.elapsed >= anim.delay)
            {
                anim.started = true;
                anim.elapsed -= anim.delay;
                anim.delay = 0.0f;
                ApplyValue(anim.widget, anim.property, anim.from);
            }
            else
            {
                ++it;
                continue;
            }
        }

        // 進行度を計算
        float t = anim.elapsed / anim.duration;
        if (t >= 1.0f) t = 1.0f;

        // イージングを適用
        float eased = EaseFunction(anim.ease, t);

        // 補間値を計算して適用
        float value = anim.from + (anim.to - anim.from) * eased;
        ApplyValue(anim.widget, anim.property, value);

        // 完了チェック
        if (t >= 1.0f)
        {
            // 最終値を確実に適用
            ApplyValue(anim.widget, anim.property, anim.to);

            if (anim.onComplete)
            {
                completedCallbacks.push_back(std::move(anim.onComplete));
            }
            it = m_animations.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 完了コールバックを呼ぶ（アニメーション操作が安全に行えるよう後から）
    for (auto& cb : completedCallbacks)
    {
        cb();
    }
}

bool UITweenManager::IsAnimating(UIAnimHandle handle) const
{
    for (const auto& anim : m_animations)
    {
        if (anim.handle == handle) return true;
    }
    return false;
}

// ============================================================================
// プリセット
// ============================================================================

UIAnimHandle UITweenManager::FadeIn(Widget* widget, float duration)
{
    return Animate(widget, UIAnimProperty::Opacity, 0.0f, 1.0f, duration,
                   UIEaseType::EaseOutQuad);
}

UIAnimHandle UITweenManager::FadeOut(Widget* widget, float duration)
{
    return Animate(widget, UIAnimProperty::Opacity, 1.0f, 0.0f, duration,
                   UIEaseType::EaseOutQuad);
}

UIAnimHandle UITweenManager::SlideInFromLeft(Widget* widget, float distance, float duration)
{
    float currentX = widget ? widget->layoutRect.x : 0.0f;
    return Animate(widget, UIAnimProperty::PositionX, currentX - distance, currentX,
                   duration, UIEaseType::EaseOutCubic);
}

UIAnimHandle UITweenManager::SlideInFromRight(Widget* widget, float distance, float duration)
{
    float currentX = widget ? widget->layoutRect.x : 0.0f;
    return Animate(widget, UIAnimProperty::PositionX, currentX + distance, currentX,
                   duration, UIEaseType::EaseOutCubic);
}

UIAnimHandle UITweenManager::ScaleIn(Widget* widget, float duration)
{
    Animate(widget, UIAnimProperty::ScaleX, 0.0f, 1.0f, duration, UIEaseType::EaseOutBack);
    return Animate(widget, UIAnimProperty::ScaleY, 0.0f, 1.0f, duration, UIEaseType::EaseOutBack);
}

UIAnimHandle UITweenManager::BounceIn(Widget* widget, float duration)
{
    Animate(widget, UIAnimProperty::ScaleX, 0.0f, 1.0f, duration, UIEaseType::EaseOutBounce);
    return Animate(widget, UIAnimProperty::ScaleY, 0.0f, 1.0f, duration, UIEaseType::EaseOutBounce);
}

void UITweenManager::SetOnComplete(UIAnimHandle handle, std::function<void()> callback)
{
    for (auto& anim : m_animations)
    {
        if (anim.handle == handle)
        {
            anim.onComplete = std::move(callback);
            return;
        }
    }
}

// ============================================================================
// イージング関数
// ============================================================================

float UITweenManager::EaseFunction(UIEaseType type, float t) const
{
    constexpr float PI = 3.14159265358979323846f;

    switch (type)
    {
    case UIEaseType::Linear:
        return t;

    case UIEaseType::EaseInQuad:
        return t * t;

    case UIEaseType::EaseOutQuad:
        return t * (2.0f - t);

    case UIEaseType::EaseInOutQuad:
        return t < 0.5f
            ? 2.0f * t * t
            : -1.0f + (4.0f - 2.0f * t) * t;

    case UIEaseType::EaseInCubic:
        return t * t * t;

    case UIEaseType::EaseOutCubic:
    {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }

    case UIEaseType::EaseInOutCubic:
        return t < 0.5f
            ? 4.0f * t * t * t
            : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;

    case UIEaseType::EaseOutBack:
    {
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.0f;
        float f = t - 1.0f;
        return 1.0f + c3 * f * f * f + c1 * f * f;
    }

    case UIEaseType::EaseOutBounce:
    {
        if (t < 1.0f / 2.75f)
        {
            return 7.5625f * t * t;
        }
        else if (t < 2.0f / 2.75f)
        {
            float f = t - 1.5f / 2.75f;
            return 7.5625f * f * f + 0.75f;
        }
        else if (t < 2.5f / 2.75f)
        {
            float f = t - 2.25f / 2.75f;
            return 7.5625f * f * f + 0.9375f;
        }
        else
        {
            float f = t - 2.625f / 2.75f;
            return 7.5625f * f * f + 0.984375f;
        }
    }

    case UIEaseType::EaseOutElastic:
    {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        constexpr float c4 = (2.0f * PI) / 3.0f;
        return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
    }

    default:
        return t;
    }
}

// ============================================================================
// プロパティの適用・取得
// ============================================================================

void UITweenManager::ApplyValue(Widget* widget, UIAnimProperty prop, float value)
{
    if (!widget) return;

    switch (prop)
    {
    case UIAnimProperty::Opacity:
        widget->opacity = value;
        break;
    case UIAnimProperty::PositionX:
        widget->layoutRect.x = value;
        widget->globalRect.x = value;
        widget->layoutDirty = true;
        break;
    case UIAnimProperty::PositionY:
        widget->layoutRect.y = value;
        widget->globalRect.y = value;
        widget->layoutDirty = true;
        break;
    case UIAnimProperty::Width:
        widget->layoutRect.width = value;
        widget->globalRect.width = value;
        widget->layoutDirty = true;
        break;
    case UIAnimProperty::Height:
        widget->layoutRect.height = value;
        widget->globalRect.height = value;
        widget->layoutDirty = true;
        break;
    case UIAnimProperty::ScaleX:
        widget->computedStyle.scaleX = value;
        break;
    case UIAnimProperty::ScaleY:
        widget->computedStyle.scaleY = value;
        break;
    case UIAnimProperty::Rotation:
        widget->computedStyle.rotate = value;
        break;
    case UIAnimProperty::ColorR:
        widget->computedStyle.backgroundColor.r = value;
        break;
    case UIAnimProperty::ColorG:
        widget->computedStyle.backgroundColor.g = value;
        break;
    case UIAnimProperty::ColorB:
        widget->computedStyle.backgroundColor.b = value;
        break;
    case UIAnimProperty::ColorA:
        widget->computedStyle.backgroundColor.a = value;
        break;
    }
}

float UITweenManager::GetCurrentValue(Widget* widget, UIAnimProperty prop) const
{
    if (!widget) return 0.0f;

    switch (prop)
    {
    case UIAnimProperty::Opacity:    return widget->opacity;
    case UIAnimProperty::PositionX:  return widget->layoutRect.x;
    case UIAnimProperty::PositionY:  return widget->layoutRect.y;
    case UIAnimProperty::Width:      return widget->layoutRect.width;
    case UIAnimProperty::Height:     return widget->layoutRect.height;
    case UIAnimProperty::ScaleX:     return widget->computedStyle.scaleX;
    case UIAnimProperty::ScaleY:     return widget->computedStyle.scaleY;
    case UIAnimProperty::Rotation:   return widget->computedStyle.rotate;
    case UIAnimProperty::ColorR:     return widget->computedStyle.backgroundColor.r;
    case UIAnimProperty::ColorG:     return widget->computedStyle.backgroundColor.g;
    case UIAnimProperty::ColorB:     return widget->computedStyle.backgroundColor.b;
    case UIAnimProperty::ColorA:     return widget->computedStyle.backgroundColor.a;
    default: return 0.0f;
    }
}

}} // namespace gx::GUI
