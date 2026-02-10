#pragma once
// 2D のアフィン変換（行列）ユーティリティ

#include "pch.h"

namespace GX
{

/// @brief 2D アフィン変換（列ベクトル前提）
/// x' = a*x + c*y + tx
/// y' = b*x + d*y + ty
struct Transform2D
{
    float a = 1.0f, b = 0.0f;
    float c = 0.0f, d = 1.0f;
    float tx = 0.0f, ty = 0.0f;

    static Transform2D Identity() { return {}; }

    static Transform2D Translation(float x, float y)
    {
        Transform2D t;
        t.tx = x;
        t.ty = y;
        return t;
    }

    static Transform2D Scale(float sx, float sy)
    {
        Transform2D t;
        t.a = sx;
        t.d = sy;
        return t;
    }

    static Transform2D Rotation(float radians)
    {
        Transform2D t;
        float c = cosf(radians);
        float s = sinf(radians);
        t.a = c;
        t.c = -s;
        t.b = s;
        t.d = c;
        return t;
    }
};

/// @brief 変換合成（lhs * rhs）
inline Transform2D Multiply(const Transform2D& lhs, const Transform2D& rhs)
{
    Transform2D o;
    o.a  = lhs.a * rhs.a + lhs.c * rhs.b;
    o.b  = lhs.b * rhs.a + lhs.d * rhs.b;
    o.c  = lhs.a * rhs.c + lhs.c * rhs.d;
    o.d  = lhs.b * rhs.c + lhs.d * rhs.d;
    o.tx = lhs.a * rhs.tx + lhs.c * rhs.ty + lhs.tx;
    o.ty = lhs.b * rhs.tx + lhs.d * rhs.ty + lhs.ty;
    return o;
}

/// @brief 点を変換
inline XMFLOAT2 TransformPoint(const Transform2D& t, float x, float y)
{
    return { x * t.a + y * t.c + t.tx, x * t.b + y * t.d + t.ty };
}

} // namespace GX

