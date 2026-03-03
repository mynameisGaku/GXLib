/// @file Compat_Math.cpp
/// @brief 簡易API 数学ユーティリティ関数の実装（VECTOR/MATRIX演算）
#include "pch.h"
#include "Compat/GXLib.h"
#include "Compat/CompatTypes.h"

namespace gx {

// ============================================================================
// VECTOR 演算
// ============================================================================
VECTOR VGet(float x, float y, float z)
{
    return Vector3{ x, y, z };
}

VECTOR VAdd(VECTOR a, VECTOR b)
{
    return a + b;
}

VECTOR VSub(VECTOR a, VECTOR b)
{
    return a - b;
}

VECTOR VScale(VECTOR v, float scale)
{
    return v * scale;
}

float VDot(VECTOR a, VECTOR b)
{
    return a.Dot(b);
}

VECTOR VCross(VECTOR a, VECTOR b)
{
    return a.Cross(b);
}

VECTOR VNorm(VECTOR v)
{
    float len = v.Length();
    if (len < 1e-8f) return Vector3{ 0.0f, 0.0f, 0.0f };
    return v.Normalized();
}

float VSize(VECTOR v)
{
    return v.Length();
}

// ============================================================================
// MATRIX 演算
// ============================================================================
MATRIX MGetIdent()
{
    return Matrix4x4{};  // デフォルトコンストラクタが単位行列
}

MATRIX MMult(MATRIX a, MATRIX b)
{
    return a * b;
}

MATRIX MGetRotX(float angle)
{
    return Matrix4x4::RotationX(angle);
}

MATRIX MGetRotY(float angle)
{
    return Matrix4x4::RotationY(angle);
}

MATRIX MGetRotZ(float angle)
{
    return Matrix4x4::RotationZ(angle);
}

MATRIX MGetTranslate(VECTOR v)
{
    return Matrix4x4::Translation(v);
}

} // namespace gx
