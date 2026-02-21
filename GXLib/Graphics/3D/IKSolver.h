#pragma once
/// @file IKSolver.h
/// @brief CCD-IK（Cyclic Coordinate Descent）ソルバー
///
/// ジョイントチェーンのエフェクタをターゲット位置に到達させるための
/// 反復型IKアルゴリズム。FootIKやLookAtIKの基盤として使用する。

#include "pch.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

/// @brief IKチェーン定義（ルートジョイント→エフェクタの順）
struct IKChain
{
    std::vector<int> jointIndices;   ///< チェーンに含まれるジョイントインデックス（ルート→エフェクタ順）
    int   effectorIndex = -1;        ///< エフェクタジョイントのインデックス
    float tolerance     = 0.001f;    ///< 収束許容誤差（ワールド単位）
    uint32_t maxIterations = 20;     ///< 最大反復回数
};

/// @brief CCD-IK ソルバー
/// エフェクタからルートに向かって1ジョイントずつ回転補正を行い、
/// ターゲット位置への到達を反復的に近似する。
class CCDIKSolver
{
public:
    /// @brief IKを解く
    /// @param chain IKチェーン定義
    /// @param targetPos ターゲット位置（ワールド座標）
    /// @param skeleton スケルトン参照
    /// @param localTransforms ローカル変換行列配列（入出力、ジョイント回転を変更）
    /// @param globalTransforms グローバル変換行列配列（入出力、FK再計算で更新）
    /// @return 収束した場合true
    bool Solve(const IKChain& chain, const XMFLOAT3& targetPos,
               const Skeleton& skeleton,
               XMFLOAT4X4* localTransforms,
               XMFLOAT4X4* globalTransforms);

private:
    /// @brief 指定ジョイント以下のFK（順運動学）を再計算する
    /// @param fromJoint 起点ジョイントのインデックス
    /// @param skeleton スケルトン参照
    /// @param localTransforms ローカル変換行列配列
    /// @param globalTransforms グローバル変換行列配列（出力）
    void RecomputeFK(int fromJoint, const Skeleton& skeleton,
                     const XMFLOAT4X4* localTransforms,
                     XMFLOAT4X4* globalTransforms);

    /// @brief グローバル変換行列からジョイントのワールド位置を取得する
    /// @param globalTransforms グローバル変換行列配列
    /// @param jointIndex ジョイントインデックス
    /// @return ワールド座標
    static XMVECTOR GetJointPosition(const XMFLOAT4X4* globalTransforms, int jointIndex);
};

} // namespace GX
