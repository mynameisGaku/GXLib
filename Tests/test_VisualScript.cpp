/// @file test_VisualScript.cpp
#include <gtest/gtest.h>
#include "Script/VisualScript.h"
using namespace gx;

// ヘルパー: シンプルなノード型を作成
static VSNodeDef MakeSimpleNode(const gx::String& name, VSNodeCategory cat = VSNodeCategory::Flow,
                                 bool isEvent = false, bool isPure = false) {
    VSNodeDef def;
    def.typeName = name;
    def.category = cat;
    def.displayName = name;
    def.isEvent = isEvent;
    def.isPure = isPure;
    if (!isEvent && !isPure) {
        def.inputPins = { {"Exec", VSPinType::Exec, 0, 0, false} };
        def.outputPins = { {"Exec", VSPinType::Exec, 0, 0, true} };
    }
    return def;
}

TEST(VisualScriptTest, RegisterNodeType) {
    VisualScript vs;
    VSNodeDef def;
    def.typeName = "TestNode";
    def.category = VSNodeCategory::Custom;
    def.displayName = "Test Node";
    vs.RegisterNodeType(def);

    const VSNodeDef* found = vs.GetNodeType("TestNode");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->typeName, "TestNode");
    EXPECT_EQ(found->category, VSNodeCategory::Custom);
}

TEST(VisualScriptTest, GetRegisteredTypeCount) {
    VisualScript vs;
    EXPECT_EQ(vs.GetRegisteredTypeCount(), 0u);

    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    EXPECT_EQ(vs.GetRegisteredTypeCount(), 2u);
}

TEST(VisualScriptTest, HasNodeType) {
    VisualScript vs;
    EXPECT_FALSE(vs.HasNodeType("Foo"));
    vs.RegisterNodeType(MakeSimpleNode("Foo"));
    EXPECT_TRUE(vs.HasNodeType("Foo"));
}

TEST(VisualScriptTest, AddNode) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("MyNode"));

    uint32_t id = vs.AddNode("MyNode", 100.0f, 200.0f);
    EXPECT_NE(id, 0xFFFFFFFFu);

    const VSNodeInstance* node = vs.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->typeName, "MyNode");
    EXPECT_FLOAT_EQ(node->posX, 100.0f);
    EXPECT_FLOAT_EQ(node->posY, 200.0f);
}

TEST(VisualScriptTest, AddNodeUnregisteredType) {
    VisualScript vs;
    uint32_t id = vs.AddNode("NonExistent");
    EXPECT_EQ(id, 0xFFFFFFFFu);
}

TEST(VisualScriptTest, RemoveNode) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("N"));
    uint32_t id = vs.AddNode("N");
    EXPECT_EQ(vs.GetNodeCount(), 1u);

    EXPECT_TRUE(vs.RemoveNode(id));
    EXPECT_EQ(vs.GetNodeCount(), 0u);
    EXPECT_EQ(vs.GetNode(id), nullptr);
}

TEST(VisualScriptTest, GetNode) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("X"));
    uint32_t id = vs.AddNode("X");
    EXPECT_NE(vs.GetNode(id), nullptr);
    EXPECT_EQ(vs.GetNode(99999), nullptr);
}

TEST(VisualScriptTest, NodeCount) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("T"));
    EXPECT_EQ(vs.GetNodeCount(), 0u);
    vs.AddNode("T");
    vs.AddNode("T");
    vs.AddNode("T");
    EXPECT_EQ(vs.GetNodeCount(), 3u);
}

TEST(VisualScriptTest, ConnectNodes) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    uint32_t a = vs.AddNode("A");
    uint32_t b = vs.AddNode("B");

    EXPECT_TRUE(vs.Connect(a, 0, b, 0));
    EXPECT_EQ(vs.GetConnectionCount(), 1u);
}

TEST(VisualScriptTest, ConnectSelfLoop) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    uint32_t a = vs.AddNode("A");
    EXPECT_FALSE(vs.Connect(a, 0, a, 0));
}

