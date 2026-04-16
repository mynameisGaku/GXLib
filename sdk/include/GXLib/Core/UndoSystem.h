#pragma once
/// @file UndoSystem.h
/// @brief アンドゥ/リドゥシステム（コマンドパターン）
///
/// ICommand インターフェースでアクションを定義し、
/// UndoSystem で実行・アンドゥ・リドゥを管理する。
/// ValueCommand テンプレートで値の変更を簡単にコマンド化可能。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

namespace gx
{

/// @brief アンドゥ可能なコマンドの基底インターフェース
class ICommand
{
public:
    virtual ~ICommand() = default;

    /// @brief コマンドを実行する
    virtual void Execute() = 0;

    /// @brief コマンドを元に戻す
    virtual void Undo() = 0;

    /// @brief コマンドの説明を取得する
    /// @return コマンドの説明文字列
    virtual gx::String GetDescription() const = 0;
};

/// @brief 値の変更をコマンド化するテンプレート
/// @tparam T 値の型
template<typename T>
class ValueCommand : public ICommand
{
public:
    /// @brief コンストラクタ
    /// @param description コマンドの説明
    /// @param oldValue 変更前の値
    /// @param newValue 変更後の値
    /// @param setter 値を適用する関数
    ValueCommand(gx::String description, T oldValue, T newValue,
                 std::function<void(const T&)> setter)
        : m_description(std::move(description))
        , m_oldValue(std::move(oldValue))
        , m_newValue(std::move(newValue))
        , m_setter(std::move(setter))
    {
    }

    void Execute() override { m_setter(m_newValue); }
    void Undo() override { m_setter(m_oldValue); }
    gx::String GetDescription() const override { return m_description; }

private:
    gx::String m_description;            ///< コマンドの説明
    T m_oldValue;                         ///< 変更前の値
    T m_newValue;                         ///< 変更後の値
    std::function<void(const T&)> m_setter; ///< 値適用関数
};

/// @brief アンドゥ/リドゥスタック管理
class UndoSystem
{
public:
    /// @brief コマンドを実行し、アンドゥスタックに積む
    /// @param command 実行するコマンド
    void Execute(std::unique_ptr<ICommand> command);

    /// @brief 直前のコマンドを元に戻す
    /// @return アンドゥが実行された場合 true
    bool Undo();

    /// @brief 直前にアンドゥしたコマンドをやり直す
    /// @return リドゥが実行された場合 true
    bool Redo();

    /// @brief アンドゥ/リドゥ履歴をクリアする
    void Clear();

    /// @brief アンドゥ可能かどうか
    /// @return アンドゥスタックが空でなければ true
    bool CanUndo() const { return !m_undoStack.empty(); }

    /// @brief リドゥ可能かどうか
    /// @return リドゥスタックが空でなければ true
    bool CanRedo() const { return !m_redoStack.empty(); }

    /// @brief アンドゥスタックのサイズ
    /// @return アンドゥ可能なコマンド数
    size_t GetUndoCount() const { return m_undoStack.size(); }

    /// @brief リドゥスタックのサイズ
    /// @return リドゥ可能なコマンド数
    size_t GetRedoCount() const { return m_redoStack.size(); }

    /// @brief 次にアンドゥされるコマンドの説明
    /// @return 説明文字列（スタック空の場合は空文字列）
    gx::String GetUndoDescription() const;

    /// @brief 次にリドゥされるコマンドの説明
    /// @return 説明文字列（スタック空の場合は空文字列）
    gx::String GetRedoDescription() const;

    /// @brief 最大履歴数を設定する（デフォルト: 100）
    /// @param maxLevels 最大レベル数
    void SetMaxHistory(size_t maxLevels) { m_maxHistory = maxLevels; }

private:
    gx::Vector<std::unique_ptr<ICommand>> m_undoStack; ///< アンドゥスタック
    gx::Vector<std::unique_ptr<ICommand>> m_redoStack; ///< リドゥスタック
    size_t m_maxHistory = 100;                          ///< 最大履歴保持数
};

} // namespace gx
/// @}
