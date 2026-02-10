#include "pch.h"
#include "GUI/Widget.h"
#include "GUI/UIRenderer.h"

namespace GX { namespace GUI {

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
    return false;
}

// ============================================================================
// ライフサイクル
// ============================================================================

void Widget::Update(float deltaTime)
{
    UpdateStyleTransition(deltaTime, computedStyle);
    for (auto& child : m_children)
    {
        if (child->visible)
            child->Update(deltaTime);
    }
}

void Widget::Render(UIRenderer& renderer)
{
    for (auto& child : m_children)
    {
        if (child->visible)
            child->Render(renderer);
    }
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
