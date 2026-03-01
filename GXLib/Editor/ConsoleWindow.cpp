/// @file ConsoleWindow.cpp
/// @brief 対応する.hの実装
#include "pch_graphics.h"

#include "Editor/ConsoleWindow.h"

namespace gx
{

namespace
{
    /// @brief 大文字小文字を区別しない部分文字列検索
    bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needle)
    {
        if (needle.empty())
            return true;
        if (haystack.size() < needle.size())
            return false;

        auto toLower = [](char ch) -> char
        {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        };

        auto it = std::search(
            haystack.begin(), haystack.end(),
            needle.begin(), needle.end(),
            [&](char a, char b) { return toLower(a) == toLower(b); });

        return it != haystack.end();
    }
} // anonymous namespace

// ============================================================================
// AddMessage
// ============================================================================

void ConsoleWindow::AddMessage(const std::string& text, LogLevel level)
{
    // 重複折りたたみ: 有効かつ最後のメッセージと一致する場合、カウントを加算
    if (m_duplicateFolding && !m_messages.empty())
    {
        ConsoleMessage& last = m_messages.back();
        if (last.text == text && last.level == level)
        {
            last.count++;
            last.timestamp = ::GetTickCount64();
            return;
        }
    }

    // 新しいメッセージを作成
    ConsoleMessage msg;
    msg.text      = text;
    msg.level     = level;
    msg.count     = 1;
    msg.timestamp = ::GetTickCount64();

    m_messages.push_back(std::move(msg));

    // レベル別カウントを更新
    switch (level)
    {
    case LogLevel::Info:  ++m_infoCount;    break;
    case LogLevel::Warn:  ++m_warningCount; break;
    case LogLevel::Error: ++m_errorCount;   break;
    }

    // 最大メッセージ数の制限を適用
    while (static_cast<int>(m_messages.size()) > m_maxMessages && m_maxMessages > 0)
    {
        // 削除されるメッセージのレベル別カウントを減算
        const ConsoleMessage& front = m_messages.front();
        switch (front.level)
        {
        case LogLevel::Info:  m_infoCount    = (m_infoCount > 0)    ? m_infoCount - 1    : 0; break;
        case LogLevel::Warn:  m_warningCount = (m_warningCount > 0) ? m_warningCount - 1 : 0; break;
        case LogLevel::Error: m_errorCount   = (m_errorCount > 0)   ? m_errorCount - 1   : 0; break;
        }
        m_messages.pop_front();
    }
}

// ============================================================================
// Clear
// ============================================================================

void ConsoleWindow::Clear()
{
    m_messages.clear();
    m_infoCount    = 0;
    m_warningCount = 0;
    m_errorCount   = 0;
}

// ============================================================================
// GetFilteredMessages
// ============================================================================

std::vector<const ConsoleMessage*> ConsoleWindow::GetFilteredMessages() const
{
    std::vector<const ConsoleMessage*> result;
    result.reserve(m_messages.size());

    for (const auto& msg : m_messages)
    {
        // レベルでフィルタリング
        switch (msg.level)
        {
        case LogLevel::Info:
            if (!m_showInfo) continue;
            break;
        case LogLevel::Warn:
            if (!m_showWarnings) continue;
            break;
        case LogLevel::Error:
            if (!m_showErrors) continue;
            break;
        }

        // テキストでフィルタリング
        if (!m_textFilter.empty())
        {
            if (!ContainsCaseInsensitive(msg.text, m_textFilter))
                continue;
        }

        result.push_back(&msg);
    }

    return result;
}

// ============================================================================
// SubmitCommand
// ============================================================================

void ConsoleWindow::SubmitCommand(const std::string& command)
{
    if (command.empty())
        return;

    // コマンドをInfoメッセージとしてエコー表示
    AddMessage("> " + command, LogLevel::Info);

    // コマンドコールバックを発火
    if (m_onCommand)
    {
        m_onCommand(command);
    }
}

// ============================================================================
// AttachToLogger / DetachFromLogger
// ============================================================================

void ConsoleWindow::AttachToLogger()
{
    // LoggerのAPIは現在カスタムフックをサポートしていないため、
    // 機能マーカーとしてフラグを設定する。Logger出力を駆動する外部コードは
    // IsAttachedToLogger()を確認し、AddMessage()経由でこのConsoleWindowに
    // メッセージを転送できる。
    m_attachedToLogger = true;
}

void ConsoleWindow::DetachFromLogger()
{
    m_attachedToLogger = false;
}

} // namespace gx
