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
    /// @param v 加算するベクトル
    /// @return 加算結果
    Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }

    /// @brief ベクトル減算
    /// @param v 減算するベクトル
    /// @return 減算結果
    Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }

    /// @brief スカラー乗算
    /// @param s 乗算するスカラー値
    /// @return 乗算結果
    Vector2 operator*(float s) const { return { x * s, y * s }; }

    /// @brief スカラー除算
    /// @param s 除算するスカラー値
    /// @return 除算結果
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
    /// @return ベクトルの長さ
    float Length() const { return std::sqrtf(x * x + y * y); }

    /// @brief ベクトルの長さの2乗を取得する(sqrt不要で高速)
    /// @return 長さの2乗
    float LengthSquared() const { return x * x + y * y; }

    /// @brief 正規化されたベクトルを返す(自身は変更しない)
    /// @return 単位ベクトル（長さがほぼ0の場合はゼロベクトル）
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
    /// @param v 内積を取る相手のベクトル
    /// @return 内積値
    float Dot(const Vector2& v) const { return x * v.x + y * v.y; }

    /// @brief 2D外積(スカラー値)を計算する
    /// @param v 外積を取る相手のベクトル
    /// @return 外積値（正なら反時計回り）
    float Cross(const Vector2& v) const { return x * v.y - y * v.x; }

    /// @brief 他のベクトルとの距離を計算する
    /// @param v 対象のベクトル
    /// @return 2点間の距離
    float Distance(const Vector2& v) const { return (*this - v).Length(); }

    /// @brief 他のベクトルとの距離の2乗を計算する(sqrt不要で高速)
    /// @param v 対象のベクトル
    /// @return 距離の2乗
    float DistanceSquared(const Vector2& v) const { return (*this - v).LengthSquared(); }

    /// @brief ゼロベクトル (0, 0) を返す
    /// @return ゼロベクトル
    static Vector2 Zero()  { return { 0, 0 }; }

    /// @brief 全成分1のベクトル (1, 1) を返す
    /// @return (1, 1)
    static Vector2 One()   { return { 1, 1 }; }

    /// @brief X軸方向の単位ベクトル (1, 0) を返す
    /// @return X軸単位ベクトル
    static Vector2 UnitX() { return { 1, 0 }; }

    /// @brief Y軸方向の単位ベクトル (0, 1) を返す
    /// @return Y軸単位ベクトル
    static Vector2 UnitY() { return { 0, 1 }; }

    /// @brief 2ベクトル間を線形補間する
    /// @param a 開始ベクトル (t=0)
    /// @param b 終了ベクトル (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    /// @brief 各成分の最小値を取る
    /// @param a ベクトル1
    /// @param b ベクトル2
    /// @return 各成分が小さい方のベクトル
    static Vector2 Min(const Vector2& a, const Vector2& b)
    {
        return { (std::min)(a.x, b.x), (std::min)(a.y, b.y) };
    }

    /// @brief 各成分の最大値を取る
    /// @param a ベクトル1
    /// @param b ベクトル2
    /// @return 各成分が大きい方のベクトル
    static Vector2 Max(const Vector2& a, const Vector2& b)
    {
        return { (std::max)(a.x, b.x), (std::max)(a.y, b.y) };
    }

    /// @brief 垂直ベクトルを返す（反時計回り90度回転）
    /// @return 垂直ベクトル
    Vector2 Perpendicular() const { return { -y, x }; }

    /// @brief 要素ごとの乗算
    /// @param a ベクトル1
    /// @param b ベクトル2
    /// @return 要素ごとの積
    static Vector2 Scale(const Vector2& a, const Vector2& b)
    {
        return { a.x * b.x, a.y * b.y };
    }

    /// @brief 現在位置からターゲットへ最大maxDistanceDelta分だけ移動する
    /// @param current 現在位置
    /// @param target 目標位置
    /// @param maxDistanceDelta 最大移動距離
    /// @return 移動後の位置
    static Vector2 MoveTowards(const Vector2& current, const Vector2& target, float maxDistanceDelta)
    {
        Vector2 diff = target - current;
        float dist = diff.Length();
        if (dist <= maxDistanceDelta || dist < 1e-8f) return target;
        return current + diff * (maxDistanceDelta / dist);
    }

    /// @brief ベクトルの長さを最大値でクランプする
    /// @param v クランプ対象のベクトル
    /// @param maxLength 許容する最大の長さ
    /// @return 長さがmaxLength以下に制限されたベクトル
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

    /// @brief 2ベクトル間の角度を求める（度数法、常に正）
    /// @param from 始点ベクトル
    /// @param to 終点ベクトル
    /// @return 角度（度）。常に0以上
    static float Angle(const Vector2& from, const Vector2& to)
    {
        float d = from.Dot(to);
        float denom = std::sqrtf(from.LengthSquared() * to.LengthSquared());
        if (denom < 1e-8f) return 0.0f;
        float cosAngle = MathUtil::Clamp(d / denom, -1.0f, 1.0f);
        return std::acosf(cosAngle) * (180.0f / MathUtil::PI);
    }

    /// @brief 2ベクトル間の符号付き角度を求める（度数法）
    /// @param from 始点ベクトル
    /// @param to 終点ベクトル
    /// @return 符号付き角度（度）。反時計回りが正
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
