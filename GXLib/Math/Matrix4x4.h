#pragma once
#include "Vector3.h"

namespace GX {

/// @brief 4x4行列 (XMFLOAT4X4継承、ゼロオーバーヘッド)
struct Matrix4x4 : public XMFLOAT4X4
{
    /// @brief 単位行列で初期化するデフォルトコンストラクタ
    Matrix4x4()
    {
        XMMATRIX identity = XMMatrixIdentity();
        XMStoreFloat4x4(this, identity);
    }

    /// @brief XMFLOAT4X4からの変換コンストラクタ
    /// @param m 変換元のXMFLOAT4X4
    Matrix4x4(const XMFLOAT4X4& m) : XMFLOAT4X4(m) {}

    /// @brief XMMATRIXに変換する
    /// @return DirectXMathのXMMATRIX型
    XMMATRIX ToXMMATRIX() const
    {
        return XMLoadFloat4x4(this);
    }

    /// @brief XMMATRIXからMatrix4x4を生成する
    /// @param m 変換元のXMMATRIX
    /// @return 変換されたMatrix4x4
    static Matrix4x4 FromXMMATRIX(const XMMATRIX& m)
    {
        Matrix4x4 result;
        XMStoreFloat4x4(&result, m);
        return result;
    }

    /// @brief 行列の乗算
    /// @param m 右側の行列
    /// @return 乗算結果の行列
    Matrix4x4 operator*(const Matrix4x4& m) const
    {
        XMMATRIX a = XMLoadFloat4x4(this);
        XMMATRIX b = XMLoadFloat4x4(&m);
        return FromXMMATRIX(XMMatrixMultiply(a, b));
    }

    /// @brief 逆行列を取得する
    /// @return 逆行列
    Matrix4x4 Inverse() const
    {
        XMMATRIX m = XMLoadFloat4x4(this);
        XMVECTOR det;
        return FromXMMATRIX(XMMatrixInverse(&det, m));
    }

    /// @brief 転置行列を取得する
    /// @return 転置行列
    Matrix4x4 Transpose() const
    {
        XMMATRIX m = XMLoadFloat4x4(this);
        return FromXMMATRIX(XMMatrixTranspose(m));
    }

    /// @brief 行列式を計算する
    /// @return 行列式の値
    float Determinant() const
    {
        XMMATRIX m = XMLoadFloat4x4(this);
        return XMVectorGetX(XMMatrixDeterminant(m));
    }

    /// @brief 点(位置)を行列で変換する (w=1、射影除算あり)
    /// @param point 変換対象の点
    /// @return 変換後の点
    Vector3 TransformPoint(const Vector3& point) const
    {
        return Vector3::Transform(point, ToXMMATRIX());
    }

    /// @brief ベクトル(方向)を行列で変換する (w=0、平行移動なし)
    /// @param vector 変換対象のベクトル
    /// @return 変換後のベクトル
    Vector3 TransformVector(const Vector3& vector) const
    {
        return Vector3::TransformNormal(vector, ToXMMATRIX());
    }

    /// @brief 単位行列を返す
    /// @return 単位行列
    static Matrix4x4 Identity()
    {
        return FromXMMATRIX(XMMatrixIdentity());
    }

    /// @brief 平行移動行列を作成する
    /// @param x X方向の移動量
    /// @param y Y方向の移動量
    /// @param z Z方向の移動量
    /// @return 平行移動行列
    static Matrix4x4 Translation(float x, float y, float z)
    {
        return FromXMMATRIX(XMMatrixTranslation(x, y, z));
    }

    /// @brief 平行移動行列を作成する(ベクトル指定)
    /// @param t 移動量ベクトル
    /// @return 平行移動行列
    static Matrix4x4 Translation(const Vector3& t)
    {
        return Translation(t.x, t.y, t.z);
    }

    /// @brief 拡大縮小行列を作成する
    /// @param x X方向のスケール
    /// @param y Y方向のスケール
    /// @param z Z方向のスケール
    /// @return 拡大縮小行列
    static Matrix4x4 Scaling(float x, float y, float z)
    {
        return FromXMMATRIX(XMMatrixScaling(x, y, z));
    }

    /// @brief 均一拡大縮小行列を作成する
    /// @param uniform 全軸共通のスケール値
    /// @return 拡大縮小行列
    static Matrix4x4 Scaling(float uniform)
    {
        return Scaling(uniform, uniform, uniform);
    }

    /// @brief X軸回転行列を作成する
    /// @param radians 回転角度(ラジアン)
    /// @return X軸回転行列
    static Matrix4x4 RotationX(float radians)
    {
        return FromXMMATRIX(XMMatrixRotationX(radians));
    }

    /// @brief Y軸回転行列を作成する
    /// @param radians 回転角度(ラジアン)
    /// @return Y軸回転行列
    static Matrix4x4 RotationY(float radians)
    {
        return FromXMMATRIX(XMMatrixRotationY(radians));
    }

    /// @brief Z軸回転行列を作成する
    /// @param radians 回転角度(ラジアン)
    /// @return Z軸回転行列
    static Matrix4x4 RotationZ(float radians)
    {
        return FromXMMATRIX(XMMatrixRotationZ(radians));
    }

    /// @brief オイラー角(ピッチ・ヨー・ロール)から回転行列を作成する
    /// @param pitch ピッチ角(X軸回転、ラジアン)
    /// @param yaw ヨー角(Y軸回転、ラジアン)
    /// @param roll ロール角(Z軸回転、ラジアン)
    /// @return 回転行列
    static Matrix4x4 RotationRollPitchYaw(float pitch, float yaw, float roll)
    {
        return FromXMMATRIX(XMMatrixRotationRollPitchYaw(pitch, yaw, roll));
    }

    /// @brief 左手座標系のビュー行列(LookAt)を作成する
    /// @param eye カメラ位置
    /// @param target 注視点
    /// @param up 上方向ベクトル
    /// @return ビュー行列
    static Matrix4x4 LookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up)
    {
        XMVECTOR e = XMLoadFloat3(&eye);
        XMVECTOR t = XMLoadFloat3(&target);
        XMVECTOR u = XMLoadFloat3(&up);
        return FromXMMATRIX(XMMatrixLookAtLH(e, t, u));
    }

    /// @brief 左手座標系の透視投影行列を作成する
    /// @param fovY 垂直方向の視野角(ラジアン)
    /// @param aspect アスペクト比 (幅/高さ)
    /// @param nearZ ニアクリップ面の距離
    /// @param farZ ファークリップ面の距離
    /// @return 透視投影行列
    static Matrix4x4 PerspectiveFovLH(float fovY, float aspect, float nearZ, float farZ)
    {
        return FromXMMATRIX(XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ));
    }

    /// @brief 左手座標系の正射影行列を作成する
    /// @param width ビューの幅
    /// @param height ビューの高さ
    /// @param nearZ ニアクリップ面の距離
    /// @param farZ ファークリップ面の距離
    /// @return 正射影行列
    static Matrix4x4 OrthographicLH(float width, float height, float nearZ, float farZ)
    {
        return FromXMMATRIX(XMMatrixOrthographicLH(width, height, nearZ, farZ));
    }
};

} // namespace GX
