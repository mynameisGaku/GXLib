#pragma once
/// @file SceneHierarchyPanel.h
/// @brief シーン階層パネル（エンティティ一覧とボーンツリー）

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Skeleton.h"

/// @brief シーン内の全エンティティをツリー表示し、選択・追加・削除を行うパネル
class SceneHierarchyPanel
{
public:
    /// @brief 階層パネルを描画する。エンティティの選択やコンテキストメニューもここで処理。
    /// @param scene 操作対象のシーングラフ
    void Draw(SceneGraph& scene);

private:
    /// @brief スキンドモデルのボーン階層をツリーノードとして再帰描画する
    /// @param skeleton ボーン情報
    /// @param scene ボーン選択状態の読み書き先
    /// @param jointIndex 描画対象のジョイントインデックス
    void DrawBoneTree(const GX::Skeleton* skeleton, SceneGraph& scene, int jointIndex);
};
