#pragma once
/// @file PhysicsShape.h
/// @brief 3D物理シェイプのハンドル構造体 (Jolt Physics内部シェイプへのラッパー)

namespace GX {

/// @brief 3D物理シェイプハンドル
///
/// Jolt Physics のシェイプ参照を内部に保持するラッパー構造体。
/// PhysicsWorld3D::CreateXxxShape() で作成し、AddBody() で使用する。
/// 使用後は DestroyShape() で解放すること。
struct PhysicsShape {
    void* internal = nullptr; ///< 内部ポインタ (JPH::ShapeRefC*)
};

} // namespace GX
