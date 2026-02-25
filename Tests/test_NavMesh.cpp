/// @file test_NavMesh.cpp
/// @brief NavMesh A* pathfinding + NavAgent tests

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"

using namespace GX;

TEST(NavMeshTest, Build_FlatGrid)
{
    NavMesh mesh;
    bool ok = mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(mesh.IsBuilt());
}

TEST(NavMeshTest, Build_GridDimensions)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    // 10.0 / 0.5 = 20 cells in each direction
    EXPECT_EQ(mesh.GetGridWidth(), 20);
    EXPECT_EQ(mesh.GetGridHeight(), 20);
    EXPECT_NEAR(mesh.GetCellSize(), 0.5f, 1e-5f);
}

TEST(NavMeshTest, IsWalkable_Default)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    // All cells should be walkable by default
    EXPECT_TRUE(mesh.IsWalkable({5.0f, 0.0f, 5.0f}));
    EXPECT_TRUE(mesh.IsWalkable({0.25f, 0.0f, 0.25f}));
}

TEST(NavMeshTest, SetCellWalkable_False)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    mesh.SetCellWalkable(10, 10, false);
    // Cell (10,10) corresponds to world position (5.0, 0, 5.0) approximately
    EXPECT_FALSE(mesh.IsWalkable({5.25f, 0.0f, 5.25f}));
}

TEST(NavMeshTest, FindPath_StraightLine)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    std::vector<XMFLOAT3> path;
    bool found = mesh.FindPath({1.0f, 0.0f, 1.0f}, {8.0f, 0.0f, 8.0f}, path);
    EXPECT_TRUE(found);
    EXPECT_GT(path.size(), 0u);
}

TEST(NavMeshTest, FindPath_AroundObstacle)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    // Create a wall across the middle
    for (int x = 0; x < 20; ++x)
    {
        if (x != 0 && x != 19)  // Leave openings at edges
            mesh.SetCellWalkable(x, 10, false);
    }
    std::vector<XMFLOAT3> path;
    bool found = mesh.FindPath({5.0f, 0.0f, 2.0f}, {5.0f, 0.0f, 8.0f}, path);
    EXPECT_TRUE(found);
    EXPECT_GT(path.size(), 2u);  // Must detour
}

TEST(NavMeshTest, FindPath_NoPath)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    // Completely surround destination cell
    int cx = 16, cz = 16;  // target at (8,8) area
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            if (dx != 0 || dz != 0)
                mesh.SetCellWalkable(cx + dx, cz + dz, false);
    mesh.SetCellWalkable(cx, cz, false);  // block the target itself too
    std::vector<XMFLOAT3> path;
    bool found = mesh.FindPath({1.0f, 0.0f, 1.0f}, {8.25f, 0.0f, 8.25f}, path);
    EXPECT_FALSE(found);
}

TEST(NavMeshTest, FindPath_SamePoint)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    std::vector<XMFLOAT3> path;
    bool found = mesh.FindPath({5.0f, 0.0f, 5.0f}, {5.0f, 0.0f, 5.0f}, path);
    // Should succeed with 0 or 1 waypoints
    EXPECT_TRUE(found);
}

TEST(NavMeshTest, FindNearestWalkable)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    mesh.SetCellWalkable(10, 10, false);
    XMFLOAT3 nearest;
    bool ok = mesh.FindNearestWalkable({5.25f, 0.0f, 5.25f}, nearest);
    EXPECT_TRUE(ok);
    // nearest should be walkable
    EXPECT_TRUE(mesh.IsWalkable(nearest));
}

TEST(NavAgentTest, Initialize)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    NavAgent agent;
    agent.Initialize(&mesh);
    EXPECT_FALSE(agent.HasPath());
    EXPECT_FALSE(agent.HasReachedDestination());
}

TEST(NavAgentTest, SetDestination)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    NavAgent agent;
    agent.Initialize(&mesh);
    agent.SetPosition({1.0f, 0.0f, 1.0f});
    agent.SetDestination({8.0f, 0.0f, 8.0f});
    EXPECT_TRUE(agent.HasPath());
}

TEST(NavAgentTest, Update_Moves)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    NavAgent agent;
    agent.Initialize(&mesh);
    agent.SetPosition({1.0f, 0.0f, 1.0f});
    agent.SetDestination({8.0f, 0.0f, 8.0f});
    XMFLOAT3 before = agent.GetPosition();
    agent.Update(1.0f);
    XMFLOAT3 after = agent.GetPosition();
    float moved = std::abs(after.x - before.x) + std::abs(after.z - before.z);
    EXPECT_GT(moved, 0.1f);
}

TEST(NavAgentTest, HasReached)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    NavAgent agent;
    agent.Initialize(&mesh);
    // Place very close to destination
    agent.SetPosition({5.0f, 0.0f, 5.0f});
    agent.SetDestination({5.05f, 0.0f, 5.05f});
    // Update many steps to ensure arrival
    for (int i = 0; i < 100; ++i)
        agent.Update(0.1f);
    EXPECT_TRUE(agent.HasReachedDestination());
}

TEST(NavAgentTest, Stop)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    NavAgent agent;
    agent.Initialize(&mesh);
    agent.SetPosition({1.0f, 0.0f, 1.0f});
    agent.SetDestination({8.0f, 0.0f, 8.0f});
    EXPECT_TRUE(agent.HasPath());
    agent.Stop();
    EXPECT_FALSE(agent.HasPath());
}
