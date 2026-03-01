#pragma once
/// @file CrashReporter.h
/// @brief クラッシュレポーター（スタックトレース、ダンプファイル、SEHハンドラー）
///
/// アプリケーションのクラッシュ情報を収集し、テキストファイルとして保存する。
/// SEH（構造化例外処理）ハンドラーの登録、スタックトレースの取得、
/// ログバッファの保持、カスタムメタデータの添付をサポートする。
/// @addtogroup grp_core/// @{

#include "pch_common.h"

#include <mutex>

namespace gx
{

/// @brief エラーコード（カテゴリ上位16bit + 固有コード下位16bit）
enum class ErrorCode : uint32_t
{
    None                         = 0x00000000,
    // Core (0x0001'xxxx)
    CoreUnknown                  = 0x00010000,
    CoreWindowCreationFailed     = 0x00010001,
    CoreOutOfMemory              = 0x00010002,
    CoreInitializationFailed     = 0x00010003,
    // Graphics (0x0002'xxxx)
    GraphicsUnknown              = 0x00020000,
    GraphicsDeviceCreationFailed = 0x00020001,
    GraphicsSwapChainFailed      = 0x00020002,
    GraphicsShaderCompileFailed  = 0x00020005,
    GraphicsResourceFailed       = 0x00020006,
    GraphicsDeviceLost           = 0x00020007,
    GraphicsOutOfVRAM            = 0x00020008,
    // Audio (0x0003'xxxx)
    AudioUnknown                 = 0x00030000,
    AudioDeviceFailed            = 0x00030001,
    AudioFileLoadFailed          = 0x00030003,
    // Physics (0x0004'xxxx)
    PhysicsUnknown               = 0x00040000,
    PhysicsInitFailed            = 0x00040001,
    // IO (0x0005'xxxx)
    IOUnknown                    = 0x00050000,
    IOFileNotFound               = 0x00050001,
    IOReadFailed                 = 0x00050002,
    // Script (0x0006'xxxx)
    ScriptUnknown                = 0x00060000,
    ScriptRuntimeError           = 0x00060002,
    // GUI (0x0007'xxxx)
    GUIUnknown                   = 0x00070000,
    // AI (0x0008'xxxx)
    AIUnknown                    = 0x00080000,
    // Scene (0x0009'xxxx)
    SceneUnknown                 = 0x00090000,
    SceneLoadFailed              = 0x00090001,
};

/// @brief エラーコードのカテゴリ名を取得する
/// @param code エラーコード
/// @return カテゴリ名（"Core", "Graphics" 等）
inline const char* GetErrorCategoryName(ErrorCode code)
{
    uint32_t category = static_cast<uint32_t>(code) >> 16;
    switch (category)
    {
    case 0x0001: return "Core";
    case 0x0002: return "Graphics";
    case 0x0003: return "Audio";
    case 0x0004: return "Physics";
    case 0x0005: return "IO";
    case 0x0006: return "Script";
    case 0x0007: return "GUI";
    case 0x0008: return "AI";
    case 0x0009: return "Scene";
    default:     return "Unknown";
    }
}

/// @brief エラーコードを16進文字列に変換する
/// @param code エラーコード
/// @return "0x00020005" 形式の文字列
inline std::string ErrorCodeToString(ErrorCode code)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08X", static_cast<uint32_t>(code));
    return buf;
}

/// @brief クラッシュ情報
struct CrashInfo
{
    uint64_t                                        timestamp = 0;                  ///< タイムスタンプ（Unix time）
    std::string                                     message;                        ///< クラッシュメッセージ
    ErrorCode                                       errorCode = ErrorCode::None;    ///< エラーコード
    std::string                                     probableCause;                  ///< 推測原因
    std::vector<std::string>                        stackTrace;                     ///< スタックトレース
    uint32_t                                        threadId = 0;                   ///< スレッドID
    std::string                                     moduleInfo;                     ///< モジュール情報
    std::vector<std::string>                        logTail;                        ///< 直近のログ行（最大50行）
    std::unordered_map<std::string, std::string>    customData;                     ///< カスタムメタデータ
};

/// @brief クラッシュレポーター設定
struct CrashReporterConfig
{
    std::string crashDumpDir     = "crashes/";   ///< ダンプファイルディレクトリ
    uint32_t    maxDumpFiles     = 10;           ///< 最大ダンプファイル数
    bool        enableMinidump   = true;         ///< ミニダンプ有効
    bool        enableLogCapture = true;         ///< ログキャプチャ有効
    bool        enableStackTrace = true;         ///< スタックトレース有効
    std::string appName          = "GXLib";      ///< アプリケーション名
    std::string appVersion       = "1.0.0";      ///< アプリケーションバージョン
};

/// @brief クラッシュレポーター（シングルトン）
///
/// クラッシュ発生時のスタックトレース取得、レポート生成、
/// ダンプファイルの保存/読み込み/管理を行う。
class CrashReporter
{
public:
    /// @brief シングルトンインスタンスを取得する
    static CrashReporter& Instance();

    /// @brief 初期化する
    /// @param config クラッシュレポーター設定
    void Initialize(const CrashReporterConfig& config);

    /// @brief シャットダウンする
    void Shutdown();

