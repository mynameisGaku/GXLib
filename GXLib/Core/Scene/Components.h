#pragma once
/// @file Components.h
/// @brief ビルトインコンポーネント集約

#include "pch.h"
#include "Core/Scene/Component.h"
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"
#include "Graphics/3D/Terrain.h"
#include "Graphics/3D/LODGroup.h"

namespace GX
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

/// @brief パーティクルシステムコンポーネント
struct ParticleSystemComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::ParticleSystem;
    ComponentType GetType() const override { return k_Type; }
};

/// @brief オーディオソースコンポーネント
struct AudioSourceComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::AudioSource;
    ComponentType GetType() const override { return k_Type; }
    int soundHandle = -1;
    bool playOnStart = false;
    bool loop = false;
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

/// @brief ユーザー定義ロジック用スクリプトコンポーネント
struct ScriptComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Script;
    ComponentType GetType() const override { return k_Type; }
    std::function<void(float)> onUpdate;
    std::function<void()> onStart;
    std::function<void()> onDestroy;
    bool started = false;
};

} // namespace GX
