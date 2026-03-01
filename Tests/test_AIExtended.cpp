/// @file test_AIExtended.cpp
/// @brief NavMeshパススムージング + RVO障害物回避のテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"
#include "AI/RVOSolver.h"

using namespace gx;

// ============================================================================
// NavMeshパススムージング
// ============================================================================

TEST(NavMeshSmoothTest, StraightLine_FewerWaypoints)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 20.0f, 20.0f, 0.5f);

    // 生パス（スムージングなし）
    std::vector<XMFLOAT3> rawPath;
    bool found = mesh.FindPath({1, 0, 1}, {19, 0, 1}, rawPath, false);
    ASSERT_TRUE(found);

    // スムージング済みパス
    std::vector<XMFLOAT3> smoothPath;
    found = mesh.FindPath({1, 0, 1}, {19, 0, 1}, smoothPath, true);
    ASSERT_TRUE(found);

    // 直線の場合、スムージング済みパスのウェイポイント数は同じかそれ以下
    EXPECT_LE(smoothPath.size(), rawPath.size());
}

TEST(NavMeshSmoothTest, Diagonal_FewerWaypoints)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 20.0f, 20.0f, 0.5f);

    std::vector<XMFLOAT3> rawPath;
    mesh.FindPath({1, 0, 1}, {19, 0, 19}, rawPath, false);

    std::vector<XMFLOAT3> smoothPath;
    mesh.FindPath({1, 0, 1}, {19, 0, 19}, smoothPath, true);

    // スムージング済みの対角線はずっと短くなるはず
    EXPECT_LE(smoothPath.size(), rawPath.size());
}

TEST(NavMeshSmoothTest, AroundObstacle_StillFindsPath)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 20.0f, 20.0f, 0.5f);

    // 中央に壁を作成
    for (int x = 5; x < 35; ++x)
        mesh.SetCellWalkable(x, 20, false);

    std::vector<XMFLOAT3> smoothPath;
    bool found = mesh.FindPath({10, 0, 5}, {10, 0, 15}, smoothPath, true);
    EXPECT_TRUE(found);
    EXPECT_GT(smoothPath.size(), 2u);
}

TEST(NavMeshSmoothTest, LShape_ReducedWaypoints)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 20.0f, 20.0f, 0.5f);

    // L字型障害物を作成
    for (int x = 10; x < 35; ++x)
        mesh.SetCellWalkable(x, 20, false);
    for (int z = 10; z < 20; ++z)
        mesh.SetCellWalkable(20, z, false);

    std::vector<XMFLOAT3> rawPath;
    mesh.FindPath({5, 0, 5}, {15, 0, 15}, rawPath, false);

    std::vector<XMFLOAT3> smoothPath;
    mesh.FindPath({5, 0, 5}, {15, 0, 15}, smoothPath, true);

    if (!rawPath.empty() && !smoothPath.empty())
    {
        EXPECT_LE(smoothPath.size(), rawPath.size());
    }
}

TEST(NavMeshSmoothTest, SameStartEnd_EmptyOrSinglePoint)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);

    std::vector<XMFLOAT3> path;
    mesh.FindPath({5, 0, 5}, {5, 0, 5}, path, true);
    // 空または1点 — ウェイポイント不要
    EXPECT_LE(path.size(), 2u);
}

// ============================================================================
// RVO障害物回避
// ============================================================================

TEST(RVOTest, NoNeighbors_ReturnsDesired)
{
    RVO::AgentState self;
    self.position = {0, 0, 0};
    self.velocity = {1, 0, 0};
    self.radius = 0.5f;
    self.maxSpeed = 3.5f;

    XMFLOAT3 desired = {2, 0, 0};
    std::vector<RVO::AgentState> others; // empty

    XMFLOAT3 result = RVO::ComputeAvoidanceVelocity(self, desired, others);
    // 近隣エージェントがいない場合、結果は希望速度と一致するはず
    EXPECT_NEAR(result.x, desired.x, 0.01f);
    EXPECT_NEAR(result.z, desired.z, 0.01f);
}

