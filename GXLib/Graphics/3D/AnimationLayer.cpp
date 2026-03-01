/// @file AnimationLayer.cpp
/// @brief アニメーションレイヤーシステム実装

#include "pch_graphics.h"
#include "Graphics/3D/AnimationLayer.h"
#include <algorithm>
#include <cmath>

namespace gx
{

// =============================================================================
// BoneMask
// =============================================================================

void BoneMask::Initialize(uint32_t boneCount)
{
    m_weights.assign(boneCount, 1.0f);
}

void BoneMask::SetWeight(uint32_t boneIndex, float weight)
{
    if (boneIndex < static_cast<uint32_t>(m_weights.size()))
        m_weights[boneIndex] = std::clamp(weight, 0.0f, 1.0f);
}

float BoneMask::GetWeight(uint32_t boneIndex) const
{
    if (boneIndex < static_cast<uint32_t>(m_weights.size()))
        return m_weights[boneIndex];
    return 0.0f;
}

void BoneMask::SetAll(float weight)
{
    float clamped = std::clamp(weight, 0.0f, 1.0f);
    for (auto& w : m_weights)
        w = clamped;
}

void BoneMask::SetFromJointDown(uint32_t startBone, uint32_t endBone, float weight)
{
    float clamped = std::clamp(weight, 0.0f, 1.0f);
    uint32_t count = static_cast<uint32_t>(m_weights.size());
    uint32_t end = std::min(endBone + 1, count);
    for (uint32_t i = startBone; i < end; ++i)
        m_weights[i] = clamped;
}

void BoneMask::ClearAll()
{
    for (auto& w : m_weights)
        w = 0.0f;
}

bool BoneMask::IsActive() const
{
    for (float w : m_weights)
    {
        if (w > 0.0f)
            return true;
    }
    return false;
}

// =============================================================================
// AnimationLayerStack
// =============================================================================

uint32_t AnimationLayerStack::AddLayer(const std::string& name, AnimLayerBlendMode mode)
{
    AnimationLayer layer;
    layer.name = name;
    layer.blendMode = mode;
    layer.weight = 1.0f;
    layer.enabled = true;
    m_layers.push_back(std::move(layer));
    return static_cast<uint32_t>(m_layers.size() - 1);
}

void AnimationLayerStack::RemoveLayer(uint32_t index)
{
    if (index < static_cast<uint32_t>(m_layers.size()))
        m_layers.erase(m_layers.begin() + index);
}

AnimationLayer* AnimationLayerStack::GetLayer(uint32_t index)
{
    if (index < static_cast<uint32_t>(m_layers.size()))
        return &m_layers[index];
    return nullptr;
}

const AnimationLayer* AnimationLayerStack::GetLayer(uint32_t index) const
{
    if (index < static_cast<uint32_t>(m_layers.size()))
        return &m_layers[index];
    return nullptr;
}

int AnimationLayerStack::FindLayer(const std::string& name) const
{
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_layers.size()); ++i)
    {
        if (m_layers[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

void AnimationLayerStack::SetLayerWeight(uint32_t index, float weight)
{
    if (index < static_cast<uint32_t>(m_layers.size()))
        m_layers[index].weight = std::clamp(weight, 0.0f, 1.0f);
}

void AnimationLayerStack::SetLayerEnabled(uint32_t index, bool enabled)
{
    if (index < static_cast<uint32_t>(m_layers.size()))
        m_layers[index].enabled = enabled;
}

void AnimationLayerStack::Evaluate(BonePose* basePose,
                                   const std::vector<std::vector<BonePose>>& layerPoses,
                                   uint32_t boneCount) const
{
    if (!basePose) return;

    for (uint32_t layerIdx = 0; layerIdx < static_cast<uint32_t>(m_layers.size()); ++layerIdx)
    {
        const auto& layer = m_layers[layerIdx];
        if (!layer.enabled || layer.weight <= 0.0f)
            continue;

        if (layerIdx >= static_cast<uint32_t>(layerPoses.size()))
            continue;

        const auto& poses = layerPoses[layerIdx];

        for (uint32_t boneIdx = 0; boneIdx < boneCount; ++boneIdx)
        {
            if (boneIdx >= static_cast<uint32_t>(poses.size()))
                continue;

            // Compute effective weight: layer weight * bone mask weight
            float maskWeight = 1.0f;
            if (layer.boneMask.GetBoneCount() > 0)
                maskWeight = layer.boneMask.GetWeight(boneIdx);

            float effectiveWeight = layer.weight * maskWeight;
            if (effectiveWeight <= 0.0f)
                continue;

            const auto& layerPose = poses[boneIdx];
            auto& base = basePose[boneIdx];

            if (layer.blendMode == AnimLayerBlendMode::Override)
            {
                // Override: lerp(base, layer, weight)
                float t = effectiveWeight;
                float invT = 1.0f - t;

                base.position.x = invT * base.position.x + t * layerPose.position.x;
                base.position.y = invT * base.position.y + t * layerPose.position.y;
                base.position.z = invT * base.position.z + t * layerPose.position.z;

                base.scale.x = invT * base.scale.x + t * layerPose.scale.x;
                base.scale.y = invT * base.scale.y + t * layerPose.scale.y;
                base.scale.z = invT * base.scale.z + t * layerPose.scale.z;

                // Quaternion slerp approximation using nlerp
                DirectX::XMVECTOR q0 = DirectX::XMLoadFloat4(&base.rotation);
                DirectX::XMVECTOR q1 = DirectX::XMLoadFloat4(&layerPose.rotation);

                // Ensure shortest path
                float dot = base.rotation.x * layerPose.rotation.x +
                            base.rotation.y * layerPose.rotation.y +
                            base.rotation.z * layerPose.rotation.z +
                            base.rotation.w * layerPose.rotation.w;
                if (dot < 0.0f)
                    q1 = DirectX::XMVectorNegate(q1);

                DirectX::XMVECTOR result = DirectX::XMVectorLerp(q0, q1, t);
                result = DirectX::XMVector4Normalize(result);
                DirectX::XMStoreFloat4(&base.rotation, result);
            }
            else // Additive
            {
                // Additive: base.position += (layer.position - ref.position) * weight
                // Reference pose is identity (position=0, rotation=identity, scale=1)
                base.position.x += layerPose.position.x * effectiveWeight;
                base.position.y += layerPose.position.y * effectiveWeight;
                base.position.z += layerPose.position.z * effectiveWeight;

                base.scale.x += (layerPose.scale.x - 1.0f) * effectiveWeight;
                base.scale.y += (layerPose.scale.y - 1.0f) * effectiveWeight;
                base.scale.z += (layerPose.scale.z - 1.0f) * effectiveWeight;

                // Additive quaternion: multiply base by (slerp(identity, layerQ, weight))
                DirectX::XMVECTOR identity = DirectX::XMQuaternionIdentity();
                DirectX::XMVECTOR layerQ = DirectX::XMLoadFloat4(&layerPose.rotation);
                DirectX::XMVECTOR additiveQ = DirectX::XMQuaternionSlerp(identity, layerQ, effectiveWeight);
                DirectX::XMVECTOR baseQ = DirectX::XMLoadFloat4(&base.rotation);
                DirectX::XMVECTOR result = DirectX::XMQuaternionMultiply(baseQ, additiveQ);
                result = DirectX::XMQuaternionNormalize(result);
                DirectX::XMStoreFloat4(&base.rotation, result);
            }
        }
    }
}

void AnimationLayerStack::Clear()
{
    m_layers.clear();
}

} // namespace gx
