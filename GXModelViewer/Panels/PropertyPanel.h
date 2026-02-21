#pragma once
/// @file PropertyPanel.h
/// @brief 選択エンティティのプロパティインスペクタパネル
///
/// トランスフォーム、モデルマテリアル（直接編集）、マテリアルオーバーライド、
/// ギズモ設定、レンダリングオプション（ボーン表示/ワイヤフレーム）を提供する。

#include "Scene/SceneGraph.h"
#include "Graphics/3D/Material.h"
#include "Graphics/Resource/TextureManager.h"
#include <imgui.h>
#include "ImGuizmo.h"

/// @brief 選択エンティティのプロパティ（Transform/Material/Gizmo等）を編集するパネル
class PropertyPanel
{
public:
    /// @brief プロパティパネルを独立ウィンドウとして描画する
    /// @param scene シーングラフ（選択エンティティの取得に使用）
    /// @param matManager マテリアルハンドルからマテリアルを参照
    /// @param texManager テクスチャ管理（将来のテクスチャプレビュー用）
    /// @param gizmoOp ギズモ操作モード（移動/回転/拡縮）の参照
    /// @param gizmoMode 座標空間（ワールド/ローカル）の参照
    /// @param useSnap スナップON/OFFの参照
    /// @param snapT 移動スナップ値の参照
    /// @param snapR 回転スナップ値の参照
    /// @param snapS スケールスナップ値の参照
    void Draw(SceneGraph& scene, GX::MaterialManager& matManager, GX::TextureManager& texManager,
              ImGuizmo::OPERATION& gizmoOp, ImGuizmo::MODE& gizmoMode,
              bool& useSnap, float& snapT, float& snapR, float& snapS);

    /// @brief タブコンテナ内埋め込み用（Begin/Endなし）
    void DrawContent(SceneGraph& scene, GX::MaterialManager& matManager, GX::TextureManager& texManager,
                     ImGuizmo::OPERATION& gizmoOp, ImGuizmo::MODE& gizmoMode,
                     bool& useSnap, float& snapT, float& snapR, float& snapS);

private:
    /// @brief Transform（Position/Rotation/Scale）の編集UI
    void DrawTransformSection(SceneEntity& entity);

    /// @brief マテリアルオーバーライド（エンティティ全体に適用する上書きマテリアル）の編集UI
    void DrawMaterialOverrideSection(SceneEntity& entity);

    /// @brief モデル本体のサブメッシュ別マテリアルを直接編集するUI
    void DrawModelMaterials(SceneEntity& entity, GX::MaterialManager& matManager);

    /// @brief シェーダーモデル固有パラメータの編集UI（Toon/Phong/Subsurface/ClearCoat等）
    /// @param params 編集対象のパラメータ構造体
    /// @param model 現在のシェーダーモデル種別
    void DrawShaderModelParams(gxfmt::ShaderModelParams& params, gxfmt::ShaderModel model);

    char m_nameBuffer[256] = {};  ///< エンティティ名編集バッファ
};
