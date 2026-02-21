#pragma once
/// @file BlendTree.h
/// @brief 1D/2Dブレンドツリー（パラメータ駆動のアニメーションブレンド）
/// パラメータ値（速度、方向など）に応じて複数のアニメーションクリップを自動でブレンドする。
/// 例: パラメータ=0で「歩き」、=1で「走り」、中間値で両方をブレンド。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"

namespace GX
{

/// @brief ブレンドツリーの種類
enum class BlendTreeType
{
    Simple1D,              ///< 1パラメータ（threshold順の隣接2ノードを補間）
    SimpleDirectional2D,   ///< 2パラメータ（最近傍3ノードのバリセントリック補間）
};

/// @brief ブレンドツリーの1ノード（クリップとパラメータ空間上の位置）
struct BlendTreeNode
{
    const AnimationClip* clip = nullptr;    ///< このノードのクリップ
    float threshold = 0.0f;                 ///< 1D用の閾値（パラメータがこの値のとき100%再生）
    float position[2] = {0, 0};             ///< 2D用の座標 (x, y)
};

/// @brief パラメータ駆動の1D/2Dブレンドツリー
/// ノードを閾値（1D）または座標（2D）に配置し、パラメータ値に応じて
/// 隣接ノード間の重みを自動計算してブレンドする。
class BlendTree
{
public:
    /// @brief ブレンドツリーの種類を設定する
    /// @param type 1D or 2D
    void SetType(BlendTreeType type) { m_type = type; }

    /// @brief ブレンドツリーの種類を取得する
    /// @return ツリータイプ
    BlendTreeType GetType() const { return m_type; }

    /// @brief ノードを追加する
    /// @param node 追加するノード
    void AddNode(const BlendTreeNode& node);

    /// @brief 全ノードを削除する
    void ClearNodes();

    /// @brief 全ノードを取得する
    /// @return ノード配列
    const std::vector<BlendTreeNode>& GetNodes() const { return m_nodes; }

    /// @brief 1Dパラメータを設定する
    /// @param value パラメータ値
    void SetParameter(float value) { m_param1D = value; }

    /// @brief 2Dパラメータを設定する
    /// @param x X軸パラメータ
    /// @param y Y軸パラメータ
    void SetParameter2D(float x, float y) { m_param2D[0] = x; m_param2D[1] = y; }

    /// @brief 1Dパラメータを取得する
    /// @return パラメータ値
    float GetParameter() const { return m_param1D; }

    /// @brief パラメータに応じた最終ポーズを計算する
    /// @param time アニメーション時間（秒）
    /// @param jointCount 関節の数
    /// @param bindPose バインドポーズ（nullならIdentity）
    /// @param outPose 出力先のポーズ配列
    void Evaluate(float time, uint32_t jointCount,
                  const TransformTRS* bindPose,
                  TransformTRS* outPose) const;

    /// @brief ツリー内の最長クリップの再生時間を返す
    /// @return 最長の再生時間（秒）
    float GetDuration() const;

private:
    /// 1Dブレンドの評価（threshold順にソートし、隣接2ノードを線形補間）
    void Evaluate1D(float time, uint32_t jointCount,
                    const TransformTRS* bindPose,
                    TransformTRS* outPose) const;

    /// 2Dブレンドの評価（最近傍3ノードをバリセントリック座標で重み付けブレンド）
    void Evaluate2D(float time, uint32_t jointCount,
                    const TransformTRS* bindPose,
                    TransformTRS* outPose) const;

    /// 2つのTRS配列をlerp/slerpでブレンドする
    static void BlendPoses(const TransformTRS* a, const TransformTRS* b,
                           float t, uint32_t count, TransformTRS* out);

    BlendTreeType m_type = BlendTreeType::Simple1D;
    std::vector<BlendTreeNode> m_nodes;
    float m_param1D = 0.0f;
    float m_param2D[2] = {0, 0};

    mutable std::vector<TransformTRS> m_tempA;  ///< 評価用テンポラリ
    mutable std::vector<TransformTRS> m_tempB;
    mutable std::vector<TransformTRS> m_tempC;
};

} // namespace GX
