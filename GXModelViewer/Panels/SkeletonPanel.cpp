/// @file SkeletonPanel.cpp
/// @brief スケルトンパネル実装
///
/// 上半分にジョイントツリー、下半分に選択ボーンの詳細を表示する。
/// ローカルTRS（クォータニオンからオイラー角に変換）、ワールド座標、
/// ワールド変換行列、ローカル回転行列、逆バインド行列をそれぞれ折りたたみで表示。

#include "SkeletonPanel.h"
#include <imgui.h>
#include <cmath>
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/Animator.h"
#include "Graphics/3D/AnimationClip.h"

#ifndef XM_PI
#define XM_PI 3.14159265358979323846f
#endif

static constexpr float k_RadToDeg = 180.0f / XM_PI;

/// クォータニオンをZYX規約のオイラー角（度数法）に変換する（表示用）
static void QuatToEuler(const XMFLOAT4& q, float& pitch, float& yaw, float& roll)
{
    // ZYX convention
    float sinr = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    roll = atan2f(sinr, cosr) * k_RadToDeg;

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabsf(sinp) >= 1.0f)
        pitch = copysignf(90.0f, sinp);
    else
        pitch = asinf(sinp) * k_RadToDeg;

    float siny = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    yaw = atan2f(siny, cosy) * k_RadToDeg;
}

void SkeletonPanel::Draw(SceneGraph& scene, const GX::Animator* animator)
{
    if (!ImGui::Begin("Skeleton"))
    {
        ImGui::End();
        return;
    }
    DrawContent(scene, animator);
    ImGui::End();
}

