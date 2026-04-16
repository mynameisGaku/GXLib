#pragma once
/// @file SceneHierarchyPanel.h
/// @brief シーン階層パネル（エンティティ一覧、ボーンツリー、D&D親子変更、プリミティブ追加）

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Skeleton.h"

/// @brief プリミティブ追加要求の種別
enum class PrimitiveRequest
{
    None,
    Empty,
    Cube,
    Sphere,
    Plane,
    Cylinder,
    Cone
};

/// @brief シーン内の全エンティティをツリー表示し、選択・追加・削除を行うパネル
class SceneHierarchyPanel
{
public:
    /// @brief 階層パネルを描画する。エンティティの選択やコンテキストメニューもここで処理。
    /// @param scene 操作対象のシーングラフ
    void Draw(SceneGraph& scene);

    /// @brief 未処理のプリミティブ追加要求を取得する
    PrimitiveRequest GetPendingPrimitive() const { return m_pendingPrimitive; }

    /// @brief プリミティブ追加要求をクリアする
    void ClearPendingPrimitive() { m_pendingPrimitive = PrimitiveRequest::None; }

    /// @brief 未処理の親子関係変更要求があるか
    bool HasReparentRequest() const { return m_reparentEntity >= 0; }

    /// @brief 親子関係変更要求を取得する
    void GetReparentRequest(int& outEntity, int& outOldParent, int& outNewParent) const
    {
        outEntity = m_reparentEntity;
        outOldParent = m_reparentOldParent;
        outNewParent = m_reparentNewParent;
    }

    /// @brief 親子関係変更要求をクリアする
    void ClearReparentRequest() { m_reparentEntity = -1; }

private:
    /// @brief スキンドモデルのボーン階層をツリーノードとして再帰描画する
    void DrawBoneTree(const gx::Skeleton* skeleton, SceneGraph& scene, int jointIndex);

    /// @brief コンテキストメニュー（右クリック）を描画する
    void DrawContextMenu(SceneGraph& scene, int entityCount);

    PrimitiveRequest m_pendingPrimitive = PrimitiveRequest::None;
    int m_reparentEntity    = -1;
    int m_reparentOldParent = -1;
    int m_reparentNewParent = -1;
};
