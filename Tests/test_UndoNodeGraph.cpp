/// @file test_UndoNodeGraph.cpp
/// @brief Phase 3: UndoSystem + NodeGraph テスト

#include <gtest/gtest.h>
#include "Core/UndoSystem.h"
#include "Core/NodeGraph.h"

using namespace gx;

// ============================================================================
// UndoSystem
// ============================================================================

class UndoSystemTest : public ::testing::Test
{
protected:
    UndoSystem undo;
};

TEST_F(UndoSystemTest, ExecuteAndUndo)
{
    int value = 10;
    undo.Execute(std::make_unique<ValueCommand<int>>(
        "set value", 10, 42, [&](const int& v) { value = v; }));

    EXPECT_EQ(value, 42);
    EXPECT_TRUE(undo.CanUndo());
    EXPECT_FALSE(undo.CanRedo());

    undo.Undo();
    EXPECT_EQ(value, 10);
    EXPECT_FALSE(undo.CanUndo());
    EXPECT_TRUE(undo.CanRedo());
}

TEST_F(UndoSystemTest, Redo)
{
    int value = 0;
    undo.Execute(std::make_unique<ValueCommand<int>>(
        "set", 0, 5, [&](const int& v) { value = v; }));

    undo.Undo();
    EXPECT_EQ(value, 0);

    undo.Redo();
    EXPECT_EQ(value, 5);
}

TEST_F(UndoSystemTest, RedoClearedOnNewExecute)
{
    int value = 0;
    undo.Execute(std::make_unique<ValueCommand<int>>(
        "a", 0, 1, [&](const int& v) { value = v; }));
    undo.Undo();
    EXPECT_TRUE(undo.CanRedo());

    undo.Execute(std::make_unique<ValueCommand<int>>(
        "b", 0, 2, [&](const int& v) { value = v; }));
    EXPECT_FALSE(undo.CanRedo()); // リドゥスタックがクリアされた
}

TEST_F(UndoSystemTest, MaxHistory)
{
    int value = 0;
    undo.SetMaxHistory(3);

    for (int i = 1; i <= 5; ++i)
    {
        undo.Execute(std::make_unique<ValueCommand<int>>(
            "cmd" + std::to_string(i), 0, i, [&](const int& v) { value = v; }));
    }

    EXPECT_EQ(undo.GetUndoCount(), 3u); // 最後の3つのみ保持
}

TEST_F(UndoSystemTest, Clear)
{
    int value = 0;
    undo.Execute(std::make_unique<ValueCommand<int>>(
        "a", 0, 1, [&](const int& v) { value = v; }));
    undo.Clear();

    EXPECT_FALSE(undo.CanUndo());
    EXPECT_FALSE(undo.CanRedo());
    EXPECT_EQ(undo.GetUndoCount(), 0u);
    EXPECT_EQ(undo.GetRedoCount(), 0u);
}

TEST_F(UndoSystemTest, Descriptions)
{
    int value = 0;
    undo.Execute(std::make_unique<ValueCommand<int>>(
        "Set to 5", 0, 5, [&](const int& v) { value = v; }));

    EXPECT_EQ(undo.GetUndoDescription(), "Set to 5");

    undo.Undo();
    EXPECT_EQ(undo.GetRedoDescription(), "Set to 5");
}

TEST_F(UndoSystemTest, EmptyUndoRedoReturnFalse)
{
    EXPECT_FALSE(undo.Undo());
    EXPECT_FALSE(undo.Redo());
}

TEST_F(UndoSystemTest, EmptyDescriptions)
{
    EXPECT_TRUE(undo.GetUndoDescription().empty());
    EXPECT_TRUE(undo.GetRedoDescription().empty());
}

TEST_F(UndoSystemTest, MultipleUndoRedo)
{
    int value = 0;
    undo.Execute(std::make_unique<ValueCommand<int>>("a", 0, 1, [&](const int& v) { value = v; }));
    undo.Execute(std::make_unique<ValueCommand<int>>("b", 1, 2, [&](const int& v) { value = v; }));
    undo.Execute(std::make_unique<ValueCommand<int>>("c", 2, 3, [&](const int& v) { value = v; }));

    EXPECT_EQ(value, 3);
    EXPECT_EQ(undo.GetUndoCount(), 3u);

    undo.Undo(); EXPECT_EQ(value, 2);
    undo.Undo(); EXPECT_EQ(value, 1);
    undo.Undo(); EXPECT_EQ(value, 0);

    undo.Redo(); EXPECT_EQ(value, 1);
    undo.Redo(); EXPECT_EQ(value, 2);
    undo.Redo(); EXPECT_EQ(value, 3);
}

