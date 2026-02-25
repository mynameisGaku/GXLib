/// @file test_BlendSystem.cpp
/// @brief BlendStack / BlendTree tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/BlendStack.h"
#include "Graphics/3D/BlendTree.h"
#include "TestHelpers.h"

using namespace GX;
using namespace GX::TestHelpers;

// Persistent clips
static AnimationClip s_clipA = CreateSimpleClip("ClipA", 1.0f, 2.0f, 0.0f, 0.0f);
static AnimationClip s_clipB = CreateSimpleClip("ClipB", 1.0f, 0.0f, 2.0f, 0.0f);
static AnimationClip s_clipC = CreateSimpleClip("ClipC", 2.0f, 0.0f, 0.0f, 4.0f);

// ====================================================================
//  BlendStack
// ====================================================================

// ==================== Layer Management ====================

TEST(BlendStackTest, MaxLayers)
{
    EXPECT_EQ(BlendStack::k_MaxLayers, 8u);
}

TEST(BlendStackTest, Empty_NoActiveLayers)
{
    BlendStack bs;
    EXPECT_EQ(bs.GetActiveLayerCount(), 0u);
}

TEST(BlendStackTest, SetLayer_ActivatesLayer)
{
    BlendStack bs;
    BlendLayer layer;
    layer.clip = &s_clipA;
    layer.weight = 1.0f;
    bs.SetLayer(0, layer);
    EXPECT_EQ(bs.GetActiveLayerCount(), 1u);
}

TEST(BlendStackTest, SetLayer_MultipleLayers)
{
    BlendStack bs;
    BlendLayer l1; l1.clip = &s_clipA;
    BlendLayer l2; l2.clip = &s_clipB;
    BlendLayer l3; l3.clip = &s_clipC;
    bs.SetLayer(0, l1);
    bs.SetLayer(1, l2);
    bs.SetLayer(3, l3);  // Skip index 2
    EXPECT_EQ(bs.GetActiveLayerCount(), 3u);
}

TEST(BlendStackTest, GetLayer_Active)
{
    BlendStack bs;
    BlendLayer layer;
    layer.clip = &s_clipA;
    layer.weight = 0.75f;
    bs.SetLayer(2, layer);
    const BlendLayer* got = bs.GetLayer(2);
    EXPECT_NE(got, nullptr);
    EXPECT_NEAR(got->weight, 0.75f, 1e-5f);
}

TEST(BlendStackTest, GetLayer_Inactive)
{
    BlendStack bs;
    const BlendLayer* got = bs.GetLayer(0);
    EXPECT_EQ(got, nullptr);
}

TEST(BlendStackTest, GetLayer_OutOfRange)
{
    BlendStack bs;
    const BlendLayer* got = bs.GetLayer(BlendStack::k_MaxLayers);
    EXPECT_EQ(got, nullptr);
}

TEST(BlendStackTest, RemoveLayer)
{
    BlendStack bs;
    BlendLayer layer; layer.clip = &s_clipA;
    bs.SetLayer(0, layer);
    bs.RemoveLayer(0);
    EXPECT_EQ(bs.GetActiveLayerCount(), 0u);
    EXPECT_EQ(bs.GetLayer(0), nullptr);
}

TEST(BlendStackTest, RemoveLayer_InvalidIndex)
{
    BlendStack bs;
    // Should not crash
    bs.RemoveLayer(99);
    EXPECT_EQ(bs.GetActiveLayerCount(), 0u);
}

// ==================== Layer Weight ====================

TEST(BlendStackTest, SetLayerWeight)
{
    BlendStack bs;
    BlendLayer layer; layer.clip = &s_clipA; layer.weight = 1.0f;
    bs.SetLayer(0, layer);
    bs.SetLayerWeight(0, 0.5f);
    EXPECT_NEAR(bs.GetLayer(0)->weight, 0.5f, 1e-5f);
}

TEST(BlendStackTest, SetLayerClip)
{
    BlendStack bs;
    BlendLayer layer; layer.clip = &s_clipA;
    bs.SetLayer(0, layer);
    bs.SetLayerClip(0, &s_clipB);
    EXPECT_EQ(bs.GetLayer(0)->clip, &s_clipB);
}

// ==================== BlendLayer Properties ====================

