/// @file test_NavMesh.cpp
/// @brief NavMesh A*経路探索 + NavAgent のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"

using namespace gx;

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
    // 10.0 / 0.5 = 各方向20セル
    EXPECT_EQ(mesh.GetGridWidth(), 20);
    EXPECT_EQ(mesh.GetGridHeight(), 20);
    EXPECT_NEAR(mesh.GetCellSize(), 0.5f, 1e-5f);
}

TEST(NavMeshTest, IsWalkable_Default)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    // デフォルトではすべてのセルが歩行可能であるべき
    EXPECT_TRUE(mesh.IsWalkable({5.0f, 0.0f, 5.0f}));
    EXPECT_TRUE(mesh.IsWalkable({0.25f, 0.0f, 0.25f}));
}

TEST(NavMeshTest, SetCellWalkable_False)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    mesh.SetCellWalkable(10, 10, false);
    // セル(10,10)はワールド座標(5.0, 0, 5.0)付近に対応
    EXPECT_FALSE(mesh.IsWalkable({5.25f, 0.0f, 5.25f}));
}

TEST(NavMeshTest, FindPath_StraightLine)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    gx::Vector<Vector3> path;
    bool found = mesh.FindPath({1.0f, 0.0f, 1.0f}, {8.0f, 0.0f, 8.0f}, path);
    EXPECT_TRUE(found);
    EXPECT_GT(path.size(), 0u);
}

TEST(NavMeshTest, FindPath_AroundObstacle)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    // 中央に壁を作成
    for (int x = 0; x < 20; ++x)
    {
        if (x != 0 && x != 19)  // 端に開口部を残す
            mesh.SetCellWalkable(x, 10, false);
    }
    gx::Vector<Vector3> path;
    bool found = mesh.FindPath({5.0f, 0.0f, 2.0f}, {5.0f, 0.0f, 8.0f}, path);
    EXPECT_TRUE(found);
    EXPECT_GT(path.size(), 2u);  // 迂回が必要
}

TEST(NavMeshTest, FindPath_NoPath)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    // 目的地セルを完全に囲む
    int cx = 16, cz = 16;  // (8,8)エリアのターゲット
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz)
            if (dx != 0 || dz != 0)
                mesh.SetCellWalkable(cx + dx, cz + dz, false);
    mesh.SetCellWalkable(cx, cz, false);  // ターゲット自体もブロック
    gx::Vector<Vector3> path;
    bool found = mesh.FindPath({1.0f, 0.0f, 1.0f}, {8.25f, 0.0f, 8.25f}, path);
    EXPECT_FALSE(found);
}

TEST(NavMeshTest, FindPath_SamePoint)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    gx::Vector<Vector3> path;
    bool found = mesh.FindPath({5.0f, 0.0f, 5.0f}, {5.0f, 0.0f, 5.0f}, path);
    // 0または1のウェイポイントで成功すべき
    EXPECT_TRUE(found);
}

TEST(NavMeshTest, FindNearestWalkable)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    mesh.SetCellWalkable(10, 10, false);
    Vector3 nearest;
    bool ok = mesh.FindNearestWalkable({5.25f, 0.0f, 5.25f}, nearest);
    EXPECT_TRUE(ok);
    // 最近接点は歩行可能であるべき
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
    Vector3 before = agent.GetPosition();
    agent.Update(1.0f);
    Vector3 after = agent.GetPosition();
    float moved = std::abs(after.x - before.x) + std::abs(after.z - before.z);
    EXPECT_GT(moved, 0.1f);
}

TEST(NavAgentTest, HasReached)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);
    NavAgent agent;
    agent.Initialize(&mesh);
    // 目的地の非常に近くに配置
    agent.SetPosition({5.0f, 0.0f, 5.0f});
    agent.SetDestination({5.05f, 0.0f, 5.05f});
    // 到着を確実にするために多数のステップを更新
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
