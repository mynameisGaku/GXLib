#pragma once
/// @file MeshCollider.h
/// @brief メッシュコライダー用ヘルパー（静的/凸包 + スキン焼き込み）

#include "Physics/PhysicsWorld3D.h"

namespace GX
{

class Model;
class Animator;
class AnimationPlayer;

/// @brief メッシュコライダー種別
enum class MeshColliderType
{
    Static,  ///< 三角形メッシュ（静的専用）
    Convex   ///< 凸包（動的向け）
};

/// @brief メッシュコライダーの生成設定
struct MeshColliderDesc
{
    MeshColliderType type = MeshColliderType::Static;
    bool optimize = true;           ///< 凸包用に重複点を削除
    float weldTolerance = 0.0001f;  ///< 重複判定の許容誤差
    uint32_t maxConvexVertices = 256; ///< 凸包頂点数の上限（0=既定256）
    float maxConvexRadius = 0.0f;   ///< 凸半径（0=既定）
};

/// @brief メッシュコライダー生成クラス
class MeshCollider
{
public:
    MeshCollider() = default;

    bool BuildFromModel(PhysicsWorld3D& world, const Model& model, const MeshColliderDesc& desc = {});
    bool BuildFromSkinnedModel(PhysicsWorld3D& world, const Model& model, const Animator& animator,
                               const MeshColliderDesc& desc = {});
    bool BuildFromSkinnedModel(PhysicsWorld3D& world, const Model& model, const AnimationPlayer& player,
                               const MeshColliderDesc& desc = {});

    bool UpdateFromSkinnedModel(PhysicsWorld3D& world, PhysicsBodyID body,
                                const Model& model, const Animator& animator,
                                const MeshColliderDesc& desc = {}, bool activate = true);
    bool UpdateFromSkinnedModel(PhysicsWorld3D& world, PhysicsBodyID body,
                                const Model& model, const AnimationPlayer& player,
                                const MeshColliderDesc& desc = {}, bool activate = true);

    void Release(PhysicsWorld3D& world);

    PhysicsShape* GetShape() const { return m_shape; }

private:
    PhysicsShape* m_shape = nullptr;
};

} // namespace GX
