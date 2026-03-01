/// @file Accessibility.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"
#include "GUI/Accessibility.h"

#include <algorithm>

namespace gx { namespace GUI {

// ============================================================================
// Element management
// ============================================================================

uint32_t AccessibilityManager::RegisterElement(const AccessibleElement& element)
{
    AccessibleElement copy = element;
    if (copy.id == 0)
    {
        copy.id = m_nextElementId++;
    }
    else
    {
        if (copy.id >= m_nextElementId)
            m_nextElementId = copy.id + 1;
    }

    uint32_t id = copy.id;

    // If this element has a parent, add it to the parent's child list
    if (copy.parentId != 0)
    {
        auto parentIt = m_elements.find(copy.parentId);
        if (parentIt != m_elements.end())
        {
            auto& childIds = parentIt->second.childIds;
            if (std::find(childIds.begin(), childIds.end(), id) == childIds.end())
                childIds.push_back(id);
        }
    }

    m_elements[id] = std::move(copy);
    return id;
}

bool AccessibilityManager::UnregisterElement(uint32_t elementId)
{
    auto it = m_elements.find(elementId);
    if (it == m_elements.end())
        return false;

    // Remove from parent's child list
    uint32_t parentId = it->second.parentId;
    if (parentId != 0)
    {
        auto parentIt = m_elements.find(parentId);
        if (parentIt != m_elements.end())
        {
            auto& childIds = parentIt->second.childIds;
            childIds.erase(std::remove(childIds.begin(), childIds.end(), elementId), childIds.end());
        }
    }

    // If this was the focused element, clear focus
    if (m_focusedElementId == elementId)
        m_focusedElementId = 0;

    m_elements.erase(it);
    return true;
}

const AccessibleElement* AccessibilityManager::GetElement(uint32_t elementId) const
{
    auto it = m_elements.find(elementId);
    if (it == m_elements.end())
        return nullptr;
    return &it->second;
}

size_t AccessibilityManager::GetElementCount() const
{
    return m_elements.size();
}

bool AccessibilityManager::HasElement(uint32_t elementId) const
{
    return m_elements.find(elementId) != m_elements.end();
}

bool AccessibilityManager::UpdateElement(uint32_t elementId, const AccessibilityState& state)
{
    auto it = m_elements.find(elementId);
    if (it == m_elements.end())
        return false;
    it->second.state = state;
    return true;
}

// ============================================================================
// Focus management
// ============================================================================

bool AccessibilityManager::SetFocus(uint32_t elementId)
{
    // Clear previous focus
    if (m_focusedElementId != 0)
    {
        auto prevIt = m_elements.find(m_focusedElementId);
        if (prevIt != m_elements.end())
            prevIt->second.state.isFocused = false;
    }

    auto it = m_elements.find(elementId);
    if (it == m_elements.end())
    {
        m_focusedElementId = 0;
        return false;
    }

    m_focusedElementId = elementId;
    it->second.state.isFocused = true;

    // Announce focus change if screen reader is enabled
    if (m_config.enableScreenReader && !it->second.label.empty())
    {
        ScreenReaderEvent event;
        event.elementId = elementId;
        event.announcement = it->second.label;
        event.priority = AnnouncementPriority::Medium;
        m_pendingAnnouncements.push_back(std::move(event));
    }

    return true;
}

uint32_t AccessibilityManager::GetFocusedElement() const
{
    return m_focusedElementId;
}

bool AccessibilityManager::HasFocus() const
{
    return m_focusedElementId != 0;
}

bool AccessibilityManager::NavigateFocus(FocusNavigationDir direction)
{
    if (!m_config.enableKeyboardNav)
        return false;

    auto tabOrder = GetTabOrder();
    if (tabOrder.empty())
        return false;

    if (m_focusedElementId == 0)
    {
        // Nothing focused, focus the first element
        return SetFocus(tabOrder.front());
    }

    // Find current position in tab order
    int currentIndex = -1;
    for (int i = 0; i < static_cast<int>(tabOrder.size()); ++i)
    {
        if (tabOrder[i] == m_focusedElementId)
        {
            currentIndex = i;
            break;
        }
    }

    if (currentIndex < 0)
    {
        // Current focus not in tab order, focus first element
        return SetFocus(tabOrder.front());
    }

    int nextIndex = currentIndex;

    switch (direction)
    {
    case FocusNavigationDir::Next:
    case FocusNavigationDir::Down:
    case FocusNavigationDir::Right:
        nextIndex = (currentIndex + 1) % static_cast<int>(tabOrder.size());
        break;
    case FocusNavigationDir::Previous:
    case FocusNavigationDir::Up:
    case FocusNavigationDir::Left:
        nextIndex = (currentIndex - 1 + static_cast<int>(tabOrder.size())) % static_cast<int>(tabOrder.size());
        break;
    }

    return SetFocus(tabOrder[nextIndex]);
}

bool AccessibilityManager::ActivateFocused()
{
    if (m_focusedElementId == 0)
        return false;

    auto it = m_elements.find(m_focusedElementId);
    if (it == m_elements.end())
        return false;

    if (!it->second.state.isEnabled)
        return false;

    m_activatedThisFrame = true;

    // Announce activation
    if (m_config.enableScreenReader)
    {
        ScreenReaderEvent event;
        event.elementId = m_focusedElementId;
        event.announcement = it->second.label + " activated";
        event.priority = AnnouncementPriority::High;
        m_pendingAnnouncements.push_back(std::move(event));
    }

    return true;
}

// ============================================================================
// Screen reader
// ============================================================================

void AccessibilityManager::Announce(const std::string& text, AnnouncementPriority priority)
{
    ScreenReaderEvent event;
    event.elementId = 0;
    event.announcement = text;
    event.priority = priority;
    m_pendingAnnouncements.push_back(std::move(event));
}

std::vector<ScreenReaderEvent> AccessibilityManager::GetPendingAnnouncements() const
{
    return m_pendingAnnouncements;
}

void AccessibilityManager::ClearAnnouncements()
{
    m_pendingAnnouncements.clear();
}

// ============================================================================
// Tab order
// ============================================================================

std::vector<uint32_t> AccessibilityManager::GetTabOrder() const
{
    std::vector<std::pair<int, uint32_t>> sortable;
    for (const auto& [id, elem] : m_elements)
    {
        if (elem.state.isEnabled)
            sortable.push_back({ elem.tabOrder, id });
    }

    std::sort(sortable.begin(), sortable.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first)
            return a.first < b.first;
        return a.second < b.second;  // Stable sort by ID for same tab order
    });

