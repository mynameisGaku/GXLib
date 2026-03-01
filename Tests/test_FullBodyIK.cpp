/// @file test_FullBodyIK.cpp
/// @brief FullBodyIK全身IKソルバーのテスト

#include <gtest/gtest.h>
#include "Graphics/3D/FullBodyIK.h"

using namespace gx;

TEST(FullBodyIKTest, DefaultState)
{
    FullBodyIK ik;
    EXPECT_EQ(ik.GetChainCount(), 0u);
    EXPECT_FALSE(ik.HasLookAtTarget());
    EXPECT_FALSE(ik.IsFootPlacementEnabled());
}

TEST(FullBodyIKTest, SetupChain)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::LeftArm, {0, 1, 2});
    EXPECT_TRUE(ik.HasChain(IKChainId::LeftArm));
    EXPECT_EQ(ik.GetChainCount(), 1u);
}

TEST(FullBodyIKTest, HasChainFalse)
{
    FullBodyIK ik;
    EXPECT_FALSE(ik.HasChain(IKChainId::RightLeg));
}

TEST(FullBodyIKTest, MultipleChains)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::Spine, {0, 1, 2, 3});
    ik.SetupChain(IKChainId::LeftArm, {3, 4, 5});
    ik.SetupChain(IKChainId::RightArm, {3, 6, 7});
    EXPECT_EQ(ik.GetChainCount(), 3u);
}

TEST(FullBodyIKTest, SetTarget)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::LeftArm, {0, 1, 2});
    ik.SetTarget(IKChainId::LeftArm, {1.0f, 2.0f, 3.0f});
    auto t = ik.GetTarget(IKChainId::LeftArm);
    EXPECT_FLOAT_EQ(t.x, 1.0f);
    EXPECT_FLOAT_EQ(t.y, 2.0f);
    EXPECT_FLOAT_EQ(t.z, 3.0f);
}

TEST(FullBodyIKTest, GetTargetMissing)
{
    FullBodyIK ik;
    auto t = ik.GetTarget(IKChainId::Spine);
    EXPECT_FLOAT_EQ(t.x, 0.0f);
}

TEST(FullBodyIKTest, SetWeight)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::RightLeg, {0, 1, 2});
    ik.SetWeight(IKChainId::RightLeg, 0.5f);
    EXPECT_FLOAT_EQ(ik.GetWeight(IKChainId::RightLeg), 0.5f);
}

TEST(FullBodyIKTest, WeightClamp)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::Spine, {0, 1});
    ik.SetWeight(IKChainId::Spine, 2.0f);
    EXPECT_LE(ik.GetWeight(IKChainId::Spine), 1.0f);
    ik.SetWeight(IKChainId::Spine, -1.0f);
    EXPECT_GE(ik.GetWeight(IKChainId::Spine), 0.0f);
}

TEST(FullBodyIKTest, SetConstraint)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::LeftLeg, {0, 1, 2});
    JointConstraint c;
    c.type = JointConstraintType::Hinge;
    c.minAngle = -5.0f;
    c.maxAngle = 150.0f;
    ik.SetConstraint(IKChainId::LeftLeg, 1, c);
    auto got = ik.GetConstraint(IKChainId::LeftLeg, 1);
    EXPECT_EQ(got.type, JointConstraintType::Hinge);
    EXPECT_FLOAT_EQ(got.minAngle, -5.0f);
}

TEST(FullBodyIKTest, ConstraintTypes)
{
    EXPECT_NE(JointConstraintType::None, JointConstraintType::Hinge);
    EXPECT_NE(JointConstraintType::BallAndSocket, JointConstraintType::Twist);
}

TEST(FullBodyIKTest, LookAt)
{
    FullBodyIK ik;
    EXPECT_FALSE(ik.HasLookAtTarget());
    ik.SetLookAtTarget({0, 0, 10});
    EXPECT_TRUE(ik.HasLookAtTarget());
    auto t = ik.GetLookAtTarget();
    EXPECT_FLOAT_EQ(t.z, 10.0f);
}

TEST(FullBodyIKTest, ClearLookAt)
{
    FullBodyIK ik;
    ik.SetLookAtTarget({1, 2, 3});
    ik.ClearLookAtTarget();
    EXPECT_FALSE(ik.HasLookAtTarget());
}

TEST(FullBodyIKTest, FootPlacement)
{
    FullBodyIK ik;
    ik.SetFootPlacement(true, -0.5f);
    EXPECT_TRUE(ik.IsFootPlacementEnabled());
    EXPECT_FLOAT_EQ(ik.GetGroundHeight(), -0.5f);
}

TEST(FullBodyIKTest, SolveOrder)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::LeftArm, {3, 4, 5});
    ik.SetupChain(IKChainId::Spine, {0, 1, 2, 3});
    ik.SetupChain(IKChainId::RightLeg, {8, 9, 10});
    auto order = ik.GetSolveOrder();
    ASSERT_GE(order.size(), 3u);
    EXPECT_EQ(order[0], IKChainId::Spine); // Spine first
}

TEST(FullBodyIKTest, SolveNoChains)
{
    FullBodyIK ik;
    ik.Solve(); // Should not crash
}

TEST(FullBodyIKTest, SolveWithChains)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::Spine, {0, 1, 2});
    ik.SetTarget(IKChainId::Spine, {0, 1, 0});
    ik.SetWeight(IKChainId::Spine, 1.0f);
    ik.Solve(); // Should not crash
}

TEST(FullBodyIKTest, Reset)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::Spine, {0, 1});
    ik.SetLookAtTarget({0, 0, 1});
    ik.SetFootPlacement(true, 0.0f);
    ik.Reset();
    EXPECT_EQ(ik.GetChainCount(), 0u);
    EXPECT_FALSE(ik.HasLookAtTarget());
    EXPECT_FALSE(ik.IsFootPlacementEnabled());
}

TEST(FullBodyIKTest, MaxIterationsAndTolerance)
{
    FullBodyIK ik;
    ik.SetupChain(IKChainId::LeftArm, {0, 1, 2});
    ik.SetMaxIterations(IKChainId::LeftArm, 50);
    ik.SetTolerance(IKChainId::LeftArm, 0.01f);
    // No crash, no getter verification needed (internal state)
}
