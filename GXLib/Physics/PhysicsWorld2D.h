#pragma once
/// @file PhysicsWorld2D.h
/// @brief 2D物理ワールド — ブロードフェーズ衝突検出・衝突応答・レイキャスト
///
/// カスタム2D物理エンジン。重力・衝突・摩擦・トリガーをサポートする。
/// Step() を毎フレーム呼び出してシミュレーションを進める。
///
/// @note 画面座標系 (Y-down) で使用する場合、重力のYは正の値にすること。
#include "RigidBody2D.h"
#include "Math/Collision/Collision2D.h"

namespace GX {

/// @brief 2D衝突情報
struct ContactInfo2D {
    RigidBody2D* bodyA = nullptr;   ///< 衝突ボディA
    RigidBody2D* bodyB = nullptr;   ///< 衝突ボディB
    Vector2 point;                   ///< 衝突点 (ワールド座標)
    Vector2 normal;                  ///< 衝突法線 (AからBへの方向)
    float depth = 0.0f;             ///< めり込み深さ
};

/// @brief 2D物理ワールド
class PhysicsWorld2D
{
public:
    PhysicsWorld2D();
    ~PhysicsWorld2D();

    /// @brief 新しい2D剛体を作成してワールドに追加する
    /// @return 作成された剛体へのポインタ (ワールドが所有権を持つ)
    RigidBody2D* AddBody();

    /// @brief 指定した剛体をワールドから削除する
    /// @param body 削除する剛体 (以後ポインタは無効になる)
    void RemoveBody(RigidBody2D* body);

    /// @brief 物理シミュレーションを1ステップ進める
    /// @param deltaTime 経過時間 (秒)
    /// @param velocityIterations 速度反復回数 (デフォルト: 8)
    /// @param positionIterations 位置補正反復回数 (デフォルト: 3)
    void Step(float deltaTime, int velocityIterations = 8, int positionIterations = 3);

    /// @brief 重力を設定する
    /// @param gravity 重力ベクトル (画面座標系ではY正が下方向)
    void SetGravity(const Vector2& gravity) { m_gravity = gravity; }

    /// @brief 現在の重力を取得する
    /// @return 重力ベクトル
    Vector2 GetGravity() const { return m_gravity; }

    /// @brief レイキャストを実行する
    /// @param origin レイの始点
    /// @param direction レイの方向 (正規化推奨)
    /// @param maxDistance 最大距離
    /// @param outBody ヒットしたボディへのポインタの出力先 (nullptrで省略可)
    /// @param outPoint ヒット点の出力先 (nullptrで省略可)
    /// @param outNormal ヒット法線の出力先 (nullptrで省略可)
    /// @return ヒットした場合true
    bool Raycast(const Vector2& origin, const Vector2& direction, float maxDistance,
                 RigidBody2D** outBody = nullptr, Vector2* outPoint = nullptr,
                 Vector2* outNormal = nullptr);

    /// @brief AABB範囲内のボディを検索する
    /// @param area 検索範囲のAABB
    /// @param results 見つかったボディの出力先
    void QueryAABB(const AABB2D& area, std::vector<RigidBody2D*>& results);

    /// @brief 衝突発生時のコールバック
    std::function<void(const ContactInfo2D&)> onCollision;
    /// @brief トリガー開始時のコールバック
    std::function<void(RigidBody2D*, RigidBody2D*)> onTriggerEnter;
    /// @brief トリガー終了時のコールバック
    std::function<void(RigidBody2D*, RigidBody2D*)> onTriggerExit;

private:
    std::vector<std::unique_ptr<RigidBody2D>> m_bodies;
    Vector2 m_gravity = { 0.0f, -9.81f };

    void BroadPhase(std::vector<std::pair<RigidBody2D*, RigidBody2D*>>& pairs);
    bool NarrowPhase(RigidBody2D* a, RigidBody2D* b, ContactInfo2D& contact);
    void ResolveCollision(const ContactInfo2D& contact);
    void IntegrateBodies(float dt);

    AABB2D GetBodyAABB(const RigidBody2D& body) const;
    Circle GetBodyCircle(const RigidBody2D& body) const;
};

} // namespace GX
