#pragma once
/// @file PhysicsWorld3D.h
/// @brief 3D物理ワールド — Jolt Physics ラッパー
///
/// Jolt Physics エンジンをPIMPLパターンで内部に保持し、
/// ボディ管理・シミュレーション・レイキャストなどの機能を提供する。
///
/// @note Initialize() 後に使用し、Shutdown() で解放すること。
#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Math/Matrix4x4.h"
#include "PhysicsShape.h"

namespace GX {

/// @brief 物理ボディの識別ID
struct PhysicsBodyID {
    uint32_t id = 0xFFFFFFFF;                       ///< 内部ID (無効値: 0xFFFFFFFF)
    /// @brief IDが有効かどうか判定する
    bool IsValid() const { return id != 0xFFFFFFFF; }
};

/// @brief 3Dボディのモーションタイプ
enum class MotionType3D {
    Static,     ///< 静的 (移動しない、地面・壁など)
    Kinematic,  ///< キネマティック (コードで移動、他のDynamicを押す)
    Dynamic     ///< 動的 (力・重力・衝突で移動する)
};

/// @brief 3D物理ボディの作成設定
struct PhysicsBodySettings {
    Vector3 position;                                   ///< 初期位置
    Quaternion rotation;                                ///< 初期回転
    MotionType3D motionType = MotionType3D::Dynamic;    ///< モーションタイプ
    float mass = 1.0f;                                  ///< 質量 (kg)
    float friction = 0.5f;                              ///< 摩擦係数
    float restitution = 0.3f;                           ///< 反発係数
    float linearDamping = 0.05f;                        ///< 線速度の減衰率
    float angularDamping = 0.05f;                       ///< 角速度の減衰率
    uint16_t layer = 1;                                 ///< 衝突レイヤー (0=Static, 1=Moving)
    void* userData = nullptr;                           ///< ユーザー任意データポインタ
};

/// @brief 3D物理ワールド (Jolt Physics ラッパー)
class PhysicsWorld3D
{
public:
    PhysicsWorld3D();
    ~PhysicsWorld3D();

    /// @brief 物理ワールドを初期化する
    /// @param maxBodies 最大ボディ数 (デフォルト: 10240)
    /// @return 初期化に成功した場合true
    bool Initialize(uint32_t maxBodies = 10240);

    /// @brief 物理ワールドを終了し、全リソースを解放する
    void Shutdown();

    /// @brief 物理シミュレーションを1ステップ進める
    /// @param deltaTime 経過時間 (秒)
    void Step(float deltaTime);

    /// @brief 重力を設定する
    /// @param gravity 重力ベクトル (デフォルト: {0, -9.81, 0})
    void SetGravity(const Vector3& gravity);

    /// @brief 現在の重力を取得する
    /// @return 重力ベクトル
    Vector3 GetGravity() const;

    // ----- シェイプ作成 -----

    /// @brief ボックス形状を作成する
    /// @param halfExtents 各軸の半サイズ
    /// @return 作成されたシェイプ (使用後はDestroyShape()で解放)
    PhysicsShape* CreateBoxShape(const Vector3& halfExtents);

    /// @brief 球体形状を作成する
    /// @param radius 半径
    /// @return 作成されたシェイプ
    PhysicsShape* CreateSphereShape(float radius);

    /// @brief カプセル形状を作成する
    /// @param halfHeight 半分の高さ (球を除く円柱部分)
    /// @param radius 半径
    /// @return 作成されたシェイプ
    PhysicsShape* CreateCapsuleShape(float halfHeight, float radius);

    /// @brief メッシュ形状を作成する (Static専用)
    /// @param vertices 頂点配列
    /// @param vertexCount 頂点数
    /// @param indices インデックス配列
    /// @param indexCount インデックス数
    /// @return 作成されたシェイプ
    PhysicsShape* CreateMeshShape(const Vector3* vertices, uint32_t vertexCount,
                                   const uint32_t* indices, uint32_t indexCount);

    /// @brief 凸包形状を作成する (Dynamic/Convex用)
    /// @param vertices 頂点配列
    /// @param vertexCount 頂点数
    /// @param maxConvexRadius 凸半径 (0=default)
    /// @return 作成されたシェイプ
    PhysicsShape* CreateConvexHullShape(const Vector3* vertices, uint32_t vertexCount,
                                        float maxConvexRadius = 0.0f);

    // ----- ボディ管理 -----

    /// @brief ワールドにボディを追加する
    /// @param shape コライダーシェイプ
    /// @param settings ボディ設定
    /// @return ボディID (以後の操作に使用)
    PhysicsBodyID AddBody(PhysicsShape* shape, const PhysicsBodySettings& settings);

