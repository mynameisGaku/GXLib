/// @file CrashReporter.cpp
/// @brief クラッシュレポーターの実装
#include "pch_common.h"
#include "Core/CrashReporter.h"
#include "Core/Logger.h"

#include <DbgHelp.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <iomanip>

#pragma comment(lib, "dbghelp.lib")

namespace gx
{

// ============================================================================
// シングルトン
// ============================================================================

CrashReporter& CrashReporter::Instance()
{
    static CrashReporter instance;
    return instance;
}

// ============================================================================
// 初期化 / シャットダウン
// ============================================================================

void CrashReporter::Initialize(const CrashReporterConfig& config)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    m_config = config;
    m_initialized = true;
    m_logBuffer.clear();
    m_logBufferIndex = 0;
    m_customData.clear();

    // ダンプディレクトリ作成
    try
    {
        std::filesystem::create_directories(m_config.crashDumpDir);
    }
    catch (...)
    {
        GX_LOG_WARN("CrashReporter: Failed to create crash dump directory: %s",
                     m_config.crashDumpDir.c_str());
    }

    GX_LOG_INFO("CrashReporter: Initialized (%s v%s, dump dir: %s)",
                m_config.appName.c_str(), m_config.appVersion.c_str(),
                m_config.crashDumpDir.c_str());
}

void CrashReporter::Shutdown()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_handlerInstalled)
    {
        UninstallHandler();
    }

    m_initialized = false;
    m_logBuffer.clear();
    m_customData.clear();
    m_hwnd = nullptr;

    GX_LOG_INFO("CrashReporter: Shutdown");
}

// ============================================================================
// カスタムデータ
// ============================================================================

void CrashReporter::SetCustomData(const std::string& key, const std::string& value)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_customData[key] = value;
}

std::string CrashReporter::GetCustomData(const std::string& key) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = m_customData.find(key);
    if (it != m_customData.end())
        return it->second;
    return "";
}

void CrashReporter::RemoveCustomData(const std::string& key)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_customData.erase(key);
}

// ============================================================================
// スタックトレース
// ============================================================================

std::vector<std::string> CrashReporter::CaptureStackTrace(uint32_t maxFrames) const
{
    std::vector<std::string> frames;

    if (!m_config.enableStackTrace)
        return frames;

    // Windows APIを使用してスタックトレースを取得
    std::vector<void*> stack(maxFrames);
    USHORT capturedFrames = CaptureStackBackTrace(1, static_cast<DWORD>(maxFrames),
                                                   stack.data(), nullptr);

    // シンボル解決を試みる
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    char symbolBuffer[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (USHORT i = 0; i < capturedFrames; ++i)
    {
        DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);

        if (SymFromAddr(process, address, nullptr, symbol))
        {
            std::ostringstream oss;
            oss << "frame_" << i << ": " << symbol->Name
                << " (0x" << std::hex << address << ")";
            frames.push_back(oss.str());
        }
        else
        {
            std::ostringstream oss;
            oss << "frame_" << i << ": 0x" << std::hex << address;
            frames.push_back(oss.str());
        }
    }

    SymCleanup(process);

    return frames;
}

// ============================================================================
// クラッシュレポート生成
// ============================================================================

CrashInfo CrashReporter::CreateCrashReport(const std::string& message)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    CrashInfo info;

    // タイムスタンプ
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    info.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(epoch).count());

    info.message = message;

    // スレッドID
    info.threadId = GetCurrentThreadId();

    // モジュール情報
    {
        char modulePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
        info.moduleInfo = m_config.appName + " v" + m_config.appVersion + " (" + modulePath + ")";
    }

    // スタックトレース（ロック解除してから取得する必要があるため、ここではm_mutexを保持しない）
    // ただしm_configはアクセス安全なので直接呼ぶ
    if (m_config.enableStackTrace)
    {
        std::vector<void*> stack(32);
        USHORT capturedFrames = CaptureStackBackTrace(1, 32, stack.data(), nullptr);

        HANDLE process = GetCurrentProcess();
        SymInitialize(process, nullptr, TRUE);

        char symbolBuffer[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        for (USHORT i = 0; i < capturedFrames; ++i)
        {
            DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);
            if (SymFromAddr(process, address, nullptr, symbol))
            {
                std::ostringstream oss;
                oss << "frame_" << i << ": " << symbol->Name
                    << " (0x" << std::hex << address << ")";
                info.stackTrace.push_back(oss.str());
            }
            else
            {
                std::ostringstream oss;
                oss << "frame_" << i << ": 0x" << std::hex << address;
                info.stackTrace.push_back(oss.str());
            }
        }

        SymCleanup(process);
    }

    // ログバッファコピー
    if (m_config.enableLogCapture)
    {
        info.logTail = m_logBuffer;
    }

    // カスタムデータコピー
    info.customData = m_customData;

    return info;
}

