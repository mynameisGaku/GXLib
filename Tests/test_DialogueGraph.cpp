/// @file test_DialogueGraph.cpp
/// @brief DialogueGraphのテスト

#include <gtest/gtest.h>
#include "Core/DialogueGraph.h"

using namespace gx;

TEST(DialogueGraphTest, DefaultState)
{
    DialogueGraph graph;
    EXPECT_EQ(graph.GetNodeCount(), 0u);
    EXPECT_TRUE(graph.IsFinished());
    EXPECT_FALSE(graph.IsWaitingForChoice());
}

TEST(DialogueGraphTest, AddNode)
{
    DialogueGraph graph;
    uint32_t id = graph.AddNode(DialogueNodeType::Text, "Alice", "Hello!");
    EXPECT_EQ(id, 0u);
    EXPECT_EQ(graph.GetNodeCount(), 1u);
    const auto* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->speaker, "Alice");
    EXPECT_EQ(node->text, "Hello!");
}

TEST(DialogueGraphTest, RemoveNode)
{
    DialogueGraph graph;
    uint32_t id = graph.AddNode(DialogueNodeType::Text, "Bob", "Hi");
    graph.RemoveNode(id);
    EXPECT_EQ(graph.GetNodeCount(), 0u);
    EXPECT_EQ(graph.GetNode(id), nullptr);
}

TEST(DialogueGraphTest, AddLink)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Text, "A", "Line 1");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "A", "Line 2");
    graph.AddLink(n0, n1);
    const auto* node = graph.GetNode(n0);
    ASSERT_NE(node, nullptr);
    ASSERT_EQ(node->links.size(), 1u);
    EXPECT_EQ(node->links[0].targetNodeId, n1);
}

TEST(DialogueGraphTest, RemoveLink)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Text, "A", "1");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "A", "2");
    graph.AddLink(n0, n1);
    graph.RemoveLink(n0, n1);
    EXPECT_TRUE(graph.GetNode(n0)->links.empty());
}

TEST(DialogueGraphTest, VariableSetGet)
{
    DialogueGraph graph;
    graph.SetVariable("score", 42);
    auto val = graph.GetVariable("score");
    EXPECT_EQ(std::get<int>(val), 42);
    EXPECT_TRUE(graph.HasVariable("score"));
    EXPECT_FALSE(graph.HasVariable("missing"));
}

TEST(DialogueGraphTest, ClearVariables)
{
    DialogueGraph graph;
    graph.SetVariable("x", 1);
    graph.ClearVariables();
    EXPECT_FALSE(graph.HasVariable("x"));
}

TEST(DialogueGraphTest, StartAndAdvance)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Text, "A", "Hello");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "B", "World");
    graph.AddLink(n0, n1);

    graph.Start(n0);
    EXPECT_FALSE(graph.IsFinished());
    const auto* cur = graph.GetCurrentNode();
    ASSERT_NE(cur, nullptr);
    EXPECT_EQ(cur->text, "Hello");

    bool advanced = graph.Advance();
    EXPECT_TRUE(advanced);
    cur = graph.GetCurrentNode();
    ASSERT_NE(cur, nullptr);
    EXPECT_EQ(cur->text, "World");

    // No more links: advance should finish
    advanced = graph.Advance();
    EXPECT_FALSE(advanced);
    EXPECT_TRUE(graph.IsFinished());
}

TEST(DialogueGraphTest, ChoiceSelection)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Choice, "NPC", "Choose:");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "", "Option A chosen");
    uint32_t n2 = graph.AddNode(DialogueNodeType::Text, "", "Option B chosen");
    graph.AddLink(n0, n1, "Option A");
    graph.AddLink(n0, n2, "Option B");

    graph.Start(n0);
    EXPECT_TRUE(graph.IsWaitingForChoice());

    // Advance should fail on Choice nodes
    EXPECT_FALSE(graph.Advance());

    // Select second choice
    graph.SelectChoice(1);
    const auto* cur = graph.GetCurrentNode();
    ASSERT_NE(cur, nullptr);
    EXPECT_EQ(cur->text, "Option B chosen");
}

