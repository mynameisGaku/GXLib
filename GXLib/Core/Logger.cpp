/// @file Logger.cpp
/// @brief ログシステム実装
#include "pch.h"
#include "Core/Logger.h"
#include <cstdio>
#include <cstdarg>

namespace GX
{

void Logger::Info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Log(LogLevel::Info, format, args);
    va_end(args);
}

void Logger::Warn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Log(LogLevel::Warn, format, args);
    va_end(args);
}

void Logger::Error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Log(LogLevel::Error, format, args);
    va_end(args);
}

void Logger::Log(LogLevel level, const char* format, va_list args)
{
    // ログレベルに応じたプレフィックス
    const char* prefix = "";
    switch (level)
    {
    case LogLevel::Info:  prefix = "[INFO]  "; break;
    case LogLevel::Warn:  prefix = "[WARN]  "; break;
    case LogLevel::Error: prefix = "[ERROR] "; break;
    }

    // メッセージをフォーマット
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);

    // 最終出力文字列を構築
    char output[2048 + 64];
    snprintf(output, sizeof(output), "%s%s\n", prefix, buffer);

    // Visual Studioの出力ウィンドウに表示
    OutputDebugStringA(output);

    // コンソールにも表示
    printf("%s", output);
}

} // namespace GX
