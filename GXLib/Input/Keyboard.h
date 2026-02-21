#pragma once
/// @file Keyboard.h
/// @brief キーボード入力管理
///
/// DxLibのCheckHitKey / GetHitKeyStateAllに相当する機能を提供する。
/// Win32のWM_KEYDOWN/WM_KEYUPメッセージを受信し、
/// 256個の仮想キーコード（VK_UP, VK_SPACEなど）で押下状態を管理する。
/// 毎フレームUpdate()を呼ぶことで、押下/トリガー/リリースの3状態を判定できる。

#include "pch.h"

namespace GX
{

/// @brief キーボード入力を管理するクラス（DxLibのCheckHitKey相当）
class Keyboard
{
public:
    static constexpr int k_KeyCount = 256;  ///< Win32仮想キーコードの総数

    Keyboard() = default;
    ~Keyboard() = default;

    /// @brief 全キー状態を初期化する
    void Initialize();

    /// @brief フレーム更新。前フレーム状態を保存し、メッセージで受信した生の入力を現在の状態に反映する
    void Update();

    /// @brief Win32メッセージを処理してキー状態を更新する
    /// @param msg Win32メッセージID（WM_KEYDOWN等）
    /// @param wParam 仮想キーコード
    /// @param lParam キーメッセージの付加情報（未使用）
    /// @return このクラスが処理したメッセージならtrue
    bool ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    /// @brief キーが押されているか判定する（DxLibのCheckHitKey相当）
    /// @param key 仮想キーコード（VK_UP, VK_SPACEなど）
    /// @return 押されていればtrue
    bool IsKeyDown(int key) const;

    /// @brief キーが今フレーム押されたか判定する（トリガー判定）
    /// @param key 仮想キーコード
    /// @return 前フレームは離されていて今フレーム押されていればtrue
    bool IsKeyTriggered(int key) const;

    /// @brief キーが今フレーム離されたか判定する
    /// @param key 仮想キーコード
    /// @return 前フレームは押されていて今フレーム離されていればtrue
    bool IsKeyReleased(int key) const;

private:
    std::array<bool, k_KeyCount> m_currentState = {};   ///< 現在フレームのキー状態
    std::array<bool, k_KeyCount> m_previousState = {};  ///< 前フレームのキー状態
    std::array<bool, k_KeyCount> m_rawState = {};       ///< メッセージから受信した生の状態（Update時にcurrentへコピー）
};

} // namespace GX
