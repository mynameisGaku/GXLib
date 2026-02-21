#pragma once
/// @file Skeleton.h
/// @brief ジョイント階層、逆バインド行列、ボーン行列計算

#include "pch.h"

namespace GX
{

/// @brief ジョイント（ボーン）情報
/// DxLibの MV1GetFrameName / MV1GetFramePosition でアクセスするボーン1本に相当
struct Joint
{
    std::string name;                ///< ジョイント名
    int         parentIndex = -1;    ///< 親ジョイントのインデックス（ルートなら -1）
    XMFLOAT4X4  inverseBindMatrix;   ///< バインドポーズの逆行列（スキニング計算用）
    XMFLOAT4X4  localTransform;      ///< 親ジョイント空間でのローカル変換行列
};

/// @brief ボーン定数バッファ（b4スロット、GPUスキニング用）
struct BoneConstants
{
    static constexpr uint32_t k_MaxBones = 128;     ///< 1モデルの最大ボーン数
    XMFLOAT4X4 boneMatrices[k_MaxBones];            ///< 転置済みボーン行列配列
};

/// @brief スケルトン（ボーン階層の管理）
/// DxLibの MV1GetFrameNum / MV1GetFrameParent に相当するボーン階層を保持し、
/// アニメーション再生時にローカル変換 → グローバル変換 → ボーン行列の計算を行う
class Skeleton
{
public:
    Skeleton() = default;
    ~Skeleton() = default;

    /// @brief ジョイントを追加する
    /// @param joint 追加するジョイント情報
    void AddJoint(const Joint& joint) { m_joints.push_back(joint); }

    /// @brief 親子関係に従ってローカル行列をグローバル行列に変換する
    /// @param localTransforms 各ジョイントのローカル変換行列（入力）
    /// @param globalTransforms 計算されたグローバル変換行列（出力）
    void ComputeGlobalTransforms(const XMFLOAT4X4* localTransforms,
                                  XMFLOAT4X4* globalTransforms) const;

    /// @brief GPUスキニング用のボーン行列を計算する（inverseBindMatrix * globalTransform、転置済み）
    /// @param globalTransforms 各ジョイントのグローバル変換行列（入力）
    /// @param boneMatrices 計算されたボーン行列（出力、シェーダーに渡す形式）
    void ComputeBoneMatrices(const XMFLOAT4X4* globalTransforms,
                              XMFLOAT4X4* boneMatrices) const;

    /// @brief 全ジョイントを取得する
    /// @return ジョイント配列への const 参照
    const std::vector<Joint>& GetJoints() const { return m_joints; }

    /// @brief ジョイント数を取得する
    /// @return ジョイントの数
    uint32_t GetJointCount() const { return static_cast<uint32_t>(m_joints.size()); }

    /// @brief ジョイント名からインデックスを検索する
    /// @param name ジョイント名
    /// @return インデックス（見つからない場合 -1）
    int FindJointIndex(const std::string& name) const;

private:
    std::vector<Joint> m_joints;
};

} // namespace GX
