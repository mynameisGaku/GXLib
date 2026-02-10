#pragma once
#include "MathUtil.h"

namespace GX {

/// @brief 3D浮動小数点ベクトル (XMFLOAT3継承、ゼロオーバーヘッド)
struct Vector3 : public XMFLOAT3
{
    /// @brief ゼロベクトルで初期化するデフォルトコンストラクタ
    Vector3() : XMFLOAT3(0, 0, 0) {}

    /// @brief 指定した成分で初期化するコンストラクタ
    /// @param x X成分
    /// @param y Y成分
    /// @param z Z成分
    Vector3(float x, float y, float z) : XMFLOAT3(x, y, z) {}

    /// @brief XMFLOAT3からの変換コンストラクタ
    /// @param v 変換元のXMFLOAT3
    Vector3(const XMFLOAT3& v) : XMFLOAT3(v) {}

    /// @brief ベクトル加算
    /// @param v 加算するベクトル
    /// @return 加算結果のベクトル
    Vector3 operator+(const Vector3& v) const { return { x + v.x, y + v.y, z + v.z }; }

    /// @brief ベクトル減算
    /// @param v 減算するベクトル
    /// @return 減算結果のベクトル
    Vector3 operator-(const Vector3& v) const { return { x - v.x, y - v.y, z - v.z }; }

    /// @brief スカラー乗算
    /// @param s 乗算するスカラー値
    /// @return スカラー乗算結果のベクトル
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }

    /// @brief スカラー除算
    /// @param s 除算するスカラー値
    /// @return スカラー除算結果のベクトル
    Vector3 operator/(float s) const { float inv = 1.0f / s; return { x * inv, y * inv, z * inv }; }

