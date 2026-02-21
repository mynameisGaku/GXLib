#include "pch.h"
/// @file NavAgent.cpp
/// @brief Navigation agent implementation

#include "AI/NavAgent.h"
#include "AI/NavMesh.h"

namespace GX
{

// ============================================================================
// Initialize
// ============================================================================
void NavAgent::Initialize(NavMesh* navMesh)
{
    m_navMesh = navMesh;
    m_path.clear();
    m_currentPathIndex = 0;
    m_reached = false;
}

// ============================================================================
// SetDestination
// ============================================================================
void NavAgent::SetDestination(const XMFLOAT3& target)
{
    if (!m_navMesh || !m_navMesh->IsBuilt())
        return;

    m_path.clear();
    m_currentPathIndex = 0;
    m_reached = false;

    // Try to find a path
    if (!m_navMesh->FindPath(m_position, target, m_path))
    {
        // Path not found -- try to find nearest walkable to destination
        XMFLOAT3 nearTarget;
        if (m_navMesh->FindNearestWalkable(target, nearTarget))
        {
            m_navMesh->FindPath(m_position, nearTarget, m_path);
        }
    }

    // Skip the first waypoint if it is very close to current position
    // (it corresponds to the start cell)
    if (m_path.size() > 1)
    {
        float dx = m_path[0].x - m_position.x;
        float dz = m_path[0].z - m_position.z;
        float dist2 = dx * dx + dz * dz;
        if (dist2 < stoppingDistance * stoppingDistance * 4.0f)
            m_currentPathIndex = 1;
    }
}

// ============================================================================
// Stop
// ============================================================================
void NavAgent::Stop()
{
    m_path.clear();
    m_currentPathIndex = 0;
    m_reached = false;
}

// ============================================================================
// Update
// ============================================================================
void NavAgent::Update(float deltaTime)
{
    if (m_reached || m_path.empty())
        return;
    if (m_currentPathIndex >= static_cast<int>(m_path.size()))
    {
        m_reached = true;
        return;
    }

    // Current target waypoint
    XMFLOAT3 target = m_path[m_currentPathIndex];
    target.y = m_position.y; // Move on XZ plane; height will be set from navmesh

    float dx = target.x - m_position.x;
    float dz = target.z - m_position.z;
    float distSq = dx * dx + dz * dz;
    float dist   = std::sqrt(distSq);

    // Check if we've reached the current waypoint
    if (dist <= stoppingDistance)
    {
        ++m_currentPathIndex;
        if (m_currentPathIndex >= static_cast<int>(m_path.size()))
        {
            m_reached = true;
            return;
        }
        // Recalculate for next waypoint
        target = m_path[m_currentPathIndex];
        target.y = m_position.y;
        dx = target.x - m_position.x;
        dz = target.z - m_position.z;
        dist = std::sqrt(dx * dx + dz * dz);
        if (dist < 1e-6f) return;
    }

    // Compute desired yaw (atan2 gives angle from +Z axis in XZ plane)
    float desiredYaw = std::atan2(dx, dz);

    // Smooth rotation toward desired yaw
    float angularSpeedRad = angularSpeed * (XM_PI / 180.0f) * deltaTime;

    // Compute shortest angle difference
    float yawDiff = desiredYaw - m_yaw;

    // Wrap to [-PI, PI]
    while (yawDiff > XM_PI)  yawDiff -= XM_2PI;
    while (yawDiff < -XM_PI) yawDiff += XM_2PI;

    // Clamp rotation step
    if (std::abs(yawDiff) <= angularSpeedRad)
    {
        m_yaw = desiredYaw;
    }
    else
    {
        m_yaw += (yawDiff > 0.0f ? 1.0f : -1.0f) * angularSpeedRad;
    }

    // Wrap yaw to [-PI, PI]
    while (m_yaw > XM_PI)  m_yaw -= XM_2PI;
    while (m_yaw < -XM_PI) m_yaw += XM_2PI;

    // Move forward toward waypoint
    float moveStep = speed * deltaTime;
    if (moveStep > dist) moveStep = dist; // Don't overshoot

    float invDist = 1.0f / dist;
    m_position.x += dx * invDist * moveStep;
    m_position.z += dz * invDist * moveStep;

    // Update Y from navmesh cell height
    if (m_navMesh && m_navMesh->IsBuilt() && m_currentPathIndex < static_cast<int>(m_path.size()))
    {
        XMFLOAT3 wp = m_path[m_currentPathIndex];
        m_position.y = wp.y + height;
    }
}

} // namespace GX
