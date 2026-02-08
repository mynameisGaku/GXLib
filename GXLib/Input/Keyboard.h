#pragma once
/// @file Keyboard.h
/// @brief キーボード入力管理
///
/// 【初学者向け解説】
/// キーボードの状態を毎フレーム管理するクラスです。
/// 各キーについて「押されている」「今押された（トリガー）」「今離された」を判定できます。
///
/// Win32のWM_KEYDOWN/WM_KEYUPメッセージを受信し、
/// 256個のキー配列で状態を管理します。
/// 仮想キーコード（VK_UP, VK_SPACEなど）でアクセスします。

#include "pch.h"

namespace GX
{

/// @brief キーボード入力を管理するクラス
class Keyboard
{
public:
    static constexpr int k_KeyCount = 256;

    Keyboard() = default;
    ~Keyboard() = default;

    /// 初期化（全キーをリセット）
    void Initialize();

    /// フレーム更新（前フレーム状態を保存、現在フレームの状態を確定）
    void Update();

    /// Win32メッセージを処理
    /// @return メッセージを処理した場合true
    bool ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    /// キーが押されているか
    bool IsKeyDown(int key) const;

    /// キーが今フレーム押されたか（トリガー）
    bool IsKeyTriggered(int key) const;

    /// キーが今フレーム離されたか
    bool IsKeyReleased(int key) const;

private:
    std::array<bool, k_KeyCount> m_currentState = {};   ///< 現在フレームのキー状態
    std::array<bool, k_KeyCount> m_previousState = {};   ///< 前フレームのキー状態
    std::array<bool, k_KeyCount> m_rawState = {};        ///< メッセージから受信した生の状態
};

} // namespace GX
