#pragma once
/// @file MathBatch.h
/// @brief 点・ベクトル配列の一括行列変換（SSE最適化）
///
/// 頂点バッファやパーティクルなど大量の座標を一度に
/// 行列変換するときに使う。通常のループより高速。
/// @addtogroup grp_math/// @{

#include "Vector3.h"
#include "Matrix4x4.h"

namespace gx::batch {

/// @brief 点の配列を行列で変換する (w=1、射影除算あり)
/// @param dst 出力先配列
/// @param src 入力配列
/// @param m 変換行列
/// @param count 要素数
inline void TransformPoints(Vector3* dst, const Vector3* src, const Matrix4x4& m, size_t count)
{
    __m128 r0 = _mm_loadu_ps(m.m[0]);
    __m128 r1 = _mm_loadu_ps(m.m[1]);
    __m128 r2 = _mm_loadu_ps(m.m[2]);
    __m128 r3 = _mm_loadu_ps(m.m[3]);

    for (size_t i = 0; i < count; ++i)
    {
        __m128 v = simd::Load3(&src[i].x);
        __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        __m128 result = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, r0), _mm_mul_ps(vy, r1)),
            _mm_add_ps(_mm_mul_ps(vz, r2), r3));

        __m128 ww = _mm_shuffle_ps(result, result, _MM_SHUFFLE(3, 3, 3, 3));
        result = _mm_div_ps(result, ww);

        simd::Store3(&dst[i].x, result);
    }
}

/// @brief 方向ベクトルの配列を行列で変換する (w=0、平行移動なし)
/// @param dst 出力先配列
/// @param src 入力配列
/// @param m 変換行列
/// @param count 要素数
inline void TransformVectors(Vector3* dst, const Vector3* src, const Matrix4x4& m, size_t count)
{
    __m128 r0 = _mm_loadu_ps(m.m[0]);
    __m128 r1 = _mm_loadu_ps(m.m[1]);
    __m128 r2 = _mm_loadu_ps(m.m[2]);

    for (size_t i = 0; i < count; ++i)
    {
        __m128 v = simd::Load3(&src[i].x);
        __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        __m128 result = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, r0), _mm_mul_ps(vy, r1)),
            _mm_mul_ps(vz, r2));

        simd::Store3(&dst[i].x, result);
    }
}

/// @brief Vector3配列を一括正規化する
/// @param dst 出力先配列
/// @param src 入力配列
/// @param count 要素数
inline void NormalizeArray(Vector3* dst, const Vector3* src, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        __m128 v = simd::Load3(&src[i].x);
        simd::Store3(&dst[i].x, simd::Normalize3(v));
    }
}

/// @brief Vector3配列を一括線形補間する
/// @param dst 出力先配列
/// @param a 開始配列
/// @param b 終了配列
/// @param t 補間係数
/// @param count 要素数
inline void LerpArray(Vector3* dst, const Vector3* a, const Vector3* b, float t, size_t count)
{
    __m128 vt = _mm_set1_ps(t);
    for (size_t i = 0; i < count; ++i)
    {
        __m128 va = simd::Load3(&a[i].x);
        __m128 vb = simd::Load3(&b[i].x);
        __m128 r  = _mm_add_ps(va, _mm_mul_ps(_mm_sub_ps(vb, va), vt));
        simd::Store3(&dst[i].x, r);
    }
}

/// @brief Vector3配列のドット積を一括計算する
/// @param dst 出力先float配列
/// @param a 入力配列1
/// @param b 入力配列2
/// @param count 要素数
inline void DotArray(float* dst, const Vector3* a, const Vector3* b, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        __m128 va = simd::Load3(&a[i].x);
        __m128 vb = simd::Load3(&b[i].x);
        dst[i] = _mm_cvtss_f32(simd::Dot3(va, vb));
    }
}

} // namespace gx::batch
/// @}
