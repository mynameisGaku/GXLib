#pragma once
/// @file MathInline.h
/// @brief Deferred inline implementations requiring multiple math types
///
/// このヘッダーは Quaternion::FromRotationMatrix のような
/// 複数の数学型定義に依存するインライン実装を提供する。
/// Matrix4x4.h の末尾からインクルードされる。

#include "Quaternion.h"
#include "Matrix4x4.h"

namespace gx {
/// @addtogroup grp_math
/// @{

inline Quaternion Quaternion::FromRotationMatrix(const Matrix4x4& m)
{
    // Shepperd's method for numerical stability
    float trace = m._11 + m._22 + m._33;
    Quaternion q;

    if (trace > 0.0f)
    {
        float s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4*w
        q.w = 0.25f * s;
        q.x = (m._23 - m._32) / s;
        q.y = (m._31 - m._13) / s;
        q.z = (m._12 - m._21) / s;
    }
    else if (m._11 > m._22 && m._11 > m._33)
    {
        float s = std::sqrt(1.0f + m._11 - m._22 - m._33) * 2.0f;
        q.w = (m._23 - m._32) / s;
        q.x = 0.25f * s;
        q.y = (m._12 + m._21) / s;
        q.z = (m._13 + m._31) / s;
    }
    else if (m._22 > m._33)
    {
        float s = std::sqrt(1.0f + m._22 - m._11 - m._33) * 2.0f;
        q.w = (m._31 - m._13) / s;
        q.x = (m._12 + m._21) / s;
        q.y = 0.25f * s;
        q.z = (m._23 + m._32) / s;
    }
    else
    {
        float s = std::sqrt(1.0f + m._33 - m._11 - m._22) * 2.0f;
        q.w = (m._12 - m._21) / s;
        q.x = (m._13 + m._31) / s;
        q.y = (m._23 + m._32) / s;
        q.z = 0.25f * s;
    }
    return q;
}

/// @brief Translation * Rotation * Scale の合成行列を作成する
inline Matrix4x4 Matrix4x4::TRS(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
{
    // rotation -> 3x3 matrix
    float xx = rotation.x * rotation.x;
    float yy = rotation.y * rotation.y;
    float zz = rotation.z * rotation.z;
    float xy = rotation.x * rotation.y;
    float xz = rotation.x * rotation.z;
    float yz = rotation.y * rotation.z;
    float wx = rotation.w * rotation.x;
    float wy = rotation.w * rotation.y;
    float wz = rotation.w * rotation.z;

    Matrix4x4 m;
    m._11 = (1.0f - 2.0f * (yy + zz)) * scale.x;
    m._12 = (2.0f * (xy + wz))         * scale.x;
    m._13 = (2.0f * (xz - wy))         * scale.x;
    m._14 = 0.0f;

    m._21 = (2.0f * (xy - wz))         * scale.y;
    m._22 = (1.0f - 2.0f * (xx + zz)) * scale.y;
    m._23 = (2.0f * (yz + wx))         * scale.y;
    m._24 = 0.0f;

    m._31 = (2.0f * (xz + wy))         * scale.z;
    m._32 = (2.0f * (yz - wx))         * scale.z;
    m._33 = (1.0f - 2.0f * (xx + yy)) * scale.z;
    m._34 = 0.0f;

    m._41 = translation.x;
    m._42 = translation.y;
    m._43 = translation.z;
    m._44 = 1.0f;
    return m;
}

/// @}
} // namespace gx
