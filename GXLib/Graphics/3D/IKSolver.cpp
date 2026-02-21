/// @file IKSolver.cpp
/// @brief CCD-IKソルバーの実装
#include "pch.h"
#include "Graphics/3D/IKSolver.h"

namespace GX
{

XMVECTOR CCDIKSolver::GetJointPosition(const XMFLOAT4X4* globalTransforms, int jointIndex)
{
    const XMFLOAT4X4& m = globalTransforms[jointIndex];
    return XMVectorSet(m._41, m._42, m._43, 1.0f);
}

bool CCDIKSolver::Solve(const IKChain& chain, const XMFLOAT3& targetPos,
                         const Skeleton& skeleton,
                         XMFLOAT4X4* localTransforms,
                         XMFLOAT4X4* globalTransforms)
{
    if (chain.jointIndices.empty() || chain.effectorIndex < 0)
        return false;

    XMVECTOR target = XMLoadFloat3(&targetPos);

    for (uint32_t iter = 0; iter < chain.maxIterations; ++iter)
    {
        // エフェクタのワールド位置を取得
        XMVECTOR effectorPos = GetJointPosition(globalTransforms, chain.effectorIndex);

        // 収束判定
        float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(effectorPos, target)));
        if (dist < chain.tolerance)
            return true;

        // エフェクタ側（末端）からルート側（根本）へジョイントを走査
        for (int c = static_cast<int>(chain.jointIndices.size()) - 1; c >= 0; --c)
        {
            int jointIdx = chain.jointIndices[c];

            // ジョイントのワールド位置
            XMVECTOR jointPos = GetJointPosition(globalTransforms, jointIdx);

            // 現在のエフェクタ位置（反復中に変化する）
            effectorPos = GetJointPosition(globalTransforms, chain.effectorIndex);

            // ジョイント→エフェクタ と ジョイント→ターゲット のベクトル
            XMVECTOR toEffector = XMVector3Normalize(XMVectorSubtract(effectorPos, jointPos));
            XMVECTOR toTarget   = XMVector3Normalize(XMVectorSubtract(target, jointPos));

            // 回転角度を計算
            float cosAngle = XMVectorGetX(XMVector3Dot(toEffector, toTarget));
            cosAngle = (std::max)(-1.0f, (std::min)(1.0f, cosAngle));

            // ほぼ同じ方向ならスキップ
            if (cosAngle > 0.9999f)
                continue;

            float angle = acosf(cosAngle);

            // 回転軸
            XMVECTOR axis = XMVector3Cross(toEffector, toTarget);
            float axisLen = XMVectorGetX(XMVector3Length(axis));
            if (axisLen < 0.00001f)
                continue;

            axis = XMVector3Normalize(axis);

            // ワールド空間での回転クォータニオン
            XMVECTOR worldRotation = XMQuaternionRotationAxis(axis, angle);

            // ワールド空間の回転をローカル空間に変換する:
            // ジョイントの親のグローバル変換の逆を使って、ワールド回転をローカルに変換
            const auto& joints = skeleton.GetJoints();
            int parentIdx = joints[jointIdx].parentIndex;

            XMMATRIX parentGlobalInv = XMMatrixIdentity();
            if (parentIdx >= 0)
                parentGlobalInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&globalTransforms[parentIdx]));

            // 現在のジョイントのグローバル回転を取得
            XMMATRIX jointGlobal = XMLoadFloat4x4(&globalTransforms[jointIdx]);
            XMVECTOR jointGlobalScale, jointGlobalRot, jointGlobalTrans;
            XMMatrixDecompose(&jointGlobalScale, &jointGlobalRot, &jointGlobalTrans, jointGlobal);

            // ワールド空間で回転を適用した新しいグローバル回転
            XMVECTOR newGlobalRot = XMQuaternionMultiply(jointGlobalRot, worldRotation);
            newGlobalRot = XMQuaternionNormalize(newGlobalRot);

            // ローカル変換を更新:
            // 現在のローカル変換を分解 → 回転だけ差し替え → 再構築
            XMMATRIX localMat = XMLoadFloat4x4(&localTransforms[jointIdx]);
            XMVECTOR localScale, localRot, localTrans;
            XMMatrixDecompose(&localScale, &localRot, &localTrans, localMat);

            // ローカル空間での差分回転:
            // newGlobalRot = parentGlobalRot * newLocalRot
            // newLocalRot = inv(parentGlobalRot) * newGlobalRot
            XMVECTOR parentGlobalRot = XMQuaternionIdentity();
            if (parentIdx >= 0)
            {
                XMMATRIX parentGlobal = XMLoadFloat4x4(&globalTransforms[parentIdx]);
                XMVECTOR ps, pr, pt;
                XMMatrixDecompose(&ps, &pr, &pt, parentGlobal);
                parentGlobalRot = pr;
            }

            XMVECTOR newLocalRot = XMQuaternionMultiply(XMQuaternionInverse(parentGlobalRot), newGlobalRot);
            newLocalRot = XMQuaternionNormalize(newLocalRot);

            // ローカル変換行列を再構成
            XMMATRIX S = XMMatrixScalingFromVector(localScale);
            XMMATRIX R = XMMatrixRotationQuaternion(newLocalRot);
            XMMATRIX T = XMMatrixTranslationFromVector(localTrans);
            XMStoreFloat4x4(&localTransforms[jointIdx], S * R * T);

            // このジョイント以下のFKを再計算
            RecomputeFK(jointIdx, skeleton, localTransforms, globalTransforms);
        }
    }

    // 最大反復でも収束しなかった場合
    XMVECTOR effectorPos = GetJointPosition(globalTransforms, chain.effectorIndex);
    float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(effectorPos, target)));
    return dist < chain.tolerance;
}

void CCDIKSolver::RecomputeFK(int fromJoint, const Skeleton& skeleton,
                               const XMFLOAT4X4* localTransforms,
                               XMFLOAT4X4* globalTransforms)
{
    const auto& joints = skeleton.GetJoints();
    uint32_t jointCount = skeleton.GetJointCount();

    // fromJoint から始めて、子孫を順にFK再計算
    // ジョイントはトポロジカル順（親が子より前）と仮定
    for (uint32_t i = static_cast<uint32_t>(fromJoint); i < jointCount; ++i)
    {
        XMMATRIX localMat = XMLoadFloat4x4(&localTransforms[i]);

        if (joints[i].parentIndex >= 0)
        {
            XMMATRIX parentGlobal = XMLoadFloat4x4(&globalTransforms[joints[i].parentIndex]);
            XMStoreFloat4x4(&globalTransforms[i], localMat * parentGlobal);
        }
        else
        {
            XMStoreFloat4x4(&globalTransforms[i], localMat);
        }
    }
}

} // namespace GX
