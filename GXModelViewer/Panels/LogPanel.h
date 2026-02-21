#pragma once
/// @file LogPanel.h
/// @brief スクロール可能なログ出力パネル（レベルフィルタ付き）
///
/// Info/Warning/Errorの3レベルでログを記録し、トグルボタンでフィルタリングできる。
/// 自動スクロール、クリア機能付き。

#include <string>
#include <vector>
#include <cstdarg>

/// @brief ログの重要度レベル
enum class LogLevel
{
    Info,      ///< 通常の情報
    Warning,   ///< 警告（処理は続行可能）
    Error,     ///< エラー（処理に失敗）
};

/// @brief 1件のログエントリ
struct LogEntry
{
    LogLevel    level;    ///< 重要度
    std::string message;  ///< メッセージ本文
};

/// @brief レベルフィルタ・自動スクロール付きログ表示パネル
class LogPanel
{
public:
    /// @brief Infoレベルでフォーマット付きログを追加する
    /// @param fmt printf形式のフォーマット文字列
    void AddLog(const char* fmt, ...);

    /// @brief 指定レベルでフォーマット付きログを追加する
    /// @param level ログレベル
    /// @param fmt printf形式のフォーマット文字列
    void AddLogLevel(LogLevel level, const char* fmt, ...);

    /// @brief ログパネルを描画する
    void Draw();

    /// @brief 全ログエントリを消去する
    void Clear();

private:
    /// @brief 可変引数リスト版の内部ログ追加
    void AddLogV(LogLevel level, const char* fmt, va_list args);

    std::vector<LogEntry> m_entries;
    bool m_autoScroll   = true;   ///< 新規ログ追加時に自動スクロールするか
    bool m_showInfo     = true;   ///< Infoレベル表示ON/OFF
    bool m_showWarning  = true;   ///< Warningレベル表示ON/OFF
    bool m_showError    = true;   ///< Errorレベル表示ON/OFF
};
