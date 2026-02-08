#pragma once
/// @file Skeleton.h
/// @brief ジョイント階層、逆バインド行列、ボーン行列計算

#include "pch.h"

namespace GX
{

/// @brief ジョイント情報
struct Joint
{
    std::string name;
    int         parentIndex = -1;
    XMFLOAT4X4  inverseBindMatrix;
    XMFLOAT4X4  localTransform;
};

/// ボーン定数バッファ（b4）
struct BoneConstants
{
    static constexpr uint32_t k_MaxBones = 128;
    XMFLOAT4X4 boneMatrices[k_MaxBones];
};

/// @brief スケルトン（ジョイント階層）
class Skeleton
{
public:
    Skeleton() = default;
    ~Skeleton() = default;

    void AddJoint(const Joint& joint) { m_joints.push_back(joint); }

    /// グローバルトランスフォーム行列を計算
    void ComputeGlobalTransforms(const XMFLOAT4X4* localTransforms,
                                  XMFLOAT4X4* globalTransforms) const;

    /// ボーン行列を計算（globalTransform * inverseBindMatrix）
    void ComputeBoneMatrices(const XMFLOAT4X4* globalTransforms,
                              XMFLOAT4X4* boneMatrices) const;

    const std::vector<Joint>& GetJoints() const { return m_joints; }
    uint32_t GetJointCount() const { return static_cast<uint32_t>(m_joints.size()); }

    /// ジョイント名からインデックスを取得
    int FindJointIndex(const std::string& name) const;

private:
    std::vector<Joint> m_joints;
};

} // namespace GX
