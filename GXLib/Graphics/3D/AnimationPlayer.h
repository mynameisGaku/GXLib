#pragma once
/// @file AnimationPlayer.h
/// @brief 単一クリップ再生用のアニメーションプレイヤー（姿勢キャッシュ付き）
/// DxLibの MV1SetAttachAnimTime / MV1SetAttachAnimBlendRate に相当する機能を提供する。
/// 1つのAnimationClipを再生し、スキニング用のボーン行列(BoneConstants)を生成する。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

/// @brief 単一クリップ再生プレイヤー
/// ブレンドやクロスフェードが不要な場合に使う軽量な再生器。
/// 複数アニメの合成が必要ならAnimatorを使う。
class AnimationPlayer
{
public:
    AnimationPlayer() = default;
    ~AnimationPlayer() = default;

    /// @brief スケルトンを設定する（バインドポーズの取得もここで行う）
    /// @param skeleton 対象スケルトン
    void SetSkeleton(Skeleton* skeleton);

    /// @brief アニメーションの再生を開始する
    /// @param clip 再生するクリップ
    /// @param loop ループ再生するか
    void Play(const AnimationClip* clip, bool loop = true);

    /// @brief 再生を停止する
    void Stop() { m_playing = false; }

    /// @brief 再生を一時停止する
    void Pause() { m_paused = true; }

    /// @brief 一時停止を解除する
    void Resume() { m_paused = false; }

    /// @brief 再生速度を設定する（1.0が等速、2.0で2倍速）
    /// @param speed 再生速度
    void SetSpeed(float speed) { m_speed = speed; }

    /// @brief 再生速度を取得する
    /// @return 再生速度
    float GetSpeed() const { return m_speed; }

    /// @brief 毎フレーム呼び出してアニメーションを進める
    /// @param deltaTime フレーム経過時間（秒）
    void Update(float deltaTime);

    /// @brief スキニング用のボーン定数バッファを取得する
    /// @return ボーン行列配列を含む定数データ
    const BoneConstants& GetBoneConstants() const { return m_boneConstants; }

    /// @brief 各関節のグローバル変換行列を取得する
    /// @return グローバル変換行列配列
    const std::vector<XMFLOAT4X4>& GetGlobalTransforms() const { return m_globalTransforms; }

    /// @brief 各関節のローカルTRS姿勢を取得する
    /// @return ローカルTRS配列
    const std::vector<TransformTRS>& GetLocalPose() const { return m_localPose; }

    /// @brief 再生中かどうか
    /// @return 再生中ならtrue
    bool IsPlaying() const { return m_playing; }

    /// @brief 現在の再生時刻（秒）を取得する
    /// @return 再生時刻
    float GetCurrentTime() const { return m_currentTime; }

private:
    /// 姿勢バッファのサイズをスケルトンに合わせて確保する
    void EnsurePoseStorage();

    Skeleton*            m_skeleton    = nullptr;
    const AnimationClip* m_currentClip = nullptr;
    bool  m_playing = false;
    bool  m_paused  = false;
    bool  m_loop    = true;
    float m_speed   = 1.0f;
    float m_currentTime = 0.0f;

    std::vector<TransformTRS> m_bindPose;         ///< スケルトンのバインドポーズをTRS分解したもの
    std::vector<TransformTRS> m_localPose;         ///< 現在のローカルTRS姿勢
    std::vector<XMFLOAT4X4>   m_localTransforms;  ///< ローカル変換行列
    std::vector<XMFLOAT4X4>   m_globalTransforms;  ///< グローバル変換行列
    BoneConstants             m_boneConstants;      ///< GPU転送用ボーン行列定数
};

} // namespace GX
