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

    // ウィンドウにメッセージコールバックを登録
    window.AddMessageCallback([this](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> bool
    {
        // キーボードメッセージ
        if (m_keyboard.ProcessMessage(msg, wParam, lParam))
            return true;

        // マウスメッセージ
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
    int result = 0;
    if (m_mouse.IsButtonDown(MouseButton::Left))   result |= 1;  // MOUSE_INPUT_LEFT
    if (m_mouse.IsButtonDown(MouseButton::Right))  result |= 2;  // MOUSE_INPUT_RIGHT
    if (m_mouse.IsButtonDown(MouseButton::Middle)) result |= 4;  // MOUSE_INPUT_MIDDLE
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
