#pragma once
/// @file AnimationLayer.h
/// @brief アニメーションレイヤー — ボーンマスクと複数レイヤーによる部分ブレンド
///
/// BoneMask で「上半身だけ」「右腕だけ」のような部分的なウェイト分布を定義し、
/// Override（上書き）またはAdditive（加算）でアニメーションを重ねる。
/// Animator の BlendStack モードよりきめ細かなボーン単位の制御が必要な場合に使う。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Math/MathConvert.h"
#include <vector>
#include <string>

namespace gx
{

/// @brief ブレンドモード
enum class AnimLayerBlendMode
{
    Override,   ///< 上書き
    Additive    ///< 加算
};

/// @brief ボーンマスク
class BoneMask
{
public:
    BoneMask() = default;

    /// @brief ボーン数で初期化（全ウェイト1.0）
    void Initialize(uint32_t boneCount);

    /// @brief ボーン毎のウェイト設定
    void SetWeight(uint32_t boneIndex, float weight);

    /// @brief ウェイト取得
    float GetWeight(uint32_t boneIndex) const;

    /// @brief 全ボーンのウェイト設定
    void SetAll(float weight);

    /// @brief 指定ボーン以下のサブツリーを設定（ボーンの親子関係はindexの順を仮定）
    void SetFromJointDown(uint32_t startBone, uint32_t endBone, float weight);

    /// @brief ボーン数
    uint32_t GetBoneCount() const { return static_cast<uint32_t>(m_weights.size()); }

    /// @brief 全ウェイトをクリア（0.0に設定）
    void ClearAll();

    /// @brief 有効なマスクか（1つでもウェイト>0のボーンがある）
    bool IsActive() const;

private:
    gx::Vector<float> m_weights;  ///< ボーンごとのウェイト値（0〜1）
};

/// @brief アニメーションレイヤー
struct AnimationLayer
{
    gx::String name;                                           ///< レイヤー名
    float weight = 1.0f;                                       ///< ブレンドウェイト（0〜1）
    AnimLayerBlendMode blendMode = AnimLayerBlendMode::Override;///< ブレンドモード
    BoneMask boneMask;                                         ///< ボーンマスク
    bool enabled = true;                                       ///< 有効フラグ
};

/// @brief ポーズデータ（ボーン毎の位置/回転/スケール）
struct BonePose
{
    Vector3    position = { 0, 0, 0 };    ///< ボーンの位置
    Quaternion rotation = { 0, 0, 0, 1 }; ///< ボーンの回転（クォータニオン）
    Vector3    scale    = { 1, 1, 1 };     ///< ボーンのスケール
};

/// @brief アニメーションレイヤースタック
class AnimationLayerStack
{
public:
    AnimationLayerStack() = default;
    ~AnimationLayerStack() = default;

    /// @brief レイヤーを追加
    uint32_t AddLayer(const gx::String& name, AnimLayerBlendMode mode = AnimLayerBlendMode::Override);

    /// @brief レイヤーを削除
    void RemoveLayer(uint32_t index);

    /// @brief レイヤー数
    uint32_t GetLayerCount() const { return static_cast<uint32_t>(m_layers.size()); }

    /// @brief レイヤー取得
    AnimationLayer* GetLayer(uint32_t index);
    const AnimationLayer* GetLayer(uint32_t index) const;

    /// @brief レイヤー名で検索
    int FindLayer(const gx::String& name) const;

    /// @brief ウェイト設定
    void SetLayerWeight(uint32_t index, float weight);

    /// @brief 有効/無効
    void SetLayerEnabled(uint32_t index, bool enabled);

    /// @brief ベースポーズに全レイヤーを合成
    /// @param basePose ベースポーズ（入出力）
    /// @param layerPoses 各レイヤーのポーズ [layerIndex][boneIndex]
    /// @param boneCount ボーン数
    void Evaluate(BonePose* basePose,
                  const gx::Vector<gx::Vector<BonePose>>& layerPoses,
                  uint32_t boneCount) const;

    /// @brief 全レイヤーをクリア
    void Clear();

private:
    gx::Vector<AnimationLayer> m_layers;  ///< レイヤー配列（評価順）
};

} // namespace gx
/// @}
