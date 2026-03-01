#include "pch_graphics.h"
/// @file DragDropManager.cpp
/// @brief GUI ドラッグ&ドロップマネージャー実装

#include "GUI/DragDropManager.h"
#include "Core/Logger.h"

namespace gx { namespace GUI {

// ============================================================================
// RegisterDragSource
// ============================================================================
void DragDropManager::RegisterDragSource(Widget* widget, const std::string& payloadType)
{
    if (!widget) return;
    m_sources[widget] = payloadType;
}

// ============================================================================
// RegisterDropTarget
// ============================================================================
void DragDropManager::RegisterDropTarget(Widget* widget, const DropTarget& target)
{
    if (!widget) return;
    m_targets[widget] = target;
}

// ============================================================================
// UnregisterDragSource
// ============================================================================
void DragDropManager::UnregisterDragSource(Widget* widget)
{
    if (!widget) return;

    m_sources.erase(widget);

    // このウィジェットが現在ドラッグ中の場合、ドラッグをキャンセル
    if (m_dragSource.sourceWidget == widget && m_dragSource.dragging)
    {
        CancelDrag();
    }
}

// ============================================================================
// UnregisterDropTarget
// ============================================================================
void DragDropManager::UnregisterDropTarget(Widget* widget)
{
    if (!widget) return;

    m_targets.erase(widget);

    // 現在のドロップターゲットが削除される場合、クリアする
    if (m_currentTarget == widget)
    {
        m_currentTarget = nullptr;
    }
}

// ============================================================================
// BeginDrag
// ============================================================================
void DragDropManager::BeginDrag(Widget* source, const DragPayload& payload, float x, float y)
{
    if (!source) return;

    // 進行中のドラッグをキャンセル
    if (m_dragSource.dragging)
    {
        CancelDrag();
    }

    m_dragSource.sourceWidget = source;
    m_dragSource.payload = payload;
    m_dragSource.startX = x;
    m_dragSource.startY = y;
    m_dragSource.currentX = x;
    m_dragSource.currentY = y;
    m_dragSource.dragging = true;

    m_currentTarget = nullptr;
}

// ============================================================================
// UpdateDrag
// ============================================================================
void DragDropManager::UpdateDrag(float x, float y)
{
    if (!m_dragSource.dragging) return;

    m_dragSource.currentX = x;
    m_dragSource.currentY = y;

    // カーソル下のドロップターゲットを検索
    Widget* newTarget = FindDropTargetAt(x, y);

    // ターゲット遷移を処理（進入/退出）
    if (newTarget != m_currentTarget)
    {
        // 旧ターゲットから退出
        if (m_currentTarget)
        {
            auto it = m_targets.find(m_currentTarget);
            if (it != m_targets.end() && it->second.onDragLeave)
            {
                it->second.onDragLeave();
            }
        }

        // 新ターゲットに進入
        if (newTarget)
        {
            auto it = m_targets.find(newTarget);
            if (it != m_targets.end() && it->second.onDragEnter)
            {
                it->second.onDragEnter();
            }
        }

        m_currentTarget = newTarget;
    }
}

// ============================================================================
// EndDrag
// ============================================================================
void DragDropManager::EndDrag(float x, float y)
{
    if (!m_dragSource.dragging) return;

    m_dragSource.currentX = x;
    m_dragSource.currentY = y;

    // リリース位置のターゲットを検索
    Widget* target = FindDropTargetAt(x, y);

    if (target)
    {
        auto it = m_targets.find(target);
        if (it != m_targets.end())
        {
            const DropTarget& dt = it->second;

            // 型の受け入れチェック
            bool typeAccepted = dt.acceptType.empty() ||
                                dt.acceptType == m_dragSource.payload.type;

            // カスタム受け入れチェック
            bool customAccepted = true;
            if (dt.canAccept)
            {
                customAccepted = dt.canAccept(m_dragSource.payload);
            }

            if (typeAccepted && customAccepted)
            {
                // ドロップを実行
                if (dt.onDrop)
                {
                    dt.onDrop(m_dragSource.payload);
                }
            }
        }
    }

    // 現在のターゲットから退出
    if (m_currentTarget)
    {
        auto it = m_targets.find(m_currentTarget);
        if (it != m_targets.end() && it->second.onDragLeave)
        {
            it->second.onDragLeave();
        }
    }

    // ドラッグ状態をリセット
    m_dragSource.sourceWidget = nullptr;
    m_dragSource.payload = {};
    m_dragSource.startX = 0;
    m_dragSource.startY = 0;
    m_dragSource.currentX = 0;
    m_dragSource.currentY = 0;
    m_dragSource.dragging = false;

    m_currentTarget = nullptr;
}

// ============================================================================
// CancelDrag
// ============================================================================
void DragDropManager::CancelDrag()
{
    if (!m_dragSource.dragging) return;

    // ドロップせずに現在のターゲットから退出
    if (m_currentTarget)
    {
        auto it = m_targets.find(m_currentTarget);
        if (it != m_targets.end() && it->second.onDragLeave)
        {
            it->second.onDragLeave();
        }
    }

    // ドラッグ状態をリセット
    m_dragSource.sourceWidget = nullptr;
    m_dragSource.payload = {};
    m_dragSource.startX = 0;
    m_dragSource.startY = 0;
    m_dragSource.currentX = 0;
    m_dragSource.currentY = 0;
    m_dragSource.dragging = false;

    m_currentTarget = nullptr;
}

// ============================================================================
// Clear
// ============================================================================
void DragDropManager::Clear()
{
    if (m_dragSource.dragging)
    {
        CancelDrag();
    }

    m_sources.clear();
    m_targets.clear();
    m_currentTarget = nullptr;
    m_dragPreview = nullptr;
}

// ============================================================================
// FindDropTargetAt
// ============================================================================
Widget* DragDropManager::FindDropTargetAt(float x, float y) const
{
    // ドラッグソース自身へのドロップは許可しない
    Widget* bestTarget = nullptr;

    for (const auto& [widget, target] : m_targets)
    {
        if (!widget) continue;
        if (!widget->visible) continue;
        if (!widget->enabled) continue;

        // ソースウィジェットの場合はスキップ
        if (widget == m_dragSource.sourceWidget) continue;

        // カーソルがウィジェットのグローバル矩形内にあるか確認
        if (widget->globalRect.Contains(x, y))
        {
            // 複数のターゲットが重なる場合、zIndexが最も高いものを優先
            if (!bestTarget || widget->zIndex > bestTarget->zIndex)
            {
                bestTarget = widget;
            }
        }
    }

    return bestTarget;
}

}} // namespace gx::GUI
