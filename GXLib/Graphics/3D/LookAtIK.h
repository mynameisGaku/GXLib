#pragma once
/// @file LookAtIK.h
/// @brief 視線IK（頭/首のターゲット追従）
///
/// 頭ボーンと首ボーン（任意）をターゲット方向に回転させて、
/// キャラクターが特定の位置を「見る」動作を実現する。

#include "pch.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/Transform3D.h"

namespace GX
{

/// @brief 視線IK
/// 頭と首のボーンをターゲット位置に向けて回転させる。
/// 角度制限付きで、不自然な180度回転を防止する。
class LookAtIK
{
public:
    /// @brief 視線IKのジョイントを設定する
    /// @param skeleton スケルトン参照
    /// @param headJoint 頭ジョイント名
    /// @param neckJoint 首ジョイント名（空文字で省略可）
    void Setup(const Skeleton& skeleton,
               const std::string& headJoint,
               const std::string& neckJoint = "");

    /// @brief 視線IKを適用する
    /// @param localTransforms ローカル変換行列配列（入出力）
    /// @param globalTransforms グローバル変換行列配列（入出力）
    /// @param skeleton スケルトン参照
    /// @param worldTransform モデルのワールド変換
    /// @param targetWorldPos ターゲット位置（ワールド座標）
    /// @param weight ブレンド重み（0.0=FKのまま、1.0=完全IK）
    void Apply(XMFLOAT4X4* localTransforms,
               XMFLOAT4X4* globalTransforms,
               const Skeleton& skeleton,
               const Transform3D& worldTransform,
               const XMFLOAT3& targetWorldPos,
               float weight = 1.0f);

    /// @brief 視線IKの有効/無効を切り替える
    /// @param enabled 有効にするか
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief 視線IKが有効か
    /// @return 有効ならtrue
    bool IsEnabled() const { return m_enabled; }

    /// @brief 最大回転角度を設定する（ラジアン）
    /// @param radians 最大角度
    void SetMaxAngle(float radians) { m_maxAngle = radians; }

    /// @brief 最大回転角度を取得する
    /// @return 最大角度（ラジアン）
    float GetMaxAngle() const { return m_maxAngle; }

    /// @brief セットアップ済みか
    /// @return セットアップ済みならtrue
    bool IsSetup() const { return m_headJointIndex >= 0; }

private:
    /// @brief 1つのジョイントをターゲット方向に回転させる
    /// @param jointIndex 回転するジョイントのインデックス
    /// @param targetModelPos ターゲット位置（モデル空間）
    /// @param maxAngle 最大回転角度（ラジアン）
    /// @param weight ブレンド重み
    /// @param skeleton スケルトン参照
    /// @param localTransforms ローカル変換配列（入出力）
    /// @param globalTransforms グローバル変換配列（入出力）
    void RotateJointToward(int jointIndex, const XMFLOAT3& targetModelPos,
                           float maxAngle, float weight,
                           const Skeleton& skeleton,
                           XMFLOAT4X4* localTransforms,
                           XMFLOAT4X4* globalTransforms);

    int   m_headJointIndex = -1;
    int   m_neckJointIndex = -1;
    float m_maxAngle = XM_PIDIV4;   ///< 最大45度
    bool  m_enabled = true;
};

} // namespace GX
