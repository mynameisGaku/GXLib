#pragma once
/// @file NavAgent.h
/// @brief Navigation agent that follows paths on a NavMesh
///
/// Automatically computes a path to a destination using NavMesh::FindPath,
/// then smoothly moves and rotates along the waypoints each frame.

#include "pch.h"

namespace GX
{

class NavMesh;

/// @brief Agent that moves along NavMesh paths
class NavAgent
{
public:
    NavAgent() = default;
    ~NavAgent() = default;

    /// @brief Associate this agent with a NavMesh
    void Initialize(NavMesh* navMesh);

    /// @brief Set a destination and compute a path to it
    void SetDestination(const XMFLOAT3& target);

    /// @brief Update the agent (move along the path each frame)
    /// @param deltaTime Frame delta time in seconds
    void Update(float deltaTime);

    /// @brief Stop the agent and clear its path
    void Stop();

    /// @brief Get the current world position
    XMFLOAT3 GetPosition() const { return m_position; }

    /// @brief Set the current world position
    void SetPosition(const XMFLOAT3& pos) { m_position = pos; }

    /// @brief Get the current Y-axis rotation (radians)
    float GetYaw() const { return m_yaw; }

    /// @brief Set the current Y-axis rotation (radians)
    void SetYaw(float yaw) { m_yaw = yaw; }

    /// @brief Check if the agent currently has a valid path
    bool HasPath() const { return !m_path.empty() && m_currentPathIndex < static_cast<int>(m_path.size()); }

    /// @brief Check if the agent has reached its destination
    bool HasReachedDestination() const { return m_reached; }

    /// @brief Get the computed path waypoints
    const std::vector<XMFLOAT3>& GetPath() const { return m_path; }

    /// @brief Get the current target waypoint index
    int GetCurrentWaypointIndex() const { return m_currentPathIndex; }

    // -- Public parameters --

    float speed            = 3.5f;    ///< Movement speed (units/sec)
    float angularSpeed     = 360.0f;  ///< Rotation speed (degrees/sec)
    float stoppingDistance = 0.15f;   ///< Distance to waypoint for advancement
    float height           = 0.0f;    ///< Agent Y offset above navmesh surface

private:
    NavMesh* m_navMesh = nullptr;
    std::vector<XMFLOAT3> m_path;
    int      m_currentPathIndex = 0;
    XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
    float    m_yaw      = 0.0f;
    bool     m_reached  = false;
};

} // namespace GX