TEST(RVOTest, CloseApproach_VelocityDiverges)
{
    // 近くの静止エージェントに接近するエージェント
    // 回避条件: |relVel - relPos/T| < combinedRadius/T
    // relPos = (1.2, 0), T = 2 → cutoffCenter = (0.6, 0), cutoffRadius = 0.5
    // desiredVel = (0.3, 0) → relVel = (0.3, 0) → w = (-0.3, 0), |w| = 0.3 < 0.5
    RVO::AgentState self;
    self.position = {0, 0, 0};
    self.velocity = {0.3f, 0, 0};
    self.radius = 0.5f;
    self.maxSpeed = 3.5f;

    RVO::AgentState other;
    other.position = {1.2f, 0, 0};  // 近くの静止エージェント
    other.velocity = {0, 0, 0};
    other.radius = 0.5f;
    other.maxSpeed = 3.5f;

    XMFLOAT3 desired = {0.3f, 0, 0};
    std::vector<RVO::AgentState> others = {other};

    XMFLOAT3 result = RVO::ComputeAvoidanceVelocity(self, desired, others, 2.0f);

    // 回避速度は希望速度と異なるはず
    float diffX = result.x - desired.x;
    float diffZ = result.z - desired.z;
    float diff = std::sqrt(diffX * diffX + diffZ * diffZ);
    EXPECT_GT(diff, 0.001f);
}

TEST(RVOTest, FarApartAgents_NoAvoidanceNeeded)
{
    RVO::AgentState self;
    self.position = {0, 0, 0};
    self.velocity = {1, 0, 0};
    self.radius = 0.5f;
    self.maxSpeed = 3.5f;

    RVO::AgentState other;
    other.position = {0, 0, 100};  // 非常に遠い
    other.velocity = {-1, 0, 0};
    other.radius = 0.5f;
    other.maxSpeed = 3.5f;

    XMFLOAT3 desired = {1, 0, 0};
    std::vector<RVO::AgentState> others = {other};

    XMFLOAT3 result = RVO::ComputeAvoidanceVelocity(self, desired, others);
    // 十分遠いので回避は不要なはず
    EXPECT_NEAR(result.x, desired.x, 0.1f);
    EXPECT_NEAR(result.z, desired.z, 0.1f);
}

TEST(RVOTest, SpeedClamped)
{
    RVO::AgentState self;
    self.position = {0, 0, 0};
    self.velocity = {0, 0, 0};
    self.radius = 0.5f;
    self.maxSpeed = 2.0f;

    RVO::AgentState other;
    other.position = {3, 0, 0};
    other.velocity = {-3, 0, 0};
    other.radius = 0.5f;
    other.maxSpeed = 3.5f;

    XMFLOAT3 desired = {2, 0, 0};
    std::vector<RVO::AgentState> others = {other};

    XMFLOAT3 result = RVO::ComputeAvoidanceVelocity(self, desired, others);
    float speed = std::sqrt(result.x * result.x + result.z * result.z);
    EXPECT_LE(speed, self.maxSpeed + 0.01f);
}

// ============================================================================
// NavAgentとRVO
// ============================================================================

TEST(NavAgentRVOTest, UpdateWithNeighbors_Runs)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 20.0f, 20.0f, 0.5f);

    NavAgent agent1;
    agent1.Initialize(&mesh);
    agent1.SetPosition({5, 0, 5});
    agent1.enableAvoidance = true;
    agent1.radius = 0.5f;

    NavAgent agent2;
    agent2.Initialize(&mesh);
    agent2.SetPosition({8, 0, 5});
    agent2.enableAvoidance = true;
    agent2.radius = 0.5f;

    agent1.SetDestination({15, 0, 5});
    agent2.SetDestination({0, 0, 5});

    std::vector<NavAgent*> neighbors = {&agent2};
    // クラッシュしないことを確認
    agent1.UpdateWithNeighbors(1.0f / 60.0f, neighbors);
}

TEST(NavAgentRVOTest, LastVelocity_NonZero)
{
    NavMesh mesh;
    mesh.Build(0.0f, 0.0f, 20.0f, 20.0f, 0.5f);

    NavAgent agent;
    agent.Initialize(&mesh);
    agent.SetPosition({5, 0, 5});
    agent.SetDestination({15, 0, 5});

    agent.Update(1.0f / 60.0f);

    auto vel = agent.GetLastVelocity();
    float speed = std::sqrt(vel.x * vel.x + vel.z * vel.z);
    // エージェントは移動しているはず
    EXPECT_GT(speed, 0.0f);
}
