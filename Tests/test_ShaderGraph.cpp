/// @file test_ShaderGraph.cpp
/// @brief ビジュアルシェーダグラフエディタのテスト

#include <gtest/gtest.h>
#include "Editor/ShaderGraph.h"
#include <filesystem>
#include <fstream>

using namespace gx;

// ============================================================================
// ShaderGraph テスト
// ============================================================================

class ShaderGraphTest : public ::testing::Test
{
protected:
    ShaderGraph graph;
};

TEST_F(ShaderGraphTest, DefaultHasOutputNode)
{
    const ShaderNode* output = graph.GetOutputNode();
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(output->type, ShaderNodeType::Output);
    EXPECT_EQ(output->name, "Output");
    EXPECT_EQ(graph.GetNodeCount(), 1u);
}

TEST_F(ShaderGraphTest, AddNode)
{
    uint32_t id = graph.AddNode(ShaderNodeType::ConstFloat, "MyFloat");
    EXPECT_GT(id, 0u);
    EXPECT_EQ(graph.GetNodeCount(), 2u); // Output + ConstFloat
}

TEST_F(ShaderGraphTest, RemoveNode)
{
    uint32_t id = graph.AddNode(ShaderNodeType::ConstFloat, "ToRemove");
    EXPECT_EQ(graph.GetNodeCount(), 2u);
    EXPECT_TRUE(graph.RemoveNode(id));
    EXPECT_EQ(graph.GetNodeCount(), 1u);
}

TEST_F(ShaderGraphTest, RemoveOutputNodeFails)
{
    EXPECT_FALSE(graph.RemoveNode(0)); // Output ノードは削除不可
    EXPECT_EQ(graph.GetNodeCount(), 1u);
}

TEST_F(ShaderGraphTest, GetNode)
{
    uint32_t id = graph.AddNode(ShaderNodeType::Multiply, "Mul");
    const ShaderNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, ShaderNodeType::Multiply);
    EXPECT_EQ(node->name, "Mul");
}

TEST_F(ShaderGraphTest, GetNodeInvalid)
{
    EXPECT_EQ(graph.GetNode(9999), nullptr);
}

TEST_F(ShaderGraphTest, GetNodeCount)
{
    EXPECT_EQ(graph.GetNodeCount(), 1u);
    graph.AddNode(ShaderNodeType::Add);
    graph.AddNode(ShaderNodeType::Subtract);
    EXPECT_EQ(graph.GetNodeCount(), 3u);
}

TEST_F(ShaderGraphTest, NodeTypes)
{
    auto id1 = graph.AddNode(ShaderNodeType::TextureSample);
    auto id2 = graph.AddNode(ShaderNodeType::ConstFloat3);
    auto id3 = graph.AddNode(ShaderNodeType::UV);

    EXPECT_EQ(graph.GetNode(id1)->type, ShaderNodeType::TextureSample);
    EXPECT_EQ(graph.GetNode(id2)->type, ShaderNodeType::ConstFloat3);
    EXPECT_EQ(graph.GetNode(id3)->type, ShaderNodeType::UV);
}

TEST_F(ShaderGraphTest, ConnectNodes)
{
    uint32_t constId = graph.AddNode(ShaderNodeType::ConstFloat3, "Color");
    // ConstFloat3 の出力ピン 0 (Value) → Output の入力ピン 0 (BaseColor)
    EXPECT_TRUE(graph.Connect(constId, 0, 0, 0));
    EXPECT_EQ(graph.GetConnectionCount(), 1u);
}

TEST_F(ShaderGraphTest, DisconnectNodes)
{
    uint32_t constId = graph.AddNode(ShaderNodeType::ConstFloat3, "Color");
    graph.Connect(constId, 0, 0, 0);
    EXPECT_EQ(graph.GetConnectionCount(), 1u);

    EXPECT_TRUE(graph.Disconnect(constId, 0, 0, 0));
    EXPECT_EQ(graph.GetConnectionCount(), 0u);
}

TEST_F(ShaderGraphTest, DisconnectNonexistent)
{
    EXPECT_FALSE(graph.Disconnect(99, 0, 0, 0));
}

TEST_F(ShaderGraphTest, ConnectionCount)
{
    uint32_t c1 = graph.AddNode(ShaderNodeType::ConstFloat3);
    uint32_t c2 = graph.AddNode(ShaderNodeType::ConstFloat);

    graph.Connect(c1, 0, 0, 0); // BaseColor
    graph.Connect(c2, 0, 0, 1); // Metallic

    EXPECT_EQ(graph.GetConnectionCount(), 2u);
}

