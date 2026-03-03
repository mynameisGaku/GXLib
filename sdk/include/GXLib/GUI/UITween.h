#pragma once
/// @file UITween.h
/// @brief ウィジェットをアニメーションさせるトゥイーンシステム
///
/// 透明度・位置・サイズ・色などのプロパティを時間ベースで滑らかに変化させる。
/// イージング種別・遅延・スタッガー（順番にずらす）に対応し、
/// FadeIn / SlideIn などのよく使うプリセットも用意している。
/// @addtogroup grp_gui/// @{

#include "pch_graphics.h"
#include <functional>

namespace gx { namespace GUI {

class Widget;

/// @brief アニメーションハンドル型
using UIAnimHandle = uint32_t;

/// @brief 無効なアニメーションハンドル
static constexpr UIAnimHandle k_InvalidAnimHandle = 0;

/// @brief アニメーション対象プロパティ
enum class UIAnimProperty : uint32_t
{
    Opacity,    ///< 不透明度
    PositionX,  ///< レイアウト矩形のX座標
    PositionY,  ///< レイアウト矩形のY座標
    Width,      ///< 幅
    Height,     ///< 高さ
    ScaleX,     ///< X方向スケール
    ScaleY,     ///< Y方向スケール
    Rotation,   ///< 回転角度（度）
    ColorR,     ///< 背景色の赤成分
    ColorG,     ///< 背景色の緑成分
    ColorB,     ///< 背景色の青成分
    ColorA,     ///< 背景色のアルファ成分
};

/// @brief イージング種別
enum class UIEaseType : uint32_t
{
    Linear,         ///< 線形
    EaseInQuad,     ///< 二次の加速
    EaseOutQuad,    ///< 二次の減速
    EaseInOutQuad,  ///< 二次の加速→減速
    EaseInCubic,    ///< 三次の加速
    EaseOutCubic,   ///< 三次の減速
    EaseInOutCubic, ///< 三次の加速→減速
    EaseOutBack,    ///< オーバーシュート付き減速
    EaseOutBounce,  ///< バウンス減速
    EaseOutElastic, ///< 弾性振動
};

/// @brief UIアニメーション/トゥイーンマネージャー
///
/// ウィジェットのプロパティを時間ベースで補間するアニメーションを管理する。
/// UIContext から毎フレーム Update() が呼ばれる。
class UITweenManager
{
public:
    UITweenManager() = default;
    ~UITweenManager() = default;

    /// @brief アニメーションを開始する
    /// @param widget 対象ウィジェット
    /// @param prop アニメーション対象プロパティ
    /// @param from 開始値
    /// @param to 終了値
    /// @param duration アニメーション時間（秒）
    /// @param ease イージング種別
    /// @return アニメーションハンドル
    UIAnimHandle Animate(Widget* widget, UIAnimProperty prop,
                         float from, float to, float duration,
                         UIEaseType ease = UIEaseType::EaseOutQuad);

    /// @brief 遅延付きアニメーションを開始する
    /// @param widget 対象ウィジェット
    /// @param prop アニメーション対象プロパティ
    /// @param from 開始値
    /// @param to 終了値
    /// @param duration アニメーション時間（秒）
    /// @param delay 開始までの遅延時間（秒）
    /// @param ease イージング種別
    /// @return アニメーションハンドル
    UIAnimHandle AnimateDelayed(Widget* widget, UIAnimProperty prop,
                                float from, float to, float duration,
                                float delay, UIEaseType ease = UIEaseType::EaseOutQuad);

    /// @brief 子ウィジェットに対してスタッガーアニメーションを実行する
    /// @param parent 親ウィジェット
    /// @param prop アニメーション対象プロパティ
    /// @param from 開始値
    /// @param to 終了値
    /// @param duration 各アニメーションの時間（秒）
    /// @param staggerDelay 子ウィジェット間の遅延（秒）
    /// @param ease イージング種別
    void StaggerChildren(Widget* parent, UIAnimProperty prop,
                         float from, float to, float duration,
                         float staggerDelay, UIEaseType ease = UIEaseType::EaseOutQuad);

    /// @brief 指定ハンドルのアニメーションをキャンセルする
    void Cancel(UIAnimHandle handle);

    /// @brief 指定ウィジェットの全アニメーションをキャンセルする
    void CancelAll(Widget* widget);

    /// @brief 全アニメーションを更新する（毎フレーム呼ぶ）
    /// @param dt 前フレームからの経過時間（秒）
    void Update(float dt);

    /// @brief 指定ハンドルのアニメーションが実行中かどうか
    bool IsAnimating(UIAnimHandle handle) const;

    /// @brief アクティブなアニメーション数を取得する
    uint32_t GetActiveAnimationCount() const { return static_cast<uint32_t>(m_animations.size()); }

    // --- プリセット ---

    /// @brief フェードインアニメーション（透明→不透明）
    UIAnimHandle FadeIn(Widget* widget, float duration = 0.3f);

    /// @brief フェードアウトアニメーション（不透明→透明）
    UIAnimHandle FadeOut(Widget* widget, float duration = 0.3f);

    /// @brief 左からスライドイン
    UIAnimHandle SlideInFromLeft(Widget* widget, float distance, float duration = 0.4f);

    /// @brief 右からスライドイン
    UIAnimHandle SlideInFromRight(Widget* widget, float distance, float duration = 0.4f);

    /// @brief スケールインアニメーション（0→1）
    UIAnimHandle ScaleIn(Widget* widget, float duration = 0.3f);

    /// @brief バウンスインアニメーション
    UIAnimHandle BounceIn(Widget* widget, float duration = 0.5f);

    /// @brief アニメーション完了時のコールバックを設定する
    void SetOnComplete(UIAnimHandle handle, std::function<void()> callback);

private:
    struct Animation
    {
        UIAnimHandle handle;                    ///< アニメーションハンドル
        Widget* widget;                         ///< 対象ウィジェット
        UIAnimProperty property;                ///< アニメーション対象プロパティ
        float from;                             ///< 開始値
        float to;                               ///< 終了値
        float duration;                         ///< アニメーション時間（秒）
        float delay;                            ///< 開始遅延（秒）
        float elapsed;                          ///< 経過時間（秒）
        UIEaseType ease;                        ///< イージング種別
        std::function<void()> onComplete;       ///< 完了時コールバック
        bool started;                           ///< 開始済みか（遅延考慮）
    };

    float EaseFunction(UIEaseType type, float t) const;
    void ApplyValue(Widget* widget, UIAnimProperty prop, float value);
    float GetCurrentValue(Widget* widget, UIAnimProperty prop) const;

    gx::Vector<Animation> m_animations;  ///< アクティブなアニメーション配列
    UIAnimHandle m_nextHandle = 1;       ///< 次に割り当てるハンドル
};

}} // namespace gx::GUI
/// @}
