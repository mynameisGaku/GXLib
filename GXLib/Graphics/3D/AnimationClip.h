#pragma once
/// @file AnimationClip.h
/// @brief アニメーションクリップ（関節ごとのTRSキーフレーム）
/// 初学者向け: TRSは Translation(移動) / Rotation(回転) / Scale(拡縮) の略です。

#include "pch.h"

namespace GX
{

/// @brief 関節のTRS姿勢
/// 初学者向け: 1つの関節を「移動・回転・拡縮」の3要素で表します。
struct TransformTRS
{
    XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4 rotation    = { 0.0f, 0.0f, 0.0f, 1.0f }; // クォータニオン（回転）
    XMFLOAT3 scale       = { 1.0f, 1.0f, 1.0f };
};

inline TransformTRS IdentityTRS()
{
    return TransformTRS{};
}

inline TransformTRS DecomposeTRS(const XMFLOAT4X4& mat)
{
    XMMATRIX m = XMLoadFloat4x4(&mat);
    XMVECTOR s, r, t;
    if (!XMMatrixDecompose(&s, &r, &t, m))
        return IdentityTRS();

    TransformTRS out;
    XMStoreFloat3(&out.scale, s);
    XMStoreFloat4(&out.rotation, XMQuaternionNormalize(r));
    XMStoreFloat3(&out.translation, t);
    return out;
}

inline XMFLOAT4X4 ComposeTRS(const TransformTRS& trs)
{
    XMMATRIX S = XMMatrixScaling(trs.scale.x, trs.scale.y, trs.scale.z);
    XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&trs.rotation));
    XMMATRIX T = XMMatrixTranslation(trs.translation.x, trs.translation.y, trs.translation.z);
    XMFLOAT4X4 out;
    XMStoreFloat4x4(&out, S * R * T);
    return out;
}

/// @brief キーフレームの補間方式
/// 初学者向け: どのように時間の間を補間するか（直線/段階/曲線）を選びます。
enum class InterpolationType
{
    Linear,
    Step,
    CubicSpline,
};

/// @brief キーフレームの入れ物
template<typename T>
struct Keyframe
{
    float time;
    T     value;
};

/// @brief 関節アニメーションのチャンネル（TRSキー列）
struct AnimationChannel
{
    int                             jointIndex = -1;
    std::vector<Keyframe<XMFLOAT3>> translationKeys;
    std::vector<Keyframe<XMFLOAT4>> rotationKeys;
    std::vector<Keyframe<XMFLOAT3>> scaleKeys;
    InterpolationType               interpolation = InterpolationType::Linear;
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

    /// @brief ローカルTRS姿勢をサンプルする
    /// @param time アニメーション時間（秒）
    /// @param jointCount 関節の総数
    /// @param outPose 出力先の姿勢配列
    /// @param basePose 基本姿勢（バインドポーズ）。指定するとキーで上書きします。
    /// 初学者向け: ベース姿勢があると「動いていない関節」も自然な位置になります。
    void SampleTRS(float time, uint32_t jointCount, TransformTRS* outPose,
                   const TransformTRS* basePose = nullptr) const;

    /// @brief ローカル変換行列をサンプルする
    void Sample(float time, uint32_t jointCount, XMFLOAT4X4* outLocalTransforms) const;

private:
    static XMFLOAT3 InterpolateVec3(const std::vector<Keyframe<XMFLOAT3>>& keys, float time);
    static XMFLOAT4 InterpolateQuat(const std::vector<Keyframe<XMFLOAT4>>& keys, float time);

    std::string m_name;
    float m_duration = 0.0f;
    std::vector<AnimationChannel> m_channels;
};

} // namespace GX
