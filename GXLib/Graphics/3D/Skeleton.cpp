/// @file Skeleton.cpp
/// @brief スケルトンの実装
#include "pch.h"
#include "Graphics/3D/Skeleton.h"

namespace GX
{

void Skeleton::ComputeGlobalTransforms(const XMFLOAT4X4* localTransforms,
                                         XMFLOAT4X4* globalTransforms) const
{
    for (uint32_t i = 0; i < m_joints.size(); ++i)
    {
        XMMATRIX local = XMLoadFloat4x4(&localTransforms[i]);
        if (m_joints[i].parentIndex >= 0)
        {
            XMMATRIX parent = XMLoadFloat4x4(&globalTransforms[m_joints[i].parentIndex]);
            XMMATRIX global = local * parent;
            XMStoreFloat4x4(&globalTransforms[i], global);
        }
        else
        {
            XMStoreFloat4x4(&globalTransforms[i], local);
        }
    }
}

void Skeleton::ComputeBoneMatrices(const XMFLOAT4X4* globalTransforms,
                                     XMFLOAT4X4* boneMatrices) const
{
    for (uint32_t i = 0; i < m_joints.size(); ++i)
    {
        XMMATRIX invBind = XMLoadFloat4x4(&m_joints[i].inverseBindMatrix);
        XMMATRIX global  = XMLoadFloat4x4(&globalTransforms[i]);
        XMMATRIX bone    = invBind * global;
        XMStoreFloat4x4(&boneMatrices[i], XMMatrixTranspose(bone));
    }
}

int Skeleton::FindJointIndex(const std::string& name) const
{
    for (uint32_t i = 0; i < m_joints.size(); ++i)
    {
        if (m_joints[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace GX