TEST(VisualScriptTest, DisconnectNodes) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    uint32_t a = vs.AddNode("A");
    uint32_t b = vs.AddNode("B");

    vs.Connect(a, 0, b, 0);
    EXPECT_EQ(vs.GetConnectionCount(), 1u);

    EXPECT_TRUE(vs.Disconnect(a, 0, b, 0));
    EXPECT_EQ(vs.GetConnectionCount(), 0u);
}

TEST(VisualScriptTest, ConnectionCount) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    vs.RegisterNodeType(MakeSimpleNode("C"));
    uint32_t a = vs.AddNode("A");
    uint32_t b = vs.AddNode("B");
    uint32_t c = vs.AddNode("C");

    vs.Connect(a, 0, b, 0);
    vs.Connect(b, 0, c, 0);
    EXPECT_EQ(vs.GetConnectionCount(), 2u);
}

TEST(VisualScriptTest, ValidateEmpty) {
    VisualScript vs;
    auto errors = vs.Validate();
    EXPECT_TRUE(errors.empty());
}

TEST(VisualScriptTest, ValidateUnconnected) {
    VisualScript vs;
    // ノードに接続されていないExec入力がある場合は警告
    vs.RegisterNodeType(MakeSimpleNode("Action"));
    vs.AddNode("Action");

    auto errors = vs.Validate();
    EXPECT_FALSE(errors.empty());
}

TEST(VisualScriptTest, HasCycleNone) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    vs.RegisterNodeType(MakeSimpleNode("C"));
    uint32_t a = vs.AddNode("A");
    uint32_t b = vs.AddNode("B");
    uint32_t c = vs.AddNode("C");

    vs.Connect(a, 0, b, 0);
    vs.Connect(b, 0, c, 0);
    EXPECT_FALSE(vs.HasCycle());
}

TEST(VisualScriptTest, HasCycleDetected) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    vs.RegisterNodeType(MakeSimpleNode("C"));
    uint32_t a = vs.AddNode("A");
    uint32_t b = vs.AddNode("B");
    uint32_t c = vs.AddNode("C");

    vs.Connect(a, 0, b, 0);
    vs.Connect(b, 0, c, 0);
    vs.Connect(c, 0, a, 0);
    EXPECT_TRUE(vs.HasCycle());
}

TEST(VisualScriptTest, AddVariable) {
    VisualScript vs;
    EXPECT_TRUE(vs.AddVariable("health", VSPinType::Float, "100"));
    EXPECT_FALSE(vs.AddVariable("health", VSPinType::Float, "200")); // 重複
    EXPECT_FALSE(vs.AddVariable("", VSPinType::Int, "0")); // 空名
}

TEST(VisualScriptTest, RemoveVariable) {
    VisualScript vs;
    vs.AddVariable("x", VSPinType::Int, "0");
    EXPECT_TRUE(vs.RemoveVariable("x"));
    EXPECT_FALSE(vs.RemoveVariable("x")); // もう存在しない
}

TEST(VisualScriptTest, SetGetVariable) {
    VisualScript vs;
    vs.AddVariable("score", VSPinType::Int, "0");
    EXPECT_EQ(vs.GetVariable("score"), "0");

    vs.SetVariable("score", "42");
    EXPECT_EQ(vs.GetVariable("score"), "42");

    // 存在しない変数
    EXPECT_EQ(vs.GetVariable("nonexistent"), "");
}

TEST(VisualScriptTest, VariableCount) {
    VisualScript vs;
    EXPECT_EQ(vs.GetVariableCount(), 0u);
    vs.AddVariable("a", VSPinType::Int);
    vs.AddVariable("b", VSPinType::Float);
    EXPECT_EQ(vs.GetVariableCount(), 2u);
}

TEST(VisualScriptTest, HasVariable) {
    VisualScript vs;
    EXPECT_FALSE(vs.HasVariable("foo"));
    vs.AddVariable("foo", VSPinType::String, "bar");
    EXPECT_TRUE(vs.HasVariable("foo"));
}

