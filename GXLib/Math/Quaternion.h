#pragma once
#include "Vector3.h"
#include <cmath>

namespace gx {
/// @addtogroup grp_math
/// @{

struct Matrix4x4;

/// @brief クォータニオン (回転表現用)
struct Quaternion
{
    float x = 0, y = 0, z = 0, w = 1;

    /// @brief 単位クォータニオン(回転なし)で初期化するデフォルトコンストラクタ
    Quaternion() = default;

    /// @brief 指定した成分で初期化するコンストラクタ
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

    /// @brief クォータニオンの乗算(回転の合成) (SSE)
    Quaternion operator*(const Quaternion& q) const
    {
        __m128 a = simd::Load4(&x);
        __m128 b = simd::Load4(&q.x);
        Quaternion result;
        simd::Store4(&result.x, simd::QuatMul(a, b));
        return result;
    }

    /// @brief クォータニオンの長さ(ノルム)を取得する (SSE)
    float Length() const
    {
        __m128 q = simd::Load4(&x);
        __m128 dot = simd::Dot4(q, q);
        return _mm_cvtss_f32(_mm_sqrt_ss(dot));
    }

    /// @brief 正規化されたクォータニオンを返す(自身は変更しない) (SSE)
    Quaternion Normalized() const
    {
        __m128 q = simd::Load4(&x);
        Quaternion result;
        simd::Store4(&result.x, simd::Normalize4(q));
        return result;
    }

    /// @brief 自身を正規化する(長さを1にする) (SSE)
    void Normalize()
    {
        __m128 q = simd::Load4(&x);
        simd::Store4(&x, simd::Normalize4(q));
    }

    /// @brief 共役クォータニオンを返す (SSE)
    Quaternion Conjugate() const
    {
        __m128 q = simd::Load4(&x);
        Quaternion result;
        simd::Store4(&result.x, simd::QuatConjugate(q));
        return result;
    }

    /// @brief 逆クォータニオンを返す (SSE)
    Quaternion Inverse() const
    {
        __m128 q = simd::Load4(&x);
        __m128 conj = simd::QuatConjugate(q);
        __m128 lenSq = simd::Dot4(q, q);
        Quaternion result;
        simd::Store4(&result.x, _mm_div_ps(conj, lenSq));
        return result;
    }

    /// @brief クォータニオンからオイラー角(ラジアン)に変換する
    /// @return オイラー角 (x=ピッチ, y=ヨー, z=ロール)
    Vector3 ToEuler() const
    {
        float sinP = 2.0f * (w * x - y * z);
        float pitch;
        if (std::abs(sinP) >= 1.0f)
            pitch = std::copysign(MathUtil::PI / 2.0f, sinP);
        else
            pitch = std::asin(sinP);

        float sinYcosP = 2.0f * (w * y + x * z);
        float cosYcosP = 1.0f - 2.0f * (x * x + y * y);
        float yaw = std::atan2(sinYcosP, cosYcosP);

        float sinRcosP = 2.0f * (w * z + x * y);
        float cosRcosP = 1.0f - 2.0f * (x * x + z * z);
        float roll = std::atan2(sinRcosP, cosRcosP);

        return { pitch, yaw, roll };
    }

    /// @brief クォータニオンでベクトルを回転する (SSE)
    Vector3 RotateVector(const Vector3& v) const
    {
        // q * v * q^-1  (optimized: v' = v + 2w(u × v) + 2(u × (u × v)))
        // where u = (x,y,z), w = w
        __m128 qv = simd::Load3(&x);      // u = (qx, qy, qz)
        __m128 vec = simd::Load3(&v.x);    // v

        __m128 uv = simd::Cross3(qv, vec);     // u × v
        __m128 uuv = simd::Cross3(qv, uv);     // u × (u × v)

        __m128 ww = _mm_set1_ps(w);
        __m128 two = _mm_set1_ps(2.0f);

        // result = v + 2w(u×v) + 2(u×(u×v))
        __m128 result = _mm_add_ps(vec,
            _mm_add_ps(
                _mm_mul_ps(two, _mm_mul_ps(ww, uv)),
                _mm_mul_ps(two, uuv)));

        Vector3 r;
        simd::Store3(&r.x, result);
        return r;
    }

    /// @brief 単位クォータニオン(回転なし)を返す
    static Quaternion Identity() { return { 0, 0, 0, 1 }; }

    /// @brief 任意軸回りの回転クォータニオンを作成する
    static Quaternion FromAxisAngle(const Vector3& axis, float radians)
    {
        float half = radians * 0.5f;
        float s = std::sin(half);
        float c = std::cos(half);
        // 軸を正規化
        __m128 a = simd::Load3(&axis.x);
        a = simd::Normalize3(a);
        Vector3 n;
        simd::Store3(&n.x, a);
        return { n.x * s, n.y * s, n.z * s, c };
    }

    /// @brief オイラー角から回転クォータニオンを作成する (Euler はエイリアス)
    static Quaternion Euler(float pitch, float yaw, float roll)
    {
        return FromEuler(pitch, yaw, roll);
    }

    /// @brief オイラー角から回転クォータニオンを作成する
    static Quaternion FromEuler(float pitch, float yaw, float roll)
    {
        // Rz(roll) * Rx(pitch) * Ry(yaw) の行ベクトル規約
        float hp = pitch * 0.5f, hy = yaw * 0.5f, hr = roll * 0.5f;
        float sp = std::sin(hp), cp = std::cos(hp);
        float sy = std::sin(hy), cy = std::cos(hy);
        float sr = std::sin(hr), cr = std::cos(hr);

        return {
            cr * sp * cy + sr * cp * sy,   // x
            cr * cp * sy - sr * sp * cy,   // y
            sr * cp * cy - cr * sp * sy,   // z
            cr * cp * cy + sr * sp * sy    // w
        };
    }

