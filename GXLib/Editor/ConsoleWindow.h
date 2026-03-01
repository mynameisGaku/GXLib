#pragma once
/// @file ConsoleWindow.h
/// @brief ログメッセージを表示するエディタ内コンソールウィンドウ
///
/// Info・Warning・Error の3レベルでメッセージを保持し、レベルごとにフィルタできる。
/// 同じメッセージは折りたたんでカウント表示する。
/// コマンド入力欄からデバッグコマンドを実行することもできる。
/// @addtogroup grp_editor/// @{

#include "pch_graphics.h"
#include "Core/Logger.h"
#include <functional>
#include <deque>

namespace gx
{

/// @brief 単一のコンソールメッセージ
struct ConsoleMessage
{
    std::string text;              ///< メッセージ本文
    LogLevel    level = LogLevel::Info; ///< ログレベル（Info/Warning/Error）
    int         count = 1;      ///< 重複メッセージの折りたたみカウント
    uint64_t    timestamp = 0;  ///< メッセージ追加時のシステムティック
};

/// @brief ログ出力の表示とコマンド受付を行うコンソールウィンドウ
class ConsoleWindow
{
public:
    ConsoleWindow() = default;
    ~ConsoleWindow() = default;

    /// @brief コンソールにメッセージを追加する
    void AddMessage(const std::string& text, LogLevel level = LogLevel::Info);

    /// @brief 全メッセージをクリアする
    void Clear();

    /// @brief フィルタ適用後の表示メッセージを取得する
    std::vector<const ConsoleMessage*> GetFilteredMessages() const;

    /// @brief 全メッセージを取得する
    const std::deque<ConsoleMessage>& GetAllMessages() const { return m_messages; }

    /// @brief 総メッセージ数を取得する
    int GetMessageCount() const { return static_cast<int>(m_messages.size()); }

    // --- フィルタ ---
    void SetShowInfo(bool v) { m_showInfo = v; }
    bool GetShowInfo() const { return m_showInfo; }
    void SetShowWarnings(bool v) { m_showWarnings = v; }
    bool GetShowWarnings() const { return m_showWarnings; }
    void SetShowErrors(bool v) { m_showErrors = v; }
    bool GetShowErrors() const { return m_showErrors; }

    /// @brief テキストフィルタを設定する（大文字小文字を区別しない部分文字列マッチ）
    void SetTextFilter(const std::string& filter) { m_textFilter = filter; }
    const std::string& GetTextFilter() const { return m_textFilter; }

    // --- コマンド入力 ---
    /// @brief コマンド文字列を送信する
    void SubmitCommand(const std::string& command);

    /// @brief コマンド送信時のコールバックを設定する
    using CommandCallback = std::function<void(const std::string&)>;
    void SetOnCommand(CommandCallback cb) { m_onCommand = std::move(cb); }

    // --- Logger連携 ---
    /// @brief Loggerに接続してログ出力を自動キャプチャする
    /// （各ログ行に対してAddMessageを呼び出すフックを登録する）
    void AttachToLogger();

    /// @brief Loggerから切断する
    void DetachFromLogger();

    /// @brief Loggerに接続中かチェックする
    bool IsAttachedToLogger() const { return m_attachedToLogger; }

    // --- 設定 ---
    /// @brief 保持する最大メッセージ数を設定する
    void SetMaxMessages(int max) { m_maxMessages = max; }
    int GetMaxMessages() const { return m_maxMessages; }

    /// @brief 重複メッセージの折りたたみを有効/無効にする
    void SetDuplicateFolding(bool v) { m_duplicateFolding = v; }
    bool IsDuplicateFolding() const { return m_duplicateFolding; }

    /// @brief レベル別のメッセージ数を取得する
    int GetInfoCount() const { return m_infoCount; }
    int GetWarningCount() const { return m_warningCount; }
    int GetErrorCount() const { return m_errorCount; }

private:
    std::deque<ConsoleMessage> m_messages;  ///< メッセージキュー
    int m_maxMessages = 1000;               ///< 保持する最大メッセージ数
    bool m_duplicateFolding = true;         ///< 重複メッセージの折りたたみ有効フラグ

    // フィルタ
    bool m_showInfo = true;                 ///< Infoレベルの表示フラグ
    bool m_showWarnings = true;             ///< Warningレベルの表示フラグ
    bool m_showErrors = true;               ///< Errorレベルの表示フラグ
    std::string m_textFilter;               ///< テキスト部分一致フィルタ

    // コマンド
    CommandCallback m_onCommand;            ///< コマンド送信時コールバック

    // ロガー
    bool m_attachedToLogger = false;        ///< Logger接続中フラグ

    // カウント
    int m_infoCount = 0;                    ///< Infoメッセージ数
    int m_warningCount = 0;                 ///< Warningメッセージ数
    int m_errorCount = 0;                   ///< Errorメッセージ数
};

} // namespace gx
/// @}
