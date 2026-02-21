#pragma once
/// @file AnimationClip.h
/// @brief アニメーションクリップ（関節ごとのTRSキーフレーム）
/// DxLibのMV1GetAnimTotalTime等で扱うアニメーションデータに相当する。
/// TRS = Translation(移動) / Rotation(回転) / Scale(拡縮) の3要素でボーンの姿勢を表す。

#include "pch.h"

namespace GX
{

/// @brief 関節1つ分のTRS姿勢（移動・回転・拡縮）
struct TransformTRS
{
    XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4 rotation    = { 0.0f, 0.0f, 0.0f, 1.0f }; // クォータニオン（回転）
    XMFLOAT3 scale       = { 1.0f, 1.0f, 1.0f };
};

/// @brief 単位姿勢（移動なし、回転なし、等倍スケール）を返す
/// @return 単位TRS
inline TransformTRS IdentityTRS()
{
    return TransformTRS{};
}

/// @brief 4x4行列をTRS(移動・回転・拡縮)に分解する
/// @param mat 分解対象の行列
/// @return 分解結果のTRS。分解失敗時は単位姿勢を返す
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

/// @brief TRSから4x4行列を合成する（S * R * T の順）
/// @param trs 合成するTRS姿勢
/// @return 合成された変換行列
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
enum class InterpolationType
{
    Linear,       ///< 線形補間（位置はlerp、回転はslerp）
    Step,         ///< ステップ補間（次のキーまで値が一定）
    CubicSpline,  ///< 3次スプライン補間（glTFで使われる）
};

/// @brief 時刻と値のペア（キーフレーム1つ分）
template<typename T>
struct Keyframe
{
    float time;   ///< キーの時刻（秒）
    T     value;  ///< キーの値
};

/// @brief 1つの関節が持つアニメーションチャンネル（位置・回転・拡縮のキー列）
struct AnimationChannel
{
    int                             jointIndex = -1;  ///< 対象の関節インデックス
    std::vector<Keyframe<XMFLOAT3>> translationKeys;  ///< 位置キーフレーム列
    std::vector<Keyframe<XMFLOAT4>> rotationKeys;     ///< 回転キーフレーム列（クォータニオン）
    std::vector<Keyframe<XMFLOAT3>> scaleKeys;        ///< 拡縮キーフレーム列
    InterpolationType               interpolation = InterpolationType::Linear;
};

/// @brief アニメーションクリップ（1つのアニメーション動作全体を保持する）
/// DxLibで言えば MV1AttachAnim で取得できるアニメーションデータに対応する。
/// 複数チャンネルを持ち、各チャンネルが1つの関節のキーフレーム列を保持する。
class AnimationClip
{
public:
    AnimationClip() = default;
    ~AnimationClip() = default;

    /// @brief クリップ名を設定する
    /// @param name クリップ名
    void SetName(const std::string& name) { m_name = name; }

    /// @brief クリップ名を取得する
    /// @return クリップ名
    const std::string& GetName() const { return m_name; }

    /// @brief アニメーションの再生時間（秒）を設定する
    /// @param duration 再生時間
    void SetDuration(float duration) { m_duration = duration; }

    /// @brief アニメーションの再生時間（秒）を取得する
    /// @return 再生時間
    float GetDuration() const { return m_duration; }

    /// @brief チャンネル（関節ごとのキー列）を追加する
    /// @param channel 追加するチャンネル
    void AddChannel(const AnimationChannel& channel) { m_channels.push_back(channel); }

    /// @brief 全チャンネルを取得する
    /// @return チャンネル配列
    const std::vector<AnimationChannel>& GetChannels() const { return m_channels; }

    /// @brief 指定時刻のローカルTRS姿勢をサンプルする
    /// @param time アニメーション時間（秒）
    /// @param jointCount 関節の総数
    /// @param outPose 出力先の姿勢配列（jointCount個の領域が必要）
    /// @param basePose バインドポーズ。指定するとキーが無い関節はこの姿勢を使う
    void SampleTRS(float time, uint32_t jointCount, TransformTRS* outPose,
                   const TransformTRS* basePose = nullptr) const;

    /// @brief 指定時刻のローカル変換行列をサンプルする
    /// @param time アニメーション時間（秒）
    /// @param jointCount 関節の総数
    /// @param outLocalTransforms 出力先の行列配列
    void Sample(float time, uint32_t jointCount, XMFLOAT4X4* outLocalTransforms) const;

private:
    /// 位置・スケール用のベクトル3補間（線形）
    static XMFLOAT3 InterpolateVec3(const std::vector<Keyframe<XMFLOAT3>>& keys, float time);
    /// 回転用のクォータニオン補間（球面線形 slerp）
    static XMFLOAT4 InterpolateQuat(const std::vector<Keyframe<XMFLOAT4>>& keys, float time);

    std::string m_name;
    float m_duration = 0.0f;
    std::vector<AnimationChannel> m_channels;
};

} // namespace GX
