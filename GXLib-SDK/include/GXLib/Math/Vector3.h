#pragma once
#include "MathUtil.h"
#include "MathSIMD.h"

namespace gx {

struct Matrix4x4;

/// @brief 3D浮動小数点ベクトル
struct Vector3
{
    float x = 0, y = 0, z = 0;

    /// @brief ゼロベクトルで初期化するデフォルトコンストラクタ
    Vector3() = default;

    /// @brief 指定した成分で初期化するコンストラクタ
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    /// @brief ベクトル加算（スカラー — 12バイトLoad/Storeのオーバーヘッドを避ける）
    Vector3 operator+(const Vector3& v) const { return { x + v.x, y + v.y, z + v.z }; }

    /// @brief ベクトル減算
    Vector3 operator-(const Vector3& v) const { return { x - v.x, y - v.y, z - v.z }; }

    /// @brief スカラー乗算
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }

    /// @brief スカラー除算
    Vector3 operator/(float s) const { float inv = 1.0f / s; return { x * inv, y * inv, z * inv }; }

    /// @brief ベクトル加算代入
    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }

    /// @brief ベクトル減算代入
    Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }

    /// @brief スカラー乗算代入
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    /// @brief 等値比較
    bool operator==(const Vector3& v) const { return x == v.x && y == v.y && z == v.z; }

    /// @brief 非等値比較
    bool operator!=(const Vector3& v) const { return !(*this == v); }

    /// @brief 符号反転(単項マイナス)
    Vector3 operator-() const { return { -x, -y, -z }; }

    /// @brief ベクトルの長さ(ノルム)を取得する (SSE)
    float Length() const
    {
        __m128 v = simd::Load3(&x);
        __m128 dot = simd::Dot3(v, v);
        return _mm_cvtss_f32(_mm_sqrt_ss(dot));
    }

    /// @brief ベクトルの長さの2乗を取得する (SSE)
    float LengthSquared() const
    {
        __m128 v = simd::Load3(&x);
        return _mm_cvtss_f32(simd::Dot3(v, v));
    }

    /// @brief 正規化されたベクトルを返す(自身は変更しない) (SSE)
    Vector3 Normalized() const
    {
        __m128 v = simd::Load3(&x);
        __m128 n = simd::Normalize3(v);
        Vector3 result;
        simd::Store3(&result.x, n);
        return result;
    }

    /// @brief 自身を正規化する(長さを1にする) (SSE)
    void Normalize()
    {
        __m128 v = simd::Load3(&x);
        __m128 n = simd::Normalize3(v);
        simd::Store3(&x, n);
    }

    /// @brief 内積(ドット積)を計算する (SSE)
    float Dot(const Vector3& v) const
    {
        __m128 a = simd::Load3(&x);
        __m128 b = simd::Load3(&v.x);
        return _mm_cvtss_f32(simd::Dot3(a, b));
    }

    /// @brief 外積(クロス積)を計算する (SSE)
    Vector3 Cross(const Vector3& v) const
    {
        __m128 a = simd::Load3(&x);
        __m128 b = simd::Load3(&v.x);
        Vector3 result;
        simd::Store3(&result.x, simd::Cross3(a, b));
        return result;
    }

    /// @brief 他のベクトルとの距離を計算する
    float Distance(const Vector3& v) const { return (*this - v).Length(); }

    /// @brief 他のベクトルとの距離の2乗を計算する(sqrt不要で高速)
    float DistanceSquared(const Vector3& v) const { return (*this - v).LengthSquared(); }

    static Vector3 Zero()     { return { 0, 0, 0 }; }
    static Vector3 One()      { return { 1, 1, 1 }; }
    static Vector3 Up()       { return { 0, 1, 0 }; }
    static Vector3 Down()     { return { 0,-1, 0 }; }
    static Vector3 Forward()  { return { 0, 0, 1 }; }
    static Vector3 Backward() { return { 0, 0,-1 }; }
    static Vector3 Left()     { return {-1, 0, 0 }; }
    static Vector3 Right()    { return { 1, 0, 0 }; }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
    }

    static Vector3 Min(const Vector3& a, const Vector3& b)
    {
        return { (std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::min)(a.z, b.z) };
    }

    static Vector3 Max(const Vector3& a, const Vector3& b)
    {
        return { (std::max)(a.x, b.x), (std::max)(a.y, b.y), (std::max)(a.z, b.z) };
    }

    static Vector3 Reflect(const Vector3& direction, const Vector3& normal)
    {
        float d = direction.Dot(normal);
        return direction - normal * (2.0f * d);
    }

    /// @brief 行列でベクトルを座標変換する(w=1、射影除算あり) (SSE)
    static Vector3 Transform(const Vector3& v, const Matrix4x4& m);

    /// @brief 行列でベクトルを法線変換する(w=0、平行移動なし) (SSE)
    static Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);
};

static_assert(sizeof(Vector3) == 12, "Vector3 must be 12 bytes");

inline Vector3 operator*(float s, const Vector3& v) { return v * s; }

} // namespace gx
