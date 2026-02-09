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
    return false;
}

// ============================================================================
// ライフサイクル
// ============================================================================

void Widget::Update(float deltaTime)
{
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

}} // namespace GX::GUI
