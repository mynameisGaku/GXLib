/// @file Mouse.cpp
/// @brief マウス入力管理の実装
#include "pch.h"
#include "Input/Mouse.h"

namespace GX
{

void Mouse::Initialize()
{
    m_x = m_y = m_prevX = m_prevY = 0;
    m_wheelDelta = 0;
    m_wheelAccum = 0;
    m_currentButtons.fill(false);
    m_previousButtons.fill(false);
    m_rawButtons.fill(false);
}

void Mouse::Update()
{
    m_previousButtons = m_currentButtons;
    m_currentButtons = m_rawButtons;
    m_prevX = m_x;
    m_prevY = m_y;
    m_wheelDelta = m_wheelAccum;
    m_wheelAccum = 0;
}

bool Mouse::ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEMOVE:
        m_x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
        m_y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
        return true;

    case WM_LBUTTONDOWN: m_rawButtons[MouseButton::Left]   = true;  return true;
    case WM_LBUTTONUP:   m_rawButtons[MouseButton::Left]   = false; return true;
    case WM_RBUTTONDOWN: m_rawButtons[MouseButton::Right]  = true;  return true;
    case WM_RBUTTONUP:   m_rawButtons[MouseButton::Right]  = false; return true;
    case WM_MBUTTONDOWN: m_rawButtons[MouseButton::Middle] = true;  return true;
    case WM_MBUTTONUP:   m_rawButtons[MouseButton::Middle] = false; return true;

    case WM_MOUSEWHEEL:
        m_wheelAccum += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        return true;
    }

    return false;
}

bool Mouse::IsButtonDown(int button) const
{
    if (button < 0 || button >= MouseButton::Count) return false;
    return m_currentButtons[button];
}

bool Mouse::IsButtonTriggered(int button) const
{
    if (button < 0 || button >= MouseButton::Count) return false;
    return m_currentButtons[button] && !m_previousButtons[button];
}

bool Mouse::IsButtonReleased(int button) const
{
    if (button < 0 || button >= MouseButton::Count) return false;
    return !m_currentButtons[button] && m_previousButtons[button];
}

} // namespace GX
