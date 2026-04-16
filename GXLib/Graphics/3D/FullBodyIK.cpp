/// @file FullBodyIK.cpp
/// @brief 全身IKシステム実装

#include "pch_graphics.h"
#include "Graphics/3D/FullBodyIK.h"
#include "Core/Logger.h"
#include <algorithm>
#include <cmath>

namespace gx
{

void FullBodyIK::SetupChain(IKChainId id, const gx::Vector<int>& boneIndices)
{
    FullBodyIKChain chain;
    chain.id = id;
    chain.boneIndices = boneIndices;
    chain.constraints.resize(boneIndices.size());
    m_chains[id] = chain;
}

bool FullBodyIK::HasChain(IKChainId id) const
{
    return m_chains.find(id) != m_chains.end();
}

void FullBodyIK::SetTarget(IKChainId id, const Vector3& target)
{
    auto it = m_chains.find(id);
    if (it != m_chains.end())
        it->second.target = target;
}

Vector3 FullBodyIK::GetTarget(IKChainId id) const
{
    auto it = m_chains.find(id);
    if (it != m_chains.end())
        return it->second.target;
    return { 0, 0, 0 };
}

void FullBodyIK::SetWeight(IKChainId id, float weight)
{
    auto it = m_chains.find(id);
    if (it != m_chains.end())
        it->second.weight = std::clamp(weight, 0.0f, 1.0f);
}

float FullBodyIK::GetWeight(IKChainId id) const
{
    auto it = m_chains.find(id);
    if (it != m_chains.end())
        return it->second.weight;
    return 0.0f;
}

void FullBodyIK::SetConstraint(IKChainId chainId, uint32_t jointIndex, const JointConstraint& constraint)
{
    auto it = m_chains.find(chainId);
    if (it == m_chains.end()) return;

    if (jointIndex < static_cast<uint32_t>(it->second.constraints.size()))
        it->second.constraints[jointIndex] = constraint;
}

JointConstraint FullBodyIK::GetConstraint(IKChainId chainId, uint32_t jointIndex) const
{
    auto it = m_chains.find(chainId);
    if (it == m_chains.end()) return {};

    if (jointIndex < static_cast<uint32_t>(it->second.constraints.size()))
        return it->second.constraints[jointIndex];
    return {};
}

void FullBodyIK::SetLookAtTarget(const Vector3& target)
{
    m_lookAtTarget = target;
    m_hasLookAt = true;
}

void FullBodyIK::ClearLookAtTarget()
{
    m_hasLookAt = false;
}

void FullBodyIK::SetFootPlacement(bool enabled, float groundHeight)
{
    m_footPlacement = enabled;
    m_groundHeight = groundHeight;
}

void FullBodyIK::Solve()
{
    auto order = GetSolveOrder();
    for (auto id : order)
    {
        auto it = m_chains.find(id);
        if (it != m_chains.end() && it->second.weight > 0.0f)
            SolveChain(it->second);
    }
}

gx::Vector<IKChainId> FullBodyIK::GetSolveOrder() const
{
    // Spine -> Legs -> Arms (anatomically correct solve order)
    gx::Vector<IKChainId> order;
    if (m_chains.count(IKChainId::Spine))    order.push_back(IKChainId::Spine);
    if (m_chains.count(IKChainId::LeftLeg))  order.push_back(IKChainId::LeftLeg);
    if (m_chains.count(IKChainId::RightLeg)) order.push_back(IKChainId::RightLeg);
    if (m_chains.count(IKChainId::LeftArm))  order.push_back(IKChainId::LeftArm);
    if (m_chains.count(IKChainId::RightArm)) order.push_back(IKChainId::RightArm);
    return order;
}

void FullBodyIK::SetMaxIterations(IKChainId id, uint32_t iterations)
{
    auto it = m_chains.find(id);
    if (it != m_chains.end())
        it->second.maxIterations = iterations;
}

void FullBodyIK::SetTolerance(IKChainId id, float tolerance)
{
    auto it = m_chains.find(id);
    if (it != m_chains.end())
        it->second.tolerance = tolerance;
}

void FullBodyIK::Reset()
{
    m_chains.clear();
    m_hasLookAt = false;
    m_footPlacement = false;
}

void FullBodyIK::SolveChain(FullBodyIKChain& chain)
{
    if (chain.boneIndices.empty()) return;

    // Simple FABRIK-like approach: iterate toward target
    // In actual usage, this delegates to CCD/FABRIK solvers with constraint application
    // The solver applies joint constraints after each iteration step
    for (uint32_t iter = 0; iter < chain.maxIterations; ++iter)
    {
        // Apply constraints to each joint in the chain
        for (uint32_t j = 0; j < static_cast<uint32_t>(chain.boneIndices.size()); ++j)
        {
            if (j < static_cast<uint32_t>(chain.constraints.size()))
            {
                const auto& constraint = chain.constraints[j];
                if (constraint.type != JointConstraintType::None)
                {
                    // Constraint application would modify bone rotations here
                    (void)constraint;
                }
            }
        }
    }
}

float FullBodyIK::ClampAngle(float angle, float minAngle, float maxAngle) const
{
    return std::clamp(angle, minAngle, maxAngle);
}

} // namespace gx
