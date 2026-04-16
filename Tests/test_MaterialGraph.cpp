/// @file test_MaterialGraph.cpp
/// @brief MaterialGraph ノードベースマテリアルグラフテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/MaterialGraph.h"

using namespace gx;

static constexpr float kEps = 1e-5f;

// ==================== NodeCreation ====================

TEST(MaterialGraphTest, AddNode_Float)
{
    MaterialGraph graph;
    int id = graph.AddNode(MaterialNodeType::Float, "MyFloat");
    EXPECT_GE(id, 0);

    MaterialNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, MaterialNodeType::Float);
    EXPECT_EQ(node->name, "MyFloat");
    EXPECT_EQ(node->inputs.size(), 0u);
    EXPECT_EQ(node->outputs.size(), 1u);
}

TEST(MaterialGraphTest, AddNode_Vector3)
{
    MaterialGraph graph;
    int id = graph.AddNode(MaterialNodeType::Vector3, "Normal");
    MaterialNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->outputs.size(), 1u);
    EXPECT_EQ(node->outputs[0].type, PinType::Vec3);
}

TEST(MaterialGraphTest, AddNode_Texture)
{
    MaterialGraph graph;
    int id = graph.AddNode(MaterialNodeType::Texture, "Albedo");
    MaterialNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->outputs.size(), 1u);
    EXPECT_EQ(node->outputs[0].type, PinType::Vec4);
}

TEST(MaterialGraphTest, AddNode_MathNode)
{
    MaterialGraph graph;
    int id = graph.AddNode(MaterialNodeType::Add, "Add");
    MaterialNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->inputs.size(), 2u);
    EXPECT_EQ(node->outputs.size(), 1u);
}

TEST(MaterialGraphTest, AddNode_Lerp)
{
    MaterialGraph graph;
    int id = graph.AddNode(MaterialNodeType::Lerp, "Lerp");
    MaterialNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->inputs.size(), 3u); // A, B, T
    EXPECT_EQ(node->outputs.size(), 1u);
}

TEST(MaterialGraphTest, AddNode_Output)
{
    MaterialGraph graph;
    int id = graph.AddNode(MaterialNodeType::Output, "Output");
    MaterialNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->inputs.size(), 5u); // BaseColor, Metallic, Roughness, Normal, Emissive
    EXPECT_EQ(node->outputs.size(), 0u);
}

TEST(MaterialGraphTest, GetNodeCount)
{
    MaterialGraph graph;
    EXPECT_EQ(graph.GetNodeCount(), 0u);
    graph.AddNode(MaterialNodeType::Float, "A");
    graph.AddNode(MaterialNodeType::Float, "B");
    EXPECT_EQ(graph.GetNodeCount(), 2u);
}

// ==================== Connection ====================

TEST(MaterialGraphTest, Connect_ValidPins)
{
    MaterialGraph graph;
    int floatId = graph.AddNode(MaterialNodeType::Float, "Value");
    int addId = graph.AddNode(MaterialNodeType::Add, "Add");

    bool ok = graph.Connect(floatId, 0, addId, 0);
    EXPECT_TRUE(ok);

    MaterialNode* addNode = graph.GetNode(addId);
    ASSERT_NE(addNode, nullptr);
    EXPECT_TRUE(addNode->inputs[0].connected);
    EXPECT_EQ(addNode->inputs[0].connectedNodeId, floatId);
}

TEST(MaterialGraphTest, Connect_InvalidSrcPin)
{
    MaterialGraph graph;
    int floatId = graph.AddNode(MaterialNodeType::Float, "Value");
    int addId = graph.AddNode(MaterialNodeType::Add, "Add");

    bool ok = graph.Connect(floatId, 99, addId, 0);
    EXPECT_FALSE(ok);
}

TEST(MaterialGraphTest, Connect_InvalidDstPin)
{
    MaterialGraph graph;
    int floatId = graph.AddNode(MaterialNodeType::Float, "Value");
    int addId = graph.AddNode(MaterialNodeType::Add, "Add");

    bool ok = graph.Connect(floatId, 0, addId, 99);
    EXPECT_FALSE(ok);
}

