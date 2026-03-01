#pragma once
#include "Vector3.h"

namespace gx {
/// @addtogroup grp_math
/// @{

/// @brief 4D浮動小数点ベクトル
struct Vector4
{
    float x = 0, y = 0, z = 0, w = 0;

    /// @brief ゼロベクトルで初期化するデフォルトコンストラクタ
    Vector4() = default;

    /// @brief 指定した成分で初期化するコンストラクタ
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    /// @brief Vector3とW成分から初期化するコンストラクタ
    Vector4(const Vector3& v, float w) : x(v.x), y(v.y), z(v.z), w(w) {}

    /// @brief ベクトル加算 (SSE)
    Vector4 operator+(const Vector4& v) const
    {
        __m128 a = simd::Load4(&x);
        __m128 b = simd::Load4(&v.x);
        Vector4 result;
        simd::Store4(&result.x, _mm_add_ps(a, b));
        return result;
    }

    /// @brief ベクトル減算 (SSE)
    Vector4 operator-(const Vector4& v) const
    {
        __m128 a = simd::Load4(&x);
        __m128 b = simd::Load4(&v.x);
        Vector4 result;
        simd::Store4(&result.x, _mm_sub_ps(a, b));
        return result;
    }

    /// @brief スカラー乗算 (SSE)
    Vector4 operator*(float s) const
    {
        __m128 a = simd::Load4(&x);
        Vector4 result;
        simd::Store4(&result.x, _mm_mul_ps(a, _mm_set1_ps(s)));
        return result;
    }

    /// @brief スカラー除算 (SSE)
    Vector4 operator/(float s) const
    {
        __m128 a = simd::Load4(&x);
        Vector4 result;
        simd::Store4(&result.x, _mm_mul_ps(a, _mm_set1_ps(1.0f / s)));
        return result;
    }

    /// @brief 等値比較
    bool operator==(const Vector4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }

    /// @brief 非等値比較
    bool operator!=(const Vector4& v) const { return !(*this == v); }

    /// @brief 符号反転(単項マイナス)
    Vector4 operator-() const { return { -x, -y, -z, -w }; }

    /// @brief ベクトルの長さ(ノルム)を取得する (SSE)
    float Length() const
    {
        __m128 v = simd::Load4(&x);
        __m128 dot = simd::Dot4(v, v);
        return _mm_cvtss_f32(_mm_sqrt_ss(dot));
    }

    /// @brief ベクトルの長さの2乗を取得する (SSE)
    float LengthSquared() const
    {
        __m128 v = simd::Load4(&x);
        return _mm_cvtss_f32(simd::Dot4(v, v));
    }

    /// @brief 正規化されたベクトルを返す(自身は変更しない) (SSE)
    Vector4 Normalized() const
    {
        __m128 v = simd::Load4(&x);
        Vector4 result;
        simd::Store4(&result.x, simd::Normalize4(v));
        return result;
    }

    /// @brief 自身を正規化する(長さを1にする) (SSE)
    void Normalize()
    {
        __m128 v = simd::Load4(&x);
        simd::Store4(&x, simd::Normalize4(v));
    }

    /// @brief 内積(ドット積)を計算する (SSE)
    float Dot(const Vector4& v) const
    {
        __m128 a = simd::Load4(&x);
        __m128 b = simd::Load4(&v.x);
        return _mm_cvtss_f32(simd::Dot4(a, b));
    }

    static Vector4 Zero() { return { 0, 0, 0, 0 }; }
    static Vector4 One()  { return { 1, 1, 1, 1 }; }

    static Vector4 Lerp(const Vector4& a, const Vector4& b, float t)
    {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        };
    }

    static Vector4 Min(const Vector4& a, const Vector4& b)
    {
        __m128 va = simd::Load4(&a.x);
        __m128 vb = simd::Load4(&b.x);
        Vector4 result;
        simd::Store4(&result.x, _mm_min_ps(va, vb));
        return result;
    }

    static Vector4 Max(const Vector4& a, const Vector4& b)
    {
        __m128 va = simd::Load4(&a.x);
        __m128 vb = simd::Load4(&b.x);
        Vector4 result;
        simd::Store4(&result.x, _mm_max_ps(va, vb));
        return result;
    }
};

static_assert(sizeof(Vector4) == 16, "Vector4 must be 16 bytes");

inline Vector4 operator*(float s, const Vector4& v) { return v * s; }

/// @}
} // namespace gx
