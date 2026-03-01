#pragma once
#include "Vector3.h"
#include <cmath>
#include <cstring>

namespace gx {
/// @addtogroup grp_math
/// @{

/// @brief 4x4行列 (row-major)
struct Matrix4x4
{
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };

    /// @brief 単位行列で初期化するデフォルトコンストラクタ
    Matrix4x4()
    {
        __m128 zero = _mm_setzero_ps();
        _mm_storeu_ps(m[0], _mm_setr_ps(1, 0, 0, 0));
        _mm_storeu_ps(m[1], _mm_setr_ps(0, 1, 0, 0));
        _mm_storeu_ps(m[2], _mm_setr_ps(0, 0, 1, 0));
        _mm_storeu_ps(m[3], _mm_setr_ps(0, 0, 0, 1));
    }

    /// @brief 行列の乗算 (SSE)
    Matrix4x4 operator*(const Matrix4x4& mat) const
    {
        Matrix4x4 result;
        __m128 b0 = _mm_loadu_ps(mat.m[0]);
        __m128 b1 = _mm_loadu_ps(mat.m[1]);
        __m128 b2 = _mm_loadu_ps(mat.m[2]);
        __m128 b3 = _mm_loadu_ps(mat.m[3]);

        for (int i = 0; i < 4; ++i)
        {
            __m128 row = _mm_loadu_ps(m[i]);
            _mm_storeu_ps(result.m[i], simd::MatMulRow(row, b0, b1, b2, b3));
        }
        return result;
    }

