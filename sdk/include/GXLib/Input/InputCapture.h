#pragma once
/// @file InputCapture.h
/// @brief 入力キャプチャ（キーリバインド用）
///
/// 設定画面でキーコンフィグを行う際に使う。
/// Begin() で入力待ち開始 → 何かキーが押されたら Captured 状態に遷移。
/// @addtogroup grp_input/// @{

#include "pch_common.h"
#include "Input/ActionMapping.h"

namespace gx
{

class Keyboard;
class Mouse;
class Gamepad;

/// @brief キャプチャ状態
enum class CaptureState
{
    Idle,      ///< 待機中
    Waiting,   ///< 入力待ち
    Captured,  ///< キャプチャ完了
    Cancelled, ///< キャンセルされた
};

/// @brief 入力キャプチャ（キーリバインドヘルパー）
class InputCapture
{
public:
    /// @brief 入力キャプチャを開始する
    /// @param cancelKey キャンセルキー（VK_*, デフォルト=Escape）
    void Begin(int cancelKey = 0x1B /* VK_ESCAPE */);

    /// @brief 毎フレーム更新
    void Update(const Keyboard& keyboard, const Mouse& mouse, const Gamepad& gamepad);

    /// @brief 現在の状態を取得する
    CaptureState GetState() const { return m_state; }

    /// @brief キャプチャ結果を取得する
    const InputBinding& GetResult() const { return m_result; }

    /// @brief キャプチャ結果をActionMappingに適用する
    /// @param mapping 適用先のActionMapping
    /// @param actionName アクション名
    /// @param bindingIndex バインディングインデックス（-1で追加）
    /// @return 成功でtrue
    bool ApplyToAction(ActionMapping& mapping, const gx::String& actionName, int bindingIndex = -1) const;

    /// @brief キャプチャをリセットする
    void Reset();

private:
    CaptureState m_state = CaptureState::Idle;
    int m_cancelKey = 0x1B;
    InputBinding m_result;
};

} // namespace gx
/// @}