TEST(DialogueGraphTest, BranchCondition)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Branch, "", "");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "", "Has key");
    uint32_t n2 = graph.AddNode(DialogueNodeType::Text, "", "No key");
    graph.AddLink(n0, n1, "", "hasKey == true");
    graph.AddLink(n0, n2, "", ""); // fallback (no condition)

    // Without variable: first condition fails, fallback matches
    graph.Start(n0);
    graph.Advance();
    EXPECT_EQ(graph.GetCurrentNode()->text, "No key");

    // With variable set
    graph.SetVariable("hasKey", true);
    graph.Start(n0);
    graph.Advance();
    EXPECT_EQ(graph.GetCurrentNode()->text, "Has key");
}

TEST(DialogueGraphTest, RandomNode)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Random, "", "");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "", "A");
    uint32_t n2 = graph.AddNode(DialogueNodeType::Text, "", "B");
    graph.AddLink(n0, n1);
    graph.AddLink(n0, n2);

    graph.Start(n0);
    bool advanced = graph.Advance();
    EXPECT_TRUE(advanced);
    // Should land on either n1 or n2
    const auto* cur = graph.GetCurrentNode();
    ASSERT_NE(cur, nullptr);
    EXPECT_TRUE(cur->text == "A" || cur->text == "B");
}

TEST(DialogueGraphTest, VisitedHistory)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Text, "", "1");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "", "2");
    graph.AddLink(n0, n1);

    graph.Start(n0);
    EXPECT_TRUE(graph.HasVisited(n0));
    EXPECT_FALSE(graph.HasVisited(n1));

    graph.Advance();
    EXPECT_TRUE(graph.HasVisited(n1));

    const auto& visited = graph.GetVisitedNodes();
    EXPECT_EQ(visited.size(), 2u);
}

TEST(DialogueGraphTest, ClearHistory)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Text, "", "");
    graph.Start(n0);
    graph.ClearHistory();
    EXPECT_TRUE(graph.GetVisitedNodes().empty());
}

TEST(DialogueGraphTest, SaveAndLoad)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Text, "Alice", "Hello");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Choice, "NPC", "Choose");
    graph.AddLink(n0, n1, "next");
    graph.SetVariable("gold", 100);

    gx::String data = graph.Save();
    EXPECT_FALSE(data.empty());

    DialogueGraph loaded;
    EXPECT_TRUE(loaded.Load(data));
    EXPECT_EQ(loaded.GetNodeCount(), 2u);
    const auto* node0 = loaded.GetNode(n0);
    ASSERT_NE(node0, nullptr);
    EXPECT_EQ(node0->speaker, "Alice");
    EXPECT_EQ(node0->text, "Hello");
    EXPECT_EQ(node0->links.size(), 1u);

    auto goldVal = loaded.GetVariable("gold");
    EXPECT_EQ(std::get<int>(goldVal), 100);
}

TEST(DialogueGraphTest, LoadEmpty)
{
    DialogueGraph graph;
    EXPECT_FALSE(graph.Load(""));
}

TEST(DialogueGraphTest, Callback)
{
    DialogueGraph graph;
    uint32_t n0 = graph.AddNode(DialogueNodeType::Text, "A", "Test");
    gx::String entered;
    graph.SetOnNodeEnter([&](const DialogueGraphNode& node) {
        entered = node.text;
    });
    graph.Start(n0);
    EXPECT_EQ(entered, "Test");
}

TEST(DialogueGraphTest, SetStartNode)
{
    DialogueGraph graph;
    graph.AddNode(DialogueNodeType::Text, "", "First");
    uint32_t n1 = graph.AddNode(DialogueNodeType::Text, "", "Second");
    graph.SetStartNode(n1);
    EXPECT_EQ(graph.GetStartNode(), n1);
}
