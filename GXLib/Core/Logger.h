#pragma once
/// @file Logger.h
/// @brief ログ出力（OutputDebugString + コンソール printf）
///
/// DxLib の printfDx() に相当するが、ファイルレベル分類やプレフィックス付き出力が可能。
/// 出力先は Visual Studio の出力ウィンドウと、アタッチされていればコンソールの 2 箇所。
/// 通常は GX_LOG_INFO / GX_LOG_WARN / GX_LOG_ERROR マクロ経由で使う。

#include "pch.h"

namespace GX
{

/// @brief ログの重要度
enum class LogLevel
{
    Info,   ///< 通常の情報（初期化完了など）
    Warn,   ///< 警告（動作に支障はないが注意が必要）
    Error   ///< エラー（処理の続行が難しい問題）
};

/// @brief printf スタイルのログ出力を提供する静的クラス
/// DxLib の printfDx() と違い、画面上には表示されず VS 出力ウィンドウに出る。
class Logger
{
public:
    /// @brief 情報レベルのログを出力する
    /// @param format printf 形式のフォーマット文字列
    /// @param ... 可変長引数
    static void Info(const char* format, ...);

    /// @brief 警告レベルのログを出力する
    /// @param format printf 形式のフォーマット文字列
    /// @param ... 可変長引数
    static void Warn(const char* format, ...);

    /// @brief エラーレベルのログを出力する
    /// @param format printf 形式のフォーマット文字列
    /// @param ... 可変長引数
    static void Error(const char* format, ...);

private:
    /// レベルプレフィックスを付けて OutputDebugStringA + printf に出力する
    static void Log(LogLevel level, const char* format, va_list args);
};

} // namespace GX

/// @name ログ出力マクロ
/// Logger::Info/Warn/Error のショートカット。GX:: を省略できる。
/// @{
#define GX_LOG_INFO(fmt, ...)  GX::Logger::Info(fmt, ##__VA_ARGS__)
#define GX_LOG_WARN(fmt, ...)  GX::Logger::Warn(fmt, ##__VA_ARGS__)
#define GX_LOG_ERROR(fmt, ...) GX::Logger::Error(fmt, ##__VA_ARGS__)
/// @}
