/// @file GxmdModelLoader.cpp
/// @brief GXMD model loader adapter - converts gxloader::LoadedModel to GX::Model

#include "pch.h"
#include "GxmdModelLoader.h"
#include "Graphics/3D/Vertex3D.h"
#include "Graphics/3D/Mesh.h"
#include "Graphics/3D/Skeleton.h"
#include "Graphics/3D/AnimationClip.h"
#include "Core/Logger.h"

#include <model_loader.h>
#include <filesystem>

// Verify vertex layout compatibility
static_assert(sizeof(gxfmt::VertexStandard) == sizeof(GX::Vertex3D_PBR),
    "VertexStandard(48B) must match Vertex3D_PBR(48B)");
static_assert(sizeof(gxfmt::VertexSkinned) == sizeof(GX::Vertex3D_Skinned),
    "VertexSkinned(80B) must match Vertex3D_Skinned(80B)");

namespace GX
{

static std::wstring Utf8ToWide(const std::string& str)
{
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

static std::wstring GetDirectory(const std::wstring& filePath)
{
    size_t pos = filePath.find_last_of(L"/\\");
    if (pos == std::wstring::npos) return L".";
    return filePath.substr(0, pos);
}

std::unique_ptr<Model> GxmdModelLoader::LoadFromGxmd(const std::wstring& filePath,
                                                       ID3D12Device* device,
                                                       TextureManager& texManager,
                                                       MaterialManager& matManager)
{
    auto loaded = gxloader::LoadGxmdW(filePath);
    if (!loaded)
        return nullptr;

    auto model = std::make_unique<Model>();
    std::wstring dir = GetDirectory(filePath);

    // --- Materials ---
    for (auto& srcMat : loaded->materials)
    {
        Material mat{};

        // シェーダーモデルとパラメータを直接格納
        mat.shaderModel  = srcMat.shaderModel;
        mat.shaderParams = srcMat.params;

        // 後方互換: MaterialConstantsにも主要値を格納
        mat.constants.albedoFactor = {
            srcMat.params.baseColor[0], srcMat.params.baseColor[1],
            srcMat.params.baseColor[2], srcMat.params.baseColor[3]
        };
        mat.constants.emissiveFactor = {
            srcMat.params.emissiveFactor[0], srcMat.params.emissiveFactor[1],
            srcMat.params.emissiveFactor[2]
        };
        mat.constants.emissiveStrength = srcMat.params.emissiveStrength;
        mat.constants.aoStrength = srcMat.params.aoStrength;
        mat.constants.flags = 0;

        // Shader model specific mapping
        switch (srcMat.shaderModel)
        {
        case gxfmt::ShaderModel::Standard:
            mat.constants.metallicFactor = srcMat.params.metallic;
            mat.constants.roughnessFactor = srcMat.params.roughness;
            break;
        case gxfmt::ShaderModel::Phong:
            mat.constants.metallicFactor = 0.0f;
            mat.constants.roughnessFactor = 1.0f - (srcMat.params.shininess / 128.0f);
            if (mat.constants.roughnessFactor < 0.05f) mat.constants.roughnessFactor = 0.05f;
            break;
        case gxfmt::ShaderModel::Unlit:
            mat.constants.metallicFactor = 0.0f;
            mat.constants.roughnessFactor = 1.0f;
            break;
        default:
            mat.constants.metallicFactor = srcMat.params.metallic;
            mat.constants.roughnessFactor = srcMat.params.roughness;
            break;
        }

        // Clear stale string table offsets (these are meaningless at runtime)
        for (int t = 0; t < 8; ++t)
            mat.shaderParams.textureNames[t] = -1;

        // Load textures (slots 0-7)
        auto loadTex = [&](int slot) -> int {
            if (slot < 0 || slot >= 8 || srcMat.texturePaths[slot].empty())
                return -1;
            std::wstring texName = Utf8ToWide(srcMat.texturePaths[slot]);
            // Strip directory from texture path (FBX stores absolute paths)
            size_t lastSlash = texName.find_last_of(L"/\\");
            if (lastSlash != std::wstring::npos)
                texName = texName.substr(lastSlash + 1);
            // Try: dir/textureName
            std::wstring texPath = dir + L"/" + texName;
            int h = texManager.LoadTexture(texPath);
            if (h >= 0) return h;
            // Try: dir/textures/textureName
            texPath = dir + L"/textures/" + texName;
            h = texManager.LoadTexture(texPath);
            if (h >= 0) return h;
            // Try: dir/*.fbm/textureName (FBX texture subdirectory convention)
            {
                std::error_code ec;
                for (auto& entry : std::filesystem::directory_iterator(std::filesystem::path(dir), ec))
                {
                    if (!entry.is_directory()) continue;
                    auto dirName = entry.path().filename().wstring();
                    if (dirName.size() > 4 && dirName.substr(dirName.size() - 4) == L".fbm")
                    {
                        texPath = entry.path().wstring() + L"/" + texName;
                        h = texManager.LoadTexture(texPath);
                        if (h >= 0) return h;
                    }
                }
            }
            GX_LOG_WARN("GXMD texture not found: %s (slot %d, material '%s')",
                        srcMat.texturePaths[slot].c_str(), slot, srcMat.name.c_str());
            return -1;
        };

        // Log if no texture paths at all
        {
            bool anyTex = false;
            for (int t = 0; t < 8; ++t)
                if (!srcMat.texturePaths[t].empty()) { anyTex = true; break; }
            if (!anyTex)
                GX_LOG_INFO("GXMD material '%s': no texture paths in file", srcMat.name.c_str());
        }

        mat.albedoMapHandle = loadTex(0);
        if (mat.albedoMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasAlbedoMap;

        mat.normalMapHandle = loadTex(1);
        if (mat.normalMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasNormalMap;

        mat.metRoughMapHandle = loadTex(2);
        if (mat.metRoughMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasMetRoughMap;

        mat.aoMapHandle = loadTex(3);
        if (mat.aoMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasAOMap;

        mat.emissiveMapHandle = loadTex(4);
        if (mat.emissiveMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasEmissiveMap;

        mat.toonRampMapHandle = loadTex(5);
        if (mat.toonRampMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasToonRampMap;

        mat.subsurfaceMapHandle = loadTex(6);
        if (mat.subsurfaceMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasSubsurfaceMap;

        mat.clearCoatMaskMapHandle = loadTex(7);
        if (mat.clearCoatMaskMapHandle >= 0)
            mat.constants.flags |= MaterialFlags::HasClearCoatMaskMap;

        int handle = matManager.CreateMaterial(mat);
        model->AddMaterial(handle);
    }

    // --- Mesh ---
    MeshCPUData cpuData;
    bool isSkinned = loaded->isSkinned;

    if (isSkinned)
    {
        model->SetVertexType(MeshVertexType::SkinnedPBR);

        // Copy skinned vertices (binary compatible layout)
        cpuData.skinnedVertices.resize(loaded->skinnedVertices.size());
        std::memcpy(cpuData.skinnedVertices.data(), loaded->skinnedVertices.data(),
                    loaded->skinnedVertices.size() * sizeof(Vertex3D_Skinned));

        // Create GPU vertex buffer
        model->GetMesh().CreateVertexBuffer(device,
            loaded->skinnedVertices.data(),
            static_cast<uint32_t>(loaded->skinnedVertices.size() * sizeof(Vertex3D_Skinned)),
            sizeof(Vertex3D_Skinned));
    }
    else
    {
        model->SetVertexType(MeshVertexType::PBR);

        // Copy standard vertices (binary compatible layout)
        cpuData.staticVertices.resize(loaded->standardVertices.size());
        std::memcpy(cpuData.staticVertices.data(), loaded->standardVertices.data(),
                    loaded->standardVertices.size() * sizeof(Vertex3D_PBR));

        // Create GPU vertex buffer
        model->GetMesh().CreateVertexBuffer(device,
            loaded->standardVertices.data(),
            static_cast<uint32_t>(loaded->standardVertices.size() * sizeof(Vertex3D_PBR)),
            sizeof(Vertex3D_PBR));
    }

    // Indices - always convert to uint32 for GPU
    if (loaded->uses16BitIndices)
    {
        cpuData.indices.resize(loaded->indices16.size());
        for (size_t i = 0; i < loaded->indices16.size(); ++i)
            cpuData.indices[i] = loaded->indices16[i];
    }
    else
    {
        cpuData.indices = loaded->indices32;
    }

    model->GetMesh().CreateIndexBuffer(device,
        cpuData.indices.data(),
        static_cast<uint32_t>(cpuData.indices.size() * sizeof(uint32_t)),
        DXGI_FORMAT_R32_UINT);

    // Sub-meshes
    for (auto& sm : loaded->subMeshes)
    {
        SubMesh sub;
        sub.vertexOffset = sm.vertexOffset;
        sub.indexOffset = sm.indexOffset;
        sub.indexCount = sm.indexCount;
        sub.materialHandle = -1;

        // Map material index to handle
        if (sm.materialIndex < model->GetMaterialHandles().size())
            sub.materialHandle = model->GetMaterialHandles()[sm.materialIndex];

        model->GetMesh().AddSubMesh(sub);
    }

    model->SetCPUData(std::move(cpuData));

    // スムース法線計算
    {
        const auto* cpu = model->GetCPUData();
        if (cpu)
        {
            uint32_t vtxCount = isSkinned ? static_cast<uint32_t>(cpu->skinnedVertices.size())
                                           : static_cast<uint32_t>(cpu->staticVertices.size());
            std::vector<XMFLOAT3> positions(vtxCount), normals(vtxCount);
            if (isSkinned)
            {
                for (uint32_t i = 0; i < vtxCount; ++i)
                {
                    positions[i] = cpu->skinnedVertices[i].position;
                    normals[i]   = cpu->skinnedVertices[i].normal;
                }
            }
            else
            {
                for (uint32_t i = 0; i < vtxCount; ++i)
                {
                    positions[i] = cpu->staticVertices[i].position;
                    normals[i]   = cpu->staticVertices[i].normal;
                }
            }
            auto smooth = ComputeSmoothNormals(positions.data(), normals.data(), vtxCount);
            model->GetMesh().CreateSmoothNormalBuffer(device, smooth.data(), vtxCount);
        }
    }

    // --- Skeleton ---
    if (!loaded->joints.empty())
    {
        auto skeleton = std::make_unique<Skeleton>();
        for (auto& srcJoint : loaded->joints)
        {
            Joint j;
            j.name = srcJoint.name;
            j.parentIndex = srcJoint.parentIndex;
            std::memcpy(&j.inverseBindMatrix, srcJoint.inverseBindMatrix, sizeof(XMFLOAT4X4));

            // Compose local transform from TRS
            XMMATRIX S = DirectX::XMMatrixScaling(
                srcJoint.localScale[0], srcJoint.localScale[1], srcJoint.localScale[2]);
            XMVECTOR q = DirectX::XMVectorSet(
                srcJoint.localRotation[0], srcJoint.localRotation[1],
                srcJoint.localRotation[2], srcJoint.localRotation[3]);
            XMMATRIX R = DirectX::XMMatrixRotationQuaternion(q);
            XMMATRIX T = DirectX::XMMatrixTranslation(
                srcJoint.localTranslation[0], srcJoint.localTranslation[1],
                srcJoint.localTranslation[2]);
            DirectX::XMStoreFloat4x4(&j.localTransform, S * R * T);

            skeleton->AddJoint(j);
        }
        model->SetSkeleton(std::move(skeleton));
    }

    // --- Animations ---
    for (auto& srcAnim : loaded->animations)
    {
        AnimationClip clip;
        clip.SetName(srcAnim.name);
        clip.SetDuration(srcAnim.duration);

        // Group channels by joint index (merge T/R/S into single AnimationChannel)
        std::unordered_map<uint32_t, AnimationChannel> channelMap;

        for (auto& srcCh : srcAnim.channels)
        {
            auto& ch = channelMap[srcCh.jointIndex];
            ch.jointIndex = static_cast<int>(srcCh.jointIndex);

            if (srcCh.target == 0) // Translation
            {
                for (auto& k : srcCh.vecKeys)
                {
                    Keyframe<XMFLOAT3> kf;
                    kf.time = k.time;
                    kf.value = { k.value[0], k.value[1], k.value[2] };
                    ch.translationKeys.push_back(kf);
                }
            }
            else if (srcCh.target == 1) // Rotation
            {
                for (auto& k : srcCh.quatKeys)
                {
                    Keyframe<XMFLOAT4> kf;
                    kf.time = k.time;
                    kf.value = { k.value[0], k.value[1], k.value[2], k.value[3] };
                    ch.rotationKeys.push_back(kf);
                }
            }
            else if (srcCh.target == 2) // Scale
            {
                for (auto& k : srcCh.vecKeys)
                {
                    Keyframe<XMFLOAT3> kf;
                    kf.time = k.time;
                    kf.value = { k.value[0], k.value[1], k.value[2] };
                    ch.scaleKeys.push_back(kf);
                }
            }
        }

        for (auto& [idx, ch] : channelMap)
            clip.AddChannel(ch);

        model->AddAnimation(std::move(clip));
    }

    return model;
}

} // namespace GX