    /// @brief ワールドからボディを削除する
    /// @param id 削除するボディのID
    void RemoveBody(PhysicsBodyID id);

    /// @brief シェイプを破棄する
    /// @param shape 破棄するシェイプ
    void DestroyShape(PhysicsShape* shape);

    // ----- ボディ操作 -----

    /// @brief ボディの位置を設定する
    /// @param id ボディID
    /// @param pos 新しい位置
    void SetPosition(PhysicsBodyID id, const Vector3& pos);

    /// @brief ボディの回転を設定する
    /// @param id ボディID
    /// @param rot 新しい回転クォータニオン
    void SetRotation(PhysicsBodyID id, const Quaternion& rot);

    /// @brief ボディの線速度を設定する
    /// @param id ボディID
    /// @param vel 線速度ベクトル
    void SetLinearVelocity(PhysicsBodyID id, const Vector3& vel);

    /// @brief ボディの角速度を設定する
    /// @param id ボディID
    /// @param vel 角速度ベクトル
    void SetAngularVelocity(PhysicsBodyID id, const Vector3& vel);

    /// @brief ボディに力を加える (次のStep()で適用)
    /// @param id ボディID
    /// @param force 力ベクトル
    void ApplyForce(PhysicsBodyID id, const Vector3& force);

    /// @brief ボディに衝撃を加える (即座に速度変化)
    /// @param id ボディID
    /// @param impulse 衝撃ベクトル
    void ApplyImpulse(PhysicsBodyID id, const Vector3& impulse);

    /// @brief ボディにトルクを加える (次のStep()で適用)
    /// @param id ボディID
    /// @param torque トルクベクトル
    void ApplyTorque(PhysicsBodyID id, const Vector3& torque);

    /// @brief ボディのモーションタイプを変更する
    /// @param id ボディID
    /// @param type 新しいモーションタイプ
    void SetMotionType(PhysicsBodyID id, MotionType3D type);

    /// @brief 既存ボディの形状を更新する
    /// @param id ボディID
    /// @param shape 新しい形状
    /// @param updateMassProperties trueで質量/慣性を再計算
    /// @param activate trueでボディを起動状態にする
    /// @return 成功時true
    bool SetBodyShape(PhysicsBodyID id, PhysicsShape* shape,
                      bool updateMassProperties = true, bool activate = true);

    // ----- ボディ状態取得 -----

    /// @brief ボディの位置を取得する
    /// @param id ボディID
    /// @return 現在の位置
    Vector3 GetPosition(PhysicsBodyID id) const;

    /// @brief ボディの回転を取得する
    /// @param id ボディID
    /// @return 現在の回転クォータニオン
    Quaternion GetRotation(PhysicsBodyID id) const;

    /// @brief ボディの線速度を取得する
    /// @param id ボディID
    /// @return 現在の線速度ベクトル
    Vector3 GetLinearVelocity(PhysicsBodyID id) const;

    /// @brief ボディのワールド変換行列を取得する
    /// @param id ボディID
    /// @return ワールド変換行列 (描画に直接使用可能)
    Matrix4x4 GetWorldTransform(PhysicsBodyID id) const;

    /// @brief ボディがアクティブ (スリープしていない) かどうか判定する
    /// @param id ボディID
    /// @return アクティブの場合true
    bool IsActive(PhysicsBodyID id) const;

    // ----- レイキャスト -----

    /// @brief レイキャスト結果
    struct RaycastResult {
        bool hit = false;           ///< ヒットしたかどうか
        PhysicsBodyID bodyID;       ///< ヒットしたボディのID
        Vector3 point;              ///< ヒット点 (ワールド座標)
        Vector3 normal;             ///< ヒット法線
        float fraction = 0.0f;      ///< レイ全長に対するヒット位置の割合 [0,1]
    };

    /// @brief レイキャストを実行する
    /// @param origin レイの始点
    /// @param direction レイの方向 (正規化推奨)
    /// @param maxDistance 最大距離
    /// @return レイキャスト結果
    RaycastResult Raycast(const Vector3& origin, const Vector3& direction, float maxDistance);

    // ----- コールバック -----

    /// @brief 接触開始時のコールバック
    std::function<void(PhysicsBodyID, PhysicsBodyID, const Vector3& contactPoint)> onContactAdded;
    /// @brief 接触終了時のコールバック
    std::function<void(PhysicsBodyID, PhysicsBodyID)> onContactRemoved;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace GX
