/// @file InputCapture.cpp
/// @brief 入力キャプチャの実装
#include "pch_common.h"
#include "Input/InputCapture.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "Input/Gamepad.h"

namespace gx
{

void InputCapture::Begin(int cancelKey)
{
    m_state = CaptureState::Waiting;
    m_cancelKey = cancelKey;
    m_result = {};
}

void InputCapture::Update(const Keyboard& keyboard, const Mouse& mouse, const Gamepad& gamepad)
{
    if (m_state != CaptureState::Waiting) return;

    // キャンセルキーを確認
    if (m_cancelKey != 0 && keyboard.IsKeyTriggered(m_cancelKey))
    {
        m_state = CaptureState::Cancelled;
        return;
    }

    // キーボードキーをスキャン（キャンセルキーはスキップ）
    for (int vk = 0x08; vk <= 0xFE; ++vk)
    {
        if (vk == m_cancelKey) continue;
        if (keyboard.IsKeyTriggered(vk))
        {
            m_result = InputBinding::Key(vk);
            m_state = CaptureState::Captured;
            return;
        }
    }

    // マウスボタンをスキャン
    for (int btn = 0; btn < 3; ++btn)
    {
        if (mouse.IsButtonTriggered(btn))
        {
            m_result = InputBinding::MouseBtn(btn);
            m_state = CaptureState::Captured;
            return;
        }
    }

    // ゲームパッドボタンをスキャン
    static const int padButtons[] = {
        XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
        XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_BACK,
        XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN,
        XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
    };

    for (int pad = 0; pad < Gamepad::k_MaxPads; ++pad)
    {
        if (!gamepad.IsConnected(pad)) continue;
        for (int btn : padButtons)
        {
            if (gamepad.IsButtonTriggered(pad, btn))
            {
                m_result = InputBinding::PadBtn(btn, pad);
                m_state = CaptureState::Captured;
                return;
            }
        }
    }
}

bool InputCapture::ApplyToAction(ActionMapping& mapping, const std::string& actionName, int bindingIndex) const
{
    if (m_state != CaptureState::Captured) return false;

    // 新しいバインディングでアクションを再定義する
    // 簡略化のため、このバインディングのみでアクションを定義する
    mapping.DefineAction(actionName, { m_result });
    return true;
}

void InputCapture::Reset()
{
    m_state = CaptureState::Idle;
    m_result = {};
}

} // namespace gx
