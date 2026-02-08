#pragma once
/// @file Gamepad.h
/// @brief ゲームパッド入力管理（XInput対応）
///
/// 【初学者向け解説】
/// XInputはMicrosoftが提供するゲームパッド入力APIで、
/// Xboxコントローラーなどに対応しています。
/// 最大4台のゲームパッドを同時に管理できます。
///
/// スティックには「デッドゾーン」があります。スティックを触っていなくても
/// 微小な値が返ることがあるので、一定範囲以下の入力は0として扱います。

#include "pch.h"
#include <Xinput.h>

namespace GX
{

/// @brief ゲームパッドボタン定数（XInputのビットマスクと同じ）
namespace PadButton
{
    constexpr int DPadUp        = XINPUT_GAMEPAD_DPAD_UP;
    constexpr int DPadDown      = XINPUT_GAMEPAD_DPAD_DOWN;
    constexpr int DPadLeft      = XINPUT_GAMEPAD_DPAD_LEFT;
    constexpr int DPadRight     = XINPUT_GAMEPAD_DPAD_RIGHT;
    constexpr int Start         = XINPUT_GAMEPAD_START;
    constexpr int Back          = XINPUT_GAMEPAD_BACK;
    constexpr int LeftThumb     = XINPUT_GAMEPAD_LEFT_THUMB;
    constexpr int RightThumb    = XINPUT_GAMEPAD_RIGHT_THUMB;
    constexpr int LeftShoulder  = XINPUT_GAMEPAD_LEFT_SHOULDER;
    constexpr int RightShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER;
    constexpr int A             = XINPUT_GAMEPAD_A;
    constexpr int B             = XINPUT_GAMEPAD_B;
    constexpr int X             = XINPUT_GAMEPAD_X;
    constexpr int Y             = XINPUT_GAMEPAD_Y;
}

/// @brief XInput対応ゲームパッド管理クラス
class Gamepad
{
public:
    static constexpr int k_MaxPads = 4;
    static constexpr float k_StickDeadZone = 0.24f;   ///< スティックデッドゾーン（0〜1）
    static constexpr float k_TriggerDeadZone = 0.12f;  ///< トリガーデッドゾーン（0〜1）

    Gamepad() = default;
    ~Gamepad() = default;

    /// 初期化
    void Initialize();

    /// フレーム更新（XInputGetStateを呼んで状態を取得）
    void Update();

    /// コントローラが接続されているか
    bool IsConnected(int pad) const;

    /// ボタンが押されているか
    bool IsButtonDown(int pad, int button) const;

    /// ボタンが今フレーム押されたか
    bool IsButtonTriggered(int pad, int button) const;

    /// ボタンが今フレーム離されたか
    bool IsButtonReleased(int pad, int button) const;

    /// 左スティックX（-1.0〜1.0）
    float GetLeftStickX(int pad) const;

    /// 左スティックY（-1.0〜1.0）
    float GetLeftStickY(int pad) const;

    /// 右スティックX（-1.0〜1.0）
    float GetRightStickX(int pad) const;

    /// 右スティックY（-1.0〜1.0）
    float GetRightStickY(int pad) const;

    /// 左トリガー（0.0〜1.0）
    float GetLeftTrigger(int pad) const;

    /// 右トリガー（0.0〜1.0）
    float GetRightTrigger(int pad) const;

private:
    /// デッドゾーン適用
    float ApplyStickDeadZone(short value, short maxVal) const;
    float ApplyTriggerDeadZone(BYTE value) const;

    struct PadState
    {
        XINPUT_STATE state = {};
        WORD previousButtons = 0;
        bool connected = false;
    };

    std::array<PadState, k_MaxPads> m_pads = {};
};

} // namespace GX