TEST(MaterialGraphTest, Connect_SelfConnection)
{
    MaterialGraph graph;
    int addId = graph.AddNode(MaterialNodeType::Add, "Add");

    bool ok = graph.Connect(addId, 0, addId, 0);
    EXPECT_FALSE(ok);
}

TEST(MaterialGraphTest, Connect_InvalidNodeId)
{
    MaterialGraph graph;
    int floatId = graph.AddNode(MaterialNodeType::Float, "Value");

    bool ok = graph.Connect(floatId, 0, 999, 0);
    EXPECT_FALSE(ok);
}

TEST(MaterialGraphTest, Disconnect_ClearsConnection)
{
    MaterialGraph graph;
    int floatId = graph.AddNode(MaterialNodeType::Float, "Value");
    int addId = graph.AddNode(MaterialNodeType::Add, "Add");

    graph.Connect(floatId, 0, addId, 0);
    graph.Disconnect(addId, 0);

    MaterialNode* addNode = graph.GetNode(addId);
    EXPECT_FALSE(addNode->inputs[0].connected);
    EXPECT_EQ(addNode->inputs[0].connectedNodeId, -1);
}

// ==================== CycleDetection ====================

TEST(MaterialGraphTest, HasCycle_NoCycleInDAG)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Float, "A");
    int b = graph.AddNode(MaterialNodeType::Float, "B");
    int add = graph.AddNode(MaterialNodeType::Add, "Add");
    int out = graph.AddNode(MaterialNodeType::Output, "Output");

    graph.Connect(a, 0, add, 0);
    graph.Connect(b, 0, add, 1);
    graph.Connect(add, 0, out, 0);

    EXPECT_FALSE(graph.HasCycle());
}

TEST(MaterialGraphTest, HasCycle_CycleDetected)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Add, "A");
    int b = graph.AddNode(MaterialNodeType::Add, "B");

    // A→B, B→A = サイクル
    graph.Connect(a, 0, b, 0);
    graph.Connect(b, 0, a, 0);

    EXPECT_TRUE(graph.HasCycle());
}

// ==================== TopologicalSort ====================

TEST(MaterialGraphTest, TopologicalSort_ValidOrder)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Float, "A");
    int b = graph.AddNode(MaterialNodeType::Float, "B");
    int add = graph.AddNode(MaterialNodeType::Add, "Add");
    int out = graph.AddNode(MaterialNodeType::Output, "Output");

    graph.Connect(a, 0, add, 0);
    graph.Connect(b, 0, add, 1);
    graph.Connect(add, 0, out, 0);

    auto sorted = graph.TopologicalSort();
    EXPECT_EQ(sorted.size(), 4u);

    // add は a, b の後にある
    auto posA = std::find(sorted.begin(), sorted.end(), a);
    auto posB = std::find(sorted.begin(), sorted.end(), b);
    auto posAdd = std::find(sorted.begin(), sorted.end(), add);
    auto posOut = std::find(sorted.begin(), sorted.end(), out);

    EXPECT_LT(posA, posAdd);
    EXPECT_LT(posB, posAdd);
    EXPECT_LT(posAdd, posOut);
}

TEST(MaterialGraphTest, TopologicalSort_EmptyGraph)
{
    MaterialGraph graph;
    auto sorted = graph.TopologicalSort();
    EXPECT_TRUE(sorted.empty());
}

TEST(MaterialGraphTest, TopologicalSort_WithCycle_ReturnsEmpty)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Add, "A");
    int b = graph.AddNode(MaterialNodeType::Add, "B");
    graph.Connect(a, 0, b, 0);
    graph.Connect(b, 0, a, 0);

    auto sorted = graph.TopologicalSort();
    EXPECT_TRUE(sorted.empty());
}

// ==================== Compile ====================

