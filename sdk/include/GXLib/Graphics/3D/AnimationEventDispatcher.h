#pragma once
/// @file AnimationEventDispatcher.h
/// @brief アニメーションイベントの発火管理 — 特定フレームでコールバックを呼ぶ
///
/// AnimationClip に埋め込んだ AnimationEvent を再生時刻に応じて検出し、
/// 登録済みコールバックを呼び出す。足音・攻撃ヒット・エフェクト発生などを
/// アニメーションのタイミングに合わせて実行したい場面で使う。
/// @addtogroup grp_gfx_3d/// @{
#include "pch_graphics.h"
#include "Graphics/3D/AnimationClip.h"
#include <functional>
#include <unordered_map>

namespace gx
{

/// @brief アニメーションイベントコールバック型
using AnimationEventCallback = std::function<void(const AnimationEvent& event)>;

/// @brief アニメーションイベントディスパッチャー
/// AnimationPlayerと連携して、再生中のアニメーション内の特定時刻でイベントを発火する。
class AnimationEventDispatcher
{
public:
    AnimationEventDispatcher() = default;
    ~AnimationEventDispatcher() = default;

    /// @brief イベント名に対するコールバックを登録する
    /// @param eventName イベント名
    /// @param callback コールバック関数
    void RegisterHandler(const gx::String& eventName, AnimationEventCallback callback);

    /// @brief 全イベント共通のコールバックを登録する
    /// @param callback コールバック関数
    void RegisterGlobalHandler(AnimationEventCallback callback);

    /// @brief 指定イベント名のハンドラを解除する
    /// @param eventName 解除するイベント名
    void UnregisterHandler(const gx::String& eventName);

    /// @brief 全てのハンドラを解除する
    void ClearAllHandlers();

    /// @brief フレーム更新 -- 前回時刻から現在時刻の間にあるイベントを発火する
    /// @param clip アニメーションクリップ
    /// @param previousTime 前フレームの再生時刻(秒)
    /// @param currentTime 現在フレームの再生時刻(秒)
    void Update(const AnimationClip* clip, float previousTime, float currentTime);

    /// @brief 発火済みイベント数を取得する（デバッグ用）
    uint32_t GetFiredEventCount() const { return m_firedCount; }

    /// @brief 発火済みイベント数をリセットする
    void ResetFiredCount() { m_firedCount = 0; }

    /// @brief 登録ハンドラ数を取得する
    uint32_t GetHandlerCount() const { return static_cast<uint32_t>(m_handlers.size()); }

    /// @brief 指定イベント名のハンドラが登録されているか
    /// @param eventName イベント名
    /// @return 登録されていれば true
    bool HasHandler(const gx::String& eventName) const;

private:
    gx::HashMap<gx::String, AnimationEventCallback> m_handlers;
    AnimationEventCallback m_globalHandler;
    uint32_t m_firedCount = 0;
};

} // namespace gx
/// @}
