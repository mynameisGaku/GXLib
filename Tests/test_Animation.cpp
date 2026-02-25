/// @file test_Animation.cpp
/// @brief AnimationClip / Skeleton / TransformTRS tests

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Skeleton.h"

using namespace GX;

// Helper: create 2-joint skeleton
static Skeleton CreateTestSkeleton()
{
    Skeleton skel;
    Joint root;
    root.name = "Root";
    root.parentIndex = -1;
    XMStoreFloat4x4(&root.inverseBindMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&root.localTransform, XMMatrixIdentity());
    skel.AddJoint(root);

    Joint child;
    child.name = "Child";
    child.parentIndex = 0;
    XMStoreFloat4x4(&child.inverseBindMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&child.localTransform, XMMatrixTranslation(0, 1, 0));
    skel.AddJoint(child);

    return skel;
}

// Helper: create simple animation clip
static AnimationClip CreateTestClip(float duration = 1.0f)
{
    AnimationClip clip;
    clip.SetName("TestClip");
    clip.SetDuration(duration);

    AnimationChannel ch;
    ch.jointIndex = 0;
    ch.translationKeys.push_back({0.0f, {0, 0, 0}});
    ch.translationKeys.push_back({duration, {1, 0, 0}});
    ch.rotationKeys.push_back({0.0f, {0, 0, 0, 1}});
    ch.rotationKeys.push_back({duration, {0, 0, 0, 1}});
    ch.scaleKeys.push_back({0.0f, {1, 1, 1}});
    ch.scaleKeys.push_back({duration, {1, 1, 1}});
    clip.AddChannel(ch);

    return clip;
}

TEST(AnimationTest, IdentityTRS_Values)
{
    TransformTRS trs = IdentityTRS();
    EXPECT_NEAR(trs.translation.x, 0.0f, 1e-5f);
    EXPECT_NEAR(trs.translation.y, 0.0f, 1e-5f);
    EXPECT_NEAR(trs.translation.z, 0.0f, 1e-5f);
    EXPECT_NEAR(trs.rotation.x, 0.0f, 1e-5f);
    EXPECT_NEAR(trs.rotation.y, 0.0f, 1e-5f);
    EXPECT_NEAR(trs.rotation.z, 0.0f, 1e-5f);
    EXPECT_NEAR(trs.rotation.w, 1.0f, 1e-5f);
    EXPECT_NEAR(trs.scale.x, 1.0f, 1e-5f);
    EXPECT_NEAR(trs.scale.y, 1.0f, 1e-5f);
    EXPECT_NEAR(trs.scale.z, 1.0f, 1e-5f);
}

TEST(AnimationTest, ComposeTRS_Identity)
{
    TransformTRS trs = IdentityTRS();
    XMFLOAT4X4 mat = ComposeTRS(trs);
    EXPECT_NEAR(mat._11, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._22, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._33, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._44, 1.0f, 1e-5f);
    EXPECT_NEAR(mat._41, 0.0f, 1e-5f);
}

TEST(AnimationTest, ComposeTRS_Translation)
{
    TransformTRS trs = IdentityTRS();
    trs.translation = {5.0f, 0.0f, 0.0f};
    XMFLOAT4X4 mat = ComposeTRS(trs);
    EXPECT_NEAR(mat._41, 5.0f, 1e-5f);
}

TEST(AnimationTest, DecomposeTRS_RoundTrip)
{
    TransformTRS original = IdentityTRS();
    original.translation = {3.0f, 4.0f, 5.0f};
    original.scale = {2.0f, 2.0f, 2.0f};
    XMFLOAT4X4 mat = ComposeTRS(original);
    TransformTRS decomposed = DecomposeTRS(mat);
    EXPECT_NEAR(decomposed.translation.x, 3.0f, 1e-3f);
    EXPECT_NEAR(decomposed.translation.y, 4.0f, 1e-3f);
    EXPECT_NEAR(decomposed.translation.z, 5.0f, 1e-3f);
    EXPECT_NEAR(decomposed.scale.x, 2.0f, 1e-3f);
    EXPECT_NEAR(decomposed.scale.y, 2.0f, 1e-3f);
    EXPECT_NEAR(decomposed.scale.z, 2.0f, 1e-3f);
}

TEST(AnimationTest, AnimationClip_NameAndDuration)
{
    AnimationClip clip;
    clip.SetName("Walk");
    clip.SetDuration(2.5f);
    EXPECT_EQ(clip.GetName(), "Walk");
    EXPECT_NEAR(clip.GetDuration(), 2.5f, 1e-5f);
}

TEST(AnimationTest, AnimationClip_AddChannel)
{
    AnimationClip clip;
    AnimationChannel ch;
    ch.jointIndex = 0;
    clip.AddChannel(ch);
    EXPECT_EQ(clip.GetChannels().size(), 1u);
}

TEST(AnimationTest, AnimationClip_SampleTRS_Static)
{
    AnimationClip clip;
    clip.SetName("Static");
    clip.SetDuration(1.0f);
    AnimationChannel ch;
    ch.jointIndex = 0;
    ch.translationKeys.push_back({0.0f, {5, 0, 0}});
    ch.rotationKeys.push_back({0.0f, {0, 0, 0, 1}});
    ch.scaleKeys.push_back({0.0f, {1, 1, 1}});
    clip.AddChannel(ch);

    TransformTRS pose[1];
    clip.SampleTRS(0.0f, 1, pose);
    EXPECT_NEAR(pose[0].translation.x, 5.0f, 1e-3f);
    clip.SampleTRS(0.5f, 1, pose);
    EXPECT_NEAR(pose[0].translation.x, 5.0f, 1e-3f);
    clip.SampleTRS(1.0f, 1, pose);
    EXPECT_NEAR(pose[0].translation.x, 5.0f, 1e-3f);
}

TEST(AnimationTest, AnimationClip_SampleTRS_Interpolated)
{
    AnimationClip clip = CreateTestClip(1.0f);
    TransformTRS pose[1];
    clip.SampleTRS(0.5f, 1, pose);
    // At t=0.5, translation should be midway between (0,0,0) and (1,0,0)
    EXPECT_NEAR(pose[0].translation.x, 0.5f, 0.05f);
}

TEST(AnimationTest, Skeleton_AddJoint)
{
    Skeleton skel = CreateTestSkeleton();
    EXPECT_EQ(skel.GetJointCount(), 2u);
}

TEST(AnimationTest, Skeleton_FindJointIndex)
{
    Skeleton skel = CreateTestSkeleton();
    EXPECT_EQ(skel.FindJointIndex("Root"), 0);
    EXPECT_EQ(skel.FindJointIndex("Child"), 1);
}

TEST(AnimationTest, Skeleton_FindJointIndex_NotFound)
{
    Skeleton skel = CreateTestSkeleton();
    EXPECT_EQ(skel.FindJointIndex("NoExist"), -1);
}

TEST(AnimationTest, Skeleton_ComputeGlobalTransforms)
{
    Skeleton skel = CreateTestSkeleton();
    XMFLOAT4X4 local[2];
    XMStoreFloat4x4(&local[0], XMMatrixIdentity());
    XMStoreFloat4x4(&local[1], XMMatrixTranslation(0, 1, 0));

    XMFLOAT4X4 global[2];
    skel.ComputeGlobalTransforms(local, global);

    // Root global == root local (identity)
    EXPECT_NEAR(global[0]._41, 0.0f, 1e-5f);
    // Child global = parent * child = Identity * Translation(0,1,0)
    EXPECT_NEAR(global[1]._42, 1.0f, 1e-5f);
}