TEST(BlendStackTest, LayerDefaults)
{
    BlendLayer layer;
    EXPECT_EQ(layer.clip, nullptr);
    EXPECT_NEAR(layer.time, 0.0f, 1e-5f);
    EXPECT_NEAR(layer.weight, 1.0f, 1e-5f);
    EXPECT_NEAR(layer.speed, 1.0f, 1e-5f);
    EXPECT_TRUE(layer.loop);
    EXPECT_EQ(layer.mode, AnimBlendMode::Override);
    EXPECT_EQ(layer.maskBits, 0xFFFFFFFF);
}

TEST(BlendStackTest, AnimBlendMode_Values)
{
    EXPECT_NE(AnimBlendMode::Override, AnimBlendMode::Additive);
}

// ==================== Update ====================

TEST(BlendStackTest, Update_SingleLayer_ProducesValidPose)
{
    BlendStack bs;
    BlendLayer layer;
    layer.clip = &s_clipA;
    layer.weight = 1.0f;
    layer.speed = 1.0f;
    layer.loop = true;
    bs.SetLayer(0, layer);

    TransformTRS pose[1];
    bs.Update(0.5f, 1, nullptr, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
    EXPECT_FALSE(std::isnan(pose[0].rotation.w));
}

TEST(BlendStackTest, Update_SingleLayer_Interpolation)
{
    BlendStack bs;
    BlendLayer layer;
    layer.clip = &s_clipA;  // translates to (2,0,0) over 1s
    layer.weight = 1.0f;
    layer.speed = 1.0f;
    layer.loop = false;
    bs.SetLayer(0, layer);

    TransformTRS pose[1];
    bs.Update(0.5f, 1, nullptr, pose);
    // At t=0.5, translation.x should be ~1.0
    EXPECT_NEAR(pose[0].translation.x, 1.0f, 0.2f);
}

TEST(BlendStackTest, Update_TwoLayers_Override)
{
    BlendStack bs;
    BlendLayer l1; l1.clip = &s_clipA; l1.weight = 1.0f; l1.mode = AnimBlendMode::Override;
    BlendLayer l2; l2.clip = &s_clipB; l2.weight = 1.0f; l2.mode = AnimBlendMode::Override;
    bs.SetLayer(0, l1);
    bs.SetLayer(1, l2);

    TransformTRS pose[1];
    bs.Update(0.5f, 1, nullptr, pose);
    // Layer 1 (Override w=1.0) should dominate
    // clipB translates to (0,2,0), at t=0.5 should be ~(0,1,0)
    EXPECT_NEAR(pose[0].translation.y, 1.0f, 0.3f);
}

TEST(BlendStackTest, Update_PartialWeight)
{
    BlendStack bs;
    BlendLayer l1;
    l1.clip = &s_clipA;  // translates to (2,0,0)
    l1.weight = 0.5f;
    l1.mode = AnimBlendMode::Override;
    bs.SetLayer(0, l1);

    TransformTRS pose[1];
    bs.Update(1.0f, 1, nullptr, pose);
    // With half weight, effect should be reduced
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
}

TEST(BlendStackTest, Update_Additive)
{
    BlendStack bs;
    BlendLayer base; base.clip = &s_clipA; base.weight = 1.0f; base.mode = AnimBlendMode::Override;
    BlendLayer add;  add.clip  = &s_clipB; add.weight  = 1.0f; add.mode  = AnimBlendMode::Additive;
    bs.SetLayer(0, base);
    bs.SetLayer(1, add);

    TransformTRS pose[1];
    bs.Update(0.5f, 1, nullptr, pose);
    // Additive layer should add to the base
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
    EXPECT_FALSE(std::isnan(pose[0].translation.y));
}

TEST(BlendStackTest, Update_WithBindPose)
{
    BlendStack bs;
    BlendLayer layer; layer.clip = &s_clipA; layer.weight = 1.0f;
    bs.SetLayer(0, layer);

    TransformTRS bind[1] = {IdentityTRS()};
    TransformTRS pose[1];
    bs.Update(0.5f, 1, bind, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
}

TEST(BlendStackTest, Update_ZeroDeltaTime)
{
    BlendStack bs;
    BlendLayer layer; layer.clip = &s_clipA; layer.weight = 1.0f;
    bs.SetLayer(0, layer);

    TransformTRS pose[1];
    bs.Update(0.0f, 1, nullptr, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
}

// ====================================================================
//  BlendTree
// ====================================================================

// ==================== Type ====================

TEST(BlendTreeTest, DefaultType_1D)
{
    BlendTree tree;
    EXPECT_EQ(tree.GetType(), BlendTreeType::Simple1D);
}

TEST(BlendTreeTest, SetType_2D)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::SimpleDirectional2D);
    EXPECT_EQ(tree.GetType(), BlendTreeType::SimpleDirectional2D);
}

// ==================== Nodes ====================

TEST(BlendTreeTest, AddNode)
{
    BlendTree tree;
    BlendTreeNode node; node.clip = &s_clipA; node.threshold = 0.0f;
    tree.AddNode(node);
    EXPECT_EQ(tree.GetNodes().size(), 1u);
}

TEST(BlendTreeTest, AddNode_Multiple)
{
    BlendTree tree;
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.0f;
    BlendTreeNode n2; n2.clip = &s_clipB; n2.threshold = 1.0f;
    tree.AddNode(n1);
    tree.AddNode(n2);
    EXPECT_EQ(tree.GetNodes().size(), 2u);
}

TEST(BlendTreeTest, ClearNodes)
{
    BlendTree tree;
    BlendTreeNode node; node.clip = &s_clipA;
    tree.AddNode(node);
    tree.ClearNodes();
    EXPECT_EQ(tree.GetNodes().size(), 0u);
}

TEST(BlendTreeTest, NodeDefaults)
{
    BlendTreeNode node;
    EXPECT_EQ(node.clip, nullptr);
    EXPECT_NEAR(node.threshold, 0.0f, 1e-5f);
    EXPECT_NEAR(node.position[0], 0.0f, 1e-5f);
    EXPECT_NEAR(node.position[1], 0.0f, 1e-5f);
}

// ==================== Parameter ====================

TEST(BlendTreeTest, Parameter_Default)
{
    BlendTree tree;
    EXPECT_NEAR(tree.GetParameter(), 0.0f, 1e-5f);
}

TEST(BlendTreeTest, SetParameter_1D)
{
    BlendTree tree;
    tree.SetParameter(0.75f);
    EXPECT_NEAR(tree.GetParameter(), 0.75f, 1e-5f);
}

TEST(BlendTreeTest, SetParameter_2D)
{
    BlendTree tree;
    tree.SetParameter2D(0.5f, -0.3f);
    // 2D parameter can't be easily retrieved individually; just verify no crash
    EXPECT_NEAR(tree.GetParameter(), 0.0f, 1e-5f);  // 1D param unchanged
}

// ==================== Duration ====================

TEST(BlendTreeTest, Duration_Empty)
{
    BlendTree tree;
    EXPECT_NEAR(tree.GetDuration(), 0.0f, 1e-5f);
}

TEST(BlendTreeTest, Duration_ReturnsLongest)
{
    BlendTree tree;
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.0f;  // 1.0s
    BlendTreeNode n2; n2.clip = &s_clipC; n2.threshold = 1.0f;  // 2.0s
    tree.AddNode(n1);
    tree.AddNode(n2);
    EXPECT_NEAR(tree.GetDuration(), 2.0f, 1e-5f);
}

// ==================== Evaluate 1D ====================

TEST(BlendTreeTest, Evaluate1D_AtFirstNode)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::Simple1D);
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.0f;
    BlendTreeNode n2; n2.clip = &s_clipB; n2.threshold = 1.0f;
    tree.AddNode(n1);
    tree.AddNode(n2);
    tree.SetParameter(0.0f);

    TransformTRS pose[1];
    tree.Evaluate(0.5f, 1, nullptr, pose);
    // At parameter=0, should output clipA's pose at t=0.5
    // clipA translates to (2,0,0) at t=1.0, so at t=0.5 → ~(1,0,0)
    EXPECT_NEAR(pose[0].translation.x, 1.0f, 0.2f);
    EXPECT_NEAR(pose[0].translation.y, 0.0f, 0.2f);
}