TEST(VisualScriptTest, VariableType) {
    VisualScript vs;
    vs.AddVariable("name", VSPinType::String, "hello");
    vs.AddVariable("count", VSPinType::Int, "5");
    EXPECT_EQ(vs.GetVariableType("name"), VSPinType::String);
    EXPECT_EQ(vs.GetVariableType("count"), VSPinType::Int);
    EXPECT_EQ(vs.GetVariableType("missing"), VSPinType::Any);
}

TEST(VisualScriptTest, ExecuteSimple) {
    VisualScript vs;

    // イベントノードを登録
    VSNodeDef eventDef;
    eventDef.typeName = "Start";
    eventDef.category = VSNodeCategory::Event;
    eventDef.isEvent = true;
    eventDef.outputPins = { {"Exec", VSPinType::Exec, 0, 0, true} };
    vs.RegisterNodeType(eventDef);

    // アクションノードを登録
    VSNodeDef actionDef;
    actionDef.typeName = "DoSomething";
    actionDef.category = VSNodeCategory::Flow;
    actionDef.inputPins = { {"Exec", VSPinType::Exec, 0, 0, false} };
    actionDef.outputPins = { {"Exec", VSPinType::Exec, 0, 0, true} };
    vs.RegisterNodeType(actionDef);

    uint32_t startNode = vs.AddNode("Start");
    uint32_t actionNode = vs.AddNode("DoSomething");
    vs.Connect(startNode, 0, actionNode, 0);

    bool startHandled = false;
    bool actionHandled = false;
    vs.SetNodeHandler("Start", [&](VSNodeInstance&, VSExecutionContext&) {
        startHandled = true;
        return true;
    });
    vs.SetNodeHandler("DoSomething", [&](VSNodeInstance&, VSExecutionContext&) {
        actionHandled = true;
        return true;
    });

    VSExecutionContext ctx;
    bool result = vs.Execute(startNode, ctx);
    EXPECT_TRUE(result);
    EXPECT_TRUE(startHandled);
    EXPECT_TRUE(actionHandled);
    EXPECT_EQ(ctx.executionCount, 2u);
}

TEST(VisualScriptTest, ExecuteBranch) {
    VisualScript vs;
    vs.RegisterBuiltinTypes();

    uint32_t startNode = vs.AddNode("OnBeginPlay");
    uint32_t branchNode = vs.AddNode("Branch");
    uint32_t trueNode = vs.AddNode("Print");
    uint32_t falseNode = vs.AddNode("Print");

    vs.Connect(startNode, 0, branchNode, 0);
    vs.Connect(branchNode, 0, trueNode, 0);   // True出力
    vs.Connect(branchNode, 1, falseNode, 0);   // False出力

    bool truePath = false;
    bool falsePath = false;

    vs.SetNodeHandler("OnBeginPlay", [](VSNodeInstance&, VSExecutionContext&) { return true; });
    vs.SetNodeHandler("Branch", [](VSNodeInstance&, VSExecutionContext& ctx) {
        // Condition=trueの場合、exec pin 0 (True)を辿る
        ctx.variables["__next_exec_pin"] = "0";
        return true;
    });
    vs.SetNodeHandler("Print", [&](VSNodeInstance& node, VSExecutionContext&) {
        // trueNodeかfalseNodeか判定
        if (node.id == trueNode) truePath = true;
        if (node.id == falseNode) falsePath = true;
        return true;
    });

    VSExecutionContext ctx;
    vs.Execute(startNode, ctx);
    EXPECT_TRUE(truePath);
    EXPECT_FALSE(falsePath);
}