void SkeletonPanel::DrawContent(SceneGraph& scene, const GX::Animator* animator)
{
    const SceneEntity* entity = scene.GetEntity(scene.selectedEntity);
    if (!entity || !entity->model || !entity->model->HasSkeleton())
    {
        ImGui::TextDisabled("No skeleton available.");
        return;
    }

    const GX::Skeleton* skeleton = entity->model->GetSkeleton();
    const auto& joints = skeleton->GetJoints();

    ImGui::Text("Joints: %u", static_cast<uint32_t>(joints.size()));
    ImGui::Separator();

    // Build tree from roots (parentIndex == -1)
    if (ImGui::BeginChild("JointTree", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), ImGuiChildFlags_Borders))
    {
        for (size_t i = 0; i < joints.size(); ++i)
        {
            if (joints[i].parentIndex == -1)
                DrawJointTree(skeleton, scene, animator, static_cast<int>(i));
        }
    }
    ImGui::EndChild();

    // --- Selected bone details ---
    if (scene.selectedBone >= 0 && scene.selectedBone < static_cast<int>(joints.size()))
    {
        ImGui::Separator();
        const auto& joint = joints[scene.selectedBone];
        ImGui::Text("Selected: %s", joint.name.c_str());
        ImGui::Text("Index: %d", scene.selectedBone);
        ImGui::Text("Parent: %d %s", joint.parentIndex,
                     joint.parentIndex >= 0 ? joints[joint.parentIndex].name.c_str() : "(root)");

        // Show current TRS from animator
        if (animator)
        {
            const auto& localPose = animator->GetLocalPose();
            if (scene.selectedBone < static_cast<int>(localPose.size()))
            {
                const auto& trs = localPose[scene.selectedBone];
                ImGui::Separator();
                ImGui::Text("Local Transform:");
                ImGui::Text("  T: (%.4f, %.4f, %.4f)",
                            trs.translation.x, trs.translation.y, trs.translation.z);

                float pitch, yaw, roll;
                QuatToEuler(trs.rotation, pitch, yaw, roll);
                ImGui::Text("  R: (%.1f, %.1f, %.1f) deg", pitch, yaw, roll);
                ImGui::Text("  Q: (%.4f, %.4f, %.4f, %.4f)",
                            trs.rotation.x, trs.rotation.y, trs.rotation.z, trs.rotation.w);
                ImGui::Text("  S: (%.4f, %.4f, %.4f)",
                            trs.scale.x, trs.scale.y, trs.scale.z);
            }
        }

        // World position
        if (animator)
        {
            const auto& gt = animator->GetGlobalTransforms();
            if (scene.selectedBone < static_cast<int>(gt.size()))
            {
                XMVECTOR posLocal = XMVectorSet(
                    gt[scene.selectedBone]._41,
                    gt[scene.selectedBone]._42,
                    gt[scene.selectedBone]._43, 1.0f);
                XMMATRIX worldMat = entity->transform.GetWorldMatrix();
                XMVECTOR posWorld = XMVector3Transform(posLocal, worldMat);
                XMFLOAT3 wp;
                XMStoreFloat3(&wp, posWorld);

                ImGui::Separator();
                ImGui::Text("World Position: (%.4f, %.4f, %.4f)", wp.x, wp.y, wp.z);

                // World transform matrix
                if (ImGui::CollapsingHeader("World Transform Matrix"))
                {
                    XMMATRIX jointWorld = XMLoadFloat4x4(&gt[scene.selectedBone]) * worldMat;
                    XMFLOAT4X4 mat;
                    XMStoreFloat4x4(&mat, jointWorld);
                    for (int r = 0; r < 4; ++r)
                    {
                        ImGui::Text("  [%.3f, %.3f, %.3f, %.3f]",
                                    (&mat._11)[r * 4 + 0],
                                    (&mat._11)[r * 4 + 1],
                                    (&mat._11)[r * 4 + 2],
                                    (&mat._11)[r * 4 + 3]);
                    }
                }
            }

            // Local rotation matrix from quaternion
            const auto& localPose = animator->GetLocalPose();
            if (scene.selectedBone < static_cast<int>(localPose.size()))
            {
                if (ImGui::CollapsingHeader("Local Rotation Matrix"))
                {
                    const auto& q = localPose[scene.selectedBone].rotation;
                    XMVECTOR quat = XMLoadFloat4(&q);
                    XMMATRIX rotMat = XMMatrixRotationQuaternion(quat);
                    XMFLOAT4X4 mat;
                    XMStoreFloat4x4(&mat, rotMat);
                    for (int r = 0; r < 3; ++r)
                    {
                        ImGui::Text("  [%.4f, %.4f, %.4f]",
                                    (&mat._11)[r * 4 + 0],
                                    (&mat._11)[r * 4 + 1],
                                    (&mat._11)[r * 4 + 2]);
                    }
                }
            }
        }

        // Inverse bind matrix (read-only)
        if (ImGui::CollapsingHeader("Inverse Bind Matrix"))
        {
            const auto& ibm = joint.inverseBindMatrix;
            for (int r = 0; r < 4; ++r)
            {
                ImGui::Text("  [%.3f, %.3f, %.3f, %.3f]",
                            (&ibm._11)[r * 4 + 0],
                            (&ibm._11)[r * 4 + 1],
                            (&ibm._11)[r * 4 + 2],
                            (&ibm._11)[r * 4 + 3]);
            }
        }
    }
}

void SkeletonPanel::DrawJointTree(const GX::Skeleton* skeleton, SceneGraph& scene,
                                   const GX::Animator* animator, int jointIndex)
{
    const auto& joints = skeleton->GetJoints();
    if (jointIndex < 0 || jointIndex >= static_cast<int>(joints.size()))
        return;

    // Find children
    bool hasChildren = false;
    for (size_t i = 0; i < joints.size(); ++i)
    {
        if (joints[i].parentIndex == jointIndex)
        {
            hasChildren = true;
            break;
        }
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
    if (jointIndex == scene.selectedBone) flags |= ImGuiTreeNodeFlags_Selected;

    char label[128];
    snprintf(label, sizeof(label), "[%d] %s", jointIndex, joints[jointIndex].name.c_str());

    bool open = ImGui::TreeNodeEx(label, flags);

    if (ImGui::IsItemClicked())
        scene.selectedBone = jointIndex;

    if (open)
    {
        for (size_t i = 0; i < joints.size(); ++i)
        {
            if (joints[i].parentIndex == jointIndex)
                DrawJointTree(skeleton, scene, animator, static_cast<int>(i));
        }
        ImGui::TreePop();
    }
}
