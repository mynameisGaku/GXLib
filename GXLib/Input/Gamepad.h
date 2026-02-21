#pragma once
/// @file Gamepad.h
/// @brief ゲームパッド入力管理（XInput対応）
///
/// DxLibのGetJoypadInputState / GetJoypadAnalogInputに相当する機能を提供する。
/// XInput APIを使い、Xboxコントローラーなど最大4台を同時に管理する。
/// スティック・トリガーにはデッドゾーン処理を適用し、
/// 微小な入力ノイズを自動的にカットする。

#include "pch.h"
#include <Xinput.h>

namespace GX
{

/// @brief ゲームパッドボタンの定数。XInputのビットマスク値と一致している
namespace PadButton
{
    constexpr int DPadUp        = XINPUT_GAMEPAD_DPAD_UP;        ///< 十字キー上
    constexpr int DPadDown      = XINPUT_GAMEPAD_DPAD_DOWN;      ///< 十字キー下
    constexpr int DPadLeft      = XINPUT_GAMEPAD_DPAD_LEFT;      ///< 十字キー左
    constexpr int DPadRight     = XINPUT_GAMEPAD_DPAD_RIGHT;     ///< 十字キー右
    constexpr int Start         = XINPUT_GAMEPAD_START;          ///< Startボタン
    constexpr int Back          = XINPUT_GAMEPAD_BACK;           ///< Backボタン
    constexpr int LeftThumb     = XINPUT_GAMEPAD_LEFT_THUMB;     ///< 左スティック押し込み
    constexpr int RightThumb    = XINPUT_GAMEPAD_RIGHT_THUMB;    ///< 右スティック押し込み
    constexpr int LeftShoulder  = XINPUT_GAMEPAD_LEFT_SHOULDER;  ///< LBボタン
    constexpr int RightShoulder = XINPUT_GAMEPAD_RIGHT_SHOULDER; ///< RBボタン
    constexpr int A             = XINPUT_GAMEPAD_A;              ///< Aボタン
    constexpr int B             = XINPUT_GAMEPAD_B;              ///< Bボタン
    constexpr int X             = XINPUT_GAMEPAD_X;              ///< Xボタン
    constexpr int Y             = XINPUT_GAMEPAD_Y;              ///< Yボタン
}

/// @brief XInput対応ゲームパッド管理クラス（DxLibのGetJoypadInputState相当）
class Gamepad
{
public:
    static constexpr int k_MaxPads = 4;                          ///< XInputの最大パッド数
    static constexpr float k_StickDeadZone = 0.24f;              ///< スティックデッドゾーン閾値（正規化値0〜1）
    static constexpr float k_TriggerDeadZone = 0.12f;            ///< トリガーデッドゾーン閾値（正規化値0〜1）

    Gamepad() = default;
    ~Gamepad() = default;

    /// @brief パッド状態を初期化する
    void Initialize();

    /// @brief フレーム更新。XInputGetStateで全パッドの状態をポーリング取得する
    void Update();

    /// @brief 指定パッドが接続されているか判定する
    /// @param pad パッドインデックス（0〜3）
    /// @return 接続されていればtrue
    bool IsConnected(int pad) const;

    /// @brief ボタンが押されているか判定する
    /// @param pad パッドインデックス（0〜3）
    /// @param button PadButton定数（ビットマスク値）
    /// @return 押されていればtrue
    bool IsButtonDown(int pad, int button) const;

    /// @brief ボタンが今フレーム押されたか判定する（トリガー判定）
    /// @param pad パッドインデックス（0〜3）
    /// @param button PadButton定数（ビットマスク値）
    /// @return 前フレームは離されていて今フレーム押されていればtrue
    bool IsButtonTriggered(int pad, int button) const;

    /// @brief ボタンが今フレーム離されたか判定する
    /// @param pad パッドインデックス（0〜3）
    /// @param button PadButton定数（ビットマスク値）
    /// @return 前フレームは押されていて今フレーム離されていればtrue
    bool IsButtonReleased(int pad, int button) const;

    /// @brief 左スティックのX軸入力を取得する（デッドゾーン適用済み）
    /// @param pad パッドインデックス（0〜3）
    /// @return -1.0（左）〜 1.0（右）
    float GetLeftStickX(int pad) const;

    /// @brief 左スティックのY軸入力を取得する（デッドゾーン適用済み）
    /// @param pad パッドインデックス（0〜3）
    /// @return -1.0（下）〜 1.0（上）
    float GetLeftStickY(int pad) const;

    /// @brief 右スティックのX軸入力を取得する（デッドゾーン適用済み）
    /// @param pad パッドインデックス（0〜3）
    /// @return -1.0（左）〜 1.0（右）
    float GetRightStickX(int pad) const;

    /// @brief 右スティックのY軸入力を取得する（デッドゾーン適用済み）
    /// @param pad パッドインデックス（0〜3）
    /// @return -1.0（下）〜 1.0（上）
    float GetRightStickY(int pad) const;

    /// @brief 左トリガーの入力量を取得する（デッドゾーン適用済み）
    /// @param pad パッドインデックス（0〜3）
    /// @return 0.0（未入力）〜 1.0（最大）
    float GetLeftTrigger(int pad) const;

    /// @brief 右トリガーの入力量を取得する（デッドゾーン適用済み）
    /// @param pad パッドインデックス（0〜3）
    /// @return 0.0（未入力）〜 1.0（最大）
    float GetRightTrigger(int pad) const;

private:
    /// @brief スティック値にデッドゾーンを適用し、-1.0〜1.0に正規化する
    float ApplyStickDeadZone(short value, short maxVal) const;

    /// @brief トリガー値にデッドゾーンを適用し、0.0〜1.0に正規化する
    float ApplyTriggerDeadZone(BYTE value) const;

    /// @brief 1台分のパッド状態
    struct PadState
    {
        XINPUT_STATE state = {};        ///< XInputから取得した現在の状態
        WORD previousButtons = 0;       ///< 前フレームのボタンビットマスク
        bool connected = false;         ///< 接続済みか
    };

    std::array<PadState, k_MaxPads> m_pads = {};
};

} // namespace GX
