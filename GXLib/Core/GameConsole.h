#pragma once
/// @file GameConsole.h
/// @brief ゲーム内コンソール（REPL）— コマンド登録・実行・CVar管理
///
/// 開発中にコマンドラインからエンジン設定を変更したり、
/// デバッグコマンドを実行するためのインタラクティブコンソール。
/// RegisterCommand() でカスタムコマンドを追加し、
/// Execute() で文字列入力を処理する。
/// @addtogroup grp_core/// @{

#include "pch_common.h"
#include <functional>
#include <unordered_map>

namespace gx
{

/// @brief コンソールコマンド定義
struct ConsoleCommand
{
    std::string name;        ///< コマンド名（小文字推奨）
    std::string description; ///< コマンドの説明
    std::string usage;       ///< 使用法の例
    std::function<std::string(const std::vector<std::string>& args)> handler; ///< 実行ハンドラ
};

/// @brief コンソール履歴エントリ
struct ConsoleEntry
{
    /// @brief エントリの種別
    enum Type
    {
        Input,   ///< ユーザー入力
        Output,  ///< コマンド出力
        Error,   ///< エラーメッセージ
        Warning, ///< 警告メッセージ
        Info     ///< 情報メッセージ
    };
    Type        type; ///< エントリ種別
    std::string text; ///< テキスト内容
};

/// @brief ゲーム内コンソール（REPL）
class GameConsole
{
public:
    // ----- コマンド管理 -----

    /// @brief コマンドを登録する
    /// @param cmd コマンド定義
    void RegisterCommand(const ConsoleCommand& cmd);

    /// @brief コマンドの登録を解除する
    /// @param name 解除するコマンド名
    void UnregisterCommand(const std::string& name);

    /// @brief コマンドが登録済みか問い合わせる
    /// @param name コマンド名
    /// @return 登録済みなら true
    bool HasCommand(const std::string& name) const;

    /// @brief 登録コマンド数を返す
    /// @return コマンド数
    size_t GetCommandCount() const;

    /// @brief 登録コマンド名の一覧を返す
    /// @return コマンド名の配列
    std::vector<std::string> GetCommandNames() const;

    // ----- 実行 -----

    /// @brief 入力文字列を解析して対応コマンドを実行する
    /// @param input 入力文字列（コマンド名＋引数）
    /// @return 実行結果の文字列
    std::string Execute(const std::string& input);

    // ----- 自動補完 -----

    /// @brief 部分文字列に一致するコマンド名の候補を返す
    /// @param partial 入力中の部分文字列
    /// @return 一致するコマンド名の配列
    std::vector<std::string> GetAutoComplete(const std::string& partial) const;

    // ----- 履歴 -----

    /// @brief 全履歴エントリを返す
    /// @return 履歴エントリ配列への参照
    const std::vector<ConsoleEntry>& GetHistory() const;

    /// @brief 現在の履歴サイズを返す
    /// @return 履歴エントリ数
    size_t GetHistorySize() const;

    /// @brief 最大履歴数を設定する
    /// @param max 最大履歴エントリ数
    void SetMaxHistory(size_t max);

    /// @brief 履歴をクリアする
    void ClearHistory();

    // ----- CVar -----

    /// @brief コンソール変数を設定する
    /// @param name 変数名
    /// @param value 変数値
    void SetCVar(const std::string& name, const std::string& value);

    /// @brief コンソール変数を取得する（未登録時は defaultValue を返す）
    /// @param name 変数名
    /// @param defaultValue 未登録時の既定値
    /// @return 変数値
    std::string GetCVar(const std::string& name, const std::string& defaultValue = "") const;

    /// @brief コンソール変数が存在するか
    /// @param name 変数名
    /// @return 存在すれば true
    bool HasCVar(const std::string& name) const;

    // ----- ビルトインコマンド -----

    /// @brief 基本コマンド (help, echo, set, get, clear) を登録する
    void RegisterBuiltinCommands();

    // ----- 有効/無効 -----

    /// @brief コンソールの有効/無効を切り替える
    /// @param enabled true で有効
    void SetEnabled(bool enabled);

    /// @brief コンソールが有効かどうかを返す
    /// @return 有効なら true
    bool IsEnabled() const;

private:
    std::unordered_map<std::string, ConsoleCommand> m_commands;  ///< 登録コマンドマップ
    std::unordered_map<std::string, std::string>    m_cvars;     ///< コンソール変数マップ
    std::vector<ConsoleEntry>                       m_history;   ///< 出力履歴
    size_t m_maxHistory = 1000;                                  ///< 最大履歴数
    bool   m_enabled    = true;                                  ///< 有効フラグ

    /// @brief 入力文字列を空白区切りでトークン化する（引用符対応）
    std::vector<std::string> ParseArgs(const std::string& input) const;

    /// @brief 履歴にエントリを追加する（最大数を超えたら先頭を削除）
    void AddHistoryEntry(ConsoleEntry::Type type, const std::string& text);
};

} // namespace gx
/// @}