// ============================================================================
// ダンプファイル保存 / 読み込み
// ============================================================================

bool CrashReporter::SaveCrashDump(const CrashInfo& crashInfo, const std::string& filePath)
{
    try
    {
        // ディレクトリ作成
        std::filesystem::path path(filePath);
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        std::ofstream file(filePath);
        if (!file.is_open())
            return false;

        file << "=== GXLib Crash Report ===\n";
        file << "Timestamp: " << crashInfo.timestamp << "\n";
        file << "Message: " << crashInfo.message << "\n";
        file << "ErrorCode: " << ErrorCodeToString(crashInfo.errorCode) << "\n";
        file << "ProbableCause: " << crashInfo.probableCause << "\n";
        file << "ThreadId: " << crashInfo.threadId << "\n";
        file << "Module: " << crashInfo.moduleInfo << "\n";

        file << "\n--- Stack Trace ---\n";
        for (const auto& frame : crashInfo.stackTrace)
        {
            file << "  " << frame << "\n";
        }

        file << "\n--- Log Tail ---\n";
        for (const auto& line : crashInfo.logTail)
        {
            file << "  " << line << "\n";
        }

        file << "\n--- Custom Data ---\n";
        for (const auto& [key, value] : crashInfo.customData)
        {
            file << "  " << key << "=" << value << "\n";
        }

        file << "\n=== End Crash Report ===\n";
        file.close();

        return true;
    }
    catch (...)
    {
        return false;
    }
}

CrashInfo CrashReporter::LoadCrashDump(const std::string& filePath) const
{
    CrashInfo info;

    try
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return info;

        std::string line;
        enum class Section { Header, StackTrace, LogTail, CustomData, None };
        Section currentSection = Section::Header;

        while (std::getline(file, line))
        {
            // セクションヘッダー検出
            if (line == "--- Stack Trace ---")
            {
                currentSection = Section::StackTrace;
                continue;
            }
            else if (line == "--- Log Tail ---")
            {
                currentSection = Section::LogTail;
                continue;
            }
            else if (line == "--- Custom Data ---")
            {
                currentSection = Section::CustomData;
                continue;
            }
            else if (line == "=== End Crash Report ===" || line == "=== GXLib Crash Report ===")
            {
                continue;
            }

            switch (currentSection)
            {
            case Section::Header:
            {
                if (line.substr(0, 11) == "Timestamp: ")
                {
                    try { info.timestamp = std::stoull(line.substr(11)); } catch (...) {}
                }
                else if (line.substr(0, 9) == "Message: ")
                {
                    info.message = line.substr(9);
                }
                else if (line.substr(0, 11) == "ErrorCode: ")
                {
                    try
                    {
                        uint32_t code = static_cast<uint32_t>(std::stoul(line.substr(11), nullptr, 16));
                        info.errorCode = static_cast<ErrorCode>(code);
                    }
                    catch (...) {}
                }
                else if (line.substr(0, 15) == "ProbableCause: ")
                {
                    info.probableCause = line.substr(15);
                }
                else if (line.substr(0, 10) == "ThreadId: ")
                {
                    try { info.threadId = static_cast<uint32_t>(std::stoul(line.substr(10))); } catch (...) {}
                }
                else if (line.substr(0, 8) == "Module: ")
                {
                    info.moduleInfo = line.substr(8);
                }
                break;
            }
            case Section::StackTrace:
            {
                if (line.size() > 2 && line[0] == ' ' && line[1] == ' ')
                    info.stackTrace.push_back(line.substr(2));
                break;
            }
            case Section::LogTail:
            {
                if (line.size() > 2 && line[0] == ' ' && line[1] == ' ')
                    info.logTail.push_back(line.substr(2));
                break;
            }
            case Section::CustomData:
            {
                if (line.size() > 2 && line[0] == ' ' && line[1] == ' ')
                {
                    std::string content = line.substr(2);
                    size_t eq = content.find('=');
                    if (eq != std::string::npos)
                    {
                        info.customData[content.substr(0, eq)] = content.substr(eq + 1);
                    }
                }
                break;
            }
            case Section::None:
                break;
            }
        }
    }
    catch (...)
    {
        GX_LOG_ERROR("CrashReporter: Failed to load crash dump: %s", filePath.c_str());
    }

    return info;
}

