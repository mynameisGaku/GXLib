#pragma once
/// @file Animator.h
/// @brief アニメーション統合管理（クロスフェード・BlendStack・ステートマシン対応）
/// DxLibではアニメーションのアタッチ+ブレンドレートを個別に管理する必要があるが、
/// Animatorはそれらを統合し、クロスフェードやレイヤーブレンドを自動処理する。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/BlendStack.h"
#include "Graphics/3D/AnimatorStateMachine.h"

namespace GX
{

/// @brief アニメーション再生の統合管理クラス
/// Simple/BlendStack/StateMachineの3モードをサポートし、
/// 最終的なスキニング用ボーン行列(BoneConstants)を出力する。
class Animator
{
public:
    /// @brief アニメーションの動作モード
    enum class AnimMode
    {
        Simple,        ///< 単純再生＋クロスフェード
        BlendStack,    ///< レイヤー合成（Override/Additive）
        StateMachine,  ///< 状態遷移による自動制御
    };

    Animator() = default;
    ~Animator() = default;

    /// @brief スケルトンを設定する（バインドポーズも同時に取得）
    /// @param skeleton 対象スケルトン
    void SetSkeleton(Skeleton* skeleton);

    /// @brief アニメーションを即座に再生する
    /// @param clip 再生するクリップ
    /// @param loop ループ再生するか
    /// @param speed 再生速度
    void Play(const AnimationClip* clip, bool loop = true, float speed = 1.0f);

    /// @brief 現在の再生から別のクリップへ滑らかに切り替える
    /// @param clip 次のクリップ
    /// @param duration クロスフェード時間（秒）
    /// @param loop ループ再生するか
    /// @param speed 再生速度
    void CrossFade(const AnimationClip* clip, float duration, bool loop = true, float speed = 1.0f);

    /// @brief 再生を停止する
    void Stop() { m_playing = false; }

    /// @brief 再生を一時停止する
    void Pause() { m_paused = true; }

    /// @brief 一時停止を解除する
    void Resume() { m_paused = false; }

    /// @brief BlendStackモードに切り替える（nullを渡すとSimpleに戻る）
    /// @param stack BlendStackインスタンス
    void SetBlendStack(BlendStack* stack);

    /// @brief StateMachineモードに切り替える（nullを渡すとSimpleに戻る）
    /// @param sm AnimatorStateMachineインスタンス
    void SetStateMachine(AnimatorStateMachine* sm);

    /// @brief 現在の動作モードを取得する
    /// @return 動作モード
    AnimMode GetAnimMode() const { return m_mode; }

    /// @brief バインドポーズでボーン行列を初期化する（再生前のTポーズ表示等に使用）
    void EvaluateBindPose();

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

    /// @brief 現在再生中のクリップを取得する
    /// @return クリップへのポインタ（未再生時はnullptr）
    const AnimationClip* GetCurrentClip() const { return m_current.clip; }

    /// @brief 現在の再生時刻（秒）を取得する
    /// @return 再生時刻
    float GetCurrentTime() const { return m_current.time; }

    /// @brief 再生時刻を直接設定する（タイムラインUI等で使用）
    /// @param time 設定する時刻（秒）
    void  SetCurrentTime(float time);

    /// @brief 再生速度を取得する
    /// @return 再生速度
    float GetSpeed() const { return m_current.speed; }

    /// @brief 再生速度を設定する（1.0が等速）
    /// @param speed 再生速度
    void  SetSpeed(float speed) { m_current.speed = speed; }

    /// @brief 一時停止中かどうか
    /// @return 一時停止中ならtrue
    bool  IsPaused() const { return m_paused; }

    /// @brief ルートボーンの位置をバインドポーズに固定する
    /// @param lock 固定するか
    void SetLockRootPosition(bool lock) { m_lockRootPosition = lock; }

    /// @brief ルートボーンの回転をバインドポーズに固定する
    /// @param lock 固定するか
    void SetLockRootRotation(bool lock) { m_lockRootRotation = lock; }

    /// @brief ルート位置が固定されているか
    /// @return 固定中ならtrue
    bool IsRootPositionLocked() const { return m_lockRootPosition; }

    /// @brief ルート回転が固定されているか
    /// @return 固定中ならtrue
    bool IsRootRotationLocked() const { return m_lockRootRotation; }

private:
    /// クリップの再生状態
    struct ClipState
    {
        const AnimationClip* clip = nullptr;
        float time = 0.0f;
        float speed = 1.0f;
        bool  loop = true;
    };

    void EnsurePoseStorage();
    void AdvanceClip(ClipState& state, float deltaTime);
    void SampleClip(const ClipState& state, std::vector<TransformTRS>& outPose);
    void BlendPoses(const std::vector<TransformTRS>& a,
                    const std::vector<TransformTRS>& b,
                    float t,
                    std::vector<TransformTRS>& outPose);

    Skeleton* m_skeleton = nullptr;

    ClipState m_current;          ///< 現在再生中のクリップ
    ClipState m_next;             ///< クロスフェード先のクリップ
    bool      m_playing = false;
    bool      m_paused  = false;

    float m_fadeDuration = 0.0f;  ///< クロスフェードの総時間
    float m_fadeTime = 0.0f;      ///< クロスフェードの経過時間
    bool  m_fading = false;       ///< クロスフェード中フラグ

    AnimMode m_mode = AnimMode::Simple;
    BlendStack* m_blendStack = nullptr;
    AnimatorStateMachine* m_stateMachine = nullptr;

    bool m_lockRootPosition = false;
    bool m_lockRootRotation = false;

    std::vector<TransformTRS> m_bindPose;       ///< スケルトンのバインドポーズ（TRS分解済み）
    std::vector<TransformTRS> m_poseA;          ///< ブレンド用テンポラリ
    std::vector<TransformTRS> m_poseB;          ///< ブレンド用テンポラリ
    std::vector<TransformTRS> m_localPose;      ///< 最終ローカルTRS姿勢
    std::vector<XMFLOAT4X4>   m_localTransforms;
    std::vector<XMFLOAT4X4>   m_globalTransforms;
    BoneConstants             m_boneConstants;
};

} // namespace GX
