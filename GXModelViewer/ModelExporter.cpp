/// @file ModelExporter.cpp
/// @brief GX::Model -> gxconv::Scene -> GXMD/GXAN binary export

#include "ModelExporter.h"
#include "Core/Logger.h"

#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/3D/Mesh.h"

// gxconv intermediate + exporters
#include "intermediate/scene.h"
#include "exporters/gxmd_exporter.h"
#include "exporters/gxan_exporter.h"

#include <cstring>

// ============================================================
// Helper: Convert GX::Model -> gxconv::Scene
// ============================================================
static gxconv::Scene ConvertModelToScene(const SceneEntity& entity,
                                          GX::MaterialManager& matManager,
                                          GX::TextureManager* texManager)
{
    gxconv::Scene scene;
    const GX::Model* model = entity.model;
    if (!model) return scene;

    const auto* cpuData = model->GetCPUData();
    if (!cpuData)
    {
        GX_LOG_WARN("Model has no CPU data for export");
        return scene;
    }

    // --- Meshes ---
    const auto& subMeshes = model->GetMesh().GetSubMeshes();
    bool skinned = model->IsSkinned();

    for (size_t si = 0; si < subMeshes.size(); ++si)
    {
        const auto& sub = subMeshes[si];
        gxconv::IntermediateMesh mesh;
        mesh.name = "mesh_" + std::to_string(si);
        mesh.materialIndex = static_cast<uint32_t>(si);
        mesh.hasSkinning = skinned;

        // Extract vertices for this submesh's index range
        uint32_t minVert = UINT32_MAX, maxVert = 0;
        for (uint32_t i = 0; i < sub.indexCount; ++i)
        {
            uint32_t idx = cpuData->indices[sub.indexOffset + i];
            if (idx < minVert) minVert = idx;
            if (idx > maxVert) maxVert = idx;
        }

        if (minVert > maxVert)
        {
            scene.meshes.push_back(std::move(mesh));
            continue;
        }

        uint32_t vertCount = maxVert - minVert + 1;
        mesh.vertices.resize(vertCount);

        if (skinned && !cpuData->skinnedVertices.empty())
        {
            for (uint32_t vi = 0; vi < vertCount; ++vi)
            {
                uint32_t srcIdx = minVert + vi;
                if (srcIdx >= cpuData->skinnedVertices.size()) break;
                const auto& sv = cpuData->skinnedVertices[srcIdx];
                auto& dv = mesh.vertices[vi];
                memcpy(dv.position, &sv.position, sizeof(float) * 3);
                memcpy(dv.normal, &sv.normal, sizeof(float) * 3);
                memcpy(dv.texcoord, &sv.texcoord, sizeof(float) * 2);
                memcpy(dv.tangent, &sv.tangent, sizeof(float) * 4);
                dv.joints[0] = sv.joints.x;
                dv.joints[1] = sv.joints.y;
                dv.joints[2] = sv.joints.z;
                dv.joints[3] = sv.joints.w;
                memcpy(dv.weights, &sv.weights, sizeof(float) * 4);
            }
        }
        else if (!cpuData->staticVertices.empty())
        {
            for (uint32_t vi = 0; vi < vertCount; ++vi)
            {
                uint32_t srcIdx = minVert + vi;
                if (srcIdx >= cpuData->staticVertices.size()) break;
                const auto& sv = cpuData->staticVertices[srcIdx];
                auto& dv = mesh.vertices[vi];
                memcpy(dv.position, &sv.position, sizeof(float) * 3);
                memcpy(dv.normal, &sv.normal, sizeof(float) * 3);
                memcpy(dv.texcoord, &sv.texcoord, sizeof(float) * 2);
                memcpy(dv.tangent, &sv.tangent, sizeof(float) * 4);
            }
        }

        // Re-index relative to minVert
        mesh.indices.resize(sub.indexCount);
        for (uint32_t i = 0; i < sub.indexCount; ++i)
            mesh.indices[i] = cpuData->indices[sub.indexOffset + i] - minVert;

        scene.meshes.push_back(std::move(mesh));
    }

    // --- Materials ---
    for (size_t si = 0; si < subMeshes.size(); ++si)
    {
        gxconv::IntermediateMaterial intMat;
        intMat.name = "material_" + std::to_string(si);

        int matHandle = subMeshes[si].materialHandle;
        if (matHandle >= 0)
        {
            GX::Material* mat = matManager.GetMaterial(matHandle);
            if (mat)
            {
                intMat.shaderModel = mat->shaderModel;
                intMat.params = mat->shaderParams;

                // Extract texture file paths for GXMD export
                if (texManager)
                {
                    int texHandles[8] = {
                        mat->albedoMapHandle, mat->normalMapHandle, mat->metRoughMapHandle,
                        mat->aoMapHandle, mat->emissiveMapHandle, mat->toonRampMapHandle,
                        mat->subsurfaceMapHandle, mat->clearCoatMaskMapHandle
                    };
                    for (int t = 0; t < 8; ++t)
                    {
                        if (texHandles[t] >= 0)
                        {
                            const auto& wpath = texManager->GetFilePath(texHandles[t]);
                            if (!wpath.empty())
                            {
                                auto slash = wpath.find_last_of(L"/\\");
                                std::wstring filename = (slash != std::wstring::npos) ? wpath.substr(slash + 1) : wpath;
                                int sz = WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0, nullptr, nullptr);
                                std::string utf8(sz - 1, '\0');
                                WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, utf8.data(), sz, nullptr, nullptr);
                                intMat.texturePaths[t] = utf8;
                            }
                        }
                    }
                }
            }
        }
        scene.materials.push_back(std::move(intMat));
    }

    // --- Skeleton ---
    const GX::Skeleton* skeleton = model->GetSkeleton();
    if (skeleton && skeleton->GetJointCount() > 0)
    {
        scene.hasSkeleton = true;
        const auto& joints = skeleton->GetJoints();
        scene.skeleton.resize(joints.size());

        for (size_t ji = 0; ji < joints.size(); ++ji)
        {
            auto& ij = scene.skeleton[ji];
            const auto& gj = joints[ji];
            ij.name = gj.name;
            ij.parentIndex = gj.parentIndex;
            memcpy(ij.inverseBindMatrix, &gj.inverseBindMatrix, sizeof(float) * 16);

            // Decompose localTransform to TRS
            GX::TransformTRS trs = GX::DecomposeTRS(gj.localTransform);
            memcpy(ij.localTranslation, &trs.translation, sizeof(float) * 3);
            memcpy(ij.localRotation, &trs.rotation, sizeof(float) * 4);
            memcpy(ij.localScale, &trs.scale, sizeof(float) * 3);
        }
    }

    // --- Animations ---
    const auto& clips = model->GetAnimations();
    for (const auto& clip : clips)
    {
        gxconv::IntermediateAnimation intAnim;
        intAnim.name = clip.GetName();
        intAnim.duration = clip.GetDuration();

        for (const auto& ch : clip.GetChannels())
        {
            // Translation channel
            if (!ch.translationKeys.empty())
            {
                gxconv::IntermediateAnimChannel intCh;
                intCh.jointIndex = static_cast<uint32_t>(ch.jointIndex);
                if (skeleton && ch.jointIndex >= 0 &&
                    ch.jointIndex < static_cast<int>(skeleton->GetJointCount()))
                    intCh.boneName = skeleton->GetJoints()[ch.jointIndex].name;
                intCh.target = 0; // Translation
                intCh.interpolation = static_cast<uint8_t>(ch.interpolation);
                intCh.vecKeys.resize(ch.translationKeys.size());
                for (size_t k = 0; k < ch.translationKeys.size(); ++k)
                {
                    intCh.vecKeys[k].time = ch.translationKeys[k].time;
                    memcpy(intCh.vecKeys[k].value, &ch.translationKeys[k].value, sizeof(float) * 3);
                }
                intAnim.channels.push_back(std::move(intCh));
            }

            // Rotation channel
            if (!ch.rotationKeys.empty())
            {
                gxconv::IntermediateAnimChannel intCh;
                intCh.jointIndex = static_cast<uint32_t>(ch.jointIndex);
                if (skeleton && ch.jointIndex >= 0 &&
                    ch.jointIndex < static_cast<int>(skeleton->GetJointCount()))
                    intCh.boneName = skeleton->GetJoints()[ch.jointIndex].name;
                intCh.target = 1; // Rotation
                intCh.interpolation = static_cast<uint8_t>(ch.interpolation);
                intCh.quatKeys.resize(ch.rotationKeys.size());
                for (size_t k = 0; k < ch.rotationKeys.size(); ++k)
                {
                    intCh.quatKeys[k].time = ch.rotationKeys[k].time;
                    memcpy(intCh.quatKeys[k].value, &ch.rotationKeys[k].value, sizeof(float) * 4);
                }
                intAnim.channels.push_back(std::move(intCh));
            }

            // Scale channel
            if (!ch.scaleKeys.empty())
            {
                gxconv::IntermediateAnimChannel intCh;
                intCh.jointIndex = static_cast<uint32_t>(ch.jointIndex);
                if (skeleton && ch.jointIndex >= 0 &&
                    ch.jointIndex < static_cast<int>(skeleton->GetJointCount()))
                    intCh.boneName = skeleton->GetJoints()[ch.jointIndex].name;
                intCh.target = 2; // Scale
                intCh.interpolation = static_cast<uint8_t>(ch.interpolation);
                intCh.vecKeys.resize(ch.scaleKeys.size());
                for (size_t k = 0; k < ch.scaleKeys.size(); ++k)
                {
                    intCh.vecKeys[k].time = ch.scaleKeys[k].time;
                    memcpy(intCh.vecKeys[k].value, &ch.scaleKeys[k].value, sizeof(float) * 3);
                }
                intAnim.channels.push_back(std::move(intCh));
            }
        }

        scene.animations.push_back(std::move(intAnim));
    }

    return scene;
}

