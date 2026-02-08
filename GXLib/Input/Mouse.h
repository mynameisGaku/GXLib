#pragma once
/// @file Mouse.h
/// @brief マウス入力管理
///
/// 【初学者向け解説】
/// マウスの座標、ボタン状態、ホイール回転を管理するクラスです。
/// Win32のWM_MOUSEMOVE, WM_LBUTTONDOWN等のメッセージを受信して状態を更新します。
///
/// DXLibのGetMousePoint(), GetMouseInput()に相当する機能を提供します。

#include "pch.h"

namespace GX
{

/// @brief マウスボタン定数
namespace MouseButton
{
    constexpr int Left   = 0;
    constexpr int Right  = 1;
    constexpr int Middle = 2;
    constexpr int Count  = 3;
}

/// @brief マウス入力を管理するクラス
class Mouse
{
public:
    Mouse() = default;
    ~Mouse() = default;

    /// 初期化
    void Initialize();

    /// フレーム更新（前フレーム状態を保存、ホイールデルタをリセット）
    void Update();

    /// Win32メッセージを処理
    bool ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// マウスX座標（クライアント領域）
    int GetX() const { return m_x; }

    /// マウスY座標（クライアント領域）
    int GetY() const { return m_y; }

    /// 前フレームからのX移動量
    int GetDeltaX() const { return m_x - m_prevX; }

    /// 前フレームからのY移動量
    int GetDeltaY() const { return m_y - m_prevY; }

    /// ホイール回転量（正=上、負=下）
    int GetWheel() const { return m_wheelDelta; }

    /// ボタンが押されているか
    bool IsButtonDown(int button) const;

    /// ボタンが今フレーム押されたか
    bool IsButtonTriggered(int button) const;

    /// ボタンが今フレーム離されたか
    bool IsButtonReleased(int button) const;

private:
    int m_x = 0, m_y = 0;           ///< 現在座標
    int m_prevX = 0, m_prevY = 0;   ///< 前フレーム座標
    int m_wheelDelta = 0;            ///< ホイール回転量
    int m_wheelAccum = 0;            ///< フレーム内ホイール蓄積

    std::array<bool, MouseButton::Count> m_currentButtons = {};
    std::array<bool, MouseButton::Count> m_previousButtons = {};
    std::array<bool, MouseButton::Count> m_rawButtons = {};
};

} // namespace GX