    /// @brief 回転行列からクォータニオンを作成する
    static Quaternion FromRotationMatrix(const Matrix4x4& m);

    /// @brief 2つのクォータニオン間を球面線形補間(Slerp)する
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
    {
        __m128 qa = simd::Load4(&a.x);
        __m128 qb = simd::Load4(&b.x);

        // ドット積
        float dot = _mm_cvtss_f32(simd::Dot4(qa, qb));

        // 負の場合は最短経路を取る
        if (dot < 0.0f)
        {
            qb = _mm_sub_ps(_mm_setzero_ps(), qb);  // negate
            dot = -dot;
        }

        // ほぼ平行な場合はNLerpにフォールバック
        if (dot > 0.9995f)
        {
            __m128 result = _mm_add_ps(qa,
                _mm_mul_ps(_mm_set1_ps(t), _mm_sub_ps(qb, qa)));
            result = simd::Normalize4(result);
            Quaternion r;
            simd::Store4(&r.x, result);
            return r;
        }

        float theta = std::acos(dot);
        float sinTheta = std::sin(theta);
        float wa = std::sin((1.0f - t) * theta) / sinTheta;
        float wb = std::sin(t * theta) / sinTheta;

        __m128 result = _mm_add_ps(
            _mm_mul_ps(_mm_set1_ps(wa), qa),
            _mm_mul_ps(_mm_set1_ps(wb), qb));

        Quaternion r;
        simd::Store4(&r.x, result);
        return r;
    }

    /// @brief 前方ベクトルから回転クォータニオンを作成する
    /// @param forward 前方ベクトル (正規化される)
    /// @param up 上方向ヒント (デフォルト: Y軸)
    static Quaternion LookRotation(const Vector3& forward, const Vector3& up = Vector3::Up())
    {
        Vector3 f = forward.Normalized();
        Vector3 r = up.Cross(f).Normalized();
        Vector3 u = f.Cross(r);

        // 回転行列の3x3部分からクォータニオンを構築
        float trace = r.x + u.y + f.z;
        Quaternion q;
        if (trace > 0.0f)
        {
            float s = std::sqrt(trace + 1.0f) * 2.0f;
            q.w = 0.25f * s;
            q.x = (u.z - f.y) / s;
            q.y = (f.x - r.z) / s;
            q.z = (r.y - u.x) / s;
        }
        else if (r.x > u.y && r.x > f.z)
        {
            float s = std::sqrt(1.0f + r.x - u.y - f.z) * 2.0f;
            q.w = (u.z - f.y) / s;
            q.x = 0.25f * s;
            q.y = (r.y + u.x) / s;
            q.z = (r.z + f.x) / s;
        }
        else if (u.y > f.z)
        {
            float s = std::sqrt(1.0f + u.y - r.x - f.z) * 2.0f;
            q.w = (f.x - r.z) / s;
            q.x = (r.y + u.x) / s;
            q.y = 0.25f * s;
            q.z = (u.z + f.y) / s;
        }
        else
        {
            float s = std::sqrt(1.0f + f.z - r.x - u.y) * 2.0f;
            q.w = (r.y - u.x) / s;
            q.x = (r.z + f.x) / s;
            q.y = (u.z + f.y) / s;
            q.z = 0.25f * s;
        }
        return q.Normalized();
    }

    /// @brief あるベクトルから別のベクトルへの回転クォータニオンを作成する
    static Quaternion FromToRotation(const Vector3& from, const Vector3& to)
    {
        Vector3 f = from.Normalized();
        Vector3 t = to.Normalized();
        float dot = f.Dot(t);

        if (dot >= 1.0f - 1e-6f)
        {
            return Identity(); // Same direction
        }
        else if (dot <= -1.0f + 1e-6f)
        {
            // Opposite direction: find an orthogonal axis
            Vector3 axis = Vector3::Right().Cross(f);
            if (axis.LengthSquared() < 1e-6f)
                axis = Vector3::Up().Cross(f);
            axis.Normalize();
            return Quaternion(axis.x, axis.y, axis.z, 0.0f).Normalized();
        }

        Vector3 half = (f + t).Normalized();
        Vector3 cross = f.Cross(half);
        return Quaternion(cross.x, cross.y, cross.z, f.Dot(half)).Normalized();
    }

    /// @brief 2つのクォータニオン間の角度を返す（ラジアン）
    static float Angle(const Quaternion& a, const Quaternion& b)
    {
        float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        // Ensure dot is in [-1, 1] range (absolute value for shortest arc)
        float absDot = std::abs(dot);
        if (absDot > 1.0f) absDot = 1.0f;
        // The angle between two quaternions is 2 * acos(|dot|)
        return 2.0f * std::acos(absDot);
    }

    /// @brief 最大回転角度制限付きで目標クォータニオンに向かって回転する
    static Quaternion RotateTowards(const Quaternion& from, const Quaternion& to, float maxRadiansDelta)
    {
        float angle = Angle(from, to);
        if (angle < 1e-6f) return to;
        float t = (std::min)(1.0f, maxRadiansDelta / angle);
        return Slerp(from, to, t);
    }

    /// @brief 2つのクォータニオン間を正規化線形補間(NLerp)する
    static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t)
    {
        Quaternion result;
        result.x = a.x + (b.x - a.x) * t;
        result.y = a.y + (b.y - a.y) * t;
        result.z = a.z + (b.z - a.z) * t;
        result.w = a.w + (b.w - a.w) * t;
        result.Normalize();
        return result;
    }
};

static_assert(sizeof(Quaternion) == 16, "Quaternion must be 16 bytes");

/// @}
} // namespace gx
