#pragma once
/// @file SkeletonPanel.h
/// @brief ボーン階層表示・詳細パネル
///
/// ジョイントをツリー表示し、選択したボーンのローカルTRS（Translation/Rotation/Scale）、
/// ワールド座標、ワールド変換行列、逆バインド行列を表示する。

#include "Scene/SceneGraph.h"

namespace GX { class Animator; }

/// @brief ジョイント階層ツリーとボーン詳細（TRS/行列）を表示するパネル
class SkeletonPanel
{
public:
    /// @brief スケルトンパネルを独立ウィンドウとして描画する
    /// @param scene シーングラフ（選択エンティティ・ボーンの読み書き先）
    /// @param animator ローカルポーズ・グローバルトランスフォームの取得元
    void Draw(SceneGraph& scene, const GX::Animator* animator);

    /// @brief タブコンテナ内埋め込み用（Begin/Endなし）
    void DrawContent(SceneGraph& scene, const GX::Animator* animator);

private:
    /// @brief ジョイントツリーを再帰的に描画する
    /// @param skeleton ジョイント情報
    /// @param scene ボーン選択状態の読み書き先
    /// @param animator ローカルポーズの取得元（TRS表示用）
    /// @param jointIndex 描画対象のジョイントインデックス
    void DrawJointTree(const GX::Skeleton* skeleton, SceneGraph& scene,
                       const GX::Animator* animator, int jointIndex);
};
