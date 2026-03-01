/// @file test_AIBehaviorTree.cpp
/// @brief Phase 2: Blackboard, BT ノード全種, NavMesh 障害物

#include <gtest/gtest.h>
#include "AI/BehaviorTree.h"
#include "AI/NavMesh.h"

using namespace gx;

// ============================================================================
// Blackboard
// ============================================================================

TEST(BlackboardTest, SetAndGet)
{
    Blackboard bb;
    bb.Set("health", 100);
    bb.Set("speed", 5.5f);
    bb.Set("name", std::string("NPC"));
    bb.Set("alive", true);

    EXPECT_EQ(bb.Get<int>("health"), 100);
    EXPECT_FLOAT_EQ(bb.Get<float>("speed"), 5.5f);
    EXPECT_EQ(bb.Get<std::string>("name"), "NPC");
    EXPECT_TRUE(bb.Get<bool>("alive"));
}

TEST(BlackboardTest, HasAndRemove)
{
    Blackboard bb;
    bb.Set("key", 42);
    EXPECT_TRUE(bb.Has("key"));
    bb.Remove("key");
    EXPECT_FALSE(bb.Has("key"));
}

TEST(BlackboardTest, DefaultValue)
{
    Blackboard bb;
    EXPECT_EQ(bb.Get<int>("missing", 99), 99);
    EXPECT_FLOAT_EQ(bb.Get<float>("missing", 1.0f), 1.0f);
}

TEST(BlackboardTest, XMFLOAT3)
{
    Blackboard bb;
    bb.Set("pos", XMFLOAT3{ 1.0f, 2.0f, 3.0f });
    auto pos = bb.Get<XMFLOAT3>("pos");
    EXPECT_FLOAT_EQ(pos.x, 1.0f);
    EXPECT_FLOAT_EQ(pos.y, 2.0f);
    EXPECT_FLOAT_EQ(pos.z, 3.0f);
}

// ============================================================================
// BTノード
// ============================================================================

TEST(BTActionTest, SuccessAction)
{
    Blackboard bb;
    BTAction action([](float, Blackboard&) { return BTStatus::Success; });
    EXPECT_EQ(action.Tick(0.016f, bb), BTStatus::Success);
}

TEST(BTActionTest, FailureAction)
{
    Blackboard bb;
    BTAction action([](float, Blackboard&) { return BTStatus::Failure; });
    EXPECT_EQ(action.Tick(0.016f, bb), BTStatus::Failure);
}

TEST(BTConditionTest, TrueCondition)
{
    Blackboard bb;
    bb.Set("ready", true);
    BTCondition cond([](const Blackboard& b) { return b.Get<bool>("ready"); });
    EXPECT_EQ(cond.Tick(0.016f, bb), BTStatus::Success);
}

TEST(BTConditionTest, FalseCondition)
{
    Blackboard bb;
    BTCondition cond([](const Blackboard& b) { return b.Get<bool>("ready"); });
    EXPECT_EQ(cond.Tick(0.016f, bb), BTStatus::Failure);
}

TEST(BTSequenceTest, AllSucceed)
{
    Blackboard bb;
    auto seq = std::make_shared<BTSequence>();
    seq->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));
    seq->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));

    EXPECT_EQ(seq->Tick(0.016f, bb), BTStatus::Success);
}

TEST(BTSequenceTest, OneFailure)
{
    Blackboard bb;
    auto seq = std::make_shared<BTSequence>();
    seq->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));
    seq->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Failure; }));
    seq->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));

    EXPECT_EQ(seq->Tick(0.016f, bb), BTStatus::Failure);
}

TEST(BTSelectorTest, FirstSuccess)
{
    Blackboard bb;
    auto sel = std::make_shared<BTSelector>();
    sel->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Failure; }));
    sel->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));

    EXPECT_EQ(sel->Tick(0.016f, bb), BTStatus::Success);
}

TEST(BTSelectorTest, AllFail)
{
    Blackboard bb;
    auto sel = std::make_shared<BTSelector>();
    sel->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Failure; }));
    sel->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Failure; }));

    EXPECT_EQ(sel->Tick(0.016f, bb), BTStatus::Failure);
}

TEST(BTInverterTest, InvertsSuccess)
{
    Blackboard bb;
    auto inv = std::make_shared<BTInverter>();
    inv->SetChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));
    EXPECT_EQ(inv->Tick(0.016f, bb), BTStatus::Failure);
}

TEST(BTInverterTest, InvertsFailure)
{
    Blackboard bb;
    auto inv = std::make_shared<BTInverter>();
    inv->SetChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Failure; }));
    EXPECT_EQ(inv->Tick(0.016f, bb), BTStatus::Success);
}

TEST(BTRepeaterTest, RepeatsNTimes)
{
    Blackboard bb;
    int count = 0;
    auto rep = std::make_shared<BTRepeater>(3);
    rep->SetChild(std::make_shared<BTAction>([&](float, Blackboard&) {
        count++;
        return BTStatus::Success;
    }));

    // 最初の2ティックはRunningを返す（まだ繰り返し中）
    EXPECT_EQ(rep->Tick(0.016f, bb), BTStatus::Running);
    EXPECT_EQ(rep->Tick(0.016f, bb), BTStatus::Running);
    // 3回目のティックは子ノードのステータスを返すはず
    EXPECT_EQ(rep->Tick(0.016f, bb), BTStatus::Success);
    EXPECT_EQ(count, 3);
}

