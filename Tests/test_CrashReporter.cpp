/// @file test_CrashReporter.cpp
/// @brief CrashReporter のテスト — 設定、カスタムデータ、レポート生成、ダンプ保存/読込、
///        ErrorCode、ReportFatalError、Logger連携

#include <gtest/gtest.h>
#include "Core/CrashReporter.h"
#include "Core/Logger.h"

#include <filesystem>

using namespace gx;

// ============================================================================
// テストフィクスチャ（ダンプディレクトリの自動管理）
// ============================================================================

class CrashReporterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_testDir = "test_crash_dumps_" + std::to_string(GetCurrentProcessId());
        auto& reporter = CrashReporter::Instance();
        if (reporter.IsInitialized())
            reporter.Shutdown();
    }

    void TearDown() override
    {
        auto& reporter = CrashReporter::Instance();
        if (reporter.IsInitialized())
            reporter.Shutdown();

        // テストディレクトリ削除
        try
        {
            std::filesystem::remove_all(m_testDir);
        }
        catch (...)
        {
        }
    }

    std::string m_testDir;
};

// ============================================================================
// 設定テスト
// ============================================================================

TEST_F(CrashReporterTest, DefaultConfig)
{
    CrashReporterConfig cfg;
    EXPECT_EQ(cfg.crashDumpDir, "crashes/");
    EXPECT_EQ(cfg.maxDumpFiles, 10u);
    EXPECT_TRUE(cfg.enableMinidump);
    EXPECT_TRUE(cfg.enableLogCapture);
    EXPECT_TRUE(cfg.enableStackTrace);
    EXPECT_EQ(cfg.appName, "GXLib");
    EXPECT_EQ(cfg.appVersion, "1.0.0");
}

TEST_F(CrashReporterTest, SetConfig)
{
    auto& reporter = CrashReporter::Instance();

    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    cfg.maxDumpFiles = 5;
    cfg.appName = "TestApp";
    cfg.appVersion = "2.0.0";

    reporter.Initialize(cfg);

    EXPECT_EQ(reporter.GetConfig().crashDumpDir, m_testDir + "/");
    EXPECT_EQ(reporter.GetConfig().maxDumpFiles, 5u);
    EXPECT_EQ(reporter.GetConfig().appName, "TestApp");
    EXPECT_EQ(reporter.GetConfig().appVersion, "2.0.0");
}

// ============================================================================
// カスタムデータテスト
// ============================================================================

TEST_F(CrashReporterTest, CustomDataSet)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    reporter.SetCustomData("player_name", "TestPlayer");
    EXPECT_EQ(reporter.GetCustomData("player_name"), "TestPlayer");
}

TEST_F(CrashReporterTest, CustomDataGet)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    reporter.SetCustomData("level", "3");
    reporter.SetCustomData("score", "12345");

    EXPECT_EQ(reporter.GetCustomData("level"), "3");
    EXPECT_EQ(reporter.GetCustomData("score"), "12345");
}

TEST_F(CrashReporterTest, CustomDataRemove)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    reporter.SetCustomData("temp", "value");
    EXPECT_EQ(reporter.GetCustomData("temp"), "value");

    reporter.RemoveCustomData("temp");
    EXPECT_EQ(reporter.GetCustomData("temp"), "");
}

TEST_F(CrashReporterTest, CustomDataMissing)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    EXPECT_EQ(reporter.GetCustomData("nonexistent"), "");
}

// ============================================================================
// スタックトレーステスト
// ============================================================================

TEST_F(CrashReporterTest, CaptureStackTrace)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    cfg.enableStackTrace = true;
    reporter.Initialize(cfg);

    auto trace = reporter.CaptureStackTrace(16);

    // スタックトレースが少なくとも1フレームは返るはず
    EXPECT_GT(trace.size(), 0u);

    // 各フレームが "frame_" で始まるか確認
    for (const auto& frame : trace)
    {
        EXPECT_TRUE(frame.find("frame_") != std::string::npos);
    }
}

// ============================================================================
// クラッシュレポート生成テスト
// ============================================================================

TEST_F(CrashReporterTest, CreateCrashReport)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    reporter.SetCustomData("test_key", "test_value");

    CrashInfo info = reporter.CreateCrashReport("Test crash");

    EXPECT_EQ(info.message, "Test crash");
    EXPECT_GT(info.threadId, 0u);
    EXPECT_FALSE(info.moduleInfo.empty());
    EXPECT_TRUE(info.customData.count("test_key") > 0);
}

TEST_F(CrashReporterTest, CrashReportHasTimestamp)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    CrashInfo info = reporter.CreateCrashReport("Timestamp test");
    EXPECT_GT(info.timestamp, 0u);
}

