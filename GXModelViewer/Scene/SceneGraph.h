#pragma once
/// @file SceneGraph.h
/// @brief GXModelViewer用シンプルシーングラフ
///
/// エンティティをフラットなvectorで管理する。削除はGPUリソース解放待ちのため
/// 遅延実行（_pendingRemoval→ProcessPendingRemovals）方式を採用。

#include <string>
#include <vector>
#include <memory>

#include "Graphics/3D/Transform3D.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Animator.h"

/// @brief シーン内の1エンティティ。モデル・トランスフォーム・アニメーション・表示設定を保持。
struct SceneEntity
{
    std::string      name;                              ///< エンティティ名（ファイル名から自動設定）
    GX::Transform3D  transform;                         ///< ワールドトランスフォーム
    GX::Model*       model = nullptr;                   ///< モデルへの非所有参照
    std::unique_ptr<GX::Model> ownedModel;              ///< インポートしたモデルの所有権
    GX::Material     materialOverride;                  ///< エンティティ全体に適用する上書きマテリアル
    bool             useMaterialOverride = false;       ///< マテリアルオーバーライドを有効にするか
    int              parentIndex = -1;                  ///< 親エンティティのインデックス（-1=ルート）
    bool             visible = true;                    ///< 表示ON/OFF
    std::string      sourcePath;                        ///< インポート元ファイルパス（シーン保存用）

    // --- アニメーション ---
    std::unique_ptr<GX::Animator> animator;             ///< スキンドモデル用Animator
    int  selectedClipIndex = -1;                        ///< タイムラインで選択中のクリップインデックス

    // --- 表示制御 ---
    std::vector<bool> submeshVisibility;                ///< サブメッシュごとの表示ON/OFF
    bool showBones = false;                             ///< ボーン可視化ON/OFF
    bool showWireframe = false;                         ///< ワイヤフレーム表示ON/OFF
    bool _pendingRemoval = false;                       ///< 内部用：遅延削除フラグ
};

/// @brief フラット配列ベースのシーングラフ。フリーリスト方式でスロット再利用。
class SceneGraph
{
public:
    /// @brief 新しいエンティティを追加する（空きスロットがあれば再利用）
    /// @param name エンティティ名
    /// @return 追加されたエンティティのインデックス
    int AddEntity(const std::string& name);

    /// @brief エンティティを遅延削除する（GPUフラッシュ後にProcessPendingRemovalsで実削除）
    /// @param index 削除対象のインデックス
    void RemoveEntity(int index);

    /// @brief 指定インデックスのエンティティを取得する（無効なら nullptr）
    /// @param index エンティティインデックス
    /// @return エンティティへのポインタ、または nullptr
    SceneEntity* GetEntity(int index);

    /// @brief 指定インデックスのエンティティを取得する（const版）
    const SceneEntity* GetEntity(int index) const;

    /// @brief エンティティ配列全体を取得する（削除済みスロット含む）
    const std::vector<SceneEntity>& GetEntities() const { return m_entities; }

    /// @brief エンティティスロット数を返す（削除済み含む）
    int GetEntityCount() const { return static_cast<int>(m_entities.size()); }

    /// @brief 遅延削除待ちのエンティティがあるか
    /// @return 削除待ちがあればtrue
    bool HasPendingRemovals() const { return !m_pendingRemovals.empty(); }

    /// @brief 遅延削除待ちのエンティティを実際に破棄する。GPUフラッシュ後に呼ぶこと。
    void ProcessPendingRemovals();

    int selectedEntity = -1;  ///< 選択中エンティティインデックス（-1=未選択）
    int selectedBone = -1;    ///< 選択中ボーンインデックス（Hierarchy/Skeletonパネル共有）

private:
    std::vector<SceneEntity> m_entities;        ///< エンティティ配列
    std::vector<int>         m_freeIndices;     ///< 再利用可能なスロットインデックス
    std::vector<int>         m_pendingRemovals; ///< 遅延削除待ちのインデックス
};
