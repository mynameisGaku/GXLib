#pragma once
/// @file AnimatorStateMachine.h
/// @brief アニメーションステートマシン（状態遷移+クロスフェード）
/// 初学者向け: 「歩く→走る→止まる」のような遷移をトリガーや時間で自動制御します。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/BlendTree.h"

namespace GX
{

/// @brief アニメーションステート（1つの状態）
/// 初学者向け: 「歩き」「走り」等、1つの動作に対応します。clipかblendTreeのどちらかを使います。
struct AnimState
{
    std::string name;
    const AnimationClip* clip = nullptr;  ///< 単一クリップ（blendTree未使用時）
    BlendTree* blendTree = nullptr;       ///< ブレンドツリー（clip未使用時）
    bool loop = true;
    float speed = 1.0f;
};

/// @brief ステート間の遷移ルール
/// 初学者向け: 「トリガーが来たら」「再生終了したら」次のステートへ、という条件を設定します。
struct AnimTransition
{
    uint32_t fromState = 0;
    uint32_t toState = 0;
    float duration = 0.2f;           ///< クロスフェード時間（秒）
    std::string triggerName;         ///< トリガー名（空なら未使用）
    bool hasExitTime = false;        ///< 再生割合で遷移するか
    float exitTimeNorm = 1.0f;       ///< 正規化された再生割合 (0..1)
};

/// @brief アニメーションステートマシン
/// 初学者向け: ゲーム中の状態遷移を自動化し、滑らかなアニメ切り替えを実現します。
class AnimatorStateMachine
{
public:
    uint32_t AddState(const AnimState& state);
    void AddTransition(const AnimTransition& transition);

    void SetTrigger(const std::string& name);
    void SetFloat(const std::string& name, float value);
    float GetFloat(const std::string& name) const;

    void SetCurrentState(uint32_t index);
    uint32_t GetCurrentStateIndex() const { return m_currentState; }
    const AnimState* GetCurrentState() const;
    const AnimState* GetState(uint32_t index) const;
    uint32_t GetStateCount() const;

    /// @brief ステートマシンを更新し最終ポーズを計算
    /// @param deltaTime フレーム経過時間
    /// @param jointCount 関節の数
    /// @param bindPose バインドポーズ
    /// @param outPose 出力先のポーズ配列
    void Update(float deltaTime, uint32_t jointCount,
                const TransformTRS* bindPose,
                TransformTRS* outPose);

    bool IsTransitioning() const { return m_transitioning; }

private:
    void CheckTransitions();
    void SampleState(const AnimState& state, float time, uint32_t jointCount,
                     const TransformTRS* bindPose, TransformTRS* outPose) const;
    float GetStateDuration(const AnimState& state) const;

    std::vector<AnimState> m_states;
    std::vector<AnimTransition> m_transitions;

    uint32_t m_currentState = 0;
    float m_stateTime = 0.0f;

    // 遷移
    bool m_transitioning = false;
    uint32_t m_nextState = 0;
    float m_transitionDuration = 0.0f;
    float m_transitionTime = 0.0f;
    float m_nextStateTime = 0.0f;

    // パラメータ
    std::unordered_map<std::string, bool> m_triggers;
    std::unordered_map<std::string, float> m_floats;

    mutable std::vector<TransformTRS> m_poseA;
    mutable std::vector<TransformTRS> m_poseB;
};

} // namespace GX
