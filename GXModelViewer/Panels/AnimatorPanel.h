#pragma once
/// @file AnimatorPanel.h
/// @brief ImNodesによるステートマシン可視化パネル
///
/// AnimatorStateMachineの各ステートをノード、遷移をリンクとして描画する。
/// ステートの選択、トリガー発火、Floatパラメータ設定もこのパネルから操作できる。

namespace GX { class AnimatorStateMachine; }

/// @brief AnimatorStateMachineをノードエディタで可視化・操作するパネル
class AnimatorPanel
{
public:
    /// @brief ステートマシンパネルを描画する
    /// @param stateMachine 可視化対象のステートマシン（nullptrの場合は無効表示）
    void Draw(GX::AnimatorStateMachine* stateMachine);

private:
    bool m_initialized = false;  ///< 初回描画時のノード配置が済んだか
    int  m_selectedNode = -1;    ///< 選択中のノード（ステート）インデックス
};
