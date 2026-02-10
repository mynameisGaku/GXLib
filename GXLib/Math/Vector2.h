#pragma once
#include "MathUtil.h"

namespace GX {

/// @brief 2D浮動小数点ベクトル (XMFLOAT2継承、ゼロオーバーヘッド)
struct Vector2 : public XMFLOAT2
{
    /// @brief ゼロベクトルで初期化するデフォルトコンストラクタ
    Vector2() : XMFLOAT2(0, 0) {}

    /// @brief 指定した成分で初期化するコンストラクタ
    /// @param x X成分
    /// @param y Y成分
    Vector2(float x, float y) : XMFLOAT2(x, y) {}

    /// @brief XMFLOAT2からの変換コンストラクタ
    /// @param v 変換元のXMFLOAT2
    Vector2(const XMFLOAT2& v) : XMFLOAT2(v) {}

    /// @brief ベクトル加算
    /// @param v 加算するベクトル
    /// @return 加算結果のベクトル
    Vector2 operator+(const Vector2& v) const { return { x + v.x, y + v.y }; }

    /// @brief ベクトル減算
    /// @param v 減算するベクトル
    /// @return 減算結果のベクトル
    Vector2 operator-(const Vector2& v) const { return { x - v.x, y - v.y }; }

    /// @brief スカラー乗算
    /// @param s 乗算するスカラー値
    /// @return スカラー乗算結果のベクトル
    Vector2 operator*(float s) const { return { x * s, y * s }; }

    /// @brief スカラー除算
    /// @param s 除算するスカラー値
    /// @return スカラー除算結果のベクトル
    Vector2 operator/(float s) const { float inv = 1.0f / s; return { x * inv, y * inv }; }

    /// @brief ベクトル加算代入
    /// @param v 加算するベクトル
    /// @return 自身への参照
    Vector2& operator+=(const Vector2& v) { x += v.x; y += v.y; return *this; }

    /// @brief ベクトル減算代入
    /// @param v 減算するベクトル
    /// @return 自身への参照
    Vector2& operator-=(const Vector2& v) { x -= v.x; y -= v.y; return *this; }

    /// @brief スカラー乗算代入
    /// @param s 乗算するスカラー値
    /// @return 自身への参照
    Vector2& operator*=(float s) { x *= s; y *= s; return *this; }

    /// @brief 等値比較
    /// @param v 比較対象のベクトル
    /// @return 全成分が一致すればtrue
    bool operator==(const Vector2& v) const { return x == v.x && y == v.y; }

    /// @brief 非等値比較
    /// @param v 比較対象のベクトル
    /// @return いずれかの成分が異なればtrue
    bool operator!=(const Vector2& v) const { return !(*this == v); }

    /// @brief 符号反転(単項マイナス)
    /// @return 各成分の符号を反転したベクトル
    Vector2 operator-() const { return { -x, -y }; }

    /// @brief ベクトルの長さ(ノルム)を取得する
    /// @return ベクトルの長さ
    float Length() const
    {
        XMVECTOR v = XMLoadFloat2(this);
        return XMVectorGetX(XMVector2Length(v));
    }

    /// @brief ベクトルの長さの2乗を取得する(sqrt不要で高速)
    /// @return ベクトルの長さの2乗
    float LengthSquared() const
    {
        XMVECTOR v = XMLoadFloat2(this);
        return XMVectorGetX(XMVector2LengthSq(v));
    }

    /// @brief 正規化されたベクトルを返す(自身は変更しない)
    /// @return 長さ1に正規化されたベクトル
    Vector2 Normalized() const
    {
        XMVECTOR v = XMLoadFloat2(this);
        Vector2 result;
        XMStoreFloat2(&result, XMVector2Normalize(v));
        return result;
    }

    /// @brief 自身を正規化する(長さを1にする)
    void Normalize()
    {
        XMVECTOR v = XMLoadFloat2(this);
        XMStoreFloat2(this, XMVector2Normalize(v));
    }

    /// @brief 内積(ドット積)を計算する
    /// @param v 内積を計算する相手のベクトル
    /// @return 内積の値
    float Dot(const Vector2& v) const
    {
        XMVECTOR a = XMLoadFloat2(this);
        XMVECTOR b = XMLoadFloat2(&v);
        return XMVectorGetX(XMVector2Dot(a, b));
    }

    /// @brief 2D外積(スカラー値)を計算する
    /// @param v 外積を計算する相手のベクトル
    /// @return 外積のスカラー値 (x*v.y - y*v.x)
    float Cross(const Vector2& v) const
    {
        return x * v.y - y * v.x;
    }

    /// @brief 他のベクトルとの距離を計算する
    /// @param v 距離を測る相手のベクトル
    /// @return 2点間の距離
    float Distance(const Vector2& v) const
    {
        return (*this - v).Length();
    }

    /// @brief 他のベクトルとの距離の2乗を計算する(sqrt不要で高速)
    /// @param v 距離を測る相手のベクトル
    /// @return 2点間の距離の2乗
    float DistanceSquared(const Vector2& v) const
    {
        return (*this - v).LengthSquared();
    }

    /// @brief ゼロベクトル (0, 0) を返す
    /// @return ゼロベクトル
    static Vector2 Zero()  { return { 0, 0 }; }

    /// @brief 全成分が1のベクトル (1, 1) を返す
    /// @return (1, 1) ベクトル
    static Vector2 One()   { return { 1, 1 }; }

    /// @brief X軸単位ベクトル (1, 0) を返す
    /// @return X軸方向の単位ベクトル
    static Vector2 UnitX() { return { 1, 0 }; }

    /// @brief Y軸単位ベクトル (0, 1) を返す
    /// @return Y軸方向の単位ベクトル
    static Vector2 UnitY() { return { 0, 1 }; }

    /// @brief 2つのベクトル間を線形補間する
    /// @param a 開始ベクトル (t=0)
    /// @param b 終了ベクトル (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果のベクトル
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    /// @brief 各成分ごとの最小値を返す
    /// @param a 1つ目のベクトル
    /// @param b 2つ目のベクトル
    /// @return 各成分が最小値のベクトル
    static Vector2 Min(const Vector2& a, const Vector2& b)
    {
        return { (std::min)(a.x, b.x), (std::min)(a.y, b.y) };
    }

    /// @brief 各成分ごとの最大値を返す
    /// @param a 1つ目のベクトル
    /// @param b 2つ目のベクトル
    /// @return 各成分が最大値のベクトル
    static Vector2 Max(const Vector2& a, const Vector2& b)
    {
        return { (std::max)(a.x, b.x), (std::max)(a.y, b.y) };
    }
};

/// @brief スカラーとベクトルの乗算 (s * v)
/// @param s スカラー値
/// @param v ベクトル
/// @return スカラー乗算結果のベクトル
inline Vector2 operator*(float s, const Vector2& v) { return v * s; }

} // namespace GX
