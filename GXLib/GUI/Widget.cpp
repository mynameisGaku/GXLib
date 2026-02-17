#include "pch.h"
#include "GUI/Widget.h"
#include "GUI/UIRenderer.h"
#include "Math/Transform2D.h"

namespace GX { namespace GUI {

static Transform2D BuildLocalTransform(const Widget& widget, const Style& style)
{
    float tx = style.translateX;
    float ty = style.translateY;
    float sx = style.scaleX;
    float sy = style.scaleY;
    float rad = style.rotate * 0.0174532925f;

    float pivotX = widget.globalRect.x + widget.globalRect.width * style.pivotX;
    float pivotY = widget.globalRect.y + widget.globalRect.height * style.pivotY;

    Transform2D t = Transform2D::Identity();
    if (tx != 0.0f || ty != 0.0f)
        t = Multiply(t, Transform2D::Translation(tx, ty));
    t = Multiply(t, Transform2D::Translation(pivotX, pivotY));
    if (rad != 0.0f)
        t = Multiply(t, Transform2D::Rotation(rad));
    if (sx != 1.0f || sy != 1.0f)
        t = Multiply(t, Transform2D::Scale(sx, sy));
    t = Multiply(t, Transform2D::Translation(-pivotX, -pivotY));
    return t;
}

// ============================================================================
// ツリー構造
// ============================================================================

void Widget::AddChild(std::unique_ptr<Widget> child)
{
    child->m_parent = this;
    child->layoutDirty = true;
    m_children.push_back(std::move(child));
    layoutDirty = true;
}

void Widget::RemoveChild(Widget* child)
{
    for (auto it = m_children.begin(); it != m_children.end(); ++it)
    {
        if (it->get() == child)
        {
            child->m_parent = nullptr;
            m_children.erase(it);
            layoutDirty = true;
            return;
        }
    }
}

Widget* Widget::FindById(const std::string& searchId)
{
    if (id == searchId) return this;
    for (auto& child : m_children)
    {
        Widget* found = child->FindById(searchId);
        if (found) return found;
    }
    return nullptr;
}

// ============================================================================
// イベント
// ============================================================================

bool Widget::OnEvent(const UIEvent& event)
{
    if (onEvent) onEvent(event);

    switch (event.type)
    {
    case UIEventType::MouseEnter:
        if (onHover) onHover();
        break;
    case UIEventType::MouseLeave:
        if (onLeave) onLeave();
        break;
    case UIEventType::MouseDown:
        if (onPress) onPress();
        break;
    case UIEventType::MouseUp:
        if (onRelease) onRelease();
        break;
    case UIEventType::FocusGained:
        if (onFocus) onFocus();
        break;
    case UIEventType::FocusLost:
        if (onBlur) onBlur();
        break;
    case UIEventType::Submit:
        if (onSubmit) onSubmit();
        break;
    default:
        break;
    }
    if (event.type == UIEventType::MouseDown)
    {
        const Style& style = GetRenderStyle();
        if (style.effectType == UIEffectType::Ripple &&
            globalRect.width > 0.0f && globalRect.height > 0.0f)
        {
            m_effectActive = true;
            m_effectTime = 0.0f;
            m_effectCenterX = (event.localX - globalRect.x) / globalRect.width;
            m_effectCenterY = (event.localY - globalRect.y) / globalRect.height;
            m_effectCenterX = (std::max)(0.0f, (std::min)(1.0f, m_effectCenterX));
            m_effectCenterY = (std::max)(0.0f, (std::min)(1.0f, m_effectCenterY));
        }
    }
    return false;
}

// ============================================================================
// ライフサイクル
// ============================================================================

void Widget::Update(float deltaTime)
{
    UpdateStyleTransition(deltaTime, computedStyle);

    const Style& style = GetRenderStyle();
    if (style.effectType == UIEffectType::None)
        m_effectActive = false;
    if (m_effectActive)
    {
        float duration = style.effectDuration;
        if (duration <= 0.0f) duration = 0.8f;
        m_effectTime += deltaTime;
        if (m_effectTime >= duration)
            m_effectActive = false;
    }

    for (auto& child : m_children)
    {
        if (child->visible)
            child->Update(deltaTime);
    }
}

void Widget::Render(UIRenderer& renderer)
{
    const Style& drawStyle = GetRenderStyle();
    Transform2D local = BuildLocalTransform(*this, drawStyle);

    renderer.PushTransform(local);
    renderer.PushOpacity(opacity * drawStyle.opacity);
    RenderSelf(renderer);
    RenderChildren(renderer);
    renderer.PopOpacity();
    renderer.PopTransform();
}

void Widget::RenderChildren(UIRenderer& renderer)
{
    for (auto& child : m_children)
    {
        if (child->visible)
            child->Render(renderer);
    }
}

const UIRectEffect* Widget::GetActiveEffect(const Style& style, UIRectEffect& out) const
{
    if (style.effectType == UIEffectType::None || !m_effectActive)
        return nullptr;

    out.type = (style.effectType == UIEffectType::Ripple)
        ? UIRectEffectType::Ripple
        : UIRectEffectType::None;
    out.centerX = m_effectCenterX;
    out.centerY = m_effectCenterY;
    out.time = m_effectTime;
    out.duration = (style.effectDuration > 0.0f) ? style.effectDuration : 0.8f;
    out.strength = (style.effectStrength > 0.0f) ? style.effectStrength : 0.4f;
    out.width = (style.effectWidth > 0.0f) ? style.effectWidth : 0.08f;
    return &out;
}

void Widget::UpdateStyleTransition(float deltaTime, const Style& targetStyle)
{
    if (!m_hasRenderStyle)
    {
        m_renderStyle = targetStyle;
        m_targetStyle = targetStyle;
        m_startStyle = targetStyle;
        m_transitionTime = 0.0f;
        m_transitionDuration = 0.0f;
        m_hasRenderStyle = true;
        return;
    }

    bool visualChanged = !VisualEquals(m_targetStyle, targetStyle);
    m_targetStyle = targetStyle;

    if (visualChanged)
    {
        m_startStyle = m_renderStyle;
        m_transitionDuration = (std::max)(0.0f, targetStyle.transitionDuration);
        m_transitionTime = 0.0f;

        if (m_transitionDuration <= 0.0f)
        {
            m_renderStyle = targetStyle;
            return;
        }
    }

    if (m_transitionDuration <= 0.0f)
    {
        m_renderStyle = targetStyle;
        return;
    }

    m_transitionTime += deltaTime;
    float t = m_transitionTime / m_transitionDuration;
    if (t >= 1.0f)
    {
        m_renderStyle = targetStyle;
        m_transitionDuration = 0.0f;
        return;
    }

    // Smoothstep で少し滑らかに
    float eased = t * t * (3.0f - 2.0f * t);
    m_renderStyle = LerpVisual(m_startStyle, targetStyle, eased);
}

}} // namespace GX::GUI