// ============================================================================
// ダンプ管理
// ============================================================================

std::vector<std::string> CrashReporter::GetRecentCrashes(uint32_t maxCount) const
{
    namespace fs = std::filesystem;
    std::vector<std::string> files;

    try
    {
        if (!fs::exists(m_config.crashDumpDir))
            return files;

        // ダンプファイル一覧を取得
        std::vector<std::pair<std::string, fs::file_time_type>> entries;
        for (const auto& entry : fs::directory_iterator(m_config.crashDumpDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
            {
                entries.emplace_back(entry.path().string(), entry.last_write_time());
            }
        }

        // 新しい順にソート
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        for (uint32_t i = 0; i < std::min(maxCount, static_cast<uint32_t>(entries.size())); ++i)
        {
            files.push_back(entries[i].first);
        }
    }
    catch (...)
    {
    }

    return files;
}

void CrashReporter::PruneDumps()
{
    namespace fs = std::filesystem;

    try
    {
        if (!fs::exists(m_config.crashDumpDir))
            return;

        // ダンプファイル一覧を取得
        std::vector<std::pair<std::string, fs::file_time_type>> entries;
        for (const auto& entry : fs::directory_iterator(m_config.crashDumpDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
            {
                entries.emplace_back(entry.path().string(), entry.last_write_time());
            }
        }

        if (entries.size() <= m_config.maxDumpFiles)
            return;

        // 古い順にソート
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });

        // 超過分を削除
        uint32_t toDelete = static_cast<uint32_t>(entries.size()) - m_config.maxDumpFiles;
        for (uint32_t i = 0; i < toDelete; ++i)
        {
            fs::remove(entries[i].first);
            GX_LOG_INFO("CrashReporter: Pruned old dump: %s", entries[i].first.c_str());
        }
    }
    catch (...)
    {
        GX_LOG_WARN("CrashReporter: Failed to prune dump files");
    }
}

