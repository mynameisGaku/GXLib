/// @file FootIK.cpp
/// @brief 足IKの実装
#include "pch_graphics.h"
#include "Graphics/3D/FootIK.h"
#include "Math/MathConvert.h"
#include "Core/Logger.h"

namespace gx
{

void FootIK::Setup(const Skeleton& skeleton,
                   const gx::String& leftHipJoint, const gx::String& leftKneeJoint,
                   const gx::String& leftFootJoint,
                   const gx::String& rightHipJoint, const gx::String& rightKneeJoint,
                   const gx::String& rightFootJoint)
{
    int leftHipIdx   = skeleton.FindJointIndex(leftHipJoint);
    int leftKneeIdx  = skeleton.FindJointIndex(leftKneeJoint);
    int leftFootIdx  = skeleton.FindJointIndex(leftFootJoint);
    int rightHipIdx  = skeleton.FindJointIndex(rightHipJoint);
    int rightKneeIdx = skeleton.FindJointIndex(rightKneeJoint);
    int rightFootIdx = skeleton.FindJointIndex(rightFootJoint);

    if (leftHipIdx < 0 || leftKneeIdx < 0 || leftFootIdx < 0 ||
        rightHipIdx < 0 || rightKneeIdx < 0 || rightFootIdx < 0)
    {
        GX_LOG_WARN("FootIK: joint not found, IK disabled");
        m_setup = false;
        return;
    }

    // 左脚チェーン: ヒップ→膝→足
    m_leftLeg.jointIndices = { leftHipIdx, leftKneeIdx };
    m_leftLeg.effectorIndex = leftFootIdx;
    m_leftLeg.tolerance = 0.005f;
    m_leftLeg.maxIterations = 15;
    m_leftFootIndex = leftFootIdx;

    // 右脚チェーン: ヒップ→膝→足
    m_rightLeg.jointIndices = { rightHipIdx, rightKneeIdx };
    m_rightLeg.effectorIndex = rightFootIdx;
    m_rightLeg.tolerance = 0.005f;
    m_rightLeg.maxIterations = 15;
    m_rightFootIndex = rightFootIdx;

    m_setup = true;
}

void FootIK::SetMaxIterations(uint32_t iterations)
{
    m_leftLeg.maxIterations = iterations;
    m_rightLeg.maxIterations = iterations;
}

void FootIK::Apply(Matrix4x4* localTransforms,
                   Matrix4x4* globalTransforms,
                   const Skeleton& skeleton,
                   const Transform3D& worldTransform,
                   std::function<float(float x, float z)> getGroundHeight)
{
    if (!m_enabled || !m_setup || !getGroundHeight)
        return;

    // モデルのワールド変換行列を取得
    XMMATRIX worldMat = ToXMMATRIX(worldTransform.GetWorldMatrix());

    // 左足のワールド位置を取得して地面高さを計算
    auto processLeg = [&](IKChain& chain, int footIndex)
    {
        // 足ジョイントのモデル空間位置
        const Matrix4x4& footGlobal = globalTransforms[footIndex];
        XMVECTOR footModelPos = XMVectorSet(footGlobal._41, footGlobal._42, footGlobal._43, 1.0f);

        // ワールド空間に変換
        XMVECTOR footWorldPos = XMVector3Transform(footModelPos, worldMat);
        Vector3 footWorld;
        XMStoreFloat3(XM(&footWorld), footWorldPos);

        // 地面の高さを取得
        float groundY = getGroundHeight(footWorld.x, footWorld.z);

        // ターゲット: 足を地面高さ+オフセットに配置
        float targetWorldY = groundY + m_footOffset;

        // 足が既に地面より下にある場合のみIKを適用（浮いている足は押し下げない）
        if (footWorld.y > targetWorldY + 0.01f)
            return;

        // ワールド空間のターゲットをモデル空間に逆変換
        XMMATRIX worldInv = XMMatrixInverse(nullptr, worldMat);
        XMVECTOR targetWorld = XMVectorSet(footWorld.x, targetWorldY, footWorld.z, 1.0f);
        XMVECTOR targetModel = XMVector3Transform(targetWorld, worldInv);
        Vector3 targetModelPos;
        XMStoreFloat3(XM(&targetModelPos), targetModel);

        // CCD-IKでジョイントを回転
        m_solver.Solve(chain, targetModelPos, skeleton, localTransforms, globalTransforms);
    };

    processLeg(m_leftLeg, m_leftFootIndex);
    processLeg(m_rightLeg, m_rightFootIndex);
}

} // namespace gx