// ============================================================
// ExportToGxmd
// ============================================================
bool ModelExporter::ExportToGxmd(const SceneEntity& entity,
                                  GX::MaterialManager& matManager,
                                  GX::TextureManager& texManager,
                                  const std::string& outputPath)
{
    if (!entity.model)
    {
        GX_LOG_ERROR("ExportToGxmd: no model");
        return false;
    }

    gxconv::Scene scene = ConvertModelToScene(entity, matManager, &texManager);
    if (scene.meshes.empty())
    {
        GX_LOG_ERROR("ExportToGxmd: conversion produced no meshes");
        return false;
    }

    gxconv::GxmdExporter exporter;
    gxconv::ExportOptions opts;
    return exporter.Export(scene, outputPath, opts);
}

// ============================================================
// ExportToGxan
// ============================================================
bool ModelExporter::ExportToGxan(const SceneEntity& entity,
                                  const std::string& outputPath)
{
    if (!entity.model)
    {
        GX_LOG_ERROR("ExportToGxan: no model");
        return false;
    }

    if (entity.model->GetAnimations().empty())
    {
        GX_LOG_ERROR("ExportToGxan: model has no animations");
        return false;
    }

    // We need a dummy matManager for the conversion (materials aren't used by GXAN)
    // but ConvertModelToScene requires one. Use a temporary.
    GX::MaterialManager dummyMat;
    gxconv::Scene scene = ConvertModelToScene(entity, dummyMat, nullptr);
    if (scene.animations.empty())
    {
        GX_LOG_ERROR("ExportToGxan: conversion produced no animations");
        return false;
    }

    gxconv::GxanExporter exporter;
    return exporter.Export(scene, outputPath);
}
