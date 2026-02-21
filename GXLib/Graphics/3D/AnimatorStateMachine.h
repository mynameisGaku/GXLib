#pragma once
/// @file AnimatorStateMachine.h
/// @brief アニメーションステートマシン（状態遷移+クロスフェード）
/// Unity風のステートマシンで、トリガーや再生時間に応じた自動遷移を実現する。
/// DxLibにはない高度なアニメーション制御の仕組み。

#include "pch.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/BlendTree.h"

namespace GX
{

/// @brief アニメーションステート（「歩き」「走り」等の1動作に対応する）
/// clipとblendTreeは排他で、どちらか一方だけ使う。
struct AnimState
{
    std::string name;                      ///< ステート名
    const AnimationClip* clip = nullptr;   ///< 単一クリップ（blendTree未使用時）
    BlendTree* blendTree = nullptr;        ///< ブレンドツリー（clip未使用時）
    bool loop = true;                      ///< ループ再生するか
    float speed = 1.0f;                    ///< 再生速度
};

/// @brief ステート間の遷移ルール
/// トリガー名か再生割合(exitTime)のどちらかで遷移条件を定義する。
struct AnimTransition
{
    uint32_t fromState = 0;          ///< 遷移元ステートのインデックス
    uint32_t toState = 0;            ///< 遷移先ステートのインデックス
    float duration = 0.2f;           ///< クロスフェード時間（秒）
    std::string triggerName;         ///< トリガー名（空なら未使用）
    bool hasExitTime = false;        ///< 再生割合で遷移するか
    float exitTimeNorm = 1.0f;       ///< 正規化された再生割合 (0.0~1.0)
};

/// @brief アニメーションステートマシン
/// 複数のステートと遷移ルールを管理し、条件に応じてクロスフェード付きで遷移する。
class AnimatorStateMachine
{
public:
    /// @brief ステートを追加する
    /// @param state 追加するステート
    /// @return 追加されたステートのインデックス
    uint32_t AddState(const AnimState& state);

    /// @brief 遷移ルールを追加する
    /// @param transition 追加する遷移ルール
    void AddTransition(const AnimTransition& transition);

    /// @brief トリガーを発火する（次のCheckTransitionsで消費される）
    /// @param name トリガー名
    void SetTrigger(const std::string& name);

    /// @brief floatパラメータを設定する（BlendTreeへの受け渡しに使う）
    /// @param name パラメータ名
    /// @param value 値
    void SetFloat(const std::string& name, float value);

    /// @brief floatパラメータを取得する
    /// @param name パラメータ名
    /// @return 値（未設定なら0.0）
    float GetFloat(const std::string& name) const;

    /// @brief 現在のステートを直接切り替える（遷移アニメなし）
    /// @param index ステートインデックス
    void SetCurrentState(uint32_t index);

    /// @brief 現在のステートインデックスを取得する
    /// @return ステートインデックス
    uint32_t GetCurrentStateIndex() const { return m_currentState; }

    /// @brief 現在のステートを取得する
    /// @return ステートへのポインタ（無効ならnullptr）
    const AnimState* GetCurrentState() const;

    /// @brief 指定インデックスのステートを取得する
    /// @param index ステートインデックス
    /// @return ステートへのポインタ（範囲外ならnullptr）
    const AnimState* GetState(uint32_t index) const;

    /// @brief 登録されたステート数を取得する
    /// @return ステート数
    uint32_t GetStateCount() const;

    /// @brief ステートマシンを更新し、最終的なアニメーションポーズを計算する
    /// @param deltaTime フレーム経過時間（秒）
    /// @param jointCount 関節の数
    /// @param bindPose バインドポーズ（nullならIdentity）
    /// @param outPose 出力先のポーズ配列
    void Update(float deltaTime, uint32_t jointCount,
                const TransformTRS* bindPose,
                TransformTRS* outPose);

    /// @brief 遷移中（クロスフェード中）かどうか
    /// @return 遷移中ならtrue
    bool IsTransitioning() const { return m_transitioning; }

    /// @brief 全遷移ルールを取得する
    /// @return 遷移ルール配列
    const std::vector<AnimTransition>& GetTransitions() const { return m_transitions; }

private:
    /// 遷移条件をチェックし、条件を満たしたらクロスフェードを開始する
    void CheckTransitions();

    /// ステートからポーズをサンプルする（BlendTree or 単一クリップ）
    void SampleState(const AnimState& state, float time, uint32_t jointCount,
                     const TransformTRS* bindPose, TransformTRS* outPose) const;

    /// ステートの再生時間を返す
    float GetStateDuration(const AnimState& state) const;

    std::vector<AnimState> m_states;
    std::vector<AnimTransition> m_transitions;

    uint32_t m_currentState = 0;
    float m_stateTime = 0.0f;

    // 遷移中の状態
    bool m_transitioning = false;
    uint32_t m_nextState = 0;
    float m_transitionDuration = 0.0f;
    float m_transitionTime = 0.0f;
    float m_nextStateTime = 0.0f;

    // 名前付きパラメータ
    std::unordered_map<std::string, bool> m_triggers;   ///< トリガー（発火後に自動リセット）
    std::unordered_map<std::string, float> m_floats;    ///< floatパラメータ

    mutable std::vector<TransformTRS> m_poseA;  ///< クロスフェード用テンポラリ
    mutable std::vector<TransformTRS> m_poseB;  ///< クロスフェード用テンポラリ
};

} // namespace GX
