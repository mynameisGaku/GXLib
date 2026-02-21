#pragma once
/// @file Humanoid.h
/// @brief ヒューマノイドボーンマッピングとアニメーションリターゲット
/// 異なるスケルトンを持つモデル間でアニメーションを共有するための仕組み。
/// ボーン名の自動マッチングやJSON定義によるマッピング、骨格間のリターゲットを行う。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

/// @brief ヒューマノイドの標準ボーン定義（Unity互換の主要22部位）
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

/// @brief ヒューマノイド対応表（HumanoidBone列挙値 → スケルトンの関節インデックス）
struct HumanoidAvatar
{
    std::array<int, static_cast<size_t>(HumanoidBone::Count)> joints;

    HumanoidAvatar()
    {
        joints.fill(-1);
    }

    /// @brief 指定ボーンがマッピングされているか
    /// @param bone ヒューマノイドボーン
    /// @return マッピング済みならtrue
    bool Has(HumanoidBone bone) const
    {
        return joints[static_cast<size_t>(bone)] >= 0;
    }

    /// @brief 指定ボーンの関節インデックスを取得する
    /// @param bone ヒューマノイドボーン
    /// @return 関節インデックス（未マッピングなら-1）
    int Get(HumanoidBone bone) const
    {
        return joints[static_cast<size_t>(bone)];
    }
};

/// @brief スケルトンのボーン名から自動でヒューマノイド対応表を構築する
/// @param skeleton 対象スケルトン
/// @return 構築されたアバター（マッチしなかったボーンは-1のまま）
HumanoidAvatar BuildHumanoidAvatarAuto(const Skeleton& skeleton);

/// @brief JSONファイルからヒューマノイド対応表を構築する
/// @param skeleton 対象スケルトン
/// @param jsonPath JSONファイルパス
/// @param fallbackAuto JSONで未定義のボーンを自動マッチングで補完するか
/// @return 構築されたアバター
HumanoidAvatar BuildHumanoidAvatarFromJson(const Skeleton& skeleton,
                                           const std::wstring& jsonPath,
                                           bool fallbackAuto = true);

/// @brief ヒューマノイドリターゲッタ（異なる骨格間でアニメーションを移植する）
/// 回転は差分転写、位置は骨の長さ比でスケール補正を行い、
/// 体格が異なるモデルでも自然なアニメーション再生を実現する。
class HumanoidRetargeter
{
public:
    HumanoidRetargeter() = default;
    ~HumanoidRetargeter() = default;

    /// @brief リターゲットを初期化する
    /// @param sourceSkeleton ソース側のスケルトン
    /// @param sourceAvatar ソース側のアバター対応表
    /// @param targetSkeleton ターゲット側のスケルトン
    /// @param targetAvatar ターゲット側のアバター対応表
    /// @return 初期化成功ならtrue
    bool Initialize(const Skeleton* sourceSkeleton, const HumanoidAvatar& sourceAvatar,
                    const Skeleton* targetSkeleton, const HumanoidAvatar& targetAvatar);

    /// @brief ソースのローカルTRS姿勢をターゲット骨格にリターゲットする
    /// @param sourcePose ソース側のローカル姿勢配列
    /// @param targetPose 出力先のターゲット側ローカル姿勢配列
    void RetargetLocalPose(const TransformTRS* sourcePose,
                           TransformTRS* targetPose) const;

private:
    /// スケルトンからバインドポーズとボーン長を取得する
    void BuildBindPose(const Skeleton* skel,
                       std::vector<TransformTRS>& outBindPose,
                       std::vector<float>& outBoneLength) const;

    const Skeleton* m_sourceSkeleton = nullptr;
    const Skeleton* m_targetSkeleton = nullptr;
    HumanoidAvatar  m_sourceAvatar;
    HumanoidAvatar  m_targetAvatar;

    std::vector<TransformTRS> m_sourceBindPose;
    std::vector<TransformTRS> m_targetBindPose;
    std::vector<float>        m_sourceBoneLength;  ///< ソース側の各ボーン長
    std::vector<float>        m_targetBoneLength;  ///< ターゲット側の各ボーン長
};

} // namespace GX
