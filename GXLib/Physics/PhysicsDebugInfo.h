#pragma once
/// @file PhysicsDebugInfo.h
/// @brief 物理ワールドのデバッグ描画用データ構造（Graphics 非依存）
///
/// PhysicsWorld3D から取得したボディ・接触点・コンストレイントの情報を
/// POD 構造体でまとめたもの。Graphics モジュールの PhysicsDebugRenderer が
/// これを受け取り、ワイヤーフレームや接触点を画面に描く。
/// @addtogroup grp_physics/// @{

#include "pch_common.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

namespace gx
{

/// @brief デバッグ描画用のシェイプ種別
enum class DebugShapeType : uint32_t
{
    Box,        ///< ボックス
    Sphere,     ///< 球
    Capsule,    ///< カプセル
    Mesh,       ///< メッシュ（ワイヤー描画は非対応、AABBで代替）
    ConvexHull  ///< 凸包（ワイヤー描画は非対応、AABBで代替）
};

/// @brief 物理ボディのデバッグ情報
struct BodyDebugInfo
{
    uint32_t bodyId;            ///< ボディID
    uint32_t motionType;        ///< モーションタイプ (0=Static, 1=Kinematic, 2=Dynamic)
    bool isActive;              ///< アクティブ状態（スリープしていないか）
    Vector3 position;           ///< ワールド位置
    Quaternion rotation;        ///< ワールド回転
    DebugShapeType shapeType;   ///< シェイプの種類
    Vector3 shapeExtents;       ///< ボックスの半径（Box用）
    float shapeRadius = 0.0f;   ///< 球/カプセルの半径
    float capsuleHalfHeight = 0.0f; ///< カプセルの半高さ
    Vector3 linearVelocity;     ///< 線速度
    Vector3 angularVelocity;    ///< 角速度
};

/// @brief 接触点のデバッグ情報
struct ContactDebugInfo
{
    Vector3 pointA;     ///< ボディAの接触点
    Vector3 pointB;     ///< ボディBの接触点
    Vector3 normal;     ///< 接触法線（AからBへ）
    float penetration;  ///< 貫通深度
};

/// @brief コンストレイントのデバッグ情報
struct ConstraintDebugInfo
{
    Vector3 anchorA;    ///< ボディA上のアンカー点
    Vector3 anchorB;    ///< ボディB上のアンカー点
    uint32_t type;      ///< コンストレイント種別 (0=Fixed, 1=Point, 2=Hinge, 3=Slider, 4=Cone)
};

} // namespace gx
/// @}
