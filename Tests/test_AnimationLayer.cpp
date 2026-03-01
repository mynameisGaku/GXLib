/// @file test_AnimationLayer.cpp
/// @brief AnimationLayer / BoneMask / AnimationLayerStack のテスト

#include <gtest/gtest.h>
#include "Graphics/3D/AnimationLayer.h"

using namespace gx;

// ==================== BoneMask ====================

TEST(BoneMaskTest, DefaultEmpty)
{
    BoneMask mask;
    EXPECT_EQ(mask.GetBoneCount(), 0u);
    EXPECT_FALSE(mask.IsActive());
}

TEST(BoneMaskTest, Initialize)
{
    BoneMask mask;
    mask.Initialize(10);
    EXPECT_EQ(mask.GetBoneCount(), 10u);
    EXPECT_TRUE(mask.IsActive());
    // All weights should be 1.0
    for (uint32_t i = 0; i < 10; ++i)
        EXPECT_FLOAT_EQ(mask.GetWeight(i), 1.0f);
}

TEST(BoneMaskTest, SetWeight)
{
    BoneMask mask;
    mask.Initialize(5);
    mask.SetWeight(2, 0.5f);
    EXPECT_FLOAT_EQ(mask.GetWeight(2), 0.5f);
    EXPECT_FLOAT_EQ(mask.GetWeight(0), 1.0f);
}

TEST(BoneMaskTest, SetAll)
{
    BoneMask mask;
    mask.Initialize(4);
    mask.SetAll(0.3f);
    for (uint32_t i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ(mask.GetWeight(i), 0.3f);
}

TEST(BoneMaskTest, ClearAll)
{
    BoneMask mask;
    mask.Initialize(3);
    mask.ClearAll();
    for (uint32_t i = 0; i < 3; ++i)
        EXPECT_FLOAT_EQ(mask.GetWeight(i), 0.0f);
    EXPECT_FALSE(mask.IsActive());
}

TEST(BoneMaskTest, SetFromJointDown)
{
    BoneMask mask;
    mask.Initialize(8);
    mask.SetAll(0.0f);
    mask.SetFromJointDown(3, 6, 1.0f);
    EXPECT_FLOAT_EQ(mask.GetWeight(2), 0.0f); // before range
    EXPECT_FLOAT_EQ(mask.GetWeight(3), 1.0f); // start of range
    EXPECT_FLOAT_EQ(mask.GetWeight(5), 1.0f); // in range
    EXPECT_FLOAT_EQ(mask.GetWeight(6), 1.0f); // end of range
    EXPECT_FLOAT_EQ(mask.GetWeight(7), 0.0f); // after range
}

TEST(BoneMaskTest, OutOfBoundsWeight)
{
    BoneMask mask;
    mask.Initialize(3);
    EXPECT_FLOAT_EQ(mask.GetWeight(100), 0.0f); // out of bounds returns 0
}

// ==================== AnimationLayerStack ====================

TEST(AnimLayerStackTest, DefaultEmpty)
{
    AnimationLayerStack stack;
    EXPECT_EQ(stack.GetLayerCount(), 0u);
}

TEST(AnimLayerStackTest, AddLayer)
{
    AnimationLayerStack stack;
    uint32_t idx = stack.AddLayer("Base");
    EXPECT_EQ(idx, 0u);
    EXPECT_EQ(stack.GetLayerCount(), 1u);
}

TEST(AnimLayerStackTest, RemoveLayer)
{
    AnimationLayerStack stack;
    stack.AddLayer("Base");
    stack.AddLayer("UpperBody");
    stack.RemoveLayer(0);
    EXPECT_EQ(stack.GetLayerCount(), 1u);
    EXPECT_NE(stack.FindLayer("UpperBody"), -1);
}

TEST(AnimLayerStackTest, GetLayer)
{
    AnimationLayerStack stack;
    stack.AddLayer("Base", AnimLayerBlendMode::Override);
    const auto* layer = stack.GetLayer(0);
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->name, "Base");
    EXPECT_EQ(layer->blendMode, AnimLayerBlendMode::Override);
    EXPECT_TRUE(layer->enabled);
}

