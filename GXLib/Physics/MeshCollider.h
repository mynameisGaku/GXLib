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
    Static,  ///< 三角形メッシュ（静的専用、凹形状OK）
    Convex   ///< 凸包（動的向け、凸形状のみ）
};

/// @brief メッシュコライダーの生成設定
struct MeshColliderDesc
{
    MeshColliderType type = MeshColliderType::Static; ///< コライダー種別
    bool optimize = true;           ///< 凸包生成時に重複頂点を削除するか
    float weldTolerance = 0.0001f;  ///< 重複判定の許容誤差
    uint32_t maxConvexVertices = 256; ///< 凸包頂点数の上限（0=既定256）
    float maxConvexRadius = 0.0f;   ///< 凸半径（0=Jolt既定値）
};

/// @brief メッシュコライダー生成クラス
///
/// 3Dモデルの頂点データから物理コライダーを作成する。
/// スキンドモデルの場合はアニメーションポーズを焼き込んだ形状を生成できる。
class MeshCollider
{
public:
    MeshCollider() = default;

    /// @brief 静的モデルからコライダーを作成する
    /// @param world 物理ワールド
    /// @param model 元となる3Dモデル
    /// @param desc 生成設定
    /// @return 成功時true
    bool BuildFromModel(PhysicsWorld3D& world, const Model& model, const MeshColliderDesc& desc = {});

    /// @brief スキンドモデル+Animatorの現在ポーズからコライダーを作成する
    /// @param world 物理ワールド
    /// @param model スキンドモデル
    /// @param animator アニメーター（現在のポーズを使用）
    /// @param desc 生成設定
    /// @return 成功時true
    bool BuildFromSkinnedModel(PhysicsWorld3D& world, const Model& model, const Animator& animator,
                               const MeshColliderDesc& desc = {});

    /// @brief スキンドモデル+AnimationPlayerの現在ポーズからコライダーを作成する
    /// @param world 物理ワールド
    /// @param model スキンドモデル
    /// @param player アニメーションプレイヤー（現在のポーズを使用）
    /// @param desc 生成設定
    /// @return 成功時true
    bool BuildFromSkinnedModel(PhysicsWorld3D& world, const Model& model, const AnimationPlayer& player,
                               const MeshColliderDesc& desc = {});

    /// @brief 既存ボディのシェイプをAnimatorの現在ポーズで更新する
    /// @param world 物理ワールド
    /// @param body 更新するボディのID
    /// @param model スキンドモデル
    /// @param animator アニメーター
    /// @param desc 生成設定
    /// @param activate trueでボディを起動状態にする
    /// @return 成功時true
    bool UpdateFromSkinnedModel(PhysicsWorld3D& world, PhysicsBodyID body,
                                const Model& model, const Animator& animator,
                                const MeshColliderDesc& desc = {}, bool activate = true);

    /// @brief 既存ボディのシェイプをAnimationPlayerの現在ポーズで更新する
    /// @param world 物理ワールド
    /// @param body 更新するボディのID
    /// @param model スキンドモデル
    /// @param player アニメーションプレイヤー
    /// @param desc 生成設定
    /// @param activate trueでボディを起動状態にする
    /// @return 成功時true
    bool UpdateFromSkinnedModel(PhysicsWorld3D& world, PhysicsBodyID body,
                                const Model& model, const AnimationPlayer& player,
                                const MeshColliderDesc& desc = {}, bool activate = true);

    /// @brief コライダーシェイプを解放する
    /// @param world 物理ワールド
    void Release(PhysicsWorld3D& world);

    /// @brief 内部のシェイプを取得する
    /// @return シェイプポインタ（未作成ならnullptr）
    PhysicsShape* GetShape() const { return m_shape; }

private:
    PhysicsShape* m_shape = nullptr; ///< 保持しているコライダーシェイプ
};

} // namespace GX
