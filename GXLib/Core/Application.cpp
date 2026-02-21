/// @file Application.cpp
/// @brief Application クラスの実装
#include "pch.h"
#include "Core/Application.h"
#include "Core/Logger.h"

namespace GX
{

bool Application::Initialize(const ApplicationDesc& desc)
{
    GX_LOG_INFO("Initializing GXLib Application...");

    // ApplicationDesc → WindowDesc に変換してウィンドウを作成
    WindowDesc windowDesc;
    windowDesc.title  = desc.title;
    windowDesc.width  = desc.width;
    windowDesc.height = desc.height;

    if (!m_window.Initialize(windowDesc))
    {
        GX_LOG_ERROR("Failed to initialize window");
        return false;
    }

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

        // タイトルバーに FPS を表示（デバッグ用、1 秒ごとに更新）
        static float titleUpdateTimer = 0.0f;
        titleUpdateTimer += m_timer.GetDeltaTime();
        if (titleUpdateTimer >= 1.0f)
        {
            wchar_t title[256];
            swprintf_s(title, L"GXLib [BUILD v3] FPS: %.1f", m_timer.GetFPS());
            m_window.SetTitle(title);
            titleUpdateTimer = 0.0f;
        }
    }
}

void Application::Shutdown()
{
    GX_LOG_INFO("Shutting down application...");
    m_running = false;
}

} // namespace GX
