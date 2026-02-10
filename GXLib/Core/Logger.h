#pragma once
/// @file Logger.h
/// @brief ログシステム
///
/// 【初学者向け解説】
/// ログとは、プログラムの実行中に「何が起きているか」を記録する仕組みです。
/// バグの原因を見つけたり、動作を確認するのに不可欠です。
///
/// このLoggerクラスは3つのログレベルを提供します：
/// - Info:  通常の情報（「初期化完了」など）
/// - Warn:  警告（動作に問題はないが注意が必要）
/// - Error: エラー（問題が発生した）
///
/// 出力先は2つあります：
/// - OutputDebugString: Visual Studioの「出力」ウィンドウに表示
/// - printf: コンソール（もしあれば）に表示

#include "pch.h"

namespace GX
{

/// @brief ログレベル（重要度）を表す列挙型
enum class LogLevel
{
    Info,   ///< 情報メッセージ
    Warn,   ///< 警告メッセージ
    Error   ///< エラーメッセージ
};

/// @brief ログ出力を管理する静的クラス
class Logger
{
public:
    /// @brief 情報レベルのログを出力する
    /// @param format printfスタイルのフォーマット文字列
    /// @param ... フォーマット引数（可変長）
    static void Info(const char* format, ...);

    /// @brief 警告レベルのログを出力する
    /// @param format printfスタイルのフォーマット文字列
    /// @param ... フォーマット引数（可変長）
    static void Warn(const char* format, ...);

    /// @brief エラーレベルのログを出力する
    /// @param format printfスタイルのフォーマット文字列
    /// @param ... フォーマット引数（可変長）
    static void Error(const char* format, ...);

private:
    /// 実際のログ出力処理
    static void Log(LogLevel level, const char* format, va_list args);
};

} // namespace GX

// 便利マクロ
#define GX_LOG_INFO(fmt, ...)  GX::Logger::Info(fmt, ##__VA_ARGS__)
#define GX_LOG_WARN(fmt, ...)  GX::Logger::Warn(fmt, ##__VA_ARGS__)
#define GX_LOG_ERROR(fmt, ...) GX::Logger::Error(fmt, ##__VA_ARGS__)
