#pragma once
/// @file Humanoid.h
/// @brief ヒューマノイドのマッピング／リターゲット支援
/// 初学者向け: 別モデルのアニメを「人型の対応表」で移し替えるための仕組みです。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

/// @brief Unity風のヒューマノイド骨セット（主要部分のみ）
enum class HumanoidBone : uint32_t
{
    Hips = 0,
    Spine,
    Chest,
    UpperChest,
    Neck,
    Head,

    LeftShoulder,
    LeftUpperArm,
    LeftLowerArm,
    LeftHand,

    RightShoulder,
    RightUpperArm,
    RightLowerArm,
    RightHand,

    LeftUpperLeg,
    LeftLowerLeg,
    LeftFoot,
    LeftToes,

    RightUpperLeg,
    RightLowerLeg,
    RightFoot,
    RightToes,

    Count
};

/// @brief ヒューマノイド対応表（HumanoidBone → 関節インデックス）
/// 初学者向け: 「右腕はこの関節」など、部位と実データの対応を持ちます。
struct HumanoidAvatar
{
    std::array<int, static_cast<size_t>(HumanoidBone::Count)> joints;

    HumanoidAvatar()
    {
        joints.fill(-1);
    }

    bool Has(HumanoidBone bone) const
    {
        return joints[static_cast<size_t>(bone)] >= 0;
    }

    int Get(HumanoidBone bone) const
    {
        return joints[static_cast<size_t>(bone)];
    }
};

/// @brief スケルトンから自動でヒューマノイド対応表を構築する
HumanoidAvatar BuildHumanoidAvatarAuto(const Skeleton& skeleton);

/// @brief JSON対応表からヒューマノイド対応表を構築する（必要なら自動フォールバック）
HumanoidAvatar BuildHumanoidAvatarFromJson(const Skeleton& skeleton,
                                           const std::wstring& jsonPath,
                                           bool fallbackAuto = true);

/// @brief ヒューマノイドのリターゲッタ（ソース → ターゲット）
/// 初学者向け: 片方のアニメをもう片方の骨格に合わせて移植します。
class HumanoidRetargeter
{
public:
    HumanoidRetargeter() = default;
    ~HumanoidRetargeter() = default;

    bool Initialize(const Skeleton* sourceSkeleton, const HumanoidAvatar& sourceAvatar,
                    const Skeleton* targetSkeleton, const HumanoidAvatar& targetAvatar);

    /// @brief ローカル姿勢（TRS）をリターゲットする
    void RetargetLocalPose(const TransformTRS* sourcePose,
                           TransformTRS* targetPose) const;

private:
    void BuildBindPose(const Skeleton* skel,
                       std::vector<TransformTRS>& outBindPose,
                       std::vector<float>& outBoneLength) const;

    const Skeleton* m_sourceSkeleton = nullptr;
    const Skeleton* m_targetSkeleton = nullptr;
    HumanoidAvatar  m_sourceAvatar;
    HumanoidAvatar  m_targetAvatar;

    std::vector<TransformTRS> m_sourceBindPose;
    std::vector<TransformTRS> m_targetBindPose;
    std::vector<float>        m_sourceBoneLength;
    std::vector<float>        m_targetBoneLength;
};

} // namespace GX
