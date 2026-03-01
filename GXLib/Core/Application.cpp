/// @file Application.cpp
/// @brief Application クラスの実装
#include "pch_common.h"
#include "Core/Application.h"
#include "Core/Logger.h"
#include "Core/CrashReporter.h"

namespace gx
{

bool Application::Initialize(const ApplicationDesc& desc)
{
    // CrashReporter 初期化（最初に行う）
    if (!CrashReporter::Instance().IsInitialized())
    {
        CrashReporterConfig crashCfg;
        CrashReporter::Instance().Initialize(crashCfg);
        CrashReporter::Instance().InstallHandler();
    }

    GX_LOG_INFO("Initializing GXLib Application...");

    // ApplicationDesc → WindowDesc に変換してウィンドウを作成
    m_userTitle = desc.title;

    WindowDesc windowDesc;
    windowDesc.title  = desc.title;
    windowDesc.width  = desc.width;
    windowDesc.height = desc.height;

    if (!m_window.Initialize(windowDesc))
    {
        GX_LOG_ERROR("Failed to initialize window");
        return false;
    }

    // ウィンドウ作成後に CrashReporter にハンドルを設定
    CrashReporter::Instance().SetWindowHandle(m_window.GetHWND());

    m_timer.Reset();
    m_running = true;

    GX_LOG_INFO("Application initialized successfully");
    return true;
}

void Application::Run(std::function<void(float)> updateCallback)
{
    GX_LOG_INFO("Starting main loop...");

    // ProcessMessages() が false を返す ＝ WM_QUIT 受信 ＝ ウィンドウが閉じられた
    while (m_running)
    {
        if (!m_window.ProcessMessages())
        {
            m_running = false;
            break;
        }

        m_timer.Tick();

        if (updateCallback)
        {
            updateCallback(m_timer.GetDeltaTime());
        }

        // タイトルバーに FPS を表示（1 秒ごとに更新）
        static float titleUpdateTimer = 0.0f;
        titleUpdateTimer += m_timer.GetDeltaTime();
        if (titleUpdateTimer >= 1.0f)
        {
            wchar_t title[256];
            swprintf_s(title, L"%s [FPS: %.1f]", m_userTitle.c_str(), m_timer.GetFPS());
            m_window.SetTitle(title);
            titleUpdateTimer = 0.0f;
        }
    }
}

void Application::Shutdown()
{
    GX_LOG_INFO("Shutting down application...");
    m_running = false;

    // CrashReporter シャットダウン
    if (CrashReporter::Instance().IsInitialized())
    {
        CrashReporter::Instance().Shutdown();
    }
}

} // namespace gx