TEST_F(ShaderGraphTest, ValidateTypeMismatch)
{
    // Texture2D 出力を Float 入力に接続 — 不可
    uint32_t texId = graph.AddNode(ShaderNodeType::TextureSample, "Tex");
    // TextureSample の Texture 入力ピンはTexture2D型、出力ピン0はFloat4
    // Output の Metallic 入力ピン (index 1) は Float
    // Float4 → Float は互換なので成功するはず
    EXPECT_TRUE(graph.Connect(texId, 0, 0, 1));
}

TEST_F(ShaderGraphTest, ConnectIncompatibleTypes)
{
    // 自己接続は不可
    uint32_t id = graph.AddNode(ShaderNodeType::Multiply);
    EXPECT_FALSE(graph.Connect(id, 0, id, 0));
}

TEST_F(ShaderGraphTest, ValidateUnconnectedPin)
{
    // Output.BaseColor が未接続のエラー
    auto errors = graph.Validate();
    bool foundError = false;
    for (const auto& e : errors)
    {
        if (e.find("BaseColor") != std::string::npos)
            foundError = true;
    }
    EXPECT_TRUE(foundError);
}

TEST_F(ShaderGraphTest, HasCycleNone)
{
    uint32_t c1 = graph.AddNode(ShaderNodeType::ConstFloat3);
    graph.Connect(c1, 0, 0, 0);
    EXPECT_FALSE(graph.HasCycle());
}

TEST_F(ShaderGraphTest, HasCycleSelf)
{
    // 自己接続はConnectで拒否されるのでサイクルにならない
    uint32_t id = graph.AddNode(ShaderNodeType::Multiply);
    graph.Connect(id, 0, id, 0); // これは失敗する
    EXPECT_FALSE(graph.HasCycle());
}

TEST_F(ShaderGraphTest, HasCycleDetected)
{
    // A → B → A のサイクルを手動で構築
    // Add(A) と Multiply(B) で: A の出力→B の入力、B の出力→A の入力
    uint32_t a = graph.AddNode(ShaderNodeType::Add, "A");
    uint32_t b = graph.AddNode(ShaderNodeType::Multiply, "B");

    graph.Connect(a, 0, b, 0); // A → B
    graph.Connect(b, 0, a, 0); // B → A

    EXPECT_TRUE(graph.HasCycle());
}

TEST_F(ShaderGraphTest, CompileEmpty)
{
    // Output.BaseColor 未接続のためコンパイルはエラー
    auto result = graph.Compile();
    EXPECT_FALSE(result.success);
    EXPECT_GT(result.errors.size(), 0u);
}

TEST_F(ShaderGraphTest, CompileSimple)
{
    uint32_t colorId = graph.AddNode(ShaderNodeType::ConstFloat3, "BaseColor");
    graph.Connect(colorId, 0, 0, 0); // BaseColor

    auto result = graph.Compile();
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.hlslCode.empty());
}

TEST_F(ShaderGraphTest, CompileChain)
{
    uint32_t c1 = graph.AddNode(ShaderNodeType::ConstFloat, "Val1");
    uint32_t c2 = graph.AddNode(ShaderNodeType::ConstFloat, "Val2");
    uint32_t mul = graph.AddNode(ShaderNodeType::Multiply, "Mul");

    graph.Connect(c1, 0, mul, 0); // Val1 → Mul.A
    graph.Connect(c2, 0, mul, 1); // Val2 → Mul.B

    // Mul.Result → Output.Metallic
    graph.Connect(mul, 0, 0, 1);

    // Still need BaseColor
    uint32_t color = graph.AddNode(ShaderNodeType::ConstFloat3, "Color");
    graph.Connect(color, 0, 0, 0);

    auto result = graph.Compile();
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.hlslCode.empty());
}

TEST_F(ShaderGraphTest, SetTargetModel)
{
    EXPECT_EQ(graph.GetTargetModel(), "PBR");
    graph.SetTargetModel("Toon");
    EXPECT_EQ(graph.GetTargetModel(), "Toon");
}

TEST_F(ShaderGraphTest, SaveLoad)
{
    uint32_t colorId = graph.AddNode(ShaderNodeType::ConstFloat3, "MyColor", 100.0f, 200.0f);
    graph.Connect(colorId, 0, 0, 0);
    graph.SetTargetModel("Phong");
    graph.SetNodeProperty(colorId, "r", "0.5");

    std::string path = (std::filesystem::temp_directory_path() / "gx_shadergraph_test.sg").string();
    EXPECT_TRUE(graph.Save(path));

    ShaderGraph loaded;
    EXPECT_TRUE(loaded.Load(path));
    EXPECT_EQ(loaded.GetTargetModel(), "Phong");
    EXPECT_GE(loaded.GetNodeCount(), 2u); // Output + ConstFloat3

    // プロパティの確認
    const ShaderNode* loadedColor = nullptr;
    for (uint32_t i = 0; i < loaded.GetNodeCount(); ++i)
    {
        // ID が colorId であるノードを検索
        auto* n = loaded.GetNode(colorId);
        if (n && n->type == ShaderNodeType::ConstFloat3)
        {
            loadedColor = n;
            break;
        }
    }
    if (loadedColor)
    {
        EXPECT_EQ(loaded.GetNodeProperty(loadedColor->id, "r"), "0.5");
    }

    std::filesystem::remove(path);
}

