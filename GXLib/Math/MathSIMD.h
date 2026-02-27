#pragma once
/// @file MathSIMD.h
/// @brief SSE intrinsics ベースのSIMD数学ヘルパー
///
/// DirectXMathに依存しない、独自のSIMD演算基盤。
/// SSE4.1を最低ターゲットとする（_mm_dp_ps使用、2008年以降の全CPUでサポート）。

#include <immintrin.h>
#include <cmath>

namespace gx::simd {

// ============================================================================
// Load / Store
// ============================================================================

/// @brief Vector3用ロード: 12バイト(x,y,z) → __m128 (w=0)
inline __m128 Load3(const float* p)
{
    // x,y を64bit一括ロード、z を個別ロード、合成
    __m128 xy = _mm_loadl_pi(_mm_setzero_ps(), reinterpret_cast<const __m64*>(p));
    __m128 z  = _mm_load_ss(p + 2);
    return _mm_movelh_ps(xy, z);  // [x, y, z, 0]
}

/// @brief Vector3用ストア: __m128 → 12バイト(x,y,z)
inline void Store3(float* p, __m128 v)
{
    _mm_storel_pi(reinterpret_cast<__m64*>(p), v);      // x, y
    _mm_store_ss(p + 2, _mm_movehl_ps(v, v));           // z
}

/// @brief Vector4/Quaternion用ロード: 16バイト → __m128
inline __m128 Load4(const float* p) { return _mm_loadu_ps(p); }

/// @brief Vector4/Quaternion用ストア: __m128 → 16バイト
inline void Store4(float* p, __m128 v) { _mm_storeu_ps(p, v); }

// ============================================================================
// Dot Product (SSE4.1 _mm_dp_ps)
// ============================================================================

/// @brief 3要素ドット積 → 結果はx成分に格納
inline __m128 Dot3(__m128 a, __m128 b)
{
    return _mm_dp_ps(a, b, 0x71);  // mask: x,y,z入力 → x出力
}

/// @brief 4要素ドット積 → 全成分に結果を格納
inline __m128 Dot4(__m128 a, __m128 b)
{
    return _mm_dp_ps(a, b, 0xFF);  // mask: x,y,z,w入力 → 全出力
}

// ============================================================================
// Cross Product
// ============================================================================

/// @brief 3要素クロス積: a × b
inline __m128 Cross3(__m128 a, __m128 b)
{
    __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, b));
    return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

// ============================================================================
// Normalize
// ============================================================================

/// @brief 3要素正規化（Newton-Raphson精度改善付き）
inline __m128 Normalize3(__m128 v)
{
    __m128 dot = _mm_dp_ps(v, v, 0x77);  // x,y,z → 全成分出力
    __m128 rsqrt = _mm_rsqrt_ps(dot);
    // Newton-Raphson 1反復で精度向上
    __m128 half  = _mm_set1_ps(0.5f);
    __m128 three = _mm_set1_ps(3.0f);
    rsqrt = _mm_mul_ps(
        _mm_mul_ps(half, rsqrt),
        _mm_sub_ps(three, _mm_mul_ps(dot, _mm_mul_ps(rsqrt, rsqrt))));
    return _mm_mul_ps(v, rsqrt);
}

/// @brief 4要素正規化（Newton-Raphson精度改善付き）
inline __m128 Normalize4(__m128 v)
{
    __m128 dot = _mm_dp_ps(v, v, 0xFF);
    __m128 rsqrt = _mm_rsqrt_ps(dot);
    // Newton-Raphson 1反復
    __m128 half  = _mm_set1_ps(0.5f);
    __m128 three = _mm_set1_ps(3.0f);
    rsqrt = _mm_mul_ps(
        _mm_mul_ps(half, rsqrt),
        _mm_sub_ps(three, _mm_mul_ps(dot, _mm_mul_ps(rsqrt, rsqrt))));
    return _mm_mul_ps(v, rsqrt);
}

// ============================================================================
// Quaternion operations
// ============================================================================

/// @brief クォータニオン乗算: q1 * q2
/// レイアウト: [x, y, z, w]
inline __m128 QuatMul(__m128 q1, __m128 q2)
{
    // q1 = [x1, y1, z1, w1], q2 = [x2, y2, z2, w2]
    // result.x = w1*x2 + x1*w2 + y1*z2 - z1*y2
    // result.y = w1*y2 - x1*z2 + y1*w2 + z1*x2
    // result.z = w1*z2 + x1*y2 - y1*x2 + z1*w2
    // result.w = w1*w2 - x1*x2 - y1*y2 - z1*z2

    __m128 w1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3)); // [w1,w1,w1,w1]
    __m128 x1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 0, 0, 0)); // [x1,x1,x1,x1]
    __m128 y1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 1, 1, 1)); // [y1,y1,y1,y1]
    __m128 z1 = _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 2, 2, 2)); // [z1,z1,z1,z1]

    // w1 * [x2, y2, z2, w2]
    __m128 r0 = _mm_mul_ps(w1, q2);

    // x1 * [w2, -z2, y2, -x2]
    __m128 q2_1 = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 2, 3)); // [w2,z2,y2,x2]
    static const __m128 sign1 = _mm_setr_ps(1.0f, -1.0f, 1.0f, -1.0f);
    __m128 r1 = _mm_mul_ps(x1, _mm_mul_ps(q2_1, sign1));

    // y1 * [z2, w2, -x2, -y2]
    __m128 q2_2 = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(1, 0, 3, 2)); // [z2,w2,x2,y2]
    static const __m128 sign2 = _mm_setr_ps(1.0f, 1.0f, -1.0f, -1.0f);
    __m128 r2 = _mm_mul_ps(y1, _mm_mul_ps(q2_2, sign2));

    // z1 * [-y2, x2, w2, -z2]
    __m128 q2_3 = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 3, 0, 1)); // [y2,x2,w2,z2]
    static const __m128 sign3 = _mm_setr_ps(-1.0f, 1.0f, 1.0f, -1.0f);
    __m128 r3 = _mm_mul_ps(z1, _mm_mul_ps(q2_3, sign3));

    return _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));
}

/// @brief クォータニオン共役: [-x, -y, -z, w]
inline __m128 QuatConjugate(__m128 q)
{
    static const __m128 sign = _mm_setr_ps(-1.0f, -1.0f, -1.0f, 1.0f);
    return _mm_mul_ps(q, sign);
}

// ============================================================================
// Matrix operations
// ============================================================================

/// @brief 行列の1行を変換: row * matrix (row-major)
/// m0,m1,m2,m3 は行列の4行
inline __m128 MatMulRow(__m128 row, __m128 m0, __m128 m1, __m128 m2, __m128 m3)
{
    __m128 x = _mm_shuffle_ps(row, row, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 y = _mm_shuffle_ps(row, row, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 z = _mm_shuffle_ps(row, row, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 w = _mm_shuffle_ps(row, row, _MM_SHUFFLE(3, 3, 3, 3));
    return _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(x, m0), _mm_mul_ps(y, m1)),
        _mm_add_ps(_mm_mul_ps(z, m2), _mm_mul_ps(w, m3)));
}

} // namespace gx::simd