uint32_t CrashReporter::GetDumpCount() const
{
    namespace fs = std::filesystem;

    try
    {
        if (!fs::exists(m_config.crashDumpDir))
            return 0;

        uint32_t count = 0;
        for (const auto& entry : fs::directory_iterator(m_config.crashDumpDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
                ++count;
        }
        return count;
    }
    catch (...)
    {
        return 0;
    }
}

// ============================================================================
// レポートフォーマット
// ============================================================================

std::string CrashReporter::FormatCrashReport(const CrashInfo& crashInfo) const
{
    std::ostringstream oss;

    oss << "=== Crash Report ===\n";
    oss << "Timestamp: " << crashInfo.timestamp << "\n";
    oss << "Message: " << crashInfo.message << "\n";

    if (crashInfo.errorCode != ErrorCode::None)
    {
        oss << "Error Code: " << ErrorCodeToString(crashInfo.errorCode)
            << " [" << GetErrorCategoryName(crashInfo.errorCode) << "]\n";
    }
    if (!crashInfo.probableCause.empty())
    {
        oss << "Probable Cause: " << crashInfo.probableCause << "\n";
    }

    oss << "Thread: " << crashInfo.threadId << "\n";
    oss << "Module: " << crashInfo.moduleInfo << "\n\n";

    if (!crashInfo.stackTrace.empty())
    {
        oss << "Stack Trace:\n";
        for (const auto& frame : crashInfo.stackTrace)
        {
            oss << "  " << frame << "\n";
        }
        oss << "\n";
    }

    if (!crashInfo.logTail.empty())
    {
        oss << "Recent Log:\n";
        for (const auto& line : crashInfo.logTail)
        {
            oss << "  " << line << "\n";
        }
        oss << "\n";
    }

    if (!crashInfo.customData.empty())
    {
        oss << "Custom Data:\n";
        for (const auto& [key, value] : crashInfo.customData)
        {
            oss << "  " << key << " = " << value << "\n";
        }
    }

    return oss.str();
}

// ============================================================================
// SEHハンドラー
// ============================================================================

void CrashReporter::InstallHandler()
{
    if (m_handlerInstalled)
        return;

    m_previousHandler = SetUnhandledExceptionFilter(ExceptionFilter);
    m_handlerInstalled = true;
    GX_LOG_INFO("CrashReporter: SEH handler installed");
}

void CrashReporter::UninstallHandler()
{
    if (!m_handlerInstalled)
        return;

    SetUnhandledExceptionFilter(m_previousHandler);
    m_previousHandler = nullptr;
    m_handlerInstalled = false;
    GX_LOG_INFO("CrashReporter: SEH handler uninstalled");
}

LONG WINAPI CrashReporter::ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    auto& reporter = Instance();

    std::ostringstream msg;
    msg << "Unhandled exception 0x" << std::hex
        << exceptionInfo->ExceptionRecord->ExceptionCode
        << " at 0x" << exceptionInfo->ExceptionRecord->ExceptionAddress;

    CrashInfo crashInfo = reporter.CreateCrashReport(msg.str());

    // タイムスタンプベースのファイル名でダンプを保存
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm localTime;
    localtime_s(&localTime, &time);

    std::ostringstream filename;
    filename << reporter.m_config.crashDumpDir
             << "crash_" << std::put_time(&localTime, "%Y%m%d_%H%M%S") << ".txt";

    reporter.SaveCrashDump(crashInfo, filename.str());

    GX_LOG_ERROR("CrashReporter: %s", msg.str().c_str());

    // ポップアップ表示
    std::string popupMsg = "An unhandled exception occurred.\n\n" + msg.str()
                         + "\n\nA crash dump has been saved to:\n" + filename.str();
    MessageBoxA(reporter.m_hwnd, popupMsg.c_str(), "GXLib - Fatal Error",
                MB_OK | MB_ICONERROR | MB_TOPMOST);

    return EXCEPTION_CONTINUE_SEARCH;
}

// ============================================================================
// ログバッファ
// ============================================================================

void CrashReporter::AddLogLine(const std::string& line)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_logBuffer.size() < k_MaxLogLines)
    {
        m_logBuffer.push_back(line);
    }
    else
    {
        m_logBuffer[m_logBufferIndex % k_MaxLogLines] = line;
    }
    ++m_logBufferIndex;
}

// ============================================================================
// 致命的エラー報告
// ============================================================================

void CrashReporter::ReportFatalError(ErrorCode code, const std::string& probableCause,
                                     const std::string& message, HWND hwnd)
{
    GX_LOG_ERROR("FATAL [%s] %s: %s — %s",
                 ErrorCodeToString(code).c_str(),
                 GetErrorCategoryName(code),
                 message.c_str(),
                 probableCause.c_str());

    // クラッシュレポート生成
    CrashInfo crashInfo = CreateCrashReport(message);
    crashInfo.errorCode = code;
    crashInfo.probableCause = probableCause;

    // タイムスタンプベースのファイル名でダンプを保存
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm localTime;
    localtime_s(&localTime, &time);

    std::ostringstream filename;
    filename << m_config.crashDumpDir
             << "crash_" << std::put_time(&localTime, "%Y%m%d_%H%M%S") << ".txt";

    SaveCrashDump(crashInfo, filename.str());

    // ポップアップ表示
    HWND parentHwnd = hwnd ? hwnd : m_hwnd;
    std::ostringstream popup;
    popup << "A fatal error has occurred.\n\n";
    popup << "Error Code: " << ErrorCodeToString(code)
          << " [" << GetErrorCategoryName(code) << "]\n";
    popup << "Probable Cause: " << probableCause << "\n";
    popup << "Message: " << message << "\n\n";
    popup << "A crash dump has been saved to:\n" << filename.str();

    MessageBoxA(parentHwnd, popup.str().c_str(), "GXLib - Fatal Error",
                MB_OK | MB_ICONERROR | MB_TOPMOST);

    std::terminate();
}

// ============================================================================
// ウィンドウハンドル管理
// ============================================================================

void CrashReporter::SetWindowHandle(HWND hwnd)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hwnd = hwnd;
}

HWND CrashReporter::GetWindowHandle() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_hwnd;
}

} // namespace gx