TEST_F(CrashReporterTest, CrashReportHasMessage)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    CrashInfo info = reporter.CreateCrashReport("Specific error message");
    EXPECT_EQ(info.message, "Specific error message");
}

// ============================================================================
// レポートフォーマットテスト
// ============================================================================

TEST_F(CrashReporterTest, FormatCrashReport)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    CrashInfo info;
    info.timestamp = 1234567890;
    info.message = "Test format";
    info.threadId = 42;
    info.stackTrace = {"frame_0: TestFunc"};
    info.logTail = {"[INFO] Test log line"};
    info.customData["key"] = "value";

    std::string formatted = reporter.FormatCrashReport(info);

    EXPECT_TRUE(formatted.find("Test format") != std::string::npos);
    EXPECT_TRUE(formatted.find("1234567890") != std::string::npos);
    EXPECT_TRUE(formatted.find("TestFunc") != std::string::npos);
    EXPECT_TRUE(formatted.find("Test log line") != std::string::npos);
    EXPECT_TRUE(formatted.find("key") != std::string::npos);
    EXPECT_TRUE(formatted.find("value") != std::string::npos);
}

// ============================================================================
// ダンプ保存/読込テスト
// ============================================================================

TEST_F(CrashReporterTest, SaveAndLoadDump)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    CrashInfo original;
    original.timestamp = 1234567890;
    original.message = "Save/Load test";
    original.threadId = 100;
    original.moduleInfo = "TestModule v1.0";
    original.stackTrace = {"frame_0: FuncA", "frame_1: FuncB"};
    original.logTail = {"log line 1", "log line 2"};
    original.customData["k1"] = "v1";
    original.customData["k2"] = "v2";

    std::string dumpPath = m_testDir + "/test_dump.txt";
    EXPECT_TRUE(reporter.SaveCrashDump(original, dumpPath));

    CrashInfo loaded = reporter.LoadCrashDump(dumpPath);

    EXPECT_EQ(loaded.timestamp, original.timestamp);
    EXPECT_EQ(loaded.message, original.message);
    EXPECT_EQ(loaded.threadId, original.threadId);
    EXPECT_EQ(loaded.stackTrace.size(), original.stackTrace.size());
    EXPECT_EQ(loaded.logTail.size(), original.logTail.size());
    EXPECT_EQ(loaded.customData.size(), original.customData.size());
    EXPECT_EQ(loaded.customData["k1"], "v1");
}

// ============================================================================
// ダンプ管理テスト
// ============================================================================

TEST_F(CrashReporterTest, PruneDumps)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    cfg.maxDumpFiles = 3;
    reporter.Initialize(cfg);

    // 5個のダンプファイルを作成
    for (int i = 0; i < 5; ++i)
    {
        CrashInfo info;
        info.message = "Crash " + std::to_string(i);
        std::string path = m_testDir + "/crash_" + std::to_string(i) + ".txt";
        reporter.SaveCrashDump(info, path);
    }

    EXPECT_EQ(reporter.GetDumpCount(), 5u);

    reporter.PruneDumps();

    EXPECT_LE(reporter.GetDumpCount(), 3u);
}

TEST_F(CrashReporterTest, GetDumpCountEmpty)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    EXPECT_EQ(reporter.GetDumpCount(), 0u);
}

// ============================================================================
// 初期化/シャットダウンテスト
// ============================================================================

TEST_F(CrashReporterTest, InitializeShutdown)
{
    auto& reporter = CrashReporter::Instance();

    EXPECT_FALSE(reporter.IsInitialized());

    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);
    EXPECT_TRUE(reporter.IsInitialized());

    reporter.Shutdown();
    EXPECT_FALSE(reporter.IsInitialized());
}

// ============================================================================
// ログバッファテスト
// ============================================================================

TEST_F(CrashReporterTest, LogBufferCapture)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    cfg.enableLogCapture = true;
    reporter.Initialize(cfg);

    reporter.AddLogLine("[INFO] Line 1");
    reporter.AddLogLine("[WARN] Line 2");
    reporter.AddLogLine("[ERROR] Line 3");

    CrashInfo info = reporter.CreateCrashReport("Log capture test");

    EXPECT_GE(info.logTail.size(), 3u);
}

TEST_F(CrashReporterTest, MaxDumpFilesLimit)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    cfg.maxDumpFiles = 2;
    reporter.Initialize(cfg);

    // 4個のダンプファイルを作成
    for (int i = 0; i < 4; ++i)
    {
        CrashInfo info;
        info.message = "Crash " + std::to_string(i);
        std::string path = m_testDir + "/dump_" + std::to_string(i) + ".txt";
        reporter.SaveCrashDump(info, path);
    }

    reporter.PruneDumps();

    EXPECT_LE(reporter.GetDumpCount(), 2u);
}

// ============================================================================
// ErrorCode テスト
// ============================================================================

