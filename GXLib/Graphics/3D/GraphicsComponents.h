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
#include "Graphics/3D/Decal.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief メッシュレンダラーコンポーネント
struct MeshRendererComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::MeshRenderer;
    ComponentType GetType() const override { return k_Type; }
    Model* model = nullptr;
    std::unique_ptr<Model> ownedModel;              ///< インポートしたモデルの所有権
    gx::Vector<Material> materials;
    bool castShadow = true;
    bool receiveShadow = true;
    gx::Vector<bool> submeshVisibility;            ///< サブメッシュごとの表示ON/OFF
    gx::String sourcePath;                         ///< インポート元ファイルパス
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
    gx::String sourcePath;                         ///< インポート元ファイルパス
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

/// @brief デカールプロジェクターコンポーネント
///
/// エンティティにアタッチして、シーンにデカールを投影する。
/// DecalSystem と連携し、Transform コンポーネントの位置・回転に基づいて
/// Box Projection 方式でデカールを配置する。
struct DecalProjectorComponent : Component
{
    static constexpr ComponentType k_Type = ComponentType::Custom;
    ComponentType GetType() const override { return k_Type; }
    std::unique_ptr<Component> Clone() const override
    {
        auto c = std::make_unique<DecalProjectorComponent>();
        c->textureHandle = textureHandle;
        c->width = width;
        c->height = height;
        c->depth = depth;
        c->color = color;
        c->opacity = opacity;
        c->blendMode = blendMode;
        c->metallic = metallic;
        c->roughness = roughness;
        c->lifetime = lifetime;
        c->fadeTime = fadeTime;
        c->elapsed = elapsed;
        c->normalThreshold = normalThreshold;
        c->fadeDistance = fadeDistance;
        c->priority = priority;
        c->normalTextureHandle = normalTextureHandle;
        return c;
    }

    uint32_t textureHandle = 0;                            ///< テクスチャハンドル
    float    width         = 1.0f;                         ///< 投影幅
    float    height        = 1.0f;                         ///< 投影高さ
    float    depth         = 1.0f;                         ///< 投影奥行き
    Vector3  color         = {1.0f, 1.0f, 1.0f};          ///< デカールカラー
    float    opacity       = 1.0f;                         ///< 不透明度
    DecalBlendMode blendMode = DecalBlendMode::Alpha;      ///< ブレンドモード
    float    metallic      = 0.0f;                         ///< メタリック
    float    roughness     = 0.5f;                         ///< ラフネス
    float    lifetime      = -1.0f;                        ///< 寿命（秒、負=無限）
    float    fadeTime      = 0.5f;                         ///< フェードアウト時間
    float    elapsed       = 0.0f;                         ///< 経過時間
    float    normalThreshold = 0.7f;                       ///< 法線方向しきい値
    float    fadeDistance     = 0.5f;                       ///< エッジフェード距離
    int      priority        = 0;                          ///< 描画優先度
    int      normalTextureHandle = -1;                     ///< 法線テクスチャハンドル

    /// @brief DecalData に変換する（DecalSystem への橋渡し）
    /// @param worldTransform ワールドトランスフォーム
    /// @return DecalData
    DecalData ToDecalData(const Transform3D& worldTransform) const
    {
        DecalData d;
        d.transform = worldTransform;
        d.transform.SetScale(Vector3(width, height, depth));
        d.textureHandle = static_cast<int>(textureHandle);
        d.color = Color(color.x, color.y, color.z, opacity);
        d.blendMode = blendMode;
        d.metallic = metallic;
        d.roughness = roughness;
        d.lifetime = lifetime;
        d.age = elapsed;
        d.fadeDistance = fadeDistance;
        d.normalThreshold = normalThreshold;
        d.priority = priority;
        d.normalTextureHandle = normalTextureHandle;
        return d;
    }

    /// @brief 経過時間を更新し、フェード中のオパシティを返す
    /// @param deltaTime 経過秒数
    /// @return 現在のフェード適用後の不透明度（0.0〜1.0）
    float UpdateLifetime(float deltaTime)
    {
        if (lifetime < 0.0f)
            return opacity;

        elapsed += deltaTime;

        float fadeStartTime = lifetime - fadeTime;
        if (fadeStartTime < 0.0f)
            fadeStartTime = 0.0f;

        if (elapsed >= lifetime)
            return 0.0f;

        if (elapsed > fadeStartTime && fadeTime > 0.0f)
        {
            float fadeProgress = (elapsed - fadeStartTime) / fadeTime;
            return opacity * (1.0f - fadeProgress);
        }

        return opacity;
    }

    /// @brief 寿命が尽きたか
    /// @return 寿命が有限で、経過時間が寿命を超えた場合true
    bool IsExpired() const
    {
        return lifetime >= 0.0f && elapsed >= lifetime;
    }
};

/// @}
} // namespace gx
