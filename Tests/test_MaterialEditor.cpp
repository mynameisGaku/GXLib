/// @file test_MaterialEditor.cpp
#include <gtest/gtest.h>
#include "Editor/MaterialEditor.h"
using namespace gx;

TEST(MaterialEditorTest, DefaultState) {
    MaterialEditor me;
    EXPECT_FALSE(me.HasMaterial());
    EXPECT_EQ(me.GetShaderModel(), ShaderModelType::PBR);
    EXPECT_FALSE(me.IsDirty());
}
TEST(MaterialEditorTest, SetMaterial) {
    MaterialEditor me;
    me.SetMaterial("TestMat");
    EXPECT_TRUE(me.HasMaterial());
    EXPECT_EQ(me.GetMaterialName(), "TestMat");
}
TEST(MaterialEditorTest, ClearMaterial) {
    MaterialEditor me;
    me.SetMaterial("TestMat");
    me.ClearMaterial();
    EXPECT_FALSE(me.HasMaterial());
}
TEST(MaterialEditorTest, SetShaderModel) {
    MaterialEditor me;
    me.SetShaderModel(ShaderModelType::Toon);
    EXPECT_EQ(me.GetShaderModel(), ShaderModelType::Toon);
}
TEST(MaterialEditorTest, BaseColor) {
    MaterialEditor me;
    me.SetBaseColor(0.5f, 0.3f, 0.1f);
    float r, g, b;
    me.GetBaseColor(r, g, b);
    EXPECT_FLOAT_EQ(r, 0.5f);
    EXPECT_FLOAT_EQ(g, 0.3f);
    EXPECT_FLOAT_EQ(b, 0.1f);
}
TEST(MaterialEditorTest, BaseColorClamp) {
    MaterialEditor me;
    me.SetBaseColor(2.0f, -1.0f, 0.5f);
    float r, g, b;
    me.GetBaseColor(r, g, b);
    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 0.0f);
}
TEST(MaterialEditorTest, Metallic) {
    MaterialEditor me;
    me.SetMetallic(0.8f);
    EXPECT_FLOAT_EQ(me.GetMetallic(), 0.8f);
}
TEST(MaterialEditorTest, MetallicClamp) {
    MaterialEditor me;
    me.SetMetallic(1.5f);
    EXPECT_LE(me.GetMetallic(), 1.0f);
}
TEST(MaterialEditorTest, Roughness) {
    MaterialEditor me;
    me.SetRoughness(0.3f);
    EXPECT_FLOAT_EQ(me.GetRoughness(), 0.3f);
}
TEST(MaterialEditorTest, EmissiveColor) {
    MaterialEditor me;
    me.SetEmissiveColor(1.0f, 0.5f, 0.0f);
    float r, g, b;
    me.GetEmissiveColor(r, g, b);
    EXPECT_FLOAT_EQ(r, 1.0f);
}
TEST(MaterialEditorTest, EmissiveIntensity) {
    MaterialEditor me;
    me.SetEmissiveIntensity(5.0f);
    EXPECT_FLOAT_EQ(me.GetEmissiveIntensity(), 5.0f);
}
TEST(MaterialEditorTest, EmissiveIntensityClampNegative) {
    MaterialEditor me;
    me.SetEmissiveIntensity(-1.0f);
    EXPECT_GE(me.GetEmissiveIntensity(), 0.0f);
}
TEST(MaterialEditorTest, TextureSlot) {
    MaterialEditor me;
    me.SetTextureSlot("Albedo", "textures/albedo.dds");
    EXPECT_EQ(me.GetTextureSlot("Albedo"), "textures/albedo.dds");
    EXPECT_EQ(me.GetTextureSlotCount(), 1u);
}
TEST(MaterialEditorTest, TextureSlotOverwrite) {
    MaterialEditor me;
    me.SetTextureSlot("Normal", "a.dds");
    me.SetTextureSlot("Normal", "b.dds");
    EXPECT_EQ(me.GetTextureSlot("Normal"), "b.dds");
    EXPECT_EQ(me.GetTextureSlotCount(), 1u);
}
TEST(MaterialEditorTest, TextureSlotMissing) {
    MaterialEditor me;
    EXPECT_EQ(me.GetTextureSlot("Missing"), "");
}
TEST(MaterialEditorTest, GetTextureSlots) {
    MaterialEditor me;
    me.SetTextureSlot("Albedo", "a.dds");
    me.SetTextureSlot("Normal", "n.dds");
    auto slots = me.GetTextureSlots();
    EXPECT_EQ(slots.size(), 2u);
}
TEST(MaterialEditorTest, AddNode) {
    MaterialEditor me;
    uint32_t id = me.AddNode(MENodeType::TextureSample, "TexSample");
    EXPECT_EQ(me.GetNodeCount(), 1u);
    auto* node = me.GetNode(id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->type, MENodeType::TextureSample);
}
TEST(MaterialEditorTest, RemoveNode) {
    MaterialEditor me;
    uint32_t id = me.AddNode(MENodeType::Multiply);
    me.RemoveNode(id);
    EXPECT_EQ(me.GetNodeCount(), 0u);
}
TEST(MaterialEditorTest, ConnectNodes) {
    MaterialEditor me;
    uint32_t a = me.AddNode(MENodeType::TextureSample);
    uint32_t b = me.AddNode(MENodeType::Output);
    me.ConnectNodes(a, 0, b, 0);
    EXPECT_EQ(me.GetConnectionCount(), 1u);
}
TEST(MaterialEditorTest, DisconnectNodes) {
    MaterialEditor me;
    uint32_t a = me.AddNode(MENodeType::TextureSample);
    uint32_t b = me.AddNode(MENodeType::Output);
    me.ConnectNodes(a, 0, b, 0);
    me.DisconnectNodes(a, 0, b, 0);
    EXPECT_EQ(me.GetConnectionCount(), 0u);
}
TEST(MaterialEditorTest, RemoveNodeClearsConnections) {
    MaterialEditor me;
    uint32_t a = me.AddNode(MENodeType::TextureSample);
    uint32_t b = me.AddNode(MENodeType::Output);
    me.ConnectNodes(a, 0, b, 0);
    me.RemoveNode(a);
    EXPECT_EQ(me.GetConnectionCount(), 0u);
}
TEST(MaterialEditorTest, GetEditableParams) {
    MaterialEditor me;
    auto params = me.GetEditableParams();
    EXPECT_GE(params.size(), 5u); // BaseColor, Metallic, Roughness, EmissiveColor, EmissiveIntensity
}
TEST(MaterialEditorTest, SavePreset) {
    MaterialEditor me;
    me.SetMetallic(0.9f);
    me.SetRoughness(0.1f);
    auto preset = me.SaveAsPreset("Chrome");
    EXPECT_EQ(preset.name, "Chrome");
    EXPECT_EQ(preset.shaderModel, ShaderModelType::PBR);
}
TEST(MaterialEditorTest, LoadPreset) {
    MaterialEditor me;
    MaterialPreset preset;
    preset.shaderModel = ShaderModelType::Toon;
    MaterialParam p; p.name = "Metallic"; p.type = MaterialParam::Type::Float; p.floatValue = 0.7f;
    preset.params.push_back(p);
    me.LoadPreset(preset);
    EXPECT_EQ(me.GetShaderModel(), ShaderModelType::Toon);
    EXPECT_FLOAT_EQ(me.GetMetallic(), 0.7f);
}
TEST(MaterialEditorTest, BuiltinPresets) {
    auto names = MaterialEditor::GetBuiltinPresetNames();
    EXPECT_GE(names.size(), 4u);
}
TEST(MaterialEditorTest, DirtyFlag) {
    MaterialEditor me;
    EXPECT_FALSE(me.IsDirty());
    me.SetMetallic(0.5f);
    EXPECT_TRUE(me.IsDirty());
    me.ClearDirty();
    EXPECT_FALSE(me.IsDirty());
}
