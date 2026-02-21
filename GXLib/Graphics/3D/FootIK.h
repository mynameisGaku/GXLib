#pragma once
/// @file FootIK.h
/// @brief 足IK（地形追従）
///
/// 2本の脚にCCD-IKを適用し、地面の高さに合わせて足を配置する。
/// 不整地を歩くキャラクターの足が地面に正しく接地するようになる。

#include "pch.h"
#include "Graphics/3D/IKSolver.h"
#include "Graphics/3D/Transform3D.h"

namespace GX
{

/// @brief 足IK（地形追従）
/// 左右の脚にIKを適用して、getGroundHeightコールバックから取得した
/// 地面高さに足を配置する。ヒップ補正も自動で行う。
class FootIK
{
public:
    /// @brief 足IKのジョイントを設定する
    /// @param skeleton スケルトン参照
    /// @param leftHipJoint 左ヒップ（大腿部）ジョイント名
    /// @param leftKneeJoint 左膝ジョイント名
    /// @param leftFootJoint 左足ジョイント名
    /// @param rightHipJoint 右ヒップジョイント名
    /// @param rightKneeJoint 右膝ジョイント名
    /// @param rightFootJoint 右足ジョイント名
    void Setup(const Skeleton& skeleton,
               const std::string& leftHipJoint, const std::string& leftKneeJoint,
               const std::string& leftFootJoint,
               const std::string& rightHipJoint, const std::string& rightKneeJoint,
               const std::string& rightFootJoint);

    /// @brief 足IKを適用する
    /// @param localTransforms ローカル変換行列配列（入出力）
    /// @param globalTransforms グローバル変換行列配列（入出力）
    /// @param skeleton スケルトン参照
    /// @param worldTransform モデルのワールド変換
    /// @param getGroundHeight 地面高さ取得関数 (x, z) → y
    void Apply(XMFLOAT4X4* localTransforms,
               XMFLOAT4X4* globalTransforms,
               const Skeleton& skeleton,
               const Transform3D& worldTransform,
               std::function<float(float x, float z)> getGroundHeight);

    /// @brief 足IKの有効/無効を切り替える
    /// @param enabled 有効にするか
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief 足IKが有効か
    /// @return 有効ならtrue
    bool IsEnabled() const { return m_enabled; }

    /// @brief 足底の地面からのオフセットを設定する
    /// @param offset オフセット値（ワールド単位）
    void SetFootOffset(float offset) { m_footOffset = offset; }

    /// @brief 足底オフセットを取得する
    /// @return オフセット値
    float GetFootOffset() const { return m_footOffset; }

    /// @brief IKの最大反復回数を設定する
    /// @param iterations 反復回数
    void SetMaxIterations(uint32_t iterations);

    /// @brief セットアップ済みか
    /// @return セットアップ済みならtrue
    bool IsSetup() const { return m_setup; }

private:
    CCDIKSolver m_solver;
    IKChain     m_leftLeg;
    IKChain     m_rightLeg;
    float       m_footOffset = 0.05f;
    bool        m_enabled = true;
    bool        m_setup = false;

    int m_leftFootIndex  = -1;
    int m_rightFootIndex = -1;
};

} // namespace GX
