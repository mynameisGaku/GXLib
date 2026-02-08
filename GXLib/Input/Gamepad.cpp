/// @file Gamepad.cpp
/// @brief ゲームパッド入力管理の実装（XInput）
#include "pch.h"
#include "Input/Gamepad.h"

namespace GX
{

void Gamepad::Initialize()
{
    m_pads = {};
}

void Gamepad::Update()
{
    for (int i = 0; i < k_MaxPads; ++i)
    {
        auto& pad = m_pads[i];
        pad.previousButtons = pad.state.Gamepad.wButtons;

        XINPUT_STATE state = {};
        DWORD result = XInputGetState(static_cast<DWORD>(i), &state);

        if (result == ERROR_SUCCESS)
        {
            pad.state = state;
            pad.connected = true;
        }
        else
        {
            pad.state = {};
            pad.connected = false;
        }
    }
}

bool Gamepad::IsConnected(int pad) const
{
    if (pad < 0 || pad >= k_MaxPads) return false;
    return m_pads[pad].connected;
}

bool Gamepad::IsButtonDown(int pad, int button) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return false;
    return (m_pads[pad].state.Gamepad.wButtons & static_cast<WORD>(button)) != 0;
}

bool Gamepad::IsButtonTriggered(int pad, int button) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return false;
    WORD btn = static_cast<WORD>(button);
    bool current  = (m_pads[pad].state.Gamepad.wButtons & btn) != 0;
    bool previous = (m_pads[pad].previousButtons & btn) != 0;
    return current && !previous;
}

bool Gamepad::IsButtonReleased(int pad, int button) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return false;
    WORD btn = static_cast<WORD>(button);
    bool current  = (m_pads[pad].state.Gamepad.wButtons & btn) != 0;
    bool previous = (m_pads[pad].previousButtons & btn) != 0;
    return !current && previous;
}

float Gamepad::ApplyStickDeadZone(short value, short maxVal) const
{
    float normalized = static_cast<float>(value) / static_cast<float>(maxVal);
    if (normalized > -k_StickDeadZone && normalized < k_StickDeadZone)
        return 0.0f;

    // デッドゾーン外の値を0〜1に再マッピング
    float sign = (normalized > 0.0f) ? 1.0f : -1.0f;
    float absVal = fabsf(normalized);
    float remapped = (absVal - k_StickDeadZone) / (1.0f - k_StickDeadZone);
    return sign * (std::min)(remapped, 1.0f);
}

float Gamepad::ApplyTriggerDeadZone(BYTE value) const
{
    float normalized = static_cast<float>(value) / 255.0f;
    if (normalized < k_TriggerDeadZone)
        return 0.0f;
    return (normalized - k_TriggerDeadZone) / (1.0f - k_TriggerDeadZone);
}

float Gamepad::GetLeftStickX(int pad) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return 0.0f;
    return ApplyStickDeadZone(m_pads[pad].state.Gamepad.sThumbLX, 32767);
}

float Gamepad::GetLeftStickY(int pad) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return 0.0f;
    return ApplyStickDeadZone(m_pads[pad].state.Gamepad.sThumbLY, 32767);
}

float Gamepad::GetRightStickX(int pad) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return 0.0f;
    return ApplyStickDeadZone(m_pads[pad].state.Gamepad.sThumbRX, 32767);
}

float Gamepad::GetRightStickY(int pad) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return 0.0f;
    return ApplyStickDeadZone(m_pads[pad].state.Gamepad.sThumbRY, 32767);
}

float Gamepad::GetLeftTrigger(int pad) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return 0.0f;
    return ApplyTriggerDeadZone(m_pads[pad].state.Gamepad.bLeftTrigger);
}

float Gamepad::GetRightTrigger(int pad) const
{
    if (pad < 0 || pad >= k_MaxPads || !m_pads[pad].connected) return 0.0f;
    return ApplyTriggerDeadZone(m_pads[pad].state.Gamepad.bRightTrigger);
}

} // namespace GX
