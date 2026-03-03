#pragma once
#include "MathUtil.h"
#include "MathSIMD.h"

namespace gx {
/// @addtogroup grp_math
/// @{

struct Matrix4x4;

/// @brief 3D浮動小数点ベクトル
struct Vector3
{
    float x = 0, y = 0, z = 0;

    /// @brief ゼロベクトルで初期化するデフォルトコンストラクタ
    Vector3() = default;

    /// @brief 指定した成分で初期化するコンストラクタ
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    /// @brief ベクトル加算
    /// @param v 加算するベクトル
    /// @return 加算結果
    Vector3 operator+(const Vector3& v) const { return { x + v.x, y + v.y, z + v.z }; }

    /// @brief ベクトル減算
    /// @param v 減算するベクトル
    /// @return 減算結果
    Vector3 operator-(const Vector3& v) const { return { x - v.x, y - v.y, z - v.z }; }

    /// @brief スカラー乗算
    /// @param s 乗算するスカラー値
    /// @return 乗算結果
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }

    /// @brief スカラー除算
    /// @param s 除算するスカラー値
    /// @return 除算結果
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
    /// @return ベクトルの長さ
    float Length() const
    {
        __m128 v = simd::Load3(&x);
        __m128 dot = simd::Dot3(v, v);
        return _mm_cvtss_f32(_mm_sqrt_ss(dot));
    }

    /// @brief ベクトルの長さの2乗を取得する (SSE)
    /// @return 長さの2乗
    float LengthSquared() const
    {
        __m128 v = simd::Load3(&x);
        return _mm_cvtss_f32(simd::Dot3(v, v));
    }

    /// @brief 正規化されたベクトルを返す(自身は変更しない) (SSE)
    /// @return 単位ベクトル（長さがほぼ0の場合は不定）
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
    /// @param v 内積を取る相手のベクトル
    /// @return 内積値
    float Dot(const Vector3& v) const
    {
        __m128 a = simd::Load3(&x);
        __m128 b = simd::Load3(&v.x);
        return _mm_cvtss_f32(simd::Dot3(a, b));
    }

    /// @brief 外積(クロス積)を計算する (SSE)
    /// @param v 外積を取る相手のベクトル
    /// @return 外積ベクトル（両ベクトルに垂直）
    Vector3 Cross(const Vector3& v) const
    {
        __m128 a = simd::Load3(&x);
        __m128 b = simd::Load3(&v.x);
        Vector3 result;
        simd::Store3(&result.x, simd::Cross3(a, b));
        return result;
    }

    /// @brief 他のベクトルとの距離を計算する
    /// @param v 対象のベクトル
    /// @return 2点間の距離
    float Distance(const Vector3& v) const { return (*this - v).Length(); }

    /// @brief 他のベクトルとの距離の2乗を計算する(sqrt不要で高速)
    /// @param v 対象のベクトル
    /// @return 距離の2乗
    float DistanceSquared(const Vector3& v) const { return (*this - v).LengthSquared(); }

    /// @brief ゼロベクトル (0, 0, 0) を返す
    /// @return ゼロベクトル
    static Vector3 Zero()     { return { 0, 0, 0 }; }

    /// @brief 全成分1のベクトル (1, 1, 1) を返す
    /// @return (1, 1, 1)
    static Vector3 One()      { return { 1, 1, 1 }; }

    /// @brief Y軸正方向 (0, 1, 0) を返す
    /// @return 上方向ベクトル
    static Vector3 Up()       { return { 0, 1, 0 }; }

    /// @brief Y軸負方向 (0, -1, 0) を返す
    /// @return 下方向ベクトル
    static Vector3 Down()     { return { 0,-1, 0 }; }

    /// @brief Z軸正方向 (0, 0, 1) を返す
    /// @return 前方向ベクトル
    static Vector3 Forward()  { return { 0, 0, 1 }; }

    /// @brief Z軸負方向 (0, 0, -1) を返す
    /// @return 後方向ベクトル
    static Vector3 Backward() { return { 0, 0,-1 }; }

    /// @brief X軸負方向 (-1, 0, 0) を返す
    /// @return 左方向ベクトル
    static Vector3 Left()     { return {-1, 0, 0 }; }

    /// @brief X軸正方向 (1, 0, 0) を返す
    /// @return 右方向ベクトル
    static Vector3 Right()    { return { 1, 0, 0 }; }