    /// @brief ベクトル加算代入
    /// @param v 加算するベクトル
    /// @return 自身への参照
    Vector3& operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }

    /// @brief ベクトル減算代入
    /// @param v 減算するベクトル
    /// @return 自身への参照
    Vector3& operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }

    /// @brief スカラー乗算代入
    /// @param s 乗算するスカラー値
    /// @return 自身への参照
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    /// @brief 等値比較
    /// @param v 比較対象のベクトル
    /// @return 全成分が一致すればtrue
    bool operator==(const Vector3& v) const { return x == v.x && y == v.y && z == v.z; }

    /// @brief 非等値比較
    /// @param v 比較対象のベクトル
    /// @return いずれかの成分が異なればtrue
    bool operator!=(const Vector3& v) const { return !(*this == v); }

    /// @brief 符号反転(単項マイナス)
    /// @return 各成分の符号を反転したベクトル
    Vector3 operator-() const { return { -x, -y, -z }; }

    /// @brief ベクトルの長さ(ノルム)を取得する
    /// @return ベクトルの長さ
    float Length() const
    {
        XMVECTOR v = XMLoadFloat3(this);
        return XMVectorGetX(XMVector3Length(v));
    }

    /// @brief ベクトルの長さの2乗を取得する(sqrt不要で高速)
    /// @return ベクトルの長さの2乗
    float LengthSquared() const
    {
        XMVECTOR v = XMLoadFloat3(this);
        return XMVectorGetX(XMVector3LengthSq(v));
    }

    /// @brief 正規化されたベクトルを返す(自身は変更しない)
    /// @return 長さ1に正規化されたベクトル
    Vector3 Normalized() const
    {
        XMVECTOR v = XMLoadFloat3(this);
        Vector3 result;
        XMStoreFloat3(&result, XMVector3Normalize(v));
        return result;
    }

    /// @brief 自身を正規化する(長さを1にする)
    void Normalize()
    {
        XMVECTOR v = XMLoadFloat3(this);
        XMStoreFloat3(this, XMVector3Normalize(v));
    }

    /// @brief 内積(ドット積)を計算する
    /// @param v 内積を計算する相手のベクトル
    /// @return 内積の値
    float Dot(const Vector3& v) const
    {
        XMVECTOR a = XMLoadFloat3(this);
        XMVECTOR b = XMLoadFloat3(&v);
        return XMVectorGetX(XMVector3Dot(a, b));
    }

    /// @brief 外積(クロス積)を計算する
    /// @param v 外積を計算する相手のベクトル
    /// @return 外積結果のベクトル (this x v)
    Vector3 Cross(const Vector3& v) const
    {
        XMVECTOR a = XMLoadFloat3(this);
        XMVECTOR b = XMLoadFloat3(&v);
        Vector3 result;
        XMStoreFloat3(&result, XMVector3Cross(a, b));
        return result;
    }

    /// @brief 他のベクトルとの距離を計算する
    /// @param v 距離を測る相手のベクトル
    /// @return 2点間の距離
    float Distance(const Vector3& v) const
    {
        return (*this - v).Length();
    }

    /// @brief 他のベクトルとの距離の2乗を計算する(sqrt不要で高速)
    /// @param v 距離を測る相手のベクトル
    /// @return 2点間の距離の2乗
    float DistanceSquared(const Vector3& v) const
    {
        return (*this - v).LengthSquared();
    }

    /// @brief ゼロベクトル (0, 0, 0) を返す
    /// @return ゼロベクトル
    static Vector3 Zero()     { return { 0, 0, 0 }; }

    /// @brief 全成分が1のベクトル (1, 1, 1) を返す
    /// @return (1, 1, 1) ベクトル
    static Vector3 One()      { return { 1, 1, 1 }; }

    /// @brief 上方向ベクトル (0, 1, 0) を返す
    /// @return Y軸正方向の単位ベクトル
    static Vector3 Up()       { return { 0, 1, 0 }; }

    /// @brief 下方向ベクトル (0, -1, 0) を返す
    /// @return Y軸負方向の単位ベクトル
    static Vector3 Down()     { return { 0,-1, 0 }; }

    /// @brief 前方向ベクトル (0, 0, 1) を返す
    /// @return Z軸正方向の単位ベクトル
    static Vector3 Forward()  { return { 0, 0, 1 }; }

    /// @brief 後方向ベクトル (0, 0, -1) を返す
    /// @return Z軸負方向の単位ベクトル
    static Vector3 Backward() { return { 0, 0,-1 }; }

    /// @brief 左方向ベクトル (-1, 0, 0) を返す
    /// @return X軸負方向の単位ベクトル
    static Vector3 Left()     { return {-1, 0, 0 }; }

    /// @brief 右方向ベクトル (1, 0, 0) を返す
    /// @return X軸正方向の単位ベクトル
    static Vector3 Right()    { return { 1, 0, 0 }; }

    /// @brief 2つのベクトル間を線形補間する
    /// @param a 開始ベクトル (t=0)
    /// @param b 終了ベクトル (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果のベクトル
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t };
    }

    /// @brief 各成分ごとの最小値を返す
    /// @param a 1つ目のベクトル
    /// @param b 2つ目のベクトル
    /// @return 各成分が最小値のベクトル
    static Vector3 Min(const Vector3& a, const Vector3& b)
    {
        return { (std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::min)(a.z, b.z) };
    }

    /// @brief 各成分ごとの最大値を返す
    /// @param a 1つ目のベクトル
    /// @param b 2つ目のベクトル
    /// @return 各成分が最大値のベクトル
    static Vector3 Max(const Vector3& a, const Vector3& b)
    {
        return { (std::max)(a.x, b.x), (std::max)(a.y, b.y), (std::max)(a.z, b.z) };
    }

    /// @brief ベクトルを法線で反射する
    /// @param direction 入射方向ベクトル
    /// @param normal 反射面の法線ベクトル
    /// @return 反射後の方向ベクトル
    static Vector3 Reflect(const Vector3& direction, const Vector3& normal)
    {
        float d = direction.Dot(normal);
        return direction - normal * (2.0f * d);
    }

    /// @brief 行列でベクトルを座標変換する(w=1、射影除算あり)
    /// @param v 変換対象のベクトル(位置)
    /// @param m 変換行列
    /// @return 変換後のベクトル
    static Vector3 Transform(const Vector3& v, const XMMATRIX& m)
    {
        XMVECTOR xv = XMLoadFloat3(&v);
        Vector3 result;
        XMStoreFloat3(&result, XMVector3TransformCoord(xv, m));
        return result;
    }

    /// @brief 行列でベクトルを法線変換する(w=0、平行移動なし)
    /// @param v 変換対象のベクトル(方向/法線)
    /// @param m 変換行列
    /// @return 変換後のベクトル
    static Vector3 TransformNormal(const Vector3& v, const XMMATRIX& m)
    {
        XMVECTOR xv = XMLoadFloat3(&v);
        Vector3 result;
        XMStoreFloat3(&result, XMVector3TransformNormal(xv, m));
        return result;
    }
};

/// @brief スカラーとベクトルの乗算 (s * v)
/// @param s スカラー値
/// @param v ベクトル
/// @return スカラー乗算結果のベクトル
inline Vector3 operator*(float s, const Vector3& v) { return v * s; }

} // namespace GX
