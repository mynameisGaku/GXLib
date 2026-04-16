#pragma once
/// @file ModelInfoPanel.h
/// @brief モデル情報表示パネル
///
/// 選択エンティティのモデルについて、頂点数/三角形数/バッファサイズ、AABB、
/// サブメッシュ数、ボーン数、アニメーション一覧などの統計情報を表示する。

#include "Scene/SceneGraph.h"

/// @brief 選択モデルの統計情報（頂点数・AABB・アニメーション等）を表示するパネル
class ModelInfoPanel
{
public:
    /// @brief モデル情報パネルを独立ウィンドウとして描画する
    /// @param scene シーングラフ（選択エンティティの取得に使用）
    void Draw(const SceneGraph& scene);

    /// @brief タブコンテナ内埋め込み用（Begin/Endなし）
    /// @param scene シーングラフ
    void DrawContent(const SceneGraph& scene);
};
