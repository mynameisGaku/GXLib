/// @file LookAtIK.cpp
/// @brief 視線IKの実装
#include "pch.h"
#include "Graphics/3D/LookAtIK.h"
#include "Core/Logger.h"

namespace GX
{

void LookAtIK::Setup(const Skeleton& skeleton,
                     const std::string& headJoint,
                     const std::string& neckJoint)
{
    m_headJointIndex = skeleton.FindJointIndex(headJoint);
    if (m_headJointIndex < 0)
    {
        GX_LOG_WARN("LookAtIK: head joint '%s' not found", headJoint.c_str());
        return;
    }

    if (!neckJoint.empty())
    {
        m_neckJointIndex = skeleton.FindJointIndex(neckJoint);
        if (m_neckJointIndex < 0)
            GX_LOG_WARN("LookAtIK: neck joint '%s' not found, using head only", neckJoint.c_str());
    }
}

void LookAtIK::RotateJointToward(int jointIndex, const XMFLOAT3& targetModelPos,
                                  float maxAngle, float weight,
                                  const Skeleton& skeleton,
                                  XMFLOAT4X4* localTransforms,
                                  XMFLOAT4X4* globalTransforms)
{
    const auto& joints = skeleton.GetJoints();

    // ジョイントのグローバル位置
    const XMFLOAT4X4& jointGlobal = globalTransforms[jointIndex];
    XMVECTOR jointPos = XMVectorSet(jointGlobal._41, jointGlobal._42, jointGlobal._43, 1.0f);

    // ターゲットへの方向
    XMVECTOR targetPos = XMLoadFloat3(&targetModelPos);
    XMVECTOR toTarget = XMVectorSubtract(targetPos, jointPos);
    float dist = XMVectorGetX(XMVector3Length(toTarget));
    if (dist < 0.001f)
        return;

    toTarget = XMVector3Normalize(toTarget);

    // ジョイントの現在の前方向（グローバル変換のY軸を使用 — 一般的なボーン方向）
    // 多くのモデルではボーンのローカルY軸が「上」方向を指す
    // ここでは汎用的にジョイント→子ジョイント方向を前方向として使う
    XMVECTOR forward;

    // 子ジョイントを探す
    int childIndex = -1;
    for (uint32_t i = 0; i < skeleton.GetJointCount(); ++i)
    {
        if (joints[i].parentIndex == jointIndex)
        {
            childIndex = static_cast<int>(i);
            break;
        }
    }

    if (childIndex >= 0)
    {
        XMVECTOR childPos = XMVectorSet(
            globalTransforms[childIndex]._41,
            globalTransforms[childIndex]._42,
            globalTransforms[childIndex]._43, 1.0f);
        forward = XMVector3Normalize(XMVectorSubtract(childPos, jointPos));
    }
    else
    {
        // 子がない場合はグローバル変換のY軸を使用
        forward = XMVectorSet(jointGlobal._21, jointGlobal._22, jointGlobal._23, 0.0f);
        forward = XMVector3Normalize(forward);
    }

    // 回転角度を計算
    float cosAngle = XMVectorGetX(XMVector3Dot(forward, toTarget));
    cosAngle = (std::max)(-1.0f, (std::min)(1.0f, cosAngle));
    float angle = acosf(cosAngle);

    // 角度制限
    if (angle > maxAngle)
        angle = maxAngle;

    // ほぼ同じ方向ならスキップ
    if (angle < 0.001f)
        return;

    // ブレンド重みを適用
    angle *= weight;

    // 回転軸
    XMVECTOR axis = XMVector3Cross(forward, toTarget);
    float axisLen = XMVectorGetX(XMVector3Length(axis));
    if (axisLen < 0.00001f)
        return;
    axis = XMVector3Normalize(axis);

    // ワールド空間での回転クォータニオン
    XMVECTOR worldRotation = XMQuaternionRotationAxis(axis, angle);

    // ジョイントのグローバル回転を取得
    XMMATRIX jointGlobalMat = XMLoadFloat4x4(&globalTransforms[jointIndex]);
    XMVECTOR gScale, gRot, gTrans;
    XMMatrixDecompose(&gScale, &gRot, &gTrans, jointGlobalMat);

    // 新しいグローバル回転
    XMVECTOR newGlobalRot = XMQuaternionNormalize(XMQuaternionMultiply(gRot, worldRotation));

    // 親のグローバル回転を取得
    int parentIdx = joints[jointIndex].parentIndex;
    XMVECTOR parentGlobalRot = XMQuaternionIdentity();
    if (parentIdx >= 0)
    {
        XMMATRIX parentGlobal = XMLoadFloat4x4(&globalTransforms[parentIdx]);
        XMVECTOR ps, pr, pt;
        XMMatrixDecompose(&ps, &pr, &pt, parentGlobal);
        parentGlobalRot = pr;
    }

    // ローカル回転に変換
    XMVECTOR newLocalRot = XMQuaternionNormalize(
        XMQuaternionMultiply(XMQuaternionInverse(parentGlobalRot), newGlobalRot));

    // ローカル変換行列を再構成
    XMMATRIX localMat = XMLoadFloat4x4(&localTransforms[jointIndex]);
    XMVECTOR lScale, lRot, lTrans;
    XMMatrixDecompose(&lScale, &lRot, &lTrans, localMat);

    XMMATRIX S = XMMatrixScalingFromVector(lScale);
    XMMATRIX R = XMMatrixRotationQuaternion(newLocalRot);
    XMMATRIX T = XMMatrixTranslationFromVector(lTrans);
    XMStoreFloat4x4(&localTransforms[jointIndex], S * R * T);

    // このジョイント以下のFKを再計算
    uint32_t jointCount = skeleton.GetJointCount();
    for (uint32_t i = static_cast<uint32_t>(jointIndex); i < jointCount; ++i)
    {
        XMMATRIX lm = XMLoadFloat4x4(&localTransforms[i]);
        if (joints[i].parentIndex >= 0)
        {
            XMMATRIX pm = XMLoadFloat4x4(&globalTransforms[joints[i].parentIndex]);
            XMStoreFloat4x4(&globalTransforms[i], lm * pm);
        }
        else
        {
            XMStoreFloat4x4(&globalTransforms[i], lm);
        }
    }
}

void LookAtIK::Apply(XMFLOAT4X4* localTransforms,
                     XMFLOAT4X4* globalTransforms,
                     const Skeleton& skeleton,
                     const Transform3D& worldTransform,
                     const XMFLOAT3& targetWorldPos,
                     float weight)
{
    if (!m_enabled || m_headJointIndex < 0)
        return;

    weight = (std::max)(0.0f, (std::min)(1.0f, weight));
    if (weight < 0.001f)
        return;

    // ワールド空間のターゲットをモデル空間に逆変換
    XMMATRIX worldMat = worldTransform.GetWorldMatrix();
    XMMATRIX worldInv = XMMatrixInverse(nullptr, worldMat);
    XMVECTOR targetWorld = XMLoadFloat3(&targetWorldPos);
    XMVECTOR targetModel = XMVector3Transform(targetWorld, worldInv);
    XMFLOAT3 targetModelPos;
    XMStoreFloat3(&targetModelPos, targetModel);

    // 首がある場合、首→頭の順に回転（首が先に半分回転し、頭が残りを補う）
    if (m_neckJointIndex >= 0)
    {
        float neckWeight = weight * 0.4f;
        float headWeight = weight * 0.6f;

        RotateJointToward(m_neckJointIndex, targetModelPos,
                          m_maxAngle * 0.5f, neckWeight,
                          skeleton, localTransforms, globalTransforms);

        RotateJointToward(m_headJointIndex, targetModelPos,
                          m_maxAngle, headWeight,
                          skeleton, localTransforms, globalTransforms);
    }
    else
    {
        RotateJointToward(m_headJointIndex, targetModelPos,
                          m_maxAngle, weight,
                          skeleton, localTransforms, globalTransforms);
    }
}

} // namespace GX
