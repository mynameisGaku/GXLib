/// @file Humanoid.cpp
/// @brief ヒューマノイドのマッピング／リターゲット実装
#include "pch_graphics.h"
#include "Graphics/3D/Humanoid.h"
#include "Math/MathConvert.h"
#include "ThirdParty/json.hpp"
#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace gx
{

static gx::String ToLowerAscii(const gx::String& s)
{
    gx::String out;
    out.reserve(s.size());
    for (char c : s)
        out.push_back(static_cast<char>(::tolower(static_cast<unsigned char>(c))));
    return out;
}

static gx::String NormalizeBoneName(const gx::String& name)
{
    gx::String lower = ToLowerAscii(name);
    gx::String out;
    out.reserve(lower.size());
    for (char c : lower)
    {
        if (std::isalnum(static_cast<unsigned char>(c)))
            out.push_back(c);
    }

    // DCCツール由来の接頭辞を除去（Mixamo, Blender Armature, 3dsMax Bip等）
    const char* prefixes[] = { "mixamorig", "armature", "bip001", "bip", "rig" };
    for (const char* p : prefixes)
    {
        size_t len = strlen(p);
        if (out.rfind(p, 0) == 0 && out.size() > len)
        {
            out = out.substr(len);
            break;
        }
    }
    return out;
}

static int FindJointByName(const Skeleton& skeleton, const gx::Vector<gx::String>& candidates)
{
    const auto& joints = skeleton.GetJoints();
    gx::HashMap<gx::String, int> nameMap;
    nameMap.reserve(joints.size());
    for (uint32_t i = 0; i < joints.size(); ++i)
    {
        nameMap[NormalizeBoneName(joints[i].name)] = static_cast<int>(i);
    }

    for (const auto& candidate : candidates)
    {
        auto it = nameMap.find(NormalizeBoneName(candidate));
        if (it != nameMap.end())
            return it->second;
    }
    return -1;
}

static int FindJointByNormalizedName(const Skeleton& skeleton, const gx::String& name)
{
    gx::String key = NormalizeBoneName(name);
    const auto& joints = skeleton.GetJoints();
    for (uint32_t i = 0; i < joints.size(); ++i)
    {
        if (NormalizeBoneName(joints[i].name) == key)
            return static_cast<int>(i);
    }
    return -1;
}

static gx::Vector<gx::String> NamesFor(HumanoidBone bone)
{
    switch (bone)
    {
    case HumanoidBone::Hips:          return { "hips", "pelvis", "hip", "root" };
    case HumanoidBone::Spine:         return { "spine", "spine1", "spine01" };
    case HumanoidBone::Chest:         return { "chest", "spine2", "spine02", "upperbody" };
    case HumanoidBone::UpperChest:    return { "upperchest", "spine3", "spine03" };
    case HumanoidBone::Neck:          return { "neck", "neck1" };
    case HumanoidBone::Head:          return { "head" };

    case HumanoidBone::LeftShoulder:  return { "leftshoulder", "lshoulder", "leftclavicle", "claviclel" };
    case HumanoidBone::LeftUpperArm:  return { "leftupperarm", "leftarm", "lupperarm", "upperarml" };
    case HumanoidBone::LeftLowerArm:  return { "leftlowerarm", "leftforearm", "llowerarm", "forearml" };
    case HumanoidBone::LeftHand:      return { "lefthand", "lhand", "handl" };

    case HumanoidBone::RightShoulder: return { "rightshoulder", "rshoulder", "rightclavicle", "clavicler" };
    case HumanoidBone::RightUpperArm: return { "rightupperarm", "rightarm", "rupperarm", "upperarmr" };
    case HumanoidBone::RightLowerArm: return { "rightlowerarm", "rightforearm", "rlowerarm", "forearmr" };
    case HumanoidBone::RightHand:     return { "righthand", "rhand", "handr" };

    case HumanoidBone::LeftUpperLeg:  return { "leftupperleg", "leftthigh", "lupperleg", "thighl" };
    case HumanoidBone::LeftLowerLeg:  return { "leftlowerleg", "leftcalf", "llowerleg", "calfl" };
    case HumanoidBone::LeftFoot:      return { "leftfoot", "lfoot", "footl" };
    case HumanoidBone::LeftToes:      return { "lefttoes", "lefttoe", "ltoe", "toel" };

    case HumanoidBone::RightUpperLeg: return { "rightupperleg", "rightthigh", "rupperleg", "thighr" };
    case HumanoidBone::RightLowerLeg: return { "rightlowerleg", "rightcalf", "rlowerleg", "calfr" };
    case HumanoidBone::RightFoot:     return { "rightfoot", "rfoot", "footr" };
    case HumanoidBone::RightToes:     return { "righttoes", "righttoe", "rtoe", "toer" };
    default:                          return {};
    }
}

static bool MatchesBoneName(HumanoidBone bone, const gx::String& key)
{
    gx::String normKey = NormalizeBoneName(key);
    for (const auto& candidate : NamesFor(bone))
    {
        if (NormalizeBoneName(candidate) == normKey)
            return true;
    }
    return false;
}

HumanoidAvatar BuildHumanoidAvatarAuto(const Skeleton& skeleton)
{
    HumanoidAvatar avatar;
    for (uint32_t i = 0; i < static_cast<uint32_t>(HumanoidBone::Count); ++i)
    {
        HumanoidBone bone = static_cast<HumanoidBone>(i);
        avatar.joints[i] = FindJointByName(skeleton, NamesFor(bone));
    }
    return avatar;
}

HumanoidAvatar BuildHumanoidAvatarFromJson(const Skeleton& skeleton,
                                           const gx::WString& jsonPath,
                                           bool fallbackAuto)
{
    HumanoidAvatar avatar;
    if (!jsonPath.empty() && std::filesystem::exists(std::filesystem::path(jsonPath.c_str())))
    {
        std::ifstream file{ std::filesystem::path(jsonPath) };
        if (file)
        {
            nlohmann::json j;
            file >> j;
            if (j.is_object())
            {
                for (auto it = j.begin(); it != j.end(); ++it)
                {
                    gx::String boneName = it.key();
                    if (!it.value().is_string())
                        continue;

                    gx::String jointName = it.value().get<gx::String>();
                    for (uint32_t i = 0; i < static_cast<uint32_t>(HumanoidBone::Count); ++i)
                    {
                        HumanoidBone bone = static_cast<HumanoidBone>(i);
                        if (MatchesBoneName(bone, boneName))
                        {
                            int idx = skeleton.FindJointIndex(jointName);
                            if (idx < 0)
                                idx = FindJointByNormalizedName(skeleton, jointName);
                            if (idx >= 0)
                                avatar.joints[i] = idx;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (fallbackAuto)
    {
        HumanoidAvatar autoAvatar = BuildHumanoidAvatarAuto(skeleton);
        for (uint32_t i = 0; i < static_cast<uint32_t>(HumanoidBone::Count); ++i)
        {
            if (avatar.joints[i] < 0)
                avatar.joints[i] = autoAvatar.joints[i];
        }
    }

    return avatar;
}

bool HumanoidRetargeter::Initialize(const Skeleton* sourceSkeleton, const HumanoidAvatar& sourceAvatar,
                                    const Skeleton* targetSkeleton, const HumanoidAvatar& targetAvatar)
{
    m_sourceSkeleton = sourceSkeleton;
    m_targetSkeleton = targetSkeleton;
    m_sourceAvatar   = sourceAvatar;
    m_targetAvatar   = targetAvatar;

    if (!m_sourceSkeleton || !m_targetSkeleton)
        return false;

    BuildBindPose(m_sourceSkeleton, m_sourceBindPose, m_sourceBoneLength);
    BuildBindPose(m_targetSkeleton, m_targetBindPose, m_targetBoneLength);
    return true;
}

void HumanoidRetargeter::BuildBindPose(const Skeleton* skel,
                                       gx::Vector<TransformTRS>& outBindPose,
                                       gx::Vector<float>& outBoneLength) const
{
    if (!skel) return;

    const auto& joints = skel->GetJoints();
    const uint32_t jointCount = static_cast<uint32_t>(joints.size());
    outBindPose.resize(jointCount);
    outBoneLength.resize(jointCount, 1.0f);

    gx::Vector<Matrix4x4> local(jointCount);
    gx::Vector<Matrix4x4> global(jointCount);
    for (uint32_t i = 0; i < jointCount; ++i)
    {
        local[i] = joints[i].localTransform;
        outBindPose[i] = DecomposeTRS(joints[i].localTransform);
    }
    skel->ComputeGlobalTransforms(local.data(), global.data());

    for (uint32_t i = 0; i < jointCount; ++i)
    {
        int childIndex = -1;
        for (uint32_t j = 0; j < jointCount; ++j)
        {
            if (joints[j].parentIndex == static_cast<int>(i))
            {
                childIndex = static_cast<int>(j);
                break;
            }
        }

        if (childIndex >= 0)
        {
            Vector3 a = { global[i]._41, global[i]._42, global[i]._43 };
            Vector3 b = { global[childIndex]._41, global[childIndex]._42, global[childIndex]._43 };
            XMVECTOR va = XMLoadFloat3(XM(&a));
            XMVECTOR vb = XMLoadFloat3(XM(&b));
            float len = XMVectorGetX(XMVector3Length(vb - va));
            outBoneLength[i] = (std::max)(len, 0.001f);
        }
    }
}

void HumanoidRetargeter::RetargetLocalPose(const TransformTRS* sourcePose,
                                           TransformTRS* targetPose) const
{
    if (!m_sourceSkeleton || !m_targetSkeleton || !sourcePose || !targetPose)
        return;

    const uint32_t targetCount = m_targetSkeleton->GetJointCount();
    for (uint32_t i = 0; i < targetCount; ++i)
        targetPose[i] = m_targetBindPose[i];

    for (uint32_t i = 0; i < static_cast<uint32_t>(HumanoidBone::Count); ++i)
    {
        int srcIndex = m_sourceAvatar.joints[i];
        int dstIndex = m_targetAvatar.joints[i];
        if (srcIndex < 0 || dstIndex < 0)
            continue;

        const TransformTRS& srcBind = m_sourceBindPose[srcIndex];
        const TransformTRS& srcAnim = sourcePose[srcIndex];
        const TransformTRS& dstBind = m_targetBindPose[dstIndex];

        // 回転の差分転写: delta = inv(srcBind) * srcAnim → dstBind * delta
        XMVECTOR qBind = XMLoadFloat4(XM(&srcBind.rotation));
        XMVECTOR qAnim = XMLoadFloat4(XM(&srcAnim.rotation));
        XMVECTOR qDelta = XMQuaternionMultiply(XMQuaternionInverse(qBind), qAnim);
        XMVECTOR qDst = XMQuaternionMultiply(XMLoadFloat4(XM(&dstBind.rotation)), qDelta);
        Quaternion dstRot;
        XMStoreFloat4(XM(&dstRot), XMQuaternionNormalize(qDst));

        // 位置の差分（ボーン長の比率でスケール補正し、体格差を吸収）
        Vector3 deltaPos = {
            srcAnim.translation.x - srcBind.translation.x,
            srcAnim.translation.y - srcBind.translation.y,
            srcAnim.translation.z - srcBind.translation.z
        };
        float srcLen = m_sourceBoneLength[srcIndex];
        float dstLen = m_targetBoneLength[dstIndex];
        float scale = (srcLen > 0.0001f) ? (dstLen / srcLen) : 1.0f;

        Vector3 dstPos = {
            dstBind.translation.x + deltaPos.x * scale,
            dstBind.translation.y + deltaPos.y * scale,
            dstBind.translation.z + deltaPos.z * scale
        };

        // スケールの差分
        Vector3 dstScale = {
            dstBind.scale.x * (srcBind.scale.x != 0.0f ? (srcAnim.scale.x / srcBind.scale.x) : 1.0f),
            dstBind.scale.y * (srcBind.scale.y != 0.0f ? (srcAnim.scale.y / srcBind.scale.y) : 1.0f),
            dstBind.scale.z * (srcBind.scale.z != 0.0f ? (srcAnim.scale.z / srcBind.scale.z) : 1.0f)
        };

        targetPose[dstIndex].translation = dstPos;
        targetPose[dstIndex].rotation = dstRot;
        targetPose[dstIndex].scale = dstScale;
    }
}

} // namespace gx