TEST(AnimLayerStackTest, FindLayer)
{
    AnimationLayerStack stack;
    stack.AddLayer("Base");
    stack.AddLayer("Arms");
    EXPECT_EQ(stack.FindLayer("Arms"), 1);
    EXPECT_EQ(stack.FindLayer("Missing"), -1);
}

TEST(AnimLayerStackTest, SetLayerWeight)
{
    AnimationLayerStack stack;
    stack.AddLayer("Base");
    stack.SetLayerWeight(0, 0.7f);
    EXPECT_FLOAT_EQ(stack.GetLayer(0)->weight, 0.7f);
}

TEST(AnimLayerStackTest, SetLayerEnabled)
{
    AnimationLayerStack stack;
    stack.AddLayer("Base");
    stack.SetLayerEnabled(0, false);
    EXPECT_FALSE(stack.GetLayer(0)->enabled);
}

TEST(AnimLayerStackTest, EvaluateOverride)
{
    AnimationLayerStack stack;
    stack.AddLayer("Override", AnimLayerBlendMode::Override);
    stack.SetLayerWeight(0, 1.0f);

    const uint32_t boneCount = 2;
    BonePose basePose[boneCount];
    basePose[0].position = {0, 0, 0};
    basePose[1].position = {0, 0, 0};

    std::vector<std::vector<BonePose>> layerPoses(1);
    layerPoses[0].resize(boneCount);
    layerPoses[0][0].position = {10, 0, 0};
    layerPoses[0][1].position = {0, 20, 0};

    stack.Evaluate(basePose, layerPoses, boneCount);

    // Full override (weight=1.0): base should equal layer pose
    EXPECT_FLOAT_EQ(basePose[0].position.x, 10.0f);
    EXPECT_FLOAT_EQ(basePose[1].position.y, 20.0f);
}

TEST(AnimLayerStackTest, EvaluateAdditive)
{
    AnimationLayerStack stack;
    stack.AddLayer("Additive", AnimLayerBlendMode::Additive);
    stack.SetLayerWeight(0, 1.0f);

    const uint32_t boneCount = 1;
    BonePose basePose[boneCount];
    basePose[0].position = {5, 5, 5};

    std::vector<std::vector<BonePose>> layerPoses(1);
    layerPoses[0].resize(boneCount);
    layerPoses[0][0].position = {3, 0, 0}; // additive offset from zero reference

    stack.Evaluate(basePose, layerPoses, boneCount);

    // Additive: base + layer * weight = {5+3, 5+0, 5+0}
    EXPECT_FLOAT_EQ(basePose[0].position.x, 8.0f);
    EXPECT_FLOAT_EQ(basePose[0].position.y, 5.0f);
}

TEST(AnimLayerStackTest, EvaluateDisabledLayer)
{
    AnimationLayerStack stack;
    stack.AddLayer("Override", AnimLayerBlendMode::Override);
    stack.SetLayerEnabled(0, false);

    const uint32_t boneCount = 1;
    BonePose basePose[boneCount];
    basePose[0].position = {1, 2, 3};

    std::vector<std::vector<BonePose>> layerPoses(1);
    layerPoses[0].resize(boneCount);
    layerPoses[0][0].position = {99, 99, 99};

    stack.Evaluate(basePose, layerPoses, boneCount);

    // Disabled layer should not affect base
    EXPECT_FLOAT_EQ(basePose[0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(basePose[0].position.y, 2.0f);
}

TEST(AnimLayerStackTest, Clear)
{
    AnimationLayerStack stack;
    stack.AddLayer("A");
    stack.AddLayer("B");
    stack.Clear();
    EXPECT_EQ(stack.GetLayerCount(), 0u);
}
