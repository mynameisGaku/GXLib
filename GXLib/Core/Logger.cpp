/// @file Logger.cpp
/// @brief Logger クラスの実装
#include "pch.h"
#include "Core/Logger.h"
#include <cstdio>
#include <cstdarg>

namespace GX
{

// Info / Warn / Error は可変長引数を va_list に変換して共通の Log() に委譲する

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
    const char* prefix = "";
    switch (level)
    {
    case LogLevel::Info:  prefix = "[INFO]  "; break;
    case LogLevel::Warn:  prefix = "[WARN]  "; break;
    case LogLevel::Error: prefix = "[ERROR] "; break;
    }

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);

    char output[2048 + 64];
    snprintf(output, sizeof(output), "%s%s\n", prefix, buffer);

    // OutputDebugStringA は VS の「出力」ウィンドウに表示される
    OutputDebugStringA(output);

    // コンソールアプリとしてアタッチされていれば標準出力にも出る
    printf("%s", output);
}

} // namespace GX
