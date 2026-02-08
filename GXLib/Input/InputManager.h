#pragma once
/// @file InputManager.h
/// @brief 入力システム統合マネージャー
///
/// 【初学者向け解説】
/// InputManagerは、キーボード・マウス・ゲームパッドの3つの入力デバイスを
/// まとめて管理するクラスです。
/// DXLib互換のAPI（CheckHitKey, GetMouseInput等）も提供します。
///
/// 使い方：
/// 1. Initialize(window) — ウィンドウにメッセージコールバックを登録
/// 2. 毎フレーム Update() — 全入力デバイスの状態を更新
/// 3. CheckHitKey() 等で入力を取得
/// 4. Shutdown() — 終了処理

#include "pch.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "Input/Gamepad.h"

namespace GX
{

class Window;

/// @brief 入力システム統合管理クラス
class InputManager
{
public:
    InputManager() = default;
    ~InputManager() = default;

    /// 初期化（Windowにメッセージコールバックを登録）
    void Initialize(Window& window);

    /// フレーム更新（全入力デバイスを更新）
    void Update();

    /// 終了処理
    void Shutdown();

    // --- キーボードAPI ---
    Keyboard& GetKeyboard() { return m_keyboard; }

    /// DXLib互換: キーが押されているか (1=押されている, 0=離されている)
    int CheckHitKey(int keyCode) const;

    // --- マウスAPI ---
    Mouse& GetMouse() { return m_mouse; }

    /// DXLib互換: マウスボタン入力を取得（ビットフラグ）
    /// MOUSE_INPUT_LEFT=1, MOUSE_INPUT_RIGHT=2, MOUSE_INPUT_MIDDLE=4
    int GetMouseInput() const;

    /// DXLib互換: マウス座標を取得
    void GetMousePoint(int* x, int* y) const;

    /// マウスホイール回転量を取得
    int GetMouseWheel() const;

    // --- ゲームパッドAPI ---
    Gamepad& GetGamepad() { return m_gamepad; }

private:
    Keyboard m_keyboard;
    Mouse    m_mouse;
    Gamepad  m_gamepad;
};

} // namespace GX
