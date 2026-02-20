#pragma once
/// @file SkeletonPanel.h
/// @brief Bone hierarchy display panel

#include "Scene/SceneGraph.h"

namespace GX { class Animator; }

/// @brief ImGui panel showing joint hierarchy tree and TRS details
class SkeletonPanel
{
public:
    void Draw(SceneGraph& scene, const GX::Animator* animator);
    void DrawContent(SceneGraph& scene, const GX::Animator* animator);

private:
    void DrawJointTree(const GX::Skeleton* skeleton, SceneGraph& scene,
                       const GX::Animator* animator, int jointIndex);
};
