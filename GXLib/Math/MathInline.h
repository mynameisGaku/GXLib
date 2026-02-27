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

} // namespace gx