TEST(VisualScriptTest, ExecuteSequence) {
    VisualScript vs;
    vs.RegisterBuiltinTypes();

    uint32_t startNode = vs.AddNode("OnBeginPlay");
    uint32_t seqNode = vs.AddNode("Sequence");
    uint32_t action0 = vs.AddNode("Print");
    uint32_t action1 = vs.AddNode("Print");

    vs.Connect(startNode, 0, seqNode, 0);
    vs.Connect(seqNode, 0, action0, 0);  // Then 0
    vs.Connect(seqNode, 1, action1, 0);  // Then 1

    int execCount = 0;
    vs.SetNodeHandler("OnBeginPlay", [](VSNodeInstance&, VSExecutionContext&) { return true; });
    vs.SetNodeHandler("Sequence", [](VSNodeInstance&, VSExecutionContext& ctx) {
        ctx.variables["__sequence_pins"] = "0,1";
        return true;
    });
    vs.SetNodeHandler("Print", [&](VSNodeInstance&, VSExecutionContext&) {
        execCount++;
        return true;
    });

    VSExecutionContext ctx;
    vs.Execute(startNode, ctx);
    EXPECT_EQ(execCount, 2);
}

TEST(VisualScriptTest, RegisterBuiltins) {
    VisualScript vs;
    vs.RegisterBuiltinTypes();

    EXPECT_TRUE(vs.HasNodeType("OnBeginPlay"));
    EXPECT_TRUE(vs.HasNodeType("OnTick"));
    EXPECT_TRUE(vs.HasNodeType("OnOverlap"));
    EXPECT_TRUE(vs.HasNodeType("Branch"));
    EXPECT_TRUE(vs.HasNodeType("Sequence"));
    EXPECT_TRUE(vs.HasNodeType("ForLoop"));
    EXPECT_TRUE(vs.HasNodeType("Print"));
    EXPECT_TRUE(vs.HasNodeType("Add"));
    EXPECT_TRUE(vs.HasNodeType("Subtract"));
    EXPECT_TRUE(vs.HasNodeType("Multiply"));
    EXPECT_TRUE(vs.HasNodeType("Divide"));
    EXPECT_TRUE(vs.HasNodeType("Equal"));
    EXPECT_TRUE(vs.HasNodeType("NotEqual"));
    EXPECT_TRUE(vs.HasNodeType("GreaterThan"));
    EXPECT_TRUE(vs.HasNodeType("LessThan"));
    EXPECT_TRUE(vs.HasNodeType("And"));
    EXPECT_TRUE(vs.HasNodeType("Or"));
    EXPECT_TRUE(vs.HasNodeType("Not"));
    EXPECT_TRUE(vs.HasNodeType("GetVariable"));
    EXPECT_TRUE(vs.HasNodeType("SetVariable"));
    EXPECT_GE(vs.GetRegisteredTypeCount(), 20u);
}

TEST(VisualScriptTest, SaveLoad) {
    // Save
    {
        VisualScript vs;
        vs.RegisterNodeType(MakeSimpleNode("EventNode", VSNodeCategory::Event, true));
        vs.RegisterNodeType(MakeSimpleNode("ActionNode"));

        uint32_t n1 = vs.AddNode("EventNode", 10.0f, 20.0f);
        uint32_t n2 = vs.AddNode("ActionNode", 30.0f, 40.0f);
        vs.Connect(n1, 0, n2, 0);
        vs.AddVariable("hp", VSPinType::Float, "100");

        EXPECT_TRUE(vs.Save("test_vs_save.tmp"));
    }

    // Load
    {
        VisualScript vs;
        vs.RegisterNodeType(MakeSimpleNode("EventNode", VSNodeCategory::Event, true));
        vs.RegisterNodeType(MakeSimpleNode("ActionNode"));

        EXPECT_TRUE(vs.Load("test_vs_save.tmp"));
        EXPECT_EQ(vs.GetNodeCount(), 2u);
        EXPECT_EQ(vs.GetConnectionCount(), 1u);
        EXPECT_EQ(vs.GetVariableCount(), 1u);
        EXPECT_EQ(vs.GetVariable("hp"), "100");
    }

    // クリーンアップ
    std::remove("test_vs_save.tmp");
}

