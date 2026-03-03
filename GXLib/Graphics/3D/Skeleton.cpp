/// @file Skeleton.cpp
/// @brief スケルトンの実装
#include "pch_graphics.h"
#include "Graphics/3D/Skeleton.h"
#include "Math/MathConvert.h"

namespace gx
{

void Skeleton::ComputeGlobalTransforms(const Matrix4x4* localTransforms,
                                         Matrix4x4* globalTransforms) const
{
    // ジョイント配列は親が子より前にある前提（トポロジカル順序）で、
    // ルートから順にローカル行列を累積してグローバル行列を求める
    for (uint32_t i = 0; i < m_joints.size(); ++i)
    {
        XMMATRIX local = XMLoadFloat4x4(XM(&localTransforms[i]));
        if (m_joints[i].parentIndex >= 0)
        {
            XMMATRIX parent = XMLoadFloat4x4(XM(&globalTransforms[m_joints[i].parentIndex]));
            XMMATRIX global = local * parent;
            XMStoreFloat4x4(XM(&globalTransforms[i]), global);
        }
        else
        {
            XMStoreFloat4x4(XM(&globalTransforms[i]), local);
        }
    }
}

void Skeleton::ComputeBoneMatrices(const Matrix4x4* globalTransforms,
                                     Matrix4x4* boneMatrices) const
{
    // inverseBindMatrix でバインドポーズを打ち消し、現在のグローバル姿勢を適用する
    // HLSL側は列ベクトル規約（mul(float4,matrix)）のため、ここで転置して格納する
    for (uint32_t i = 0; i < m_joints.size(); ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(XM(&m_joints[i].inverseBindMatrix));
        XMMATRIX global  = XMLoadFloat4x4(XM(&globalTransforms[i]));
        XMMATRIX bone    = invBind * global;
        XMStoreFloat4x4(XM(&boneMatrices[i]), XMMatrixTranspose(bone));
    }
}

int Skeleton::FindJointIndex(const gx::String& name) const
{
    for (uint32_t i = 0; i < m_joints.size(); ++i)
    {
        if (m_joints[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace gx