    /// @brief 初期化済みか取得する
    /// @return 初期化済みならtrue
    bool IsInitialized() const { return m_initialized; }

    /// @brief 設定を取得する
    /// @return クラッシュレポーター設定
    const CrashReporterConfig& GetConfig() const { return m_config; }

    /// @brief カスタムデータを設定する
    /// @param key キー
    /// @param value 値
    void SetCustomData(const std::string& key, const std::string& value);

    /// @brief カスタムデータを取得する
    /// @param key キー
    /// @return 値（存在しない場合は空文字列）
    std::string GetCustomData(const std::string& key) const;

    /// @brief カスタムデータを削除する
    /// @param key キー
    void RemoveCustomData(const std::string& key);

    /// @brief スタックトレースをキャプチャする
    /// @param maxFrames 最大フレーム数
    /// @return フレーム記述の配列
    std::vector<std::string> CaptureStackTrace(uint32_t maxFrames = 32) const;

    /// @brief クラッシュレポートを生成する
    /// @param message クラッシュメッセージ
    /// @return クラッシュ情報
    CrashInfo CreateCrashReport(const std::string& message);

    /// @brief クラッシュダンプをファイルに保存する
    /// @param crashInfo クラッシュ情報
    /// @param filePath ファイルパス
    /// @return 成功でtrue
    bool SaveCrashDump(const CrashInfo& crashInfo, const std::string& filePath);

    /// @brief クラッシュダンプをファイルから読み込む
    /// @param filePath ファイルパス
    /// @return クラッシュ情報
    CrashInfo LoadCrashDump(const std::string& filePath) const;

    /// @brief 最近のクラッシュファイル一覧を取得する
    /// @param maxCount 最大件数
    /// @return ファイルパスの配列
    std::vector<std::string> GetRecentCrashes(uint32_t maxCount) const;

    /// @brief 古いダンプファイルを削除する
    void PruneDumps();

    /// @brief ダンプファイル数を取得する
    /// @return ファイル数
    uint32_t GetDumpCount() const;

    /// @brief クラッシュレポートを人間可読な文字列にフォーマットする
    /// @param crashInfo クラッシュ情報
    /// @return フォーマット済み文字列
    std::string FormatCrashReport(const CrashInfo& crashInfo) const;

    /// @brief SEHハンドラーを登録する
    void InstallHandler();

    /// @brief SEHハンドラーを解除する
    void UninstallHandler();

    /// @brief ログ行を追加する（ログバッファ用）
    /// @param line ログ行
    void AddLogLine(const std::string& line);

    /// @brief 致命的エラーを報告する（ログ→ダンプ→ポップアップ→終了）
    /// @param code エラーコード
    /// @param probableCause 推測原因
    /// @param message エラーメッセージ
    /// @param hwnd 親ウィンドウハンドル（nullptrでデフォルト使用）
    void ReportFatalError(ErrorCode code, const std::string& probableCause,
                          const std::string& message, HWND hwnd = nullptr);

    /// @brief MessageBoxの親ウィンドウハンドルを設定する
    /// @param hwnd ウィンドウハンドル
    void SetWindowHandle(HWND hwnd);

    /// @brief MessageBoxの親ウィンドウハンドルを取得する
    /// @return ウィンドウハンドル
    HWND GetWindowHandle() const;

private:
    CrashReporter() = default;
    ~CrashReporter() = default;
    CrashReporter(const CrashReporter&) = delete;
    CrashReporter& operator=(const CrashReporter&) = delete;

    /// @brief SEH例外フィルター
    static LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);

    CrashReporterConfig m_config;                              ///< レポーター設定
    bool                m_initialized = false;                  ///< 初期化済みフラグ

    std::unordered_map<std::string, std::string> m_customData; ///< カスタムメタデータ
    mutable std::recursive_mutex m_mutex;                      ///< スレッド安全用ミューテックス

    // ログバッファ（循環バッファ）
    static constexpr uint32_t k_MaxLogLines = 50;              ///< ログバッファの最大行数
    std::vector<std::string>  m_logBuffer;                     ///< ログ行の循環バッファ
    uint32_t                  m_logBufferIndex = 0;            ///< 循環バッファの書き込み位置

    // SEHハンドラー
    LPTOP_LEVEL_EXCEPTION_FILTER m_previousHandler = nullptr;  ///< 登録前のSEHハンドラー
    bool                         m_handlerInstalled = false;   ///< SEHハンドラー登録済みフラグ

    // MessageBox用ウィンドウハンドル
    HWND m_hwnd = nullptr;                                     ///< 親ウィンドウハンドル
};

} // namespace gx

/// @brief 致命的エラーを報告するマクロ
/// @param code ErrorCode 値
/// @param cause 推測原因文字列
/// @param fmt printf 形式のフォーマット文字列
#define GX_FATAL(code, cause, fmt, ...) \
    do { \
        char gx_fatal_buf__[2048]; \
        snprintf(gx_fatal_buf__, sizeof(gx_fatal_buf__), fmt, ##__VA_ARGS__); \
        gx::CrashReporter::Instance().ReportFatalError(code, cause, gx_fatal_buf__); \
    } while(0)

/// @}
