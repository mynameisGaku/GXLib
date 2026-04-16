#pragma once
/// @file TransitionEffect.h
/// @brief シーン切り替え時の画面エフェクト定義
///
/// フェード・ワイプ・ディゾルブ・円形マスクなど 8 種類の遷移を提供。
/// ITransitionEffect を継承して独自エフェクトも追加できる。
/// progress (0→1) からピクセルごとのマスク値を計算する純粋 CPU 実装。
/// @addtogroup grp_scene/// @{
#include "pch_common.h"
#include <memory>
#include <cmath>

namespace gx
{

/// @brief 遷移エフェクトの種類
enum class TransitionType
{
    Fade,         ///< 単色フェード
    Wipe,         ///< 横方向ワイプ
    WipeVertical, ///< 縦方向ワイプ
    CircleOpen,   ///< 円形マスク(開く)
    CircleClose,  ///< 円形マスク(閉じる)
    Dissolve,     ///< ランダムディゾルブ
    SlideLeft,    ///< スライド左
    SlideRight,   ///< スライド右
};

/// @brief 遷移エフェクトのパラメータ
struct TransitionParams
{
    float centerX = 0.5f;   ///< エフェクト中心X (0-1, 正規化)
    float centerY = 0.5f;   ///< エフェクト中心Y (0-1, 正規化)
    float r = 0.0f, g = 0.0f, b = 0.0f;  ///< フェード色 (Fade用)
    float softEdge = 0.05f; ///< エッジの柔らかさ (0-1)
};

/// @brief シーン遷移エフェクト基底
class ITransitionEffect
{
public:
    virtual ~ITransitionEffect() = default;

    /// @brief 遷移の進行度(0→1)からアルファマスク値を計算
    /// @param progress 遷移進行度 0.0(開始)~1.0(完了)
    /// @param screenX スクリーン上のX座標(正規化 0-1)
    /// @param screenY スクリーン上のY座標(正規化 0-1)
    /// @return マスク値 0.0(透明)~1.0(不透明)
    virtual float GetMaskValue(float progress, float screenX, float screenY) const = 0;

    /// @brief エフェクトの種類を取得する
    /// @return 遷移エフェクトの種類
    virtual TransitionType GetType() const = 0;
};

/// @brief 組み込み遷移エフェクト
class BuiltinTransition : public ITransitionEffect
{
public:
    /// @brief コンストラクタ
    /// @param type エフェクトの種類
    /// @param params エフェクトパラメータ
    BuiltinTransition(TransitionType type = TransitionType::Fade, const TransitionParams& params = {});

    /// @copydoc ITransitionEffect::GetMaskValue
    float GetMaskValue(float progress, float screenX, float screenY) const override;

    /// @copydoc ITransitionEffect::GetType
    TransitionType GetType() const override { return m_type; }

    /// @brief パラメータを設定する
    /// @param params 新しいエフェクトパラメータ
    void SetParams(const TransitionParams& params) { m_params = params; }

    /// @brief パラメータを取得する
    /// @return エフェクトパラメータへの参照
    const TransitionParams& GetParams() const { return m_params; }

private:
    TransitionType m_type;       ///< エフェクトの種類
    TransitionParams m_params;   ///< エフェクトパラメータ
};

/// @brief 遷移エフェクトマネージャー（SceneManagerと連携）
class TransitionEffectManager
{
public:
    TransitionEffectManager() = default;

    /// @brief カスタムエフェクトを設定する
    /// @param effect エフェクトオブジェクト（所有権を移譲）
    void SetEffect(std::unique_ptr<ITransitionEffect> effect);

    /// @brief 組み込みエフェクトを設定する
    /// @param type エフェクトの種類
    /// @param params エフェクトパラメータ
    void SetEffect(TransitionType type, const TransitionParams& params = {});

    /// @brief 現在のエフェクトを取得する
    /// @return エフェクトへのポインタ（未設定時は nullptr）
    ITransitionEffect* GetEffect() const { return m_effect.get(); }

    /// @brief 進行度を更新する
    /// @param progress 遷移の進行度 (0.0~1.0)
    void SetProgress(float progress) { m_progress = progress; }

    /// @brief 進行度を取得する
    /// @return 遷移の進行度 (0.0~1.0)
    float GetProgress() const { return m_progress; }

    /// @brief 遷移中かどうか
    /// @return 遷移中なら true
    bool IsActive() const { return m_active; }

    /// @brief 遷移を開始する
    void Begin() { m_active = true; m_progress = 0.0f; }

    /// @brief 遷移を終了する
    void End() { m_active = false; m_progress = 0.0f; }

    /// @brief 指定スクリーン座標のマスク値を取得する
    /// @param normalizedX スクリーン上のX座標(正規化 0-1)
    /// @param normalizedY スクリーン上のY座標(正規化 0-1)
    /// @return マスク値 0.0(透明)~1.0(不透明)
    float SampleMask(float normalizedX, float normalizedY) const;

private:
    std::unique_ptr<ITransitionEffect> m_effect; ///< 現在の遷移エフェクト（所有）
    float m_progress = 0.0f;                     ///< 遷移の進行度 (0.0~1.0)
    bool m_active = false;                       ///< 遷移中フラグ
};

} // namespace gx
/// @}
