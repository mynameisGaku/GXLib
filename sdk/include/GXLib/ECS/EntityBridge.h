#pragma once
/// @file EntityBridge.h
/// @brief 旧SceneシステムとECS Worldの橋渡しユーティリティ
///
/// 既存のgx::Entityベースのシーンオブジェクトをgx::ecs::Worldに
/// インポートしたり、ECS側のデータを元のEntityに書き戻したりする。
/// OOPとデータ指向ECSを段階的に移行する際に使う。
/// @addtogroup grp_ecs/// @{

#include "pch_common.h"
#include "ECS/World.h"

namespace gx
{
class Entity;
class Scene;
}

namespace gx { namespace ecs {

// ---------------------------------------------------------------------------
// ブリッジがトランスフォームデータに使用するシンプルなPODコンポーネント
// ---------------------------------------------------------------------------

/// @brief ECSブリッジ用位置コンポーネント
struct BridgePosition
{
    float x = 0;  ///< X座標
    float y = 0;  ///< Y座標
    float z = 0;  ///< Z座標
};

/// @brief ECSブリッジ用回転コンポーネント（オイラー角度）
struct BridgeRotation
{
    float x = 0;  ///< ピッチ（度）
    float y = 0;  ///< ヨー（度）
    float z = 0;  ///< ロール（度）
};

/// @brief ECSブリッジ用スケールコンポーネント
struct BridgeScale
{
    float x = 1;  ///< X方向スケール
    float y = 1;  ///< Y方向スケール
    float z = 1;  ///< Z方向スケール
};

/// @brief ECSブリッジ用名前タグコンポーネント
struct BridgeName
{
    char name[64] = {};  ///< エンティティ名（最大63文字+NULL）
};

// ---------------------------------------------------------------------------
// EntityBridge
// ---------------------------------------------------------------------------

/// @brief gx::Sceneとgx::ecs::World間を同期するユーティリティクラス
class EntityBridge
{
public:
    /// @brief 単一のレガシーEntityをECSワールドにインポートする
    /// @return 割り当てられたECS EntityID
    static EntityID ImportEntity(World& world, const Entity& entity);

    /// @brief ECSエンティティのデータをレガシーEntityにエクスポートする
    static void ExportEntity(const World& world, EntityID ecsEntity, Entity& entity);

    /// @brief SceneのすべてのエンティティをWorldにインポートする
    static void SyncSceneToWorld(World& world, const Scene& scene);

    /// @brief すべてのECSエンティティをSceneにエクスポートする
    static void SyncWorldToScene(const World& world, Scene& scene);

    /// @brief 現在マッピングされているエンティティ数
    static uint32_t GetImportedCount() { return static_cast<uint32_t>(s_entityMap.size()); }

    /// @brief すべてのインポート/エクスポートマッピングをクリアする
    static void ClearMappings() { s_entityMap.clear(); s_reverseMap.clear(); }

private:
    /// SceneエンティティID -> ECSエンティティID
    static inline gx::HashMap<uint32_t, EntityID> s_entityMap;

    /// ECSエンティティID -> SceneエンティティID
    static inline gx::HashMap<EntityID, uint32_t> s_reverseMap;
};

}} // namespace gx::ecs
/// @}
