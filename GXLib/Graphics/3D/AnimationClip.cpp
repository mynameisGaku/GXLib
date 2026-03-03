/// @file AnimationClip.cpp
/// @brief アニメーションクリップの実装
/// キーフレーム補間のコアロジック。位置/スケールは線形補間、回転は球面線形補間(slerp)を使う。
#include "pch_graphics.h"
#include "Graphics/3D/AnimationClip.h"

namespace gx
{

/// 指定時刻を含むキーフレームペアのうち、先頭側のインデックスを返す（線形探索）
template<typename T>
static uint32_t FindKeyframeIndex(const gx::Vector<Keyframe<T>>& keys, float time)
{
    for (uint32_t i = 0; i + 1 < keys.size(); ++i)
    {
        if (time < keys[i + 1].time)
            return i;
    }
    return static_cast<uint32_t>(keys.size()) - 2;
}

Vector3 AnimationClip::InterpolateVec3(const gx::Vector<Keyframe<Vector3>>& keys, float time)
{
    if (keys.empty())
        return { 0, 0, 0 };
    if (keys.size() == 1 || time <= keys.front().time)
        return keys.front().value;
    if (time >= keys.back().time)
        return keys.back().value;

    uint32_t i = FindKeyframeIndex(keys, time);
    float t = (time - keys[i].time) / (keys[i + 1].time - keys[i].time);
    t = (std::max)(0.0f, (std::min)(t, 1.0f));

    XMVECTOR a = XMLoadFloat3(XM(&keys[i].value));
    XMVECTOR b = XMLoadFloat3(XM(&keys[i + 1].value));
    Vector3 result;
    XMStoreFloat3(XM(&result), XMVectorLerp(a, b, t));
    return result;
}

Quaternion AnimationClip::InterpolateQuat(const gx::Vector<Keyframe<Quaternion>>& keys, float time)
{
    if (keys.empty())
        return { 0, 0, 0, 1 };
    if (keys.size() == 1 || time <= keys.front().time)
        return keys.front().value;
    if (time >= keys.back().time)
        return keys.back().value;

    uint32_t i = FindKeyframeIndex(keys, time);
    float t = (time - keys[i].time) / (keys[i + 1].time - keys[i].time);
    t = (std::max)(0.0f, (std::min)(t, 1.0f));

    XMVECTOR a = XMLoadFloat4(XM(&keys[i].value));
    XMVECTOR b = XMLoadFloat4(XM(&keys[i + 1].value));
    Quaternion result;
    XMStoreFloat4(XM(&result), XMQuaternionSlerp(a, b, t));
    return result;
}

void AnimationClip::SampleTRS(float time, uint32_t jointCount, TransformTRS* outPose,
                              const TransformTRS* basePose) const
{
    if (!outPose) return;

    // ベース姿勢があればそれで初期化、無ければ単位姿勢で初期化
    if (basePose)
    {
        for (uint32_t i = 0; i < jointCount; ++i)
            outPose[i] = basePose[i];
    }
    else
    {
        for (uint32_t i = 0; i < jointCount; ++i)
            outPose[i] = IdentityTRS();
    }

    // キーが定義されたチャンネルだけ上書き（キーが無い関節はベース姿勢のまま）
    for (const auto& channel : m_channels)
    {
        if (channel.jointIndex < 0 || channel.jointIndex >= static_cast<int>(jointCount))
            continue;

        TransformTRS trs = outPose[channel.jointIndex];
        if (!channel.translationKeys.empty())
            trs.translation = InterpolateVec3(channel.translationKeys, time);
        if (!channel.rotationKeys.empty())
            trs.rotation = InterpolateQuat(channel.rotationKeys, time);
        if (!channel.scaleKeys.empty())
            trs.scale = InterpolateVec3(channel.scaleKeys, time);

        outPose[channel.jointIndex] = trs;
    }
}

void AnimationClip::Sample(float time, uint32_t jointCount, Matrix4x4* outLocalTransforms) const
{
    if (!outLocalTransforms) return;

    gx::Vector<TransformTRS> pose(jointCount);
    SampleTRS(time, jointCount, pose.data(), nullptr);

    for (uint32_t i = 0; i < jointCount; ++i)
    {
        XMMATRIX S = XMMatrixScaling(pose[i].scale.x, pose[i].scale.y, pose[i].scale.z);
        XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(XM(&pose[i].rotation)));
        XMMATRIX T = XMMatrixTranslation(pose[i].translation.x, pose[i].translation.y, pose[i].translation.z);
        XMMATRIX local = S * R * T;
        XMStoreFloat4x4(XM(&outLocalTransforms[i]), local);
    }
}

void AnimationClip::CollectEvents(float prevTime, float curTime,
                                   gx::Vector<const AnimationEvent*>& outEvents) const
{
    if (m_events.empty()) return;

    if (curTime >= prevTime)
    {
        // 通常再生（前進）: prevTime < event.time <= curTime
        for (const auto& evt : m_events)
        {
            if (evt.time > prevTime && evt.time <= curTime)
                outEvents.push_back(&evt);
        }
    }
    else
    {
        // ループ境界をまたいだ場合: prevTime < event.time <= duration  OR  0 <= event.time <= curTime
        for (const auto& evt : m_events)
        {
            if (evt.time > prevTime || evt.time <= curTime)
                outEvents.push_back(&evt);
        }
    }
}

} // namespace gx