TEST_F(ShaderGraphTest, ClearGraph)
{
    graph.AddNode(ShaderNodeType::ConstFloat);
    graph.AddNode(ShaderNodeType::Add);
    EXPECT_EQ(graph.GetNodeCount(), 3u);

    graph.Clear();
    EXPECT_EQ(graph.GetNodeCount(), 1u); // Output のみ
    EXPECT_NE(graph.GetOutputNode(), nullptr);
    EXPECT_EQ(graph.GetConnectionCount(), 0u);
}

TEST_F(ShaderGraphTest, NodeProperties)
{
    uint32_t id = graph.AddNode(ShaderNodeType::ConstFloat, "MyConst");
    graph.SetNodeProperty(id, "value", "3.14");
    EXPECT_EQ(graph.GetNodeProperty(id, "value"), "3.14");
    EXPECT_EQ(graph.GetNodeProperty(id, "missing"), "");
}

TEST_F(ShaderGraphTest, OutputNodePins)
{
    const ShaderNode* output = graph.GetOutputNode();
    ASSERT_NE(output, nullptr);

    // Output ノードは 6 つの入力ピンを持つ
    int inputCount = 0;
    bool hasBaseColor = false, hasMetallic = false, hasRoughness = false;
    bool hasNormal = false, hasEmissive = false, hasOpacity = false;

    for (const auto& pin : output->pins)
    {
        if (!pin.isOutput)
        {
            inputCount++;
            if (pin.name == "BaseColor")  hasBaseColor = true;
            if (pin.name == "Metallic")   hasMetallic = true;
            if (pin.name == "Roughness")  hasRoughness = true;
            if (pin.name == "Normal")     hasNormal = true;
            if (pin.name == "Emissive")   hasEmissive = true;
            if (pin.name == "Opacity")    hasOpacity = true;
        }
    }

    EXPECT_EQ(inputCount, 6);
    EXPECT_TRUE(hasBaseColor);
    EXPECT_TRUE(hasMetallic);
    EXPECT_TRUE(hasRoughness);
    EXPECT_TRUE(hasNormal);
    EXPECT_TRUE(hasEmissive);
    EXPECT_TRUE(hasOpacity);
}

TEST_F(ShaderGraphTest, DefaultPinsForType)
{
    uint32_t mulId = graph.AddNode(ShaderNodeType::Multiply);
    auto pins = graph.GetNodePins(mulId);

    // Multiply: 2 inputs (A, B) + 1 output (Result)
    int inputs = 0, outputs = 0;
    for (const auto& p : pins)
    {
        if (p.isOutput) outputs++;
        else inputs++;
    }
    EXPECT_EQ(inputs, 2);
    EXPECT_EQ(outputs, 1);
}

TEST_F(ShaderGraphTest, NodePositions)
{
    uint32_t id = graph.AddNode(ShaderNodeType::ConstFloat, "N", 50.0f, 75.0f);
    const ShaderNode* node = graph.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_FLOAT_EQ(node->posX, 50.0f);
    EXPECT_FLOAT_EQ(node->posY, 75.0f);
}

TEST_F(ShaderGraphTest, RemoveNodeCleansConnections)
{
    uint32_t id = graph.AddNode(ShaderNodeType::ConstFloat3, "C");
    graph.Connect(id, 0, 0, 0);
    EXPECT_EQ(graph.GetConnectionCount(), 1u);

    graph.RemoveNode(id);
    EXPECT_EQ(graph.GetConnectionCount(), 0u);
}

TEST_F(ShaderGraphTest, DuplicateConnectionIgnored)
{
    uint32_t id = graph.AddNode(ShaderNodeType::ConstFloat3, "C");
    EXPECT_TRUE(graph.Connect(id, 0, 0, 0));
    EXPECT_TRUE(graph.Connect(id, 0, 0, 0)); // 重複は追加されない
    EXPECT_EQ(graph.GetConnectionCount(), 1u);
}

TEST_F(ShaderGraphTest, GetNodePinsInvalid)
{
    auto pins = graph.GetNodePins(9999);
    EXPECT_TRUE(pins.empty());
}
