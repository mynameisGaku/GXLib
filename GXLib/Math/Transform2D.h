#pragma once
// 2D のアフィン変換（行列）ユーティリティ

#include "pch.h"

namespace GX
{

/// @brief 2Dアフィン変換（平行移動・回転・拡大縮小を1つの行列で表現する）
///
/// DxLibのMatrix相当だが2D専用。GUI部品やスプライトの座標変換に使う。
/// 列ベクトル前提: x' = a*x + c*y + tx, y' = b*x + d*y + ty
struct Transform2D
{
    float a = 1.0f, b = 0.0f;   ///< 行列の1列目 (X軸方向)
    float c = 0.0f, d = 1.0f;   ///< 行列の2列目 (Y軸方向)
    float tx = 0.0f, ty = 0.0f; ///< 平行移動成分

    /// @brief 単位変換（変換なし）を返す
    /// @return 単位変換
    static Transform2D Identity() { return {}; }

    /// @brief 平行移動変換を作成する
    /// @param x X方向の移動量
    /// @param y Y方向の移動量
    /// @return 平行移動変換
    static Transform2D Translation(float x, float y)
    {
        Transform2D t;
        t.tx = x;
        t.ty = y;
        return t;
    }

    /// @brief 拡大縮小変換を作成する
    /// @param sx X方向のスケール
    /// @param sy Y方向のスケール
    /// @return 拡大縮小変換
    static Transform2D Scale(float sx, float sy)
    {
        Transform2D t;
        t.a = sx;
        t.d = sy;
        return t;
    }

    /// @brief 回転変換を作成する
    /// @param radians 回転角度（ラジアン）
    /// @return 回転変換
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

/// @brief 2つの変換を合成する（lhs * rhs の順で適用）
/// @param lhs 後から適用される変換
/// @param rhs 先に適用される変換
/// @return 合成された変換
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

/// @brief 変換を適用して点を変換する
/// @param t 適用する変換
/// @param x 変換前のX座標
/// @param y 変換前のY座標
/// @return 変換後の座標
inline XMFLOAT2 TransformPoint(const Transform2D& t, float x, float y)
{
    return { x * t.a + y * t.c + t.tx, x * t.b + y * t.d + t.ty };
}

/// @brief 逆変換を求める（元の座標に戻す変換）
/// @param t 逆変換を求める元の変換
/// @return 逆変換（行列式がほぼ0の場合は単位変換を返す）
inline Transform2D Inverse(const Transform2D& t)
{
    float det = t.a * t.d - t.b * t.c;
    if (fabsf(det) <= 1.0e-6f)
        return Transform2D::Identity();

    float invDet = 1.0f / det;
    Transform2D o;
    o.a =  t.d * invDet;
    o.b = -t.b * invDet;
    o.c = -t.c * invDet;
    o.d =  t.a * invDet;
    o.tx = -(o.a * t.tx + o.c * t.ty);
    o.ty = -(o.b * t.tx + o.d * t.ty);
    return o;
}

} // namespace GX
