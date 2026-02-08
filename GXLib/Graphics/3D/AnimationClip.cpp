/// @file AnimationClip.cpp
/// @brief アニメーションクリップの実装
#include "pch.h"
#include "Graphics/3D/AnimationClip.h"

namespace GX
{

template<typename T>
static uint32_t FindKeyframeIndex(const std::vector<Keyframe<T>>& keys, float time)
{
    for (uint32_t i = 0; i + 1 < keys.size(); ++i)
    {
        if (time < keys[i + 1].time)
            return i;
    }
    return static_cast<uint32_t>(keys.size()) - 2;
}

XMFLOAT3 AnimationClip::InterpolateVec3(const std::vector<Keyframe<XMFLOAT3>>& keys, float time)
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

    XMVECTOR a = XMLoadFloat3(&keys[i].value);
    XMVECTOR b = XMLoadFloat3(&keys[i + 1].value);
    XMFLOAT3 result;
    XMStoreFloat3(&result, XMVectorLerp(a, b, t));
    return result;
}

XMFLOAT4 AnimationClip::InterpolateQuat(const std::vector<Keyframe<XMFLOAT4>>& keys, float time)
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

    XMVECTOR a = XMLoadFloat4(&keys[i].value);
    XMVECTOR b = XMLoadFloat4(&keys[i + 1].value);
    XMFLOAT4 result;
    XMStoreFloat4(&result, XMQuaternionSlerp(a, b, t));
    return result;
}

void AnimationClip::Sample(float time, uint32_t jointCount, XMFLOAT4X4* outLocalTransforms) const
{
    // 初期化（デフォルト: identity）
    for (uint32_t i = 0; i < jointCount; ++i)
        XMStoreFloat4x4(&outLocalTransforms[i], XMMatrixIdentity());

    // 各チャンネルをサンプリング
    for (const auto& channel : m_channels)
    {
        if (channel.jointIndex < 0 || channel.jointIndex >= static_cast<int>(jointCount))
            continue;

        XMFLOAT3 translation = { 0, 0, 0 };
        XMFLOAT4 rotation = { 0, 0, 0, 1 };
        XMFLOAT3 scale = { 1, 1, 1 };

        if (!channel.translationKeys.empty())
            translation = InterpolateVec3(channel.translationKeys, time);
        if (!channel.rotationKeys.empty())
            rotation = InterpolateQuat(channel.rotationKeys, time);
        if (!channel.scaleKeys.empty())
            scale = InterpolateVec3(channel.scaleKeys, time);

        XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
        XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
        XMMATRIX T = XMMatrixTranslation(translation.x, translation.y, translation.z);
        XMMATRIX local = S * R * T;

        XMStoreFloat4x4(&outLocalTransforms[channel.jointIndex], local);
    }
}

} // namespace GX
