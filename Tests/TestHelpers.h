/// @file TestHelpers.h
/// @brief Shared test helpers for GXLib unit tests

#pragma once

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"

namespace GX::TestHelpers
{

/// @brief Compare two XMFLOAT3 values within epsilon
inline void ExpectFloat3Near(const XMFLOAT3& actual, const XMFLOAT3& expected, float eps = 1e-3f)
{
    EXPECT_NEAR(actual.x, expected.x, eps);
    EXPECT_NEAR(actual.y, expected.y, eps);
    EXPECT_NEAR(actual.z, expected.z, eps);
}

/// @brief Compare two XMFLOAT4 values within epsilon
inline void ExpectFloat4Near(const XMFLOAT4& actual, const XMFLOAT4& expected, float eps = 1e-3f)
{
    EXPECT_NEAR(actual.x, expected.x, eps);
    EXPECT_NEAR(actual.y, expected.y, eps);
    EXPECT_NEAR(actual.z, expected.z, eps);
    EXPECT_NEAR(actual.w, expected.w, eps);
}

/// @brief Create a simple 3-joint skeleton: Root -> Spine -> Head (Y-axis chain)
inline Skeleton CreateChainSkeleton3()
{
    Skeleton skel;

    Joint root;
    root.name = "Root";
    root.parentIndex = -1;
    XMStoreFloat4x4(&root.inverseBindMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&root.localTransform, XMMatrixIdentity());
    skel.AddJoint(root);

    Joint spine;
    spine.name = "Spine";
    spine.parentIndex = 0;
    XMStoreFloat4x4(&spine.inverseBindMatrix, XMMatrixInverse(nullptr, XMMatrixTranslation(0, 1, 0)));
    XMStoreFloat4x4(&spine.localTransform, XMMatrixTranslation(0, 1, 0));
    skel.AddJoint(spine);

    Joint head;
    head.name = "Head";
    head.parentIndex = 1;
    XMStoreFloat4x4(&head.inverseBindMatrix, XMMatrixInverse(nullptr, XMMatrixTranslation(0, 2, 0)));
    XMStoreFloat4x4(&head.localTransform, XMMatrixTranslation(0, 1, 0));
    skel.AddJoint(head);

    return skel;
}

/// @brief Create a 5-joint skeleton for IK testing: Root -> Hip -> Knee -> Foot -> Toe
inline Skeleton CreateLegSkeleton()
{
    Skeleton skel;

    Joint root;
    root.name = "Root";
    root.parentIndex = -1;
    XMStoreFloat4x4(&root.inverseBindMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&root.localTransform, XMMatrixTranslation(0, 3, 0));
    skel.AddJoint(root);

    Joint hip;
    hip.name = "Hip";
    hip.parentIndex = 0;
    XMStoreFloat4x4(&hip.inverseBindMatrix, XMMatrixInverse(nullptr, XMMatrixTranslation(0, 3, 0)));
    XMStoreFloat4x4(&hip.localTransform, XMMatrixIdentity());
    skel.AddJoint(hip);

    Joint knee;
    knee.name = "Knee";
    knee.parentIndex = 1;
    XMStoreFloat4x4(&knee.inverseBindMatrix, XMMatrixInverse(nullptr, XMMatrixTranslation(0, 2, 0)));
    XMStoreFloat4x4(&knee.localTransform, XMMatrixTranslation(0, -1, 0));
    skel.AddJoint(knee);

    Joint foot;
    foot.name = "Foot";
    foot.parentIndex = 2;
    XMStoreFloat4x4(&foot.inverseBindMatrix, XMMatrixInverse(nullptr, XMMatrixTranslation(0, 1, 0)));
    XMStoreFloat4x4(&foot.localTransform, XMMatrixTranslation(0, -1, 0));
    skel.AddJoint(foot);

    Joint toe;
    toe.name = "Toe";
    toe.parentIndex = 3;
    XMStoreFloat4x4(&toe.inverseBindMatrix, XMMatrixInverse(nullptr, XMMatrixTranslation(0.5f, 1, 0)));
    XMStoreFloat4x4(&toe.localTransform, XMMatrixTranslation(0.5f, 0, 0));
    skel.AddJoint(toe);

    return skel;
}

/// @brief Create a simple AnimationClip with translation keyframes
/// Joint 0 moves from (0,0,0) to (tx,ty,tz) over 'duration' seconds
inline AnimationClip CreateSimpleClip(const std::string& name, float duration,
                                       float tx = 1.0f, float ty = 0.0f, float tz = 0.0f)
{
    AnimationClip clip;
    clip.SetName(name);
    clip.SetDuration(duration);

    AnimationChannel ch;
    ch.jointIndex = 0;
    ch.translationKeys.push_back({0.0f, {0, 0, 0}});
    ch.translationKeys.push_back({duration, {tx, ty, tz}});
    ch.rotationKeys.push_back({0.0f, {0, 0, 0, 1}});
    ch.rotationKeys.push_back({duration, {0, 0, 0, 1}});
    ch.scaleKeys.push_back({0.0f, {1, 1, 1}});
    ch.scaleKeys.push_back({duration, {1, 1, 1}});
    clip.AddChannel(ch);

    return clip;
}

/// @brief Create a multi-joint AnimationClip for skeleton with jointCount joints
/// Each joint translates from origin to (1*jointIdx, 0, 0) over duration
inline AnimationClip CreateMultiJointClip(const std::string& name, float duration, uint32_t jointCount)
{
    AnimationClip clip;
    clip.SetName(name);
    clip.SetDuration(duration);

    for (uint32_t j = 0; j < jointCount; ++j)
    {
        AnimationChannel ch;
        ch.jointIndex = static_cast<int>(j);
        ch.translationKeys.push_back({0.0f, {0, 0, 0}});
        ch.translationKeys.push_back({duration, {static_cast<float>(j + 1), 0, 0}});
        ch.rotationKeys.push_back({0.0f, {0, 0, 0, 1}});
        ch.rotationKeys.push_back({duration, {0, 0, 0, 1}});
        ch.scaleKeys.push_back({0.0f, {1, 1, 1}});
        ch.scaleKeys.push_back({duration, {1, 1, 1}});
        clip.AddChannel(ch);
    }

    return clip;
}

} // namespace GX::TestHelpers
