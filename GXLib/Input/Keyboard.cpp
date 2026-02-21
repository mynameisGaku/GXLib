/// @file Keyboard.cpp
/// @brief キーボード入力管理の実装
#include "pch.h"
#include "Input/Keyboard.h"

namespace GX
{

void Keyboard::Initialize()
{
    m_currentState.fill(false);
    m_previousState.fill(false);
    m_rawState.fill(false);
}

void Keyboard::Update()
{
    // 前フレームの状態を保存してからメッセージの生データを現在の状態へ反映。
    // この2段階管理により、トリガー/リリースの判定が可能になる。
    m_previousState = m_currentState;
    m_currentState = m_rawState;
}

bool Keyboard::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    int key = static_cast<int>(wParam);
    if (key < 0 || key >= k_KeyCount) return false;

    // WM_SYSKEYDOWNはAltキー同時押し時に発生するので、通常のKEYDOWNと合わせて処理する
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        m_rawState[key] = true;
        return true;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        m_rawState[key] = false;
        return true;
    }

    return false;
}

bool Keyboard::IsKeyDown(int key) const
{
    if (key < 0 || key >= k_KeyCount) return false;
    return m_currentState[key];
}

bool Keyboard::IsKeyTriggered(int key) const
{
    if (key < 0 || key >= k_KeyCount) return false;
    // 今フレーム押されていて、前フレームは押されていなければトリガー成立
    return m_currentState[key] && !m_previousState[key];
}

bool Keyboard::IsKeyReleased(int key) const
{
    if (key < 0 || key >= k_KeyCount) return false;
    return !m_currentState[key] && m_previousState[key];
}

} // namespace GX
