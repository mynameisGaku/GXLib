#pragma once
#include "Vector3.h"

namespace GX {

struct Matrix4x4;

/// @brief クォータニオン (XMFLOAT4継承、回転表現用)
struct Quaternion : public XMFLOAT4
{
    /// @brief 単位クォータニオン(回転なし)で初期化するデフォルトコンストラクタ
    Quaternion() : XMFLOAT4(0, 0, 0, 1) {}

    /// @brief 指定した成分で初期化するコンストラクタ
    /// @param x X成分
    /// @param y Y成分
    /// @param z Z成分
    /// @param w W成分
    Quaternion(float x, float y, float z, float w) : XMFLOAT4(x, y, z, w) {}

    /// @brief XMFLOAT4からの変換コンストラクタ
    /// @param q 変換元のXMFLOAT4
    Quaternion(const XMFLOAT4& q) : XMFLOAT4(q) {}

    /// @brief クォータニオンの乗算(回転の合成)
    /// @param q 右側のクォータニオン
    /// @return 合成されたクォータニオン
    Quaternion operator*(const Quaternion& q) const
    {
        XMVECTOR a = XMLoadFloat4(this);
        XMVECTOR b = XMLoadFloat4(&q);
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionMultiply(a, b));
        return result;
    }

    /// @brief クォータニオンの長さ(ノルム)を取得する
    /// @return クォータニオンの長さ
    float Length() const
    {
        XMVECTOR q = XMLoadFloat4(this);
        return XMVectorGetX(XMQuaternionLength(q));
    }

    /// @brief 正規化されたクォータニオンを返す(自身は変更しない)
    /// @return 長さ1に正規化されたクォータニオン
    Quaternion Normalized() const
    {
        XMVECTOR q = XMLoadFloat4(this);
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionNormalize(q));
        return result;
    }

    /// @brief 自身を正規化する(長さを1にする)
    void Normalize()
    {
        XMVECTOR q = XMLoadFloat4(this);
        XMStoreFloat4(this, XMQuaternionNormalize(q));
    }

    /// @brief 共役クォータニオンを返す
    /// @return 共役クォータニオン (虚部の符号を反転)
    Quaternion Conjugate() const
    {
        XMVECTOR q = XMLoadFloat4(this);
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionConjugate(q));
        return result;
    }

    /// @brief 逆クォータニオンを返す
    /// @return 逆クォータニオン (回転の逆操作を表す)
    Quaternion Inverse() const
    {
        XMVECTOR q = XMLoadFloat4(this);
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionInverse(q));
        return result;
    }

    /// @brief クォータニオンからオイラー角(ラジアン)に変換する
    /// @return オイラー角 (x=ピッチ, y=ヨー, z=ロール)
    Vector3 ToEuler() const
    {
        // クォータニオンからピッチ/ヨー/ロールを算出する
        // 回転を3つの角度(ピッチ・ヨー・ロール)に分解する
        float sinP = 2.0f * (w * x + y * z);
        float pitch;
        if (std::abs(sinP) >= 1.0f)
            pitch = std::copysign(MathUtil::PI / 2.0f, sinP);
        else
            pitch = std::asin(sinP);

        float sinYcosP = 2.0f * (w * y - z * x);
        float cosYcosP = 1.0f - 2.0f * (x * x + y * y);
        float yaw = std::atan2(sinYcosP, cosYcosP);

        float sinRcosP = 2.0f * (w * z - x * y);
        float cosRcosP = 1.0f - 2.0f * (z * z + x * x);
        float roll = std::atan2(sinRcosP, cosRcosP);

        return { pitch, yaw, roll };
    }

    /// @brief クォータニオンでベクトルを回転する
    /// @param v 回転対象のベクトル
    /// @return 回転後のベクトル
    Vector3 RotateVector(const Vector3& v) const
    {
        XMVECTOR q = XMLoadFloat4(this);
        XMVECTOR xv = XMLoadFloat3(&v);
        XMMATRIX rotMat = XMMatrixRotationQuaternion(q);
        Vector3 result;
        XMStoreFloat3(&result, XMVector3TransformNormal(xv, rotMat));
        return result;
    }

    /// @brief 単位クォータニオン(回転なし)を返す
    /// @return 単位クォータニオン (0, 0, 0, 1)
    static Quaternion Identity()
    {
        return { 0, 0, 0, 1 };
    }

    /// @brief 任意軸回りの回転クォータニオンを作成する
    /// @param axis 回転軸(正規化されていること)
    /// @param radians 回転角度(ラジアン)
    /// @return 回転クォータニオン
    static Quaternion FromAxisAngle(const Vector3& axis, float radians)
    {
        XMVECTOR a = XMLoadFloat3(&axis);
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionRotationAxis(a, radians));
        return result;
    }

    /// @brief オイラー角から回転クォータニオンを作成する
    /// @param pitch ピッチ角(X軸回転、ラジアン)
    /// @param yaw ヨー角(Y軸回転、ラジアン)
    /// @param roll ロール角(Z軸回転、ラジアン)
    /// @return 回転クォータニオン
    static Quaternion FromEuler(float pitch, float yaw, float roll)
    {
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionRotationRollPitchYaw(pitch, yaw, roll));
        return result;
    }

    /// @brief 回転行列からクォータニオンを作成する
    /// @param m 回転行列 (XMFLOAT4X4)
    /// @return 回転クォータニオン
    static Quaternion FromRotationMatrix(const XMFLOAT4X4& m)
    {
        XMMATRIX mat = XMLoadFloat4x4(&m);
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionRotationMatrix(mat));
        return result;
    }

    /// @brief 2つのクォータニオン間を球面線形補間(Slerp)する
    /// @param a 開始クォータニオン (t=0)
    /// @param b 終了クォータニオン (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 補間結果のクォータニオン
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)
    {
        XMVECTOR qa = XMLoadFloat4(&a);
        XMVECTOR qb = XMLoadFloat4(&b);
        Quaternion result;
        XMStoreFloat4(&result, XMQuaternionSlerp(qa, qb, t));
        return result;
    }

    /// @brief 2つのクォータニオン間を正規化線形補間(NLerp)する
    /// @param a 開始クォータニオン (t=0)
    /// @param b 終了クォータニオン (t=1)
    /// @param t 補間係数 [0, 1]
    /// @return 正規化された補間結果のクォータニオン
    static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t)
    {
        // NLerp（正規化付き線形補間）
        // 線形補間だけでは長さが1からズレるため、最後に正規化して正しい回転にする
        XMFLOAT4 result;
        result.x = a.x + (b.x - a.x) * t;
        result.y = a.y + (b.y - a.y) * t;
        result.z = a.z + (b.z - a.z) * t;
        result.w = a.w + (b.w - a.w) * t;
        Quaternion q(result);
        q.Normalize();
        return q;
    }
};

} // namespace GX
