/// @file ModelInfoPanel.cpp
/// @brief モデル情報パネル実装
///
/// CPUDataから頂点数/インデックス数/バッファサイズを算出し、
/// 全頂点からAABBを計算して表示する。アニメーション一覧はBulletTextで列挙。

#include "ModelInfoPanel.h"
#include <imgui.h>
#include "Graphics/3D/Model.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"

void ModelInfoPanel::Draw(const SceneGraph& scene)
{
    if (!ImGui::Begin("Model Info"))
    {
        ImGui::End();
        return;
    }
    DrawContent(scene);
    ImGui::End();
}

void ModelInfoPanel::DrawContent(const SceneGraph& scene)
{
    const SceneEntity* entity = scene.GetEntity(scene.selectedEntity);
    if (!entity || !entity->model)
    {
        ImGui::TextDisabled("No model selected.");
        return;
    }

    const GX::Model* model = entity->model;

    // Source path
    if (!entity->sourcePath.empty())
        ImGui::Text("Source: %s", entity->sourcePath.c_str());

    ImGui::Separator();

    // Vertex type
    ImGui::Text("Vertex Type: %s", model->IsSkinned() ? "Skinned" : "Static");

    // CPU data statistics
    const auto* cpuData = model->GetCPUData();
    if (cpuData)
    {
        uint32_t vertexCount = 0;
        uint32_t vertexSize = 0;
        if (!cpuData->skinnedVertices.empty())
        {
            vertexCount = static_cast<uint32_t>(cpuData->skinnedVertices.size());
            vertexSize = vertexCount * 80; // Vertex3D_Skinned = 80B
        }
        else if (!cpuData->staticVertices.empty())
        {
            vertexCount = static_cast<uint32_t>(cpuData->staticVertices.size());
            vertexSize = vertexCount * 48; // Vertex3D_PBR = 48B
        }

        uint32_t indexCount = static_cast<uint32_t>(cpuData->indices.size());
        uint32_t triangleCount = indexCount / 3;
        uint32_t indexSize = indexCount * 4; // 32-bit indices

        ImGui::Text("Vertices:  %u", vertexCount);
        ImGui::Text("Triangles: %u", triangleCount);
        ImGui::Text("Indices:   %u", indexCount);

        ImGui::Separator();
        ImGui::Text("VB Size: %.1f KB", vertexSize / 1024.0f);
        ImGui::Text("IB Size: %.1f KB", indexSize / 1024.0f);
        ImGui::Text("Total:   %.1f KB", (vertexSize + indexSize) / 1024.0f);

        // AABB
        XMFLOAT3 aabbMin = { 1e30f, 1e30f, 1e30f };
        XMFLOAT3 aabbMax = { -1e30f, -1e30f, -1e30f };

        auto updateAABB = [&](float px, float py, float pz) {
            aabbMin.x = (std::min)(aabbMin.x, px);
            aabbMin.y = (std::min)(aabbMin.y, py);
            aabbMin.z = (std::min)(aabbMin.z, pz);
            aabbMax.x = (std::max)(aabbMax.x, px);
            aabbMax.y = (std::max)(aabbMax.y, py);
            aabbMax.z = (std::max)(aabbMax.z, pz);
        };

        if (!cpuData->skinnedVertices.empty())
            for (const auto& v : cpuData->skinnedVertices)
                updateAABB(v.position.x, v.position.y, v.position.z);
        else if (!cpuData->staticVertices.empty())
            for (const auto& v : cpuData->staticVertices)
                updateAABB(v.position.x, v.position.y, v.position.z);

        if (aabbMin.x < aabbMax.x)
        {
            ImGui::Separator();
            ImGui::Text("AABB Min: (%.3f, %.3f, %.3f)", aabbMin.x, aabbMin.y, aabbMin.z);
            ImGui::Text("AABB Max: (%.3f, %.3f, %.3f)", aabbMax.x, aabbMax.y, aabbMax.z);
            ImGui::Text("Extent:   (%.3f, %.3f, %.3f)",
                        aabbMax.x - aabbMin.x,
                        aabbMax.y - aabbMin.y,
                        aabbMax.z - aabbMin.z);
        }
    }

    // Sub-meshes & materials
    ImGui::Separator();
    uint32_t subMeshCount = model->GetSubMeshCount();
    ImGui::Text("Sub-meshes: %u", subMeshCount);
    ImGui::Text("Materials:  %u", subMeshCount); // 1:1 mapping in this engine

    // Skeleton
    if (model->HasSkeleton())
    {
        const GX::Skeleton* skeleton = model->GetSkeleton();
        ImGui::Separator();
        ImGui::Text("Bones: %u", skeleton->GetJointCount());
    }

    // Animations
    uint32_t animCount = model->GetAnimationCount();
    if (animCount > 0)
    {
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto& anims = model->GetAnimations();
            for (uint32_t i = 0; i < animCount; ++i)
            {
                ImGui::BulletText("[%u] %s  (%.2fs, %u ch)",
                                  i,
                                  anims[i].GetName().c_str(),
                                  anims[i].GetDuration(),
                                  static_cast<uint32_t>(anims[i].GetChannels().size()));
            }
        }
    }
}
