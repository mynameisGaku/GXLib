#pragma once
/// @file Mouse.h
/// @brief マウス入力管理
///
/// DxLibのGetMousePoint / GetMouseInputに相当する機能を提供する。
/// Win32のWM_MOUSEMOVE, WM_LBUTTONDOWN等のメッセージからマウス座標・
/// ボタン押下・ホイール回転を毎フレーム管理する。

#include "pch.h"

namespace GX
{

/// @brief マウスボタン識別用の定数。IsButtonDown等の引数に使う
namespace MouseButton
{
    constexpr int Left   = 0;   ///< 左ボタン
    constexpr int Right  = 1;   ///< 右ボタン
    constexpr int Middle = 2;   ///< 中ボタン（ホイールクリック）
    constexpr int Count  = 3;   ///< ボタン総数
}

/// @brief マウス入力を管理するクラス（DxLibのGetMousePoint / GetMouseInput相当）
class Mouse
{
public:
    Mouse() = default;
    ~Mouse() = default;

    /// @brief マウス状態を初期化する
    void Initialize();

    /// @brief フレーム更新。前フレーム状態を保存し、ホイール蓄積値を確定する
    void Update();

    /// @brief Win32メッセージを処理して座標・ボタン・ホイール状態を更新する
    /// @param hwnd ウィンドウハンドル
    /// @param msg Win32メッセージID
    /// @param wParam メッセージパラメータ（ホイールデルタ等）
    /// @param lParam マウス座標等の付加情報
    /// @return このクラスが処理したメッセージならtrue
    bool ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// @brief マウスX座標を取得する（ウィンドウのクライアント領域基準）
    /// @return X座標（ピクセル）
    int GetX() const { return m_x; }

    /// @brief マウスY座標を取得する（ウィンドウのクライアント領域基準）
    /// @return Y座標（ピクセル）
    int GetY() const { return m_y; }

    /// @brief 前フレームからのX方向移動量を取得する
    /// @return X移動量（ピクセル）
    int GetDeltaX() const { return m_x - m_prevX; }

    /// @brief 前フレームからのY方向移動量を取得する
    /// @return Y移動量（ピクセル）
    int GetDeltaY() const { return m_y - m_prevY; }

    /// @brief ホイール回転量を取得する（DxLibのGetMouseWheelRotVol相当）
    /// @return 回転ノッチ数。正=上回転、負=下回転
    int GetWheel() const { return m_wheelDelta; }

    /// @brief ボタンが押されているか判定する
    /// @param button MouseButton::Left / Right / Middleのいずれか
    /// @return 押されていればtrue
    bool IsButtonDown(int button) const;

    /// @brief ボタンが今フレーム押されたか判定する（トリガー判定）
    /// @param button MouseButton::Left / Right / Middleのいずれか
    /// @return 前フレームは離されていて今フレーム押されていればtrue
    bool IsButtonTriggered(int button) const;

    /// @brief ボタンが今フレーム離されたか判定する
    /// @param button MouseButton::Left / Right / Middleのいずれか
    /// @return 前フレームは押されていて今フレーム離されていればtrue
    bool IsButtonReleased(int button) const;

private:
    int m_x = 0, m_y = 0;           ///< 現在座標
    int m_prevX = 0, m_prevY = 0;   ///< 前フレーム座標
    int m_wheelDelta = 0;            ///< 確定済みのホイール回転量（Update時に反映）
    int m_wheelAccum = 0;            ///< フレーム内のホイール蓄積値（複数WM_MOUSEWHEELを合算）

    std::array<bool, MouseButton::Count> m_currentButtons = {};   ///< 現在フレームのボタン状態
    std::array<bool, MouseButton::Count> m_previousButtons = {};  ///< 前フレームのボタン状態
    std::array<bool, MouseButton::Count> m_rawButtons = {};       ///< メッセージから受信した生のボタン状態
};

} // namespace GX
