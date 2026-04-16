#pragma once
/// @file PhysicsConstraint.h
/// @brief ボディ間を繋ぐジョイント（コンストレイント）の設定構造体
///
/// ドア蝶番・スライドドア・ボールソケットなど 6 種類のジョイントを定義する。
/// Jolt Physics 非依存の公開 API で、PhysicsWorld3D::CreateXxxConstraint() に渡して使う。
/// @addtogroup grp_physics/// @{

#include "Math/Vector3.h"

namespace gx {

/// @brief コンストレイントの識別ID
struct PhysicsConstraintID {
    uint32_t id = 0xFFFFFFFF;
    bool IsValid() const { return id != 0xFFFFFFFF; }
};

/// @brief 固定コンストレイント設定（2つのボディを固定する）
struct FixedConstraintSettings {
    Vector3 point1;                ///< ボディA上のアタッチ点（ローカル空間）
    Vector3 point2;                ///< ボディB上のアタッチ点（ローカル空間）
    bool autoDetect = true;        ///< 現在の位置から自動計算するか
};

/// @brief ヒンジコンストレイント設定（蝶番回転）
struct HingeConstraintSettings {
    Vector3 point1;                ///< ボディA上のピボット点
    Vector3 hingeAxis1 = {0,1,0};  ///< ボディAのヒンジ軸
    Vector3 normalAxis1 = {1,0,0}; ///< ボディAの法線軸
    Vector3 point2;                ///< ボディB上のピボット点
    Vector3 hingeAxis2 = {0,1,0};  ///< ボディBのヒンジ軸
    Vector3 normalAxis2 = {1,0,0}; ///< ボディBの法線軸
    float limitsMin = -3.14159f;   ///< 最小角度制限（ラジアン）
    float limitsMax = 3.14159f;    ///< 最大角度制限（ラジアン）
    float maxFrictionTorque = 0.0f;///< 摩擦トルク
};

/// @brief スライダーコンストレイント設定（直線移動）
struct SliderConstraintSettings {
    Vector3 point1;                ///< ボディA上のピボット点
    Vector3 sliderAxis = {1,0,0};  ///< スライド軸
    Vector3 point2;                ///< ボディB上のピボット点
    float limitsMin = -1e30f;      ///< 最小移動量
    float limitsMax = 1e30f;       ///< 最大移動量
    bool autoDetect = true;        ///< 現在の位置から自動計算するか
};

/// @brief ポイントコンストレイント設定（ボールジョイント）
struct PointConstraintSettings {
    Vector3 point1;                ///< ボディA上のピボット点
    Vector3 point2;                ///< ボディB上のピボット点
};

/// @brief 距離コンストレイント設定（2点間距離制約）
struct DistanceConstraintSettings {
    Vector3 point1;                ///< ボディA上のアタッチ点
    Vector3 point2;                ///< ボディB上のアタッチ点
    float minDistance = -1.0f;     ///< 最小距離（-1=自動計算）
    float maxDistance = -1.0f;     ///< 最大距離（-1=自動計算）
    float springFrequency = 0.0f;  ///< バネ周波数（0=無限に硬い）
    float springDamping = 0.0f;    ///< バネ減衰
};

/// @brief コーンコンストレイント設定（円錐制約）
struct ConeConstraintSettings {
    Vector3 point1;                ///< ボディA上のピボット点
    Vector3 twistAxis1 = {0,1,0}; ///< ボディAのツイスト軸
    Vector3 point2;                ///< ボディB上のピボット点
    Vector3 twistAxis2 = {0,1,0}; ///< ボディBのツイスト軸
    float halfConeAngle = 0.785f;  ///< 半コーン角度（ラジアン）
};

} // namespace gx
/// @}
