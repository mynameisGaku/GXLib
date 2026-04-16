#pragma once
/// @file UndoSystem.h
/// @brief コマンドパターンによる Undo/Redo システム

#include <string>
#include <vector>
#include <memory>
#include <functional>

/// @brief 操作コマンドの抽象インターフェース
class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

/// @brief Undo/Redo スタック管理
class UndoSystem
{
public:
    /// @brief コマンドを実行してスタックにプッシュする
    void Execute(std::unique_ptr<ICommand> cmd);

    /// @brief 最後のコマンドを取り消す
    void Undo();

    /// @brief 最後の Undo をやり直す
    void Redo();

    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }

    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;

    void Clear();

private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack; ///< Undoスタック
    std::vector<std::unique_ptr<ICommand>> m_redoStack; ///< Redoスタック
    static constexpr int k_MaxUndoLevels = 100;
};