TEST(BTSucceederTest, AlwaysSucceeds)
{
    Blackboard bb;
    auto succ = std::make_shared<BTSucceeder>();
    succ->SetChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Failure; }));
    EXPECT_EQ(succ->Tick(0.016f, bb), BTStatus::Success);
}

TEST(BTParallelTest, RequireAll)
{
    Blackboard bb;
    auto par = std::make_shared<BTParallel>(BTParallelPolicy::RequireAll);
    par->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));
    par->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));
    EXPECT_EQ(par->Tick(0.016f, bb), BTStatus::Success);
}

TEST(BTParallelTest, RequireAllWithFailure)
{
    Blackboard bb;
    auto par = std::make_shared<BTParallel>(BTParallelPolicy::RequireAll);
    par->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Success; }));
    par->AddChild(std::make_shared<BTAction>([](float, Blackboard&) { return BTStatus::Failure; }));
    EXPECT_EQ(par->Tick(0.016f, bb), BTStatus::Failure);
}

// ============================================================================
// BehaviorTree（結合テスト）
// ============================================================================

TEST(BehaviorTreeTest, SimpleTree)
{
    BehaviorTree tree;
    auto& bb = tree.GetBlackboard();
    bb.Set("hp", 50);

    auto root = std::make_shared<BTSelector>();

    // HP > 30 なら攻撃
    auto attackSeq = std::make_shared<BTSequence>();
    attackSeq->AddChild(std::make_shared<BTCondition>(
        [](const Blackboard& b) { return b.Get<int>("hp") > 30; }));
    attackSeq->AddChild(std::make_shared<BTAction>(
        [](float, Blackboard& b) { b.Set("action", std::string("attack")); return BTStatus::Success; }));

    // それ以外は逃走
    auto fleeAction = std::make_shared<BTAction>(
        [](float, Blackboard& b) { b.Set("action", std::string("flee")); return BTStatus::Success; });

    root->AddChild(attackSeq);
    root->AddChild(fleeAction);
    tree.SetRoot(root);

    BTStatus status = tree.Tick(0.016f);
    EXPECT_EQ(status, BTStatus::Success);
    EXPECT_EQ(bb.Get<std::string>("action"), "attack");

    // HPを低くする
    bb.Set("hp", 10);
    tree.Reset();
    status = tree.Tick(0.016f);
    EXPECT_EQ(status, BTStatus::Success);
    EXPECT_EQ(bb.Get<std::string>("action"), "flee");
}

// ============================================================================
// NavMesh動的障害物
// ============================================================================

TEST(NavMeshObstacleTest, AddAndRemoveAABB)
{
    NavMesh nav;
    nav.Build(0, 0, 10, 10, 1.0f);

    EXPECT_TRUE(nav.IsWalkable({ 5, 0, 5 }));

    auto handle = nav.AddObstacleAABB(4.0f, 4.0f, 6.0f, 6.0f);
    EXPECT_TRUE(handle);
    EXPECT_FALSE(nav.IsWalkable({ 5, 0, 5 }));
    EXPECT_EQ(nav.GetObstacleCount(), 1u);

    nav.RemoveObstacle(handle);
    EXPECT_TRUE(nav.IsWalkable({ 5, 0, 5 }));
    EXPECT_EQ(nav.GetObstacleCount(), 0u);
}

TEST(NavMeshObstacleTest, AddCylinder)
{
    NavMesh nav;
    nav.Build(0, 0, 10, 10, 1.0f);

    auto handle = nav.AddObstacleCylinder(5.0f, 5.0f, 2.0f);
    EXPECT_FALSE(nav.IsWalkable({ 5, 0, 5 }));

    // シリンダーから遠いセルはまだ歩行可能なはず
    EXPECT_TRUE(nav.IsWalkable({ 0.5f, 0, 0.5f }));

    nav.RemoveObstacle(handle);
    EXPECT_TRUE(nav.IsWalkable({ 5, 0, 5 }));
}

TEST(NavMeshObstacleTest, ClearObstacles)
{
    NavMesh nav;
    nav.Build(0, 0, 10, 10, 1.0f);

    nav.AddObstacleAABB(2, 2, 4, 4);
    nav.AddObstacleCylinder(7, 7, 1.5f);
    EXPECT_EQ(nav.GetObstacleCount(), 2u);

    nav.ClearObstacles();
    EXPECT_EQ(nav.GetObstacleCount(), 0u);
    EXPECT_TRUE(nav.IsWalkable({ 3, 0, 3 }));
}

TEST(NavMeshObstacleTest, PathAroundObstacle)
{
    NavMesh nav;
    nav.Build(0, 0, 10, 10, 1.0f);

    // 直接パスをブロック
    nav.AddObstacleAABB(4, 0, 6, 10);

    std::vector<XMFLOAT3> path;
    // 障害物がz=0からz=10の全幅をブロックしているのでパスは失敗するはず
    bool found = nav.FindPath({ 2, 0, 5 }, { 8, 0, 5 }, path);
    EXPECT_FALSE(found);  // 障害物がZ全範囲にまたがるのでパス不可
}

TEST(NavMeshObstacleTest, MoveObstacle)
{
    NavMesh nav;
    nav.Build(0, 0, 10, 10, 1.0f);

    auto handle = nav.AddObstacleAABB(4, 4, 6, 6);
    EXPECT_FALSE(nav.IsWalkable({ 5, 0, 5 }));

    // 障害物を移動
    nav.MoveObstacle(handle, 1.0f, 1.0f);
    EXPECT_TRUE(nav.IsWalkable({ 5, 0, 5 }));
    EXPECT_FALSE(nav.IsWalkable({ 1, 0, 1 }));
}