TEST(MaterialGraphTest, Compile_SimpleGraph)
{
    MaterialGraph graph;
    int floatId = graph.AddNode(MaterialNodeType::Float, "Roughness");
    int outId = graph.AddNode(MaterialNodeType::Output, "Output");
    graph.SetOutputNode(outId);

    graph.Connect(floatId, 0, outId, 2); // Roughness input

    auto result = graph.Compile();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());
}

TEST(MaterialGraphTest, Compile_EmptyGraph)
{
    MaterialGraph graph;
    auto result = graph.Compile();
    EXPECT_TRUE(result.success);
}

TEST(MaterialGraphTest, Compile_WithCycle_Fails)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Add, "A");
    int b = graph.AddNode(MaterialNodeType::Add, "B");
    graph.Connect(a, 0, b, 0);
    graph.Connect(b, 0, a, 0);

    auto result = graph.Compile();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST(MaterialGraphTest, Compile_TextureSlotCollected)
{
    MaterialGraph graph;
    int tex = graph.AddNode(MaterialNodeType::Texture, "AlbedoTex");
    int outId = graph.AddNode(MaterialNodeType::Output, "Output");

    graph.Connect(tex, 0, outId, 0); // BaseColor

    auto result = graph.Compile();
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.textureSlots.size(), 1u);
    EXPECT_EQ(result.textureSlots[0], "AlbedoTex");
}

// ==================== RemoveNode ====================

TEST(MaterialGraphTest, RemoveNode_DecreasesCount)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Float, "A");
    graph.AddNode(MaterialNodeType::Float, "B");
    EXPECT_EQ(graph.GetNodeCount(), 2u);

    graph.RemoveNode(a);
    EXPECT_EQ(graph.GetNodeCount(), 1u);
    EXPECT_EQ(graph.GetNode(a), nullptr);
}

TEST(MaterialGraphTest, RemoveNode_DisconnectsFromOthers)
{
    MaterialGraph graph;
    int floatId = graph.AddNode(MaterialNodeType::Float, "Value");
    int addId = graph.AddNode(MaterialNodeType::Add, "Add");

    graph.Connect(floatId, 0, addId, 0);
    graph.RemoveNode(floatId);

    MaterialNode* addNode = graph.GetNode(addId);
    ASSERT_NE(addNode, nullptr);
    EXPECT_FALSE(addNode->inputs[0].connected);
}

// ==================== Serialization ====================

TEST(MaterialGraphTest, SerializeToJSON_NotEmpty)
{
    MaterialGraph graph;
    graph.AddNode(MaterialNodeType::Float, "A");
    gx::String json = graph.SerializeToJSON();
    EXPECT_FALSE(json.empty());
    EXPECT_EQ(json[0], '{');
}

TEST(MaterialGraphTest, SerializeDeserialize_RoundTrip)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Float, "ValueA");
    int b = graph.AddNode(MaterialNodeType::Float, "ValueB");
    int add = graph.AddNode(MaterialNodeType::Add, "Add");
    graph.Connect(a, 0, add, 0);
    graph.Connect(b, 0, add, 1);
    graph.SetOutputNode(add);

    gx::String json = graph.SerializeToJSON();

    MaterialGraph loaded;
    bool ok = loaded.DeserializeFromJSON(json);
    EXPECT_TRUE(ok);
    EXPECT_EQ(loaded.GetNodeCount(), 3u);
    EXPECT_EQ(loaded.GetOutputNode(), add);

    // 接続が復元されている
    MaterialNode* addNode = loaded.GetNode(add);
    ASSERT_NE(addNode, nullptr);
    EXPECT_TRUE(addNode->inputs[0].connected);
    EXPECT_EQ(addNode->inputs[0].connectedNodeId, a);
    EXPECT_TRUE(addNode->inputs[1].connected);
    EXPECT_EQ(addNode->inputs[1].connectedNodeId, b);
}

TEST(MaterialGraphTest, DeserializeFromJSON_EmptyString)
{
    MaterialGraph graph;
    bool ok = graph.DeserializeFromJSON("");
    EXPECT_FALSE(ok);
}

