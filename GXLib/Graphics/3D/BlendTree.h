#pragma once
/// @file BlendTree.h
/// @brief 1D/2Dブレンドツリー（パラメータ駆動のアニメーションブレンド）
/// 初学者向け: 速度やステアリングなどのパラメータに応じて複数アニメを自動ブレンドします。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"

namespace GX
{

/// @brief ブレンドツリーの種類
enum class BlendTreeType { Simple1D, SimpleDirectional2D };

/// @brief ブレンドツリーのノード（1クリップ + パラメータ位置）
/// 初学者向け: 1Dならthreshold値、2Dならposition[2]でクリップの「場所」を決めます。
struct BlendTreeNode
{
    const AnimationClip* clip = nullptr;
    float threshold = 0.0f;     ///< 1D用の閾値
    float position[2] = {0, 0}; ///< 2D用の位置
};

/// @brief 1D/2Dブレンドツリー
/// 初学者向け: パラメータ値を変えるだけで、走り↔歩き↔止まりが自動でブレンドされます。
class BlendTree
{
public:
    void SetType(BlendTreeType type) { m_type = type; }
    BlendTreeType GetType() const { return m_type; }

    void AddNode(const BlendTreeNode& node);
    void ClearNodes();
    const std::vector<BlendTreeNode>& GetNodes() const { return m_nodes; }

    void SetParameter(float value) { m_param1D = value; }
    void SetParameter2D(float x, float y) { m_param2D[0] = x; m_param2D[1] = y; }
    float GetParameter() const { return m_param1D; }

    /// @brief ブレンドツリーを評価し最終ポーズを計算
    /// @param time アニメーション時間（秒）
    /// @param jointCount 関節の数
    /// @param bindPose バインドポーズ
    /// @param outPose 出力先のポーズ配列
    void Evaluate(float time, uint32_t jointCount,
                  const TransformTRS* bindPose,
                  TransformTRS* outPose) const;

    /// @brief ブレンドツリー内の最長クリップの再生時間を返す
    float GetDuration() const;

private:
    void Evaluate1D(float time, uint32_t jointCount,
                    const TransformTRS* bindPose,
                    TransformTRS* outPose) const;
    void Evaluate2D(float time, uint32_t jointCount,
                    const TransformTRS* bindPose,
                    TransformTRS* outPose) const;

    /// @brief 2つのTRS配列をlerp/slerpでブレンド
    static void BlendPoses(const TransformTRS* a, const TransformTRS* b,
                           float t, uint32_t count, TransformTRS* out);

    BlendTreeType m_type = BlendTreeType::Simple1D;
    std::vector<BlendTreeNode> m_nodes;
    float m_param1D = 0.0f;
    float m_param2D[2] = {0, 0};

    mutable std::vector<TransformTRS> m_tempA;
    mutable std::vector<TransformTRS> m_tempB;
    mutable std::vector<TransformTRS> m_tempC;
};

} // namespace GX
