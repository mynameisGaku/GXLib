#pragma once
#include "Vector3.h"

namespace GX {

/// @brief 4D浮動小数点ベクトル (XMFLOAT4継承、ゼロオーバーヘッド)
struct Vector4 : public XMFLOAT4
{
    /// @brief ゼロベクトルで初期化するデフォルトコンストラクタ
    Vector4() : XMFLOAT4(0, 0, 0, 0) {}

    /// @brief 指定した成分で初期化するコンストラクタ
    /// @param x X成分
    /// @param y Y成分
    /// @param z Z成分
    /// @param w W成分
    Vector4(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w) {}

    /// @brief Vector3とW成分から初期化するコンストラクタ
    /// @param v XYZ成分を持つVector3
    /// @param w W成分
    Vector4(const Vector3& v, float w) : XMFLOAT4(v.x, v.y, v.z, w) {}

    /// @brief XMFLOAT4からの変換コンストラクタ
    /// @param v 変換元のXMFLOAT4
    Vector4(const XMFLOAT4& v) : XMFLOAT4(v) {}

    /// @brief ベクトル加算
    /// @param v 加算するベクトル
    /// @return 加算結果のベクトル
    Vector4 operator+(const Vector4& v) const { return { x + v.x, y + v.y, z + v.z, w + v.w }; }

    /// @brief ベクトル減算
    /// @param v 減算するベクトル
    /// @return 減算結果のベクトル
    Vector4 operator-(const Vector4& v) const { return { x - v.x, y - v.y, z - v.z, w - v.w }; }

    /// @brief スカラー乗算
    /// @param s 乗算するスカラー値
    /// @return スカラー乗算結果のベクトル
    Vector4 operator*(float s) const { return { x * s, y * s, z * s, w * s }; }

    /// @brief スカラー除算
    /// @param s 除算するスカラー値
    /// @return スカラー除算結果のベクトル
    Vector4 operator/(float s) const { float inv = 1.0f / s; return { x * inv, y * inv, z * inv, w * inv }; }

    /// @brief 等値比較
    /// @param v 比較対象のベクトル
    /// @return 全成分が一致すればtrue
    bool operator==(const Vector4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }

    /// @brief 非等値比較
    /// @param v 比較対象のベクトル
    /// @return いずれかの成分が異なればtrue
    bool operator!=(const Vector4& v) const { return !(*this == v); }

    /// @brief 符号反転(単項マイナス)
    /// @return 各成分の符号を反転したベクトル
    Vector4 operator-() const { return { -x, -y, -z, -w }; }

    /// @brief ベクトルの長さ(ノルム)を取得する
    /// @return ベクトルの長さ
    float Length() const
    {
        XMVECTOR v = XMLoadFloat4(this);
        return XMVectorGetX(XMVector4Length(v));
    }

    /// @brief ベクトルの長さの2乗を取得する(sqrt不要で高速)
    /// @return ベクトルの長さの2乗
    float LengthSquared() const
    {
        XMVECTOR v = XMLoadFloat4(this);
        return XMVectorGetX(XMVector4LengthSq(v));
    }

    /// @brief 正規化されたベクトルを返す(自身は変更しない)
    /// @return 長さ1に正規化されたベクトル
    Vector4 Normalized() const
    {
        XMVECTOR v = XMLoadFloat4(this);
        Vector4 result;
        XMStoreFloat4(&result, XMVector4Normalize(v));
        return result;
    }

    /// @brief 自身を正規化する(長さを1にする)
    void Normalize()
    {
        XMVECTOR v = XMLoadFloat4(this);
        XMStoreFloat4(this, XMVector4Normalize(v));
    }

    /// @brief 内積(ドット積)を計算する
    /// @param v 内積を計算する相手のベクトル
    /// @return 内積の値
    float Dot(const Vector4& v) const
    {
        XMVECTOR a = XMLoadFloat4(this);
        XMVECTOR b = XMLoadFloat4(&v);
        return XMVectorGetX(XMVector4Dot(a, b));
    }

    /// @brief ゼロベクトル (0, 0, 0, 0) を返す
    /// @return ゼロベクトル
    static Vector4 Zero() { return { 0, 0, 0, 0 }; }

    /// @brief 全成分が1のベクトル (1, 1, 1, 1) を返す
    /// @return (1, 1, 1, 1) ベクトル
    static Vector4 One()  { return { 1, 1, 1, 1 }; }

    /// @brief 2つのベクトル間を線形補間する
    /// @param a 開始ベクトル (t=0)
    /// @param b 終了ベクトル (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果のベクトル
    static Vector4 Lerp(const Vector4& a, const Vector4& b, float t)
    {
        return {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t,
            a.w + (b.w - a.w) * t
        };
    }
};

/// @brief スカラーとベクトルの乗算 (s * v)
/// @param s スカラー値
/// @param v ベクトル
/// @return スカラー乗算結果のベクトル
inline Vector4 operator*(float s, const Vector4& v) { return v * s; }

} // namespace GX
