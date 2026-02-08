#pragma once
/// @file AnimationClip.h
/// @brief アニメーションチャンネル（Translation/Rotation/Scaleキーフレーム）

#include "pch.h"

namespace GX
{

/// キーフレーム補間タイプ
enum class InterpolationType
{
    Linear,
    Step,
    CubicSpline,
};

/// キーフレームデータ
template<typename T>
struct Keyframe
{
    float time;
    T     value;
};

/// アニメーションチャンネル（1ジョイント分のTRS）
struct AnimationChannel
{
    int                          jointIndex = -1;
    std::vector<Keyframe<XMFLOAT3>> translationKeys;
    std::vector<Keyframe<XMFLOAT4>> rotationKeys;    // Quaternion
    std::vector<Keyframe<XMFLOAT3>> scaleKeys;
    InterpolationType interpolation = InterpolationType::Linear;
};

/// @brief アニメーションクリップ
class AnimationClip
{
public:
    AnimationClip() = default;
    ~AnimationClip() = default;

    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    void SetDuration(float duration) { m_duration = duration; }
    float GetDuration() const { return m_duration; }

    void AddChannel(const AnimationChannel& channel) { m_channels.push_back(channel); }
    const std::vector<AnimationChannel>& GetChannels() const { return m_channels; }

    /// 指定時刻のジョイントローカルトランスフォームをサンプリング
    /// @param time アニメーション時刻
    /// @param jointCount ジョイント総数
    /// @param outLocalTransforms 出力先（jointCount個）
    void Sample(float time, uint32_t jointCount, XMFLOAT4X4* outLocalTransforms) const;

private:
    /// キーフレーム補間
    static XMFLOAT3 InterpolateVec3(const std::vector<Keyframe<XMFLOAT3>>& keys, float time);
    static XMFLOAT4 InterpolateQuat(const std::vector<Keyframe<XMFLOAT4>>& keys, float time);

    std::string m_name;
    float m_duration = 0.0f;
    std::vector<AnimationChannel> m_channels;
};

} // namespace GX