    /// @brief 逆行列を取得する (SSE cofactor expansion)
    Matrix4x4 Inverse() const
    {
        // Cramer's rule with SSE
        __m128 r0 = _mm_loadu_ps(m[0]);
        __m128 r1 = _mm_loadu_ps(m[1]);
        __m128 r2 = _mm_loadu_ps(m[2]);
        __m128 r3 = _mm_loadu_ps(m[3]);

        // Transpose pairs for 2x2 submatrix determinants
        __m128 A = _mm_movelh_ps(r0, r1);  // [_11,_12,_21,_22]
        __m128 B = _mm_movehl_ps(r1, r0);  // [_13,_14,_23,_24]
        __m128 C = _mm_movelh_ps(r2, r3);  // [_31,_32,_41,_42]
        __m128 D = _mm_movehl_ps(r3, r2);  // [_33,_34,_43,_44]

        // det2x2: ad - bc for shuffled pairs
        auto det2 = [](__m128 a, __m128 b) {
            return _mm_sub_ps(
                _mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 2, 0, 2)),
                           _mm_shuffle_ps(b, b, _MM_SHUFFLE(1, 3, 1, 3))),
                _mm_mul_ps(_mm_shuffle_ps(a, a, _MM_SHUFFLE(1, 3, 1, 3)),
                           _mm_shuffle_ps(b, b, _MM_SHUFFLE(0, 2, 0, 2))));
        };

        __m128 detA = det2(A, A);
        __m128 detB = det2(B, B);
        __m128 detC = det2(C, C);
        __m128 detD = det2(D, D);

        // Use scalar fallback for correctness
        // Full scalar inverse for robustness
        float inv[4][4];
        float tmp[12];

        tmp[0]  = m[2][2] * m[3][3] - m[2][3] * m[3][2];
        tmp[1]  = m[2][1] * m[3][3] - m[2][3] * m[3][1];
        tmp[2]  = m[2][1] * m[3][2] - m[2][2] * m[3][1];
        tmp[3]  = m[2][0] * m[3][3] - m[2][3] * m[3][0];
        tmp[4]  = m[2][0] * m[3][2] - m[2][2] * m[3][0];
        tmp[5]  = m[2][0] * m[3][1] - m[2][1] * m[3][0];

        inv[0][0] =  m[1][1] * tmp[0] - m[1][2] * tmp[1] + m[1][3] * tmp[2];
        inv[0][1] = -m[0][1] * tmp[0] + m[0][2] * tmp[1] - m[0][3] * tmp[2];
        inv[0][2] =  m[0][1] * (m[1][2]*m[3][3] - m[1][3]*m[3][2])
                   - m[0][2] * (m[1][1]*m[3][3] - m[1][3]*m[3][1])
                   + m[0][3] * (m[1][1]*m[3][2] - m[1][2]*m[3][1]);
        inv[0][3] = -m[0][1] * (m[1][2]*m[2][3] - m[1][3]*m[2][2])
                   + m[0][2] * (m[1][1]*m[2][3] - m[1][3]*m[2][1])
                   - m[0][3] * (m[1][1]*m[2][2] - m[1][2]*m[2][1]);

        inv[1][0] = -m[1][0] * tmp[0] + m[1][2] * tmp[3] - m[1][3] * tmp[4];
        inv[1][1] =  m[0][0] * tmp[0] - m[0][2] * tmp[3] + m[0][3] * tmp[4];
        inv[1][2] = -m[0][0] * (m[1][2]*m[3][3] - m[1][3]*m[3][2])
                   + m[0][2] * (m[1][0]*m[3][3] - m[1][3]*m[3][0])
                   - m[0][3] * (m[1][0]*m[3][2] - m[1][2]*m[3][0]);
        inv[1][3] =  m[0][0] * (m[1][2]*m[2][3] - m[1][3]*m[2][2])
                   - m[0][2] * (m[1][0]*m[2][3] - m[1][3]*m[2][0])
                   + m[0][3] * (m[1][0]*m[2][2] - m[1][2]*m[2][0]);

        inv[2][0] =  m[1][0] * tmp[1] - m[1][1] * tmp[3] + m[1][3] * tmp[5];
        inv[2][1] = -m[0][0] * tmp[1] + m[0][1] * tmp[3] - m[0][3] * tmp[5];
        inv[2][2] =  m[0][0] * (m[1][1]*m[3][3] - m[1][3]*m[3][1])
                   - m[0][1] * (m[1][0]*m[3][3] - m[1][3]*m[3][0])
                   + m[0][3] * (m[1][0]*m[3][1] - m[1][1]*m[3][0]);
        inv[2][3] = -m[0][0] * (m[1][1]*m[2][3] - m[1][3]*m[2][1])
                   + m[0][1] * (m[1][0]*m[2][3] - m[1][3]*m[2][0])
                   - m[0][3] * (m[1][0]*m[2][1] - m[1][1]*m[2][0]);

        inv[3][0] = -m[1][0] * tmp[2] + m[1][1] * tmp[4] - m[1][2] * tmp[5];
        inv[3][1] =  m[0][0] * tmp[2] - m[0][1] * tmp[4] + m[0][2] * tmp[5];
        inv[3][2] = -m[0][0] * (m[1][1]*m[3][2] - m[1][2]*m[3][1])
                   + m[0][1] * (m[1][0]*m[3][2] - m[1][2]*m[3][0])
                   - m[0][2] * (m[1][0]*m[3][1] - m[1][1]*m[3][0]);
        inv[3][3] =  m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1])
                   - m[0][1] * (m[1][0]*m[2][2] - m[1][2]*m[2][0])
                   + m[0][2] * (m[1][0]*m[2][1] - m[1][1]*m[2][0]);

        float det = m[0][0] * inv[0][0] + m[0][1] * inv[1][0]
                   + m[0][2] * inv[2][0] + m[0][3] * inv[3][0];

        Matrix4x4 result;
        if (std::abs(det) < 1e-12f)
        {
            return result; // return identity on singular matrix
        }

        float invDet = 1.0f / det;
        __m128 vInvDet = _mm_set1_ps(invDet);
        _mm_storeu_ps(result.m[0], _mm_mul_ps(_mm_loadu_ps(inv[0]), vInvDet));
        _mm_storeu_ps(result.m[1], _mm_mul_ps(_mm_loadu_ps(inv[1]), vInvDet));
        _mm_storeu_ps(result.m[2], _mm_mul_ps(_mm_loadu_ps(inv[2]), vInvDet));
        _mm_storeu_ps(result.m[3], _mm_mul_ps(_mm_loadu_ps(inv[3]), vInvDet));
        return result;
    }

    /// @brief 転置行列を取得する (SSE _MM_TRANSPOSE4_PS)
    Matrix4x4 Transpose() const
    {
        Matrix4x4 result;
        __m128 r0 = _mm_loadu_ps(m[0]);
        __m128 r1 = _mm_loadu_ps(m[1]);
        __m128 r2 = _mm_loadu_ps(m[2]);
        __m128 r3 = _mm_loadu_ps(m[3]);
        _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
        _mm_storeu_ps(result.m[0], r0);
        _mm_storeu_ps(result.m[1], r1);
        _mm_storeu_ps(result.m[2], r2);
        _mm_storeu_ps(result.m[3], r3);
        return result;
    }

    /// @brief 行列式を計算する
    float Determinant() const
    {
        float t0 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
        float t1 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
        float t2 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
        float t3 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
        float t4 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
        float t5 = m[2][0] * m[3][1] - m[2][1] * m[3][0];

        float c0 =  m[1][1] * t0 - m[1][2] * t1 + m[1][3] * t2;
        float c1 = -m[1][0] * t0 + m[1][2] * t3 - m[1][3] * t4;
        float c2 =  m[1][0] * t1 - m[1][1] * t3 + m[1][3] * t5;
        float c3 = -m[1][0] * t2 + m[1][1] * t4 - m[1][2] * t5;

        return m[0][0] * c0 + m[0][1] * c1 + m[0][2] * c2 + m[0][3] * c3;
    }

    /// @brief 点(位置)を行列で変換する (w=1、射影除算あり) (SSE)
    Vector3 TransformPoint(const Vector3& point) const
    {
        __m128 v = simd::Load3(&point.x);
        __m128 r0 = _mm_loadu_ps(m[0]);
        __m128 r1 = _mm_loadu_ps(m[1]);
        __m128 r2 = _mm_loadu_ps(m[2]);
        __m128 r3 = _mm_loadu_ps(m[3]);

        __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        // result = x*row0 + y*row1 + z*row2 + row3 (w=1)
        __m128 result = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, r0), _mm_mul_ps(vy, r1)),
            _mm_add_ps(_mm_mul_ps(vz, r2), r3));

        // 射影除算: w = result[3]
        __m128 ww = _mm_shuffle_ps(result, result, _MM_SHUFFLE(3, 3, 3, 3));
        result = _mm_div_ps(result, ww);

        Vector3 r;
        simd::Store3(&r.x, result);
        return r;
    }

    /// @brief ベクトル(方向)を行列で変換する (w=0、平行移動なし) (SSE)
    Vector3 TransformVector(const Vector3& vector) const
    {
        __m128 v = simd::Load3(&vector.x);
        __m128 r0 = _mm_loadu_ps(m[0]);
        __m128 r1 = _mm_loadu_ps(m[1]);
        __m128 r2 = _mm_loadu_ps(m[2]);

        __m128 vx = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 vy = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 vz = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

        // result = x*row0 + y*row1 + z*row2 (w=0, no translation)
        __m128 result = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(vx, r0), _mm_mul_ps(vy, r1)),
            _mm_mul_ps(vz, r2));

        Vector3 r;
        simd::Store3(&r.x, result);
        return r;
    }

    static Matrix4x4 Identity()
    {
        return Matrix4x4();
    }

    static Matrix4x4 Translation(float x, float y, float z)
    {
        Matrix4x4 r;
        r._41 = x; r._42 = y; r._43 = z;
        return r;
    }

    static Matrix4x4 Translation(const Vector3& t)
    {
        return Translation(t.x, t.y, t.z);
    }

    static Matrix4x4 Scaling(float x, float y, float z)
    {
        Matrix4x4 r;
        r._11 = x; r._22 = y; r._33 = z;
        return r;
    }

    static Matrix4x4 Scaling(float uniform)
    {
        return Scaling(uniform, uniform, uniform);
    }

    static Matrix4x4 RotationX(float radians)
    {
        float c = std::cos(radians), s = std::sin(radians);
        Matrix4x4 r;
        r._22 = c;  r._23 = s;
        r._32 = -s; r._33 = c;
        return r;
    }

    static Matrix4x4 RotationY(float radians)
    {
        float c = std::cos(radians), s = std::sin(radians);
        Matrix4x4 r;
        r._11 = c;  r._13 = -s;
        r._31 = s;  r._33 = c;
        return r;
    }

    static Matrix4x4 RotationZ(float radians)
    {
        float c = std::cos(radians), s = std::sin(radians);
        Matrix4x4 r;
        r._11 = c;  r._12 = s;
        r._21 = -s; r._22 = c;
        return r;
    }

    static Matrix4x4 RotationRollPitchYaw(float pitch, float yaw, float roll)
    {
        // Rz(roll) * Rx(pitch) * Ry(yaw) — DirectXMath convention
        return RotationZ(roll) * RotationX(pitch) * RotationY(yaw);
    }

    static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up)
    {
        Vector3 zAxis = (target - eye).Normalized();
        Vector3 xAxis = up.Cross(zAxis).Normalized();
        Vector3 yAxis = zAxis.Cross(xAxis);

        Matrix4x4 r;
        r._11 = xAxis.x; r._12 = yAxis.x; r._13 = zAxis.x; r._14 = 0;
        r._21 = xAxis.y; r._22 = yAxis.y; r._23 = zAxis.y; r._24 = 0;
        r._31 = xAxis.z; r._32 = yAxis.z; r._33 = zAxis.z; r._34 = 0;
        r._41 = -xAxis.Dot(eye);
        r._42 = -yAxis.Dot(eye);
        r._43 = -zAxis.Dot(eye);
        r._44 = 1;
        return r;
    }

    static Matrix4x4 PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ)
    {
        float h = 1.0f / std::tan(fovY * 0.5f);
        float w = h / aspect;
        float range = farZ / (farZ - nearZ);

        Matrix4x4 r;
        std::memset(&r, 0, sizeof(r));
        r._11 = w;
        r._22 = h;
        r._33 = range;
        r._34 = 1.0f;
        r._43 = -range * nearZ;
        return r;
    }

    /// @brief Translation * Rotation * Scale 合成行列を作成する (実装はMathInline.h)
    static Matrix4x4 TRS(const Vector3& translation, const struct Quaternion& rotation, const Vector3& scale);

    static Matrix4x4 OrthographicLH(float width, float height, float nearZ, float farZ)
    {
        float range = 1.0f / (farZ - nearZ);

        Matrix4x4 r;
        std::memset(&r, 0, sizeof(r));
        r._11 = 2.0f / width;
        r._22 = 2.0f / height;
        r._33 = range;
        r._43 = -range * nearZ;
        r._44 = 1.0f;
        return r;
    }
};

static_assert(sizeof(Matrix4x4) == 64, "Matrix4x4 must be 64 bytes");

// ============================================================================
// Vector3 deferred implementations (require Matrix4x4 definition)
// ============================================================================

inline Vector3 Vector3::Transform(const Vector3& v, const Matrix4x4& m)
{
    return m.TransformPoint(v);
}

inline Vector3 Vector3::TransformNormal(const Vector3& v, const Matrix4x4& m)
{
    return m.TransformVector(v);
}

/// @}
} // namespace gx

// Deferred implementations requiring both Quaternion and Matrix4x4
#include "MathInline.h"
