#pragma once
#include "MathUtil.h"
#include <cmath>

namespace gx {
/// @addtogroup grp_math
/// @{

/// @brief 2D浮動小数点ベクトル
struct Vector2
{
    float x = 0, y = 0;

    /// @brief ゼロベクトルで初期化するデフォルトコンストラクタ
    Vector2() = default;

    /// @brief 指定した成分で初期化するコンストラクタ
    /// @param x X成分
    /// @param y Y成分
    Vector2(float x, float y) : x(x), y(y) {}

    /// @brief ベクトル加算
    Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }

    /// @brief ベクトル減算
    Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }

    /// @brief スカラー乗算
    Vector2 operator*(float s) const { return { x * s, y * s }; }

    /// @brief スカラー除算
    Vector2 operator/(float s) const { float inv = 1.0f / s; return { x * inv, y * inv }; }

    /// @brief ベクトル加算代入
    Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }

    /// @brief ベクトル減算代入
    Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }

    /// @brief スカラー乗算代入
    Vector2& operator*=(float s) { x *= s; y *= s; return *this; }

    /// @brief 等値比較
    bool operator==(const Vector2& v) const { return x == v.x && y == v.y; }

    /// @brief 非等値比較
    bool operator!=(const Vector2& v) const { return !(*this == v); }

    /// @brief 符号反転(単項マイナス)
    Vector2 operator-() const { return { -x, -y }; }

    /// @brief ベクトルの長さ(ノルム)を取得する
    float Length() const { return std::sqrtf(x * x + y * y); }

    /// @brief ベクトルの長さの2乗を取得する(sqrt不要で高速)
    float LengthSquared() const { return x * x + y * y; }

    /// @brief 正規化されたベクトルを返す(自身は変更しない)
    Vector2 Normalized() const
    {
        float len = Length();
        if (len < 1e-8f) return {};
        float inv = 1.0f / len;
        return { x * inv, y * inv };
    }

    /// @brief 自身を正規化する(長さを1にする)
    void Normalize()
    {
        float len = Length();
        if (len < 1e-8f) return;
        float inv = 1.0f / len;
        x *= inv;
        y *= inv;
    }

    /// @brief 内積(ドット積)を計算する
    float Dot(const Vector2& v) const { return x * v.x + y * v.y; }

    /// @brief 2D外積(スカラー値)を計算する
    float Cross(const Vector2& v) const { return x * v.y - y * v.x; }

    /// @brief 他のベクトルとの距離を計算する
    float Distance(const Vector2& v) const { return (*this - v).Length(); }

    /// @brief 他のベクトルとの距離の2乗を計算する(sqrt不要で高速)
    float DistanceSquared(const Vector2& v) const { return (*this - v).LengthSquared(); }

    static Vector2 Zero()  { return { 0, 0 }; }
    static Vector2 One()   { return { 1, 1 }; }
    static Vector2 UnitX() { return { 1, 0 }; }
    static Vector2 UnitY() { return { 0, 1 }; }

    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    static Vector2 Min(const Vector2& a, const Vector2& b)
    {
        return { (std::min)(a.x, b.x), (std::min)(a.y, b.y) };
    }

    static Vector2 Max(const Vector2& a, const Vector2& b)
    {
        return { (std::max)(a.x, b.x), (std::max)(a.y, b.y) };
    }

    /// @brief 垂直ベクトルを返す（反時計回り90度回転）
    Vector2 Perpendicular() const { return { -y, x }; }

    /// @brief 要素ごとの乗算
    static Vector2 Scale(const Vector2& a, const Vector2& b)
    {
        return { a.x * b.x, a.y * b.y };
    }

    /// @brief 現在位置からターゲットへ最大maxDistanceDelta分だけ移動
    static Vector2 MoveTowards(const Vector2& current, const Vector2& target, float maxDistanceDelta)
    {
        Vector2 diff = target - current;
        float dist = diff.Length();
        if (dist <= maxDistanceDelta || dist < 1e-8f) return target;
        return current + diff * (maxDistanceDelta / dist);
    }

    /// @brief ベクトルの長さを最大値でクランプ
    static Vector2 ClampMagnitude(const Vector2& v, float maxLength)
    {
        float lenSq = v.LengthSquared();
        if (lenSq > maxLength * maxLength)
        {
            float len = std::sqrtf(lenSq);
            return v * (maxLength / len);
        }
        return v;
    }

    /// @brief 2ベクトル間の角度（度数法、常に正）
    static float Angle(const Vector2& from, const Vector2& to)
    {
        float d = from.Dot(to);
        float denom = std::sqrtf(from.LengthSquared() * to.LengthSquared());
        if (denom < 1e-8f) return 0.0f;
        float cosAngle = MathUtil::Clamp(d / denom, -1.0f, 1.0f);
        return std::acosf(cosAngle) * (180.0f / MathUtil::PI);
    }

    /// @brief 2ベクトル間の符号付き角度（度数法）
    static float SignedAngle(const Vector2& from, const Vector2& to)
    {
        float angle = Angle(from, to);
        float cross = from.Cross(to);
        return (cross >= 0.0f) ? angle : -angle;
    }
};

static_assert(sizeof(Vector2) == 8, "Vector2 must be 8 bytes");

inline Vector2 operator*(float s, const Vector2& v) { return v * s; }

/// @}
} // namespace gx