TEST(VisualScriptTest, ClearAll) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("N"));
    vs.AddNode("N");
    vs.AddVariable("x", VSPinType::Int);

    vs.Clear();
    EXPECT_EQ(vs.GetNodeCount(), 0u);
    EXPECT_EQ(vs.GetConnectionCount(), 0u);
    EXPECT_EQ(vs.GetVariableCount(), 0u);
    // ノード型は保持される
    EXPECT_TRUE(vs.HasNodeType("N"));
}

TEST(VisualScriptTest, EntryNodes) {
    VisualScript vs;

    VSNodeDef eventDef;
    eventDef.typeName = "GameStart";
    eventDef.category = VSNodeCategory::Event;
    eventDef.isEvent = true;
    eventDef.outputPins = { {"Exec", VSPinType::Exec, 0, 0, true} };
    vs.RegisterNodeType(eventDef);

    vs.RegisterNodeType(MakeSimpleNode("Action"));

    uint32_t ev1 = vs.AddNode("GameStart");
    uint32_t ev2 = vs.AddNode("GameStart");
    vs.AddNode("Action");

    auto entries = vs.GetEntryNodes();
    EXPECT_EQ(entries.size(), 2u);

    // エントリーノードに含まれることを確認
    bool found1 = std::find(entries.begin(), entries.end(), ev1) != entries.end();
    bool found2 = std::find(entries.begin(), entries.end(), ev2) != entries.end();
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

TEST(VisualScriptTest, NodeHandler) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("TestEvent", VSNodeCategory::Event, true));

    uint32_t n = vs.AddNode("TestEvent");

    int callCount = 0;
    vs.SetNodeHandler("TestEvent", [&](VSNodeInstance&, VSExecutionContext&) {
        callCount++;
        return true;
    });

    VSExecutionContext ctx;
    vs.Execute(n, ctx);
    EXPECT_EQ(callCount, 1);
}

TEST(VisualScriptTest, PureFunctionEval) {
    VisualScript vs;

    VSNodeDef pureDef;
    pureDef.typeName = "AddNumbers";
    pureDef.category = VSNodeCategory::Math;
    pureDef.isPure = true;
    pureDef.inputPins = {
        {"A", VSPinType::Float, 0, 0, false},
        {"B", VSPinType::Float, 0, 1, false}
    };
    pureDef.outputPins = { {"Result", VSPinType::Float, 0, 0, true} };
    vs.RegisterNodeType(pureDef);

    const VSNodeDef* def = vs.GetNodeType("AddNumbers");
    ASSERT_NE(def, nullptr);
    EXPECT_TRUE(def->isPure);
    EXPECT_EQ(def->category, VSNodeCategory::Math);
    EXPECT_EQ(def->inputPins.size(), 2u);
    EXPECT_EQ(def->outputPins.size(), 1u);
}

TEST(VisualScriptTest, RemoveNodeCleansConnections) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    vs.RegisterNodeType(MakeSimpleNode("C"));

    uint32_t a = vs.AddNode("A");
    uint32_t b = vs.AddNode("B");
    uint32_t c = vs.AddNode("C");

    vs.Connect(a, 0, b, 0);
    vs.Connect(b, 0, c, 0);
    EXPECT_EQ(vs.GetConnectionCount(), 2u);

    vs.RemoveNode(b);
    EXPECT_EQ(vs.GetConnectionCount(), 0u);
}

TEST(VisualScriptTest, DuplicateConnection) {
    VisualScript vs;
    vs.RegisterNodeType(MakeSimpleNode("A"));
    vs.RegisterNodeType(MakeSimpleNode("B"));
    uint32_t a = vs.AddNode("A");
    uint32_t b = vs.AddNode("B");

    EXPECT_TRUE(vs.Connect(a, 0, b, 0));
    EXPECT_FALSE(vs.Connect(a, 0, b, 0)); // 重複は拒否
    EXPECT_EQ(vs.GetConnectionCount(), 1u);
}
