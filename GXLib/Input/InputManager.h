#pragma once
/// @file InputManager.h
/// @brief 入力システム統合マネージャー
///
/// キーボード・マウス・ゲームパッドの3デバイスをまとめて管理する。
/// DxLibではCheckHitKeyやGetMousePointなどがグローバル関数として提供されるが、
/// GXLibではこのクラスに集約している。DxLib互換のAPIも用意しているので、
/// DxLibからの移行も容易。

#include "pch.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "Input/Gamepad.h"

namespace GX
{

class Window;

/// @brief キーボード・マウス・ゲームパッドを統合管理するクラス
class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    /// @brief 入力システムを初期化し、ウィンドウにメッセージコールバックを登録する
    /// @param window メッセージを受け取るウィンドウ
    void Initialize(Window& window);

    /// @brief 全入力デバイスのフレーム更新を行う。毎フレーム1回呼ぶこと
    void Update();

    /// @brief 終了処理を行う
    void Shutdown();

    // --- キーボードAPI ---

    /// @brief Keyboardオブジェクトへの参照を取得する。トリガー判定等はこちらを使う
    /// @return Keyboardへの参照
    Keyboard& GetKeyboard() { return m_keyboard; }

    /// @brief キーが押されているか判定する（DxLibのCheckHitKey互換）
    /// @param keyCode 仮想キーコード（VK_UP, VK_SPACEなど）
    /// @return 1=押されている、0=離されている
    int CheckHitKey(int keyCode) const;

    // --- マウスAPI ---

    /// @brief Mouseオブジェクトへの参照を取得する。トリガー判定等はこちらを使う
    /// @return Mouseへの参照
    Mouse& GetMouse() { return m_mouse; }

    /// @brief マウスボタン入力をビットフラグで取得する（DxLibのGetMouseInput互換）
    /// @return ビットフラグ: bit0=左(1), bit1=右(2), bit2=中(4)
    int GetMouseInput() const;

    /// @brief マウス座標を取得する（DxLibのGetMousePoint互換）
    /// @param x X座標の格納先（nullptrで省略可）
    /// @param y Y座標の格納先（nullptrで省略可）
    void GetMousePoint(int* x, int* y) const;

    /// @brief マウスホイール回転量を取得する（DxLibのGetMouseWheelRotVol互換）
    /// @return 回転ノッチ数。正=上回転、負=下回転
    int GetMouseWheel() const;

    // --- ゲームパッドAPI ---

    /// @brief Gamepadオブジェクトへの参照を取得する
    /// @return Gamepadへの参照
    Gamepad& GetGamepad() { return m_gamepad; }

private:
    Keyboard m_keyboard;
    Mouse    m_mouse;
    Gamepad  m_gamepad;
};

} // namespace GX
