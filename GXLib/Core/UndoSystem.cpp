/// @file UndoSystem.cpp
/// @brief アンドゥ/リドゥシステム実装
#include "pch_common.h"
#include "Core/UndoSystem.h"

namespace gx
{

void UndoSystem::Execute(std::unique_ptr<ICommand> command)
{
    command->Execute();
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();

    // 最大履歴数を超えたら古いものを削除
    while (m_undoStack.size() > m_maxHistory)
    {
        m_undoStack.erase(m_undoStack.begin());
    }
}

bool UndoSystem::Undo()
{
    if (m_undoStack.empty()) return false;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->Undo();
    m_redoStack.push_back(std::move(cmd));
    return true;
}

bool UndoSystem::Redo()
{
    if (m_redoStack.empty()) return false;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));
    return true;
}

void UndoSystem::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

gx::String UndoSystem::GetUndoDescription() const
{
    if (m_undoStack.empty()) return {};
    return m_undoStack.back()->GetDescription();
}

gx::String UndoSystem::GetRedoDescription() const
{
    if (m_redoStack.empty()) return {};
    return m_redoStack.back()->GetDescription();
}

} // namespace gx
