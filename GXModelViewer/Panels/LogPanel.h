#pragma once
/// @file LogPanel.h
/// @brief ImGui panel for scrollable log output with level filtering

#include <string>
#include <vector>
#include <cstdarg>

/// Log severity level
enum class LogLevel
{
    Info,
    Warning,
    Error,
};

/// Single log entry
struct LogEntry
{
    LogLevel    level;
    std::string message;
};

/// @brief ImGui panel that displays a scrollable, filterable log
class LogPanel
{
public:
    /// Add a formatted log message at Info level.
    void AddLog(const char* fmt, ...);

    /// Add a log message with a specific level.
    void AddLogLevel(LogLevel level, const char* fmt, ...);

    /// Draw the log panel.
    void Draw();

    /// Clear all log entries.
    void Clear();

private:
    void AddLogV(LogLevel level, const char* fmt, va_list args);

    std::vector<LogEntry> m_entries;
    bool m_autoScroll   = true;
    bool m_showInfo     = true;
    bool m_showWarning  = true;
    bool m_showError    = true;
};
