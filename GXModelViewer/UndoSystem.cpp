/// @file UndoSystem.cpp
/// @brief Undo/Redo システム実装

#include "UndoSystem.h"

void UndoSystem::Execute(std::unique_ptr<ICommand> cmd)
{
    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.clear();

    // Limit stack size
    if (m_undoStack.size() > k_MaxUndoLevels)
        m_undoStack.erase(m_undoStack.begin());
}

void UndoSystem::Undo()
{
    if (m_undoStack.empty()) return;
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    cmd->Undo();
    m_redoStack.push_back(std::move(cmd));
}

void UndoSystem::Redo()
{
    if (m_redoStack.empty()) return;
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    cmd->Execute();
    m_undoStack.push_back(std::move(cmd));
}

std::string UndoSystem::GetUndoDescription() const
{
    if (m_undoStack.empty()) return "";
    return m_undoStack.back()->GetDescription();
}

std::string UndoSystem::GetRedoDescription() const
{
    if (m_redoStack.empty()) return "";
    return m_redoStack.back()->GetDescription();
}

void UndoSystem::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}