TEST(BlendTreeTest, Evaluate1D_AtSecondNode)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::Simple1D);
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.0f;
    BlendTreeNode n2; n2.clip = &s_clipB; n2.threshold = 1.0f;
    tree.AddNode(n1);
    tree.AddNode(n2);
    tree.SetParameter(1.0f);

    TransformTRS pose[1];
    tree.Evaluate(0.5f, 1, nullptr, pose);
    // At parameter=1, should output clipB's pose at t=0.5
    // clipB translates to (0,2,0) at t=1.0, so at t=0.5 → ~(0,1,0)
    EXPECT_NEAR(pose[0].translation.y, 1.0f, 0.2f);
}

TEST(BlendTreeTest, Evaluate1D_Midpoint)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::Simple1D);
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.0f;
    BlendTreeNode n2; n2.clip = &s_clipB; n2.threshold = 1.0f;
    tree.AddNode(n1);
    tree.AddNode(n2);
    tree.SetParameter(0.5f);

    TransformTRS pose[1];
    tree.Evaluate(0.5f, 1, nullptr, pose);
    // At parameter=0.5, should blend between clipA and clipB
    // Expect non-zero in both x (from clipA) and y (from clipB)
    EXPECT_GT(fabsf(pose[0].translation.x), 0.1f);
    EXPECT_GT(fabsf(pose[0].translation.y), 0.1f);
}

