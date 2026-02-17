#pragma once
/// @file BlendStack.h
/// @brief ブレンドレイヤースタック（最大8レイヤーのアニメーションブレンド）
/// 初学者向け: 複数のアニメーションを層のように重ねて合成するシステムです。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"

namespace GX
{

/// @brief アニメーションブレンドレイヤーのモード
/// 初学者向け: Override は上書き、Additive は差分を加算します。
enum class AnimBlendMode { Override, Additive };

/// @brief ブレンドレイヤー（1クリップ + ブレンド設定）
/// 初学者向け: アニメーション1つ分の再生情報と重み・マスク設定をまとめたものです。
struct BlendLayer
{
    const AnimationClip* clip = nullptr;
    float time = 0.0f;
    float weight = 1.0f;
    float speed = 1.0f;
    bool  loop = true;
    AnimBlendMode mode = AnimBlendMode::Override;
    uint32_t maskBits = 0xFFFFFFFF; ///< ボーン32グループマスク
};

/// @brief 最大8レイヤーのブレンドスタック
/// 初学者向け: レイヤー0が一番下（ベース）で、上のレイヤーが順に上書き/加算されます。
class BlendStack
{
public:
    static constexpr uint32_t k_MaxLayers = 8;

    void SetLayer(uint32_t index, const BlendLayer& layer);
    void RemoveLayer(uint32_t index);
    void SetLayerWeight(uint32_t index, float weight);
    void SetLayerClip(uint32_t index, const AnimationClip* clip);
    const BlendLayer* GetLayer(uint32_t index) const;
    uint32_t GetActiveLayerCount() const;

    /// @brief 全レイヤーを更新し最終ポーズを計算
    /// @param deltaTime フレーム経過時間
    /// @param jointCount 関節の数
    /// @param bindPose バインドポーズ（nullならIdentity）
    /// @param outPose 出力先のポーズ配列
    void Update(float deltaTime, uint32_t jointCount,
                const TransformTRS* bindPose,
                TransformTRS* outPose);

private:
    BlendLayer m_layers[k_MaxLayers];
    bool m_active[k_MaxLayers] = {};
    std::vector<TransformTRS> m_tempPose;
};

} // namespace GX