TEST(MaterialGraphTest, DeserializeFromJSON_InvalidJSON)
{
    MaterialGraph graph;
    bool ok = graph.DeserializeFromJSON("not json");
    EXPECT_FALSE(ok);
}

// ==================== NodeTypes ====================

TEST(MaterialGraphTest, NodeTypes_AllDistinct)
{
    gx::Vector<MaterialNodeType> types = {
        MaterialNodeType::Float, MaterialNodeType::Vector2,
        MaterialNodeType::Vector3, MaterialNodeType::Vector4,
        MaterialNodeType::Texture, MaterialNodeType::Add,
        MaterialNodeType::Subtract, MaterialNodeType::Multiply,
        MaterialNodeType::Divide, MaterialNodeType::Lerp,
        MaterialNodeType::Clamp, MaterialNodeType::Dot,
        MaterialNodeType::Normalize, MaterialNodeType::Output
    };

    for (size_t i = 0; i < types.size(); ++i)
    {
        for (size_t j = i + 1; j < types.size(); ++j)
        {
            EXPECT_NE(static_cast<int>(types[i]), static_cast<int>(types[j]));
        }
    }
}

// ==================== PinTypes ====================

TEST(MaterialGraphTest, PinTypes_AllDistinct)
{
    gx::Vector<PinType> types = {
        PinType::Float, PinType::Vec2, PinType::Vec3, PinType::Vec4, PinType::Texture
    };

    for (size_t i = 0; i < types.size(); ++i)
    {
        for (size_t j = i + 1; j < types.size(); ++j)
        {
            EXPECT_NE(static_cast<int>(types[i]), static_cast<int>(types[j]));
        }
    }
}

// ==================== ComplexGraph ====================

TEST(MaterialGraphTest, ComplexGraph_MultiNodeLerp)
{
    MaterialGraph graph;
    int a = graph.AddNode(MaterialNodeType::Float, "White");
    int b = graph.AddNode(MaterialNodeType::Float, "Black");
    int t = graph.AddNode(MaterialNodeType::Float, "Factor");
    int lerp = graph.AddNode(MaterialNodeType::Lerp, "Lerp");
    int out = graph.AddNode(MaterialNodeType::Output, "Output");

    graph.Connect(a, 0, lerp, 0);  // A
    graph.Connect(b, 0, lerp, 1);  // B
    graph.Connect(t, 0, lerp, 2);  // T
    graph.Connect(lerp, 0, out, 0); // BaseColor

    graph.SetOutputNode(out);

    EXPECT_FALSE(graph.HasCycle());
    auto sorted = graph.TopologicalSort();
    EXPECT_EQ(sorted.size(), 5u);

    auto result = graph.Compile();
    EXPECT_TRUE(result.success);
}

// ==================== Clear ====================

TEST(MaterialGraphTest, Clear_ResetsAllState)
{
    MaterialGraph graph;
    graph.AddNode(MaterialNodeType::Float, "A");
    graph.AddNode(MaterialNodeType::Float, "B");
    int out = graph.AddNode(MaterialNodeType::Output, "Out");
    graph.SetOutputNode(out);

    graph.Clear();
    EXPECT_EQ(graph.GetNodeCount(), 0u);
    EXPECT_EQ(graph.GetOutputNode(), -1);
}

// ==================== OutputNode ====================

TEST(MaterialGraphTest, OutputNode_SetGet)
{
    MaterialGraph graph;
    EXPECT_EQ(graph.GetOutputNode(), -1);

    int out = graph.AddNode(MaterialNodeType::Output, "Out");
    graph.SetOutputNode(out);
    EXPECT_EQ(graph.GetOutputNode(), out);
}

TEST(MaterialGraphTest, OutputNode_RemovedResetsToInvalid)
{
    MaterialGraph graph;
    int out = graph.AddNode(MaterialNodeType::Output, "Out");
    graph.SetOutputNode(out);
    graph.RemoveNode(out);
    EXPECT_EQ(graph.GetOutputNode(), -1);
}