    /// @brief 2ベクトル間を線形補間する
    /// @param a 開始ベクトル (t=0)
    /// @param b 終了ベクトル (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
    }

    /// @brief 各成分の最小値を取る
    /// @param a ベクトル1
    /// @param b ベクトル2
    /// @return 各成分が小さい方のベクトル
    static Vector3 Min(const Vector3& a, const Vector3& b)
    {
        return { (std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::min)(a.z, b.z) };
    }

    /// @brief 各成分の最大値を取る
    /// @param a ベクトル1
    /// @param b ベクトル2
    /// @return 各成分が大きい方のベクトル
    static Vector3 Max(const Vector3& a, const Vector3& b)
    {
        return { (std::max)(a.x, b.x), (std::max)(a.y, b.y), (std::max)(a.z, b.z) };
    }

    /// @brief 要素ごとの乗算
    /// @param a ベクトル1
    /// @param b ベクトル2
    /// @return 要素ごとの積
    static Vector3 Scale(const Vector3& a, const Vector3& b)
    {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }

    /// @brief ベクトルを軸方向に射影する
    /// @param v 射影するベクトル
    /// @param onNormal 射影先の軸（正規化不要）
    /// @return 射影されたベクトル
    static Vector3 Project(const Vector3& v, const Vector3& onNormal)
    {
        float d = onNormal.Dot(onNormal);
        if (d < 1e-8f) return Zero();
        return onNormal * (v.Dot(onNormal) / d);
    }

    /// @brief ベクトルを平面上に射影する
    /// @param v 射影するベクトル
    /// @param planeNormal 平面の法線
    /// @return 平面上に射影されたベクトル
    static Vector3 ProjectOnPlane(const Vector3& v, const Vector3& planeNormal)
    {
        return v - Project(v, planeNormal);
    }

    /// @brief 現在位置からターゲットへ最大maxDistanceDelta分だけ移動する
    /// @param current 現在位置
    /// @param target 目標位置
    /// @param maxDistanceDelta 最大移動距離
    /// @return 移動後の位置
    static Vector3 MoveTowards(const Vector3& current, const Vector3& target, float maxDistanceDelta)
    {
        Vector3 diff = target - current;
        float dist = diff.Length();
        if (dist <= maxDistanceDelta || dist < 1e-8f) return target;
        return current + diff * (maxDistanceDelta / dist);
    }

    /// @brief ベクトルの長さを最大値でクランプする
    /// @param v クランプ対象のベクトル
    /// @param maxLength 許容する最大の長さ
    /// @return 長さがmaxLength以下に制限されたベクトル
    static Vector3 ClampMagnitude(const Vector3& v, float maxLength)
    {
        float lenSq = v.LengthSquared();
        if (lenSq > maxLength * maxLength)
        {
            float len = std::sqrtf(lenSq);
            return v * (maxLength / len);
        }
        return v;
    }

    /// @brief 2ベクトル間の球面線形補間
    /// @param a 開始ベクトル (t=0)
    /// @param b 終了ベクトル (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 球面補間されたベクトル
    static Vector3 Slerp(const Vector3& a, const Vector3& b, float t)
    {
        float dot = a.Dot(b);
        dot = (std::max)(-1.0f, (std::min)(1.0f, dot));
        float theta = std::acosf(dot);
        if (theta < 1e-6f) return Lerp(a, b, t);
        float sinTheta = std::sinf(theta);
        float wa = std::sinf((1.0f - t) * theta) / sinTheta;
        float wb = std::sinf(t * theta) / sinTheta;
        return a * wa + b * wb;
    }

    /// @brief 2ベクトル間の角度を求める（度数法、常に正）
    /// @param from 始点ベクトル
    /// @param to 終点ベクトル
    /// @return 角度（度）。常に0以上
    static float Angle(const Vector3& from, const Vector3& to)
    {
        float d = from.Dot(to);
        float denom = std::sqrtf(from.LengthSquared() * to.LengthSquared());
        if (denom < 1e-8f) return 0.0f;
        float cosAngle = (std::max)(-1.0f, (std::min)(1.0f, d / denom));
        return std::acosf(cosAngle) * (180.0f / 3.14159265358979323846f);
    }

    /// @brief 2ベクトル間の符号付き角度を求める（度数法、axis周りで正負判定）
    /// @param from 始点ベクトル
    /// @param to 終点ベクトル
    /// @param axis 正負を判定する回転軸
    /// @return 符号付き角度（度）
    static float SignedAngle(const Vector3& from, const Vector3& to, const Vector3& axis)
    {
        float angle = Angle(from, to);
        Vector3 cross = from.Cross(to);
        float sign = cross.Dot(axis);
        return (sign >= 0.0f) ? angle : -angle;
    }

    /// @brief ベクトルを法線で反射する
    /// @param direction 入射方向ベクトル
    /// @param normal 反射面の法線（正規化済み）
    /// @return 反射後のベクトル
    static Vector3 Reflect(const Vector3& direction, const Vector3& normal)
    {
        float d = direction.Dot(normal);
        return direction - normal * (2.0f * d);
    }

    /// @brief 行列でベクトルを座標変換する(w=1、射影除算あり) (SSE)
    /// @param v 変換するベクトル
    /// @param m 変換行列
    /// @return 変換後のベクトル
    static Vector3 Transform(const Vector3& v, const Matrix4x4& m);

    /// @brief 行列でベクトルを法線変換する(w=0、平行移動なし) (SSE)
    /// @param v 変換する法線ベクトル
    /// @param m 変換行列
    /// @return 変換後の法線ベクトル
    static Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);
};

static_assert(sizeof(Vector3) == 12, "Vector3 must be 12 bytes");

inline Vector3 operator*(float s, const Vector3& v) { return v * s; }

/// @}
} // namespace gx
