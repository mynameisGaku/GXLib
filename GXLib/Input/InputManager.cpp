/// @file InputManager.cpp
/// @brief 入力システム統合マネージャーの実装
#include "pch.h"
#include "Input/InputManager.h"
#include "Core/Window.h"
#include "Core/Logger.h"

namespace GX
{

void InputManager::Initialize(Window& window)
{
    m_keyboard.Initialize();
    m_mouse.Initialize();
    m_gamepad.Initialize();

    // ウィンドウのメッセージループにコールバックを登録し、
    // WM_KEYDOWN / WM_MOUSEMOVE 等を各デバイスクラスに振り分ける。
    // ゲームパッドはXInputポーリングなのでここでは不要。
    window.AddMessageCallback([this](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> bool
    {
        if (m_keyboard.ProcessMessage(msg, wParam, lParam))
            return true;

        if (m_mouse.ProcessMessage(hwnd, msg, wParam, lParam))
            return true;

        return false;
    });

    GX_LOG_INFO("InputManager initialized");
}

void InputManager::Update()
{
    m_keyboard.Update();
    m_mouse.Update();
    m_gamepad.Update();

    // デバイス更新後にアクションマッピングを評価する
    m_actionMapping.Update(m_keyboard, m_mouse, m_gamepad);
}

void InputManager::Shutdown()
{
    GX_LOG_INFO("InputManager shutdown");
}

int InputManager::CheckHitKey(int keyCode) const
{
    return m_keyboard.IsKeyDown(keyCode) ? 1 : 0;
}

int InputManager::GetMouseInput() const
{
    // DxLibと同じビットフラグ配置: LEFT=1, RIGHT=2, MIDDLE=4
    int result = 0;
    if (m_mouse.IsButtonDown(MouseButton::Left))   result |= 1;
    if (m_mouse.IsButtonDown(MouseButton::Right))  result |= 2;
    if (m_mouse.IsButtonDown(MouseButton::Middle)) result |= 4;
    return result;
}

void InputManager::GetMousePoint(int* x, int* y) const
{
    if (x) *x = m_mouse.GetX();
    if (y) *y = m_mouse.GetY();
}

int InputManager::GetMouseWheel() const
{
    return m_mouse.GetWheel();
}

} // namespace GX
