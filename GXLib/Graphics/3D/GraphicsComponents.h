#pragma once
/// @file GraphicsComponents.h
/// @brief Graphics依存のビルトインコンポーネント
///
/// MeshRenderer, SkinnedMeshRenderer, Camera, Light, Terrain, LOD など
/// Graphics/3D の型を直接使用するコンポーネント群。
/// Core/Scene/Components.h とは分離されており、GXLib_Graphics 側で提供される。

#include "pch_graphics.h"
#include "Core/Scene/Component.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Terrain.h"
#include "Graphics/3D/LODGroup.h"

namespace gx
{

/// @brief メッシュレンダラーコンポーネント
struct MeshRendererComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::MeshRenderer;
    ComponentType GetType() const override { return k_Type; }
    Model* model = nullptr;
    std::unique_ptr<Model> ownedModel;              ///< インポートしたモデルの所有権
    std::vector<Material> materials;
    bool castShadow = true;
    bool receiveShadow = true;
    std::vector<bool> submeshVisibility;            ///< サブメッシュごとの表示ON/OFF
    std::string sourcePath;                         ///< インポート元ファイルパス
    bool useMaterialOverride = false;               ///< マテリアルオーバーライド有効化
    Material materialOverride;                      ///< オーバーライドマテリアル
};

/// @brief スキニングメッシュレンダラーコンポーネント
struct SkinnedMeshRendererComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::SkinnedMeshRenderer;
    ComponentType GetType() const override { return k_Type; }
    Model* model = nullptr;
    std::unique_ptr<Model> ownedModel;              ///< インポートしたモデルの所有権
    std::unique_ptr<Animator> animator;
    std::string sourcePath;                         ///< インポート元ファイルパス
    int selectedClipIndex = -1;                     ///< タイムラインで選択中のクリップ
};

/// @brief カメラコンポーネント
struct CameraComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Camera;
    ComponentType GetType() const override { return k_Type; }
    Camera3D camera;
    bool isMain = false;
};

/// @brief ライトコンポーネント
struct LightComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Light;
    ComponentType GetType() const override { return k_Type; }
    LightData lightData;
};

/// @brief 地形コンポーネント
struct TerrainComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Terrain;
    ComponentType GetType() const override { return k_Type; }
    Terrain* terrain = nullptr;
};

/// @brief LODコンポーネント — 距離ベースのモデル切り替え
struct LODComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::LOD;
    ComponentType GetType() const override { return k_Type; }
    LODGroup lodGroup;
};

} // namespace gx
