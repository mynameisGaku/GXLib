#pragma once
/// @file BlendStack.h
/// @brief ブレンドレイヤースタック（最大8レイヤーのアニメーションブレンド）
/// 複数のアニメーションをレイヤーとして重ね、Override(上書き)やAdditive(加算)で合成する。
/// DxLibの MV1SetAttachAnimBlendRate の多段版に相当する。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"

namespace GX
{

/// @brief アニメーションブレンドレイヤーのモード
enum class AnimBlendMode
{
    Override,  ///< 上書きブレンド（weight=1で完全上書き）
    Additive,  ///< 加算ブレンド（バインドポーズとの差分をweightに応じて加算）
};

/// @brief ブレンドレイヤー1つ分の再生設定
struct BlendLayer
{
    const AnimationClip* clip = nullptr;      ///< 再生するクリップ
    float time = 0.0f;                        ///< 現在の再生時刻
    float weight = 1.0f;                      ///< ブレンドの重み (0.0~1.0)
    float speed = 1.0f;                       ///< 再生速度
    bool  loop = true;                        ///< ループ再生するか
    AnimBlendMode mode = AnimBlendMode::Override;  ///< ブレンドモード
    uint32_t maskBits = 0xFFFFFFFF;           ///< ボーン32グループマスク（ビットが立った関節だけ適用）
};

/// @brief 最大8レイヤーのアニメーションブレンドスタック
/// レイヤー0が最下層（ベース）、上位レイヤーが順にOverride/Additiveで合成される。
/// ボーンマスクを使えば「上半身だけ別のアニメーション」といった部分ブレンドも可能。
class BlendStack
{
public:
    static constexpr uint32_t k_MaxLayers = 8;

    /// @brief 指定インデックスにレイヤーを設定する
    /// @param index レイヤーインデックス (0~7)
    /// @param layer レイヤー設定
    void SetLayer(uint32_t index, const BlendLayer& layer);

    /// @brief 指定インデックスのレイヤーを除去する
    /// @param index レイヤーインデックス
    void RemoveLayer(uint32_t index);

    /// @brief 指定レイヤーの重みを変更する
    /// @param index レイヤーインデックス
    /// @param weight 重み (0.0~1.0)
    void SetLayerWeight(uint32_t index, float weight);

    /// @brief 指定レイヤーのクリップを差し替える
    /// @param index レイヤーインデックス
    /// @param clip 新しいクリップ
    void SetLayerClip(uint32_t index, const AnimationClip* clip);

    /// @brief 指定インデックスのレイヤーを取得する
    /// @param index レイヤーインデックス
    /// @return レイヤーへのポインタ（非アクティブまたは範囲外ならnullptr）
    const BlendLayer* GetLayer(uint32_t index) const;

    /// @brief アクティブなレイヤー数を取得する
    /// @return アクティブレイヤー数
    uint32_t GetActiveLayerCount() const;

    /// @brief 全レイヤーの時間を進め、合成されたポーズを計算する
    /// @param deltaTime フレーム経過時間（秒）
    /// @param jointCount 関節の数
    /// @param bindPose バインドポーズ（nullならIdentity）
    /// @param outPose 出力先のポーズ配列
    void Update(float deltaTime, uint32_t jointCount,
                const TransformTRS* bindPose,
                TransformTRS* outPose);

private:
    BlendLayer m_layers[k_MaxLayers];
    bool m_active[k_MaxLayers] = {};
    std::vector<TransformTRS> m_tempPose;  ///< レイヤーサンプル用テンポラリ
};

} // namespace GX