TEST_F(CrashReporterTest, ErrorCodeToString)
{
    EXPECT_EQ(gx::ErrorCodeToString(ErrorCode::None), "0x00000000");
    EXPECT_EQ(gx::ErrorCodeToString(ErrorCode::GraphicsShaderCompileFailed), "0x00020005");
    EXPECT_EQ(gx::ErrorCodeToString(ErrorCode::IOFileNotFound), "0x00050001");
}

TEST_F(CrashReporterTest, GetErrorCategoryName)
{
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::CoreUnknown), "Core");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::GraphicsDeviceLost), "Graphics");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::AudioDeviceFailed), "Audio");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::PhysicsInitFailed), "Physics");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::IOReadFailed), "IO");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::ScriptRuntimeError), "Script");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::GUIUnknown), "GUI");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::AIUnknown), "AI");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::SceneLoadFailed), "Scene");
    EXPECT_STREQ(gx::GetErrorCategoryName(ErrorCode::None), "Unknown");
}

TEST_F(CrashReporterTest, CrashInfoWithErrorCode)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    CrashInfo info;
    info.timestamp = 100;
    info.message = "Shader compile failed";
    info.errorCode = ErrorCode::GraphicsShaderCompileFailed;
    info.probableCause = "HLSL syntax error in PBR.hlsl";

    std::string formatted = reporter.FormatCrashReport(info);

    EXPECT_TRUE(formatted.find("0x00020005") != std::string::npos);
    EXPECT_TRUE(formatted.find("Graphics") != std::string::npos);
    EXPECT_TRUE(formatted.find("HLSL syntax error") != std::string::npos);
}

TEST_F(CrashReporterTest, SaveLoadDumpWithErrorCode)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    CrashInfo original;
    original.timestamp = 999;
    original.message = "Device lost";
    original.errorCode = ErrorCode::GraphicsDeviceLost;
    original.probableCause = "Driver crash or TDR timeout";
    original.threadId = 42;

    std::string dumpPath = m_testDir + "/errorcode_dump.txt";
    EXPECT_TRUE(reporter.SaveCrashDump(original, dumpPath));

    CrashInfo loaded = reporter.LoadCrashDump(dumpPath);

    EXPECT_EQ(loaded.errorCode, ErrorCode::GraphicsDeviceLost);
    EXPECT_EQ(loaded.probableCause, "Driver crash or TDR timeout");
    EXPECT_EQ(loaded.message, "Device lost");
    EXPECT_EQ(loaded.timestamp, 999u);
}

TEST_F(CrashReporterTest, SaveLoadDumpBackwardCompat)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    // ErrorCode::None のダンプを保存・読込 → デフォルト値で読める
    CrashInfo original;
    original.timestamp = 500;
    original.message = "Legacy crash";
    // errorCode と probableCause はデフォルト（None / empty）

    std::string dumpPath = m_testDir + "/compat_dump.txt";
    EXPECT_TRUE(reporter.SaveCrashDump(original, dumpPath));

    CrashInfo loaded = reporter.LoadCrashDump(dumpPath);

    EXPECT_EQ(loaded.errorCode, ErrorCode::None);
    EXPECT_TRUE(loaded.probableCause.empty());
    EXPECT_EQ(loaded.message, "Legacy crash");
}

// ============================================================================
// ウィンドウハンドルテスト
// ============================================================================

TEST_F(CrashReporterTest, SetWindowHandle)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    reporter.Initialize(cfg);

    EXPECT_EQ(reporter.GetWindowHandle(), nullptr);

    // ダミーの HWND 値を設定（実際のウィンドウではない）
    HWND dummyHwnd = reinterpret_cast<HWND>(static_cast<uintptr_t>(0x12345678));
    reporter.SetWindowHandle(dummyHwnd);
    EXPECT_EQ(reporter.GetWindowHandle(), dummyHwnd);

    // nullptr にリセット
    reporter.SetWindowHandle(nullptr);
    EXPECT_EQ(reporter.GetWindowHandle(), nullptr);
}

// ============================================================================
// Logger → CrashReporter ログ転送テスト
// ============================================================================

TEST_F(CrashReporterTest, LoggerFeedsCrashReporter)
{
    auto& reporter = CrashReporter::Instance();
    CrashReporterConfig cfg;
    cfg.crashDumpDir = m_testDir + "/";
    cfg.enableLogCapture = true;
    reporter.Initialize(cfg);

    // Logger 経由でログ出力
    GX_LOG_INFO("CrashReporter feed test line");

    // CrashReport を生成して logTail を検証
    CrashInfo info = reporter.CreateCrashReport("Logger feed test");

    // logTail に Logger から転送された行が含まれるはず
    bool found = false;
    for (const auto& line : info.logTail)
    {
        if (line.find("CrashReporter feed test line") != std::string::npos)
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