TEST(BlendTreeTest, Evaluate1D_SingleNode)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::Simple1D);
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.5f;
    tree.AddNode(n1);
    tree.SetParameter(0.5f);

    TransformTRS pose[1];
    tree.Evaluate(0.5f, 1, nullptr, pose);
    EXPECT_NEAR(pose[0].translation.x, 1.0f, 0.2f);
}

TEST(BlendTreeTest, Evaluate1D_ThreeNodes)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::Simple1D);
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.0f;
    BlendTreeNode n2; n2.clip = &s_clipB; n2.threshold = 0.5f;
    BlendTreeNode n3; n3.clip = &s_clipC; n3.threshold = 1.0f;
    tree.AddNode(n1);
    tree.AddNode(n2);
    tree.AddNode(n3);

    // At parameter=0.5, should output mostly clipB
    tree.SetParameter(0.5f);
    TransformTRS pose[1];
    tree.Evaluate(0.5f, 1, nullptr, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
}

// ==================== Evaluate 2D ====================

TEST(BlendTreeTest, Evaluate2D_Center)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::SimpleDirectional2D);

    BlendTreeNode n1; n1.clip = &s_clipA; n1.position[0] = 0.0f; n1.position[1] = 1.0f;
    BlendTreeNode n2; n2.clip = &s_clipB; n2.position[0] = 1.0f; n2.position[1] = 0.0f;
    BlendTreeNode n3; n3.clip = &s_clipC; n3.position[0] = -1.0f; n3.position[1] = 0.0f;
    tree.AddNode(n1);
    tree.AddNode(n2);
    tree.AddNode(n3);

    tree.SetParameter2D(0.0f, 0.5f);  // Near first node
    TransformTRS pose[1];
    tree.Evaluate(0.5f, 1, nullptr, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
    EXPECT_FALSE(std::isnan(pose[0].translation.y));
}

TEST(BlendTreeTest, Evaluate2D_ProducesValidPose)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::SimpleDirectional2D);
    BlendTreeNode n1; n1.clip = &s_clipA; n1.position[0] = 1.0f; n1.position[1] = 0.0f;
    BlendTreeNode n2; n2.clip = &s_clipB; n2.position[0] = 0.0f; n2.position[1] = 1.0f;
    tree.AddNode(n1);
    tree.AddNode(n2);
    tree.SetParameter2D(0.5f, 0.5f);

    TransformTRS pose[1];
    tree.Evaluate(0.5f, 1, nullptr, pose);
    EXPECT_FALSE(std::isnan(pose[0].rotation.w));
    EXPECT_FALSE(std::isnan(pose[0].scale.x));
}

// ==================== WithBindPose ====================

TEST(BlendTreeTest, Evaluate_WithBindPose)
{
    BlendTree tree;
    tree.SetType(BlendTreeType::Simple1D);
    BlendTreeNode n1; n1.clip = &s_clipA; n1.threshold = 0.0f;
    tree.AddNode(n1);
    tree.SetParameter(0.0f);

    TransformTRS bind[1] = {IdentityTRS()};
    TransformTRS pose[1];
    tree.Evaluate(0.0f, 1, bind, pose);
    EXPECT_FALSE(std::isnan(pose[0].translation.x));
}