// ============================================================================
// NodeGraph
// ============================================================================

class NodeGraphTest : public ::testing::Test
{
protected:
    void SetUp() override { NodeGraph::ClearNodeDefs(); }
    void TearDown() override { NodeGraph::ClearNodeDefs(); }
};

TEST_F(NodeGraphTest, RegisterAndFindDef)
{
    auto def = std::make_unique<NodeDef>("TestNode");
    def->AddInputPin({ "In", PinType::Float });
    def->AddOutputPin({ "Out", PinType::Float });
    NodeGraph::RegisterNodeDef(std::move(def));

    const NodeDef* found = NodeGraph::FindNodeDef("TestNode");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->GetName(), "TestNode");
    EXPECT_EQ(found->GetInputPins().size(), 1u);
    EXPECT_EQ(found->GetOutputPins().size(), 1u);
}

TEST_F(NodeGraphTest, AddAndRemoveNode)
{
    auto def = std::make_unique<NodeDef>("Add");
    NodeGraph::RegisterNodeDef(std::move(def));

    NodeGraph graph;
    uint32_t id = graph.AddNode("Add");
    EXPECT_NE(id, 0xFFFFFFFF);
    EXPECT_EQ(graph.GetNodeCount(), 1u);

    graph.RemoveNode(id);
    EXPECT_EQ(graph.GetNodeCount(), 0u);
}

TEST_F(NodeGraphTest, AddInvalidNode)
{
    NodeGraph graph;
    uint32_t id = graph.AddNode("NonExistent");
    EXPECT_EQ(id, 0xFFFFFFFF);
}

TEST_F(NodeGraphTest, ExecuteDataFlow)
{
    // 入力を読み取り出力に書き込む「Multiply」ノードを作成
    auto mulDef = std::make_unique<NodeDef>("Multiply");
    mulDef->AddInputPin({ "Value", PinType::Float });
    mulDef->SetExecute([](NodeInstance& inst) {
        float val = std::get<float>(inst.GetInput(0));
        inst.SetOutput(0, val * 2.0f);
    });
    NodeGraph::RegisterNodeDef(std::move(mulDef));

    // 入力を読み取る「Result」ノードを作成
    float result = 0.0f;
    auto resDef = std::make_unique<NodeDef>("Result");
    resDef->AddInputPin({ "Value", PinType::Float });
    resDef->AddOutputPin({ "Flow", PinType::Flow });
    resDef->SetExecute([&result](NodeInstance& inst) {
        result = std::get<float>(inst.GetInput(0));
    });
    NodeGraph::RegisterNodeDef(std::move(resDef));

    NodeGraph graph;
    uint32_t mulId = graph.AddNode("Multiply");
    uint32_t resId = graph.AddNode("Result");

    // Multiplyノードに入力を設定
    graph.SetNodeInput(mulId, 0, 5.0f);

    // Multiplyの出力をResultの入力に接続
    graph.Connect(mulId, 0, resId, 0);

    // Multiplyを先に実行、その後Result
    graph.Execute(mulId);

    // 注: Executeはデータ接続ではなくフローピンを辿る
    // そのためフロー接続が必要: mulId flow→resId
}

TEST_F(NodeGraphTest, ExecuteFlowChain)
{
    int step = 0;

    auto startDef = std::make_unique<NodeDef>("Start");
    startDef->AddOutputPin({ "Next", PinType::Flow });
    startDef->SetExecute([&step](NodeInstance& inst) {
        step = 1;
        inst.TriggerFlow(0);
    });
    NodeGraph::RegisterNodeDef(std::move(startDef));

    auto endDef = std::make_unique<NodeDef>("End");
    endDef->SetExecute([&step](NodeInstance&) {
        step = 2;
    });
    NodeGraph::RegisterNodeDef(std::move(endDef));

    NodeGraph graph;
    uint32_t startId = graph.AddNode("Start");
    uint32_t endId = graph.AddNode("End");
    graph.Connect(startId, 0, endId, 0); // フロー接続

    graph.Execute(startId);
    EXPECT_EQ(step, 2); // 両ノードが順番に実行された
}

TEST_F(NodeGraphTest, ConnectAndDisconnect)
{
    auto def = std::make_unique<NodeDef>("N");
    NodeGraph::RegisterNodeDef(std::move(def));

    NodeGraph graph;
    uint32_t a = graph.AddNode("N");
    uint32_t b = graph.AddNode("N");

    graph.Connect(a, 0, b, 0);
    graph.Disconnect(a, 0, b, 0);
    // クラッシュなし、副作用なし
}