    std::vector<uint32_t> result;
    result.reserve(sortable.size());
    for (const auto& [order, id] : sortable)
        result.push_back(id);
    return result;
}

bool AccessibilityManager::SetTabOrder(uint32_t elementId, int order)
{
    auto it = m_elements.find(elementId);
    if (it == m_elements.end())
        return false;
    it->second.tabOrder = order;
    return true;
}

// ============================================================================
// Tree
// ============================================================================

std::vector<AccessibleElement> AccessibilityManager::GetAccessibleTree() const
{
    std::vector<AccessibleElement> result;
    result.reserve(m_elements.size());
    for (const auto& [id, elem] : m_elements)
        result.push_back(elem);
    return result;
}

std::vector<uint32_t> AccessibilityManager::FindByLabel(const std::string& label) const
{
    std::vector<uint32_t> result;
    for (const auto& [id, elem] : m_elements)
    {
        if (elem.label == label)
            result.push_back(id);
    }
    return result;
}

// ============================================================================
// Visual accessibility
// ============================================================================

HighContrastColors AccessibilityManager::GetHighContrastColors() const
{
    HighContrastColors colors;
    if (m_config.enableHighContrast)
    {
        colors.fg = 0xFFFFFFFF;      // White on black
        colors.bg = 0xFF000000;
        colors.accent = 0xFF00FFFF;  // Cyan accent
        colors.focus = m_config.focusIndicatorColor;
    }
    else
    {
        colors.fg = 0xFF333333;      // Dark gray
        colors.bg = 0xFFFFFFFF;      // White
        colors.accent = 0xFF0078D7;  // Blue accent
        colors.focus = m_config.focusIndicatorColor;
    }
    return colors;
}

bool AccessibilityManager::ShouldReduceMotion() const
{
    return m_config.enableReducedMotion;
}

float AccessibilityManager::GetFontScale() const
{
    return m_config.fontScaleFactor;
}

void AccessibilityManager::Clear()
{
    m_elements.clear();
    m_pendingAnnouncements.clear();
    m_focusedElementId = 0;
    m_nextElementId = 1;
    m_activatedThisFrame = false;
}

}} // namespace gx::GUI
