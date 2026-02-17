
/// @file ModelLoader.cpp
/// @brief 3Dモデルローダー（glTF/FBX/OBJ）

#include "pch.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "Graphics/3D/ModelLoader.h"
#include "Graphics/3D/GxmdModelLoader.h"
#include "Graphics/3D/Vertex3D.h"
#include "Core/Logger.h"
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <set>

#if defined(FBXSDK_SHARED)
#include <fbxsdk.h>
#endif

namespace GX
{

namespace
{

// -----------------------------------------------------------------------------
// 文字列ヘルパー
// -----------------------------------------------------------------------------
static std::wstring ToWString(const std::string& str)
{
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);
    UINT codepage = CP_UTF8;
    if (size == 0)
    {
        codepage = CP_ACP;
        size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    }
    std::wstring result(size > 0 ? size - 1 : 0, L'\0');
    if (size > 0)
        MultiByteToWideChar(codepage, 0, str.c_str(), -1, result.data(), size);
    return result;
}

static std::string ToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(size > 0 ? size - 1 : 0, '\0');
    if (size > 0)
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

static std::wstring GetBaseDir(const std::wstring& path)
{
    std::filesystem::path p(path);
    if (p.has_parent_path())
        return p.parent_path().wstring();
    return L".";
}

static std::wstring ToLower(const std::wstring& s)
{
    std::wstring out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return out;
}

static std::wstring GetExtensionLower(const std::wstring& path)
{
    std::filesystem::path p(path);
    return ToLower(p.extension().wstring());
}

// -----------------------------------------------------------------------------
// glTF用ヘルパー
// -----------------------------------------------------------------------------
static std::unordered_map<const cgltf_material*, int>
LoadGltfMaterials(const cgltf_data* data,
                  const std::wstring& baseDir,
                  TextureManager& texManager,
                  MaterialManager& matManager)
{
    std::unordered_map<const cgltf_material*, int> materialMap;

    for (cgltf_size i = 0; i < data->materials_count; ++i)
    {
        const cgltf_material& gltfMat = data->materials[i];
        Material mat;

        if (gltfMat.has_pbr_metallic_roughness)
        {
            const auto& pbr = gltfMat.pbr_metallic_roughness;

            mat.constants.albedoFactor = {
                pbr.base_color_factor[0],
                pbr.base_color_factor[1],
                pbr.base_color_factor[2],
                pbr.base_color_factor[3]
            };
            mat.constants.metallicFactor  = pbr.metallic_factor;
            mat.constants.roughnessFactor = pbr.roughness_factor;

            if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image)
            {
                const char* uri = pbr.base_color_texture.texture->image->uri;
                if (uri)
                {
                    std::wstring texPath = baseDir + L"/" + ToWString(uri);
                    mat.albedoMapHandle = texManager.LoadTexture(texPath);
                    if (mat.albedoMapHandle >= 0)
                        mat.constants.flags |= MaterialFlags::HasAlbedoMap;
                }
            }

            if (pbr.metallic_roughness_texture.texture && pbr.metallic_roughness_texture.texture->image)
            {
                const char* uri = pbr.metallic_roughness_texture.texture->image->uri;
                if (uri)
                {
                    std::wstring texPath = baseDir + L"/" + ToWString(uri);
                    mat.metRoughMapHandle = texManager.LoadTexture(texPath);
                    if (mat.metRoughMapHandle >= 0)
                        mat.constants.flags |= MaterialFlags::HasMetRoughMap;
                }
            }
        }

        if (gltfMat.normal_texture.texture && gltfMat.normal_texture.texture->image)
        {
            const char* uri = gltfMat.normal_texture.texture->image->uri;
            if (uri)
            {
                std::wstring texPath = baseDir + L"/" + ToWString(uri);
                mat.normalMapHandle = texManager.LoadTexture(texPath);
                if (mat.normalMapHandle >= 0)
                    mat.constants.flags |= MaterialFlags::HasNormalMap;
            }
        }

        if (gltfMat.occlusion_texture.texture && gltfMat.occlusion_texture.texture->image)
        {
            const char* uri = gltfMat.occlusion_texture.texture->image->uri;
            if (uri)
            {
                std::wstring texPath = baseDir + L"/" + ToWString(uri);
                mat.aoMapHandle = texManager.LoadTexture(texPath);
                if (mat.aoMapHandle >= 0)
                    mat.constants.flags |= MaterialFlags::HasAOMap;
            }
        }

        if (gltfMat.emissive_texture.texture && gltfMat.emissive_texture.texture->image)
        {
            const char* uri = gltfMat.emissive_texture.texture->image->uri;
            if (uri)
            {
                std::wstring texPath = baseDir + L"/" + ToWString(uri);
                mat.emissiveMapHandle = texManager.LoadTexture(texPath);
                if (mat.emissiveMapHandle >= 0)
                    mat.constants.flags |= MaterialFlags::HasEmissiveMap;
            }
        }
        mat.constants.emissiveFactor = {
            gltfMat.emissive_factor[0],
            gltfMat.emissive_factor[1],
            gltfMat.emissive_factor[2]
        };
        if (gltfMat.emissive_factor[0] > 0 || gltfMat.emissive_factor[1] > 0 || gltfMat.emissive_factor[2] > 0)
            mat.constants.emissiveStrength = 1.0f;

        int handle = matManager.CreateMaterial(mat);
        materialMap[&gltfMat] = handle;
    }

    return materialMap;
}

static std::unique_ptr<Skeleton> LoadGltfSkeleton(const cgltf_data* data)
{
    if (data->skins_count == 0)
        return nullptr;

    const cgltf_skin& skin = data->skins[0];
    auto skeleton = std::make_unique<Skeleton>();

    for (cgltf_size i = 0; i < skin.joints_count; ++i)
    {
        const cgltf_node* node = skin.joints[i];
        Joint joint;
        joint.name = node->name ? node->name : "";

        joint.parentIndex = -1;
        if (node->parent)
        {
            for (cgltf_size j = 0; j < skin.joints_count; ++j)
            {
                if (skin.joints[j] == node->parent)
                {
                    joint.parentIndex = static_cast<int>(j);
                    break;
                }
            }
        }

        XMStoreFloat4x4(&joint.inverseBindMatrix, XMMatrixIdentity());
        if (skin.inverse_bind_matrices)
        {
            float values[16];
            cgltf_accessor_read_float(skin.inverse_bind_matrices, i, values, 16);
            joint.inverseBindMatrix = *reinterpret_cast<XMFLOAT4X4*>(values);
        }

        if (node->has_matrix)
        {
            joint.localTransform = *reinterpret_cast<const XMFLOAT4X4*>(node->matrix);
        }
        else
        {
            XMMATRIX S = XMMatrixScaling(
                node->has_scale ? node->scale[0] : 1.0f,
                node->has_scale ? node->scale[1] : 1.0f,
                node->has_scale ? node->scale[2] : 1.0f);
            XMMATRIX R = node->has_rotation
                ? XMMatrixRotationQuaternion(XMVectorSet(
                    node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3]))
                : XMMatrixIdentity();
            XMMATRIX T = XMMatrixTranslation(
                node->has_translation ? node->translation[0] : 0.0f,
                node->has_translation ? node->translation[1] : 0.0f,
                node->has_translation ? node->translation[2] : 0.0f);
            XMMATRIX local = S * R * T;
            XMStoreFloat4x4(&joint.localTransform, local);
        }

        skeleton->AddJoint(joint);
    }

    return skeleton;
}

static void LoadGltfAnimations(const cgltf_data* data, Model& model)
{
    if (!model.HasSkeleton())
        return;

    const cgltf_skin& skin = data->skins[0];

    for (cgltf_size ai = 0; ai < data->animations_count; ++ai)
    {
        const cgltf_animation& gltfAnim = data->animations[ai];
        AnimationClip clip;
        clip.SetName(gltfAnim.name ? gltfAnim.name : "");

        float maxTime = 0.0f;
        std::unordered_map<int, AnimationChannel> channelMap;

        for (cgltf_size ci = 0; ci < gltfAnim.channels_count; ++ci)
        {
            const cgltf_animation_channel& gltfChannel = gltfAnim.channels[ci];
            if (!gltfChannel.target_node)
                continue;

            int jointIndex = -1;
            for (cgltf_size ji = 0; ji < skin.joints_count; ++ji)
            {
                if (skin.joints[ji] == gltfChannel.target_node)
                {
                    jointIndex = static_cast<int>(ji);
                    break;
                }
            }
            if (jointIndex < 0)
                continue;

            auto& channel = channelMap[jointIndex];
            channel.jointIndex = jointIndex;

            const cgltf_animation_sampler& sampler = *gltfChannel.sampler;

            std::vector<float> times(sampler.input->count);
            for (cgltf_size ti = 0; ti < sampler.input->count; ++ti)
            {
                float t;
                cgltf_accessor_read_float(sampler.input, ti, &t, 1);
                times[ti] = t;
                if (t > maxTime) maxTime = t;
            }

            switch (gltfChannel.target_path)
            {
            case cgltf_animation_path_type_translation:
                for (cgltf_size ki = 0; ki < sampler.output->count; ++ki)
                {
                    float v[3];
                    cgltf_accessor_read_float(sampler.output, ki, v, 3);
                    channel.translationKeys.push_back({ times[ki], { v[0], v[1], v[2] } });
                }
                break;
            case cgltf_animation_path_type_rotation:
                for (cgltf_size ki = 0; ki < sampler.output->count; ++ki)
                {
                    float v[4];
                    cgltf_accessor_read_float(sampler.output, ki, v, 4);
                    channel.rotationKeys.push_back({ times[ki], { v[0], v[1], v[2], v[3] } });
                }
                break;
            case cgltf_animation_path_type_scale:
                for (cgltf_size ki = 0; ki < sampler.output->count; ++ki)
                {
                    float v[3];
                    cgltf_accessor_read_float(sampler.output, ki, v, 3);
                    channel.scaleKeys.push_back({ times[ki], { v[0], v[1], v[2] } });
                }
                break;
            default:
                break;
            }
        }

        clip.SetDuration(maxTime);
        for (auto& [idx, ch] : channelMap)
            clip.AddChannel(ch);

        model.AddAnimation(std::move(clip));
    }
}

static bool GltfHasSkinningAttributes(const cgltf_data* data)
{
    for (cgltf_size mi = 0; mi < data->meshes_count; ++mi)
    {
        const cgltf_mesh& mesh = data->meshes[mi];
        for (cgltf_size pi = 0; pi < mesh.primitives_count; ++pi)
        {
            const cgltf_primitive& prim = mesh.primitives[pi];
            const cgltf_accessor* joints = nullptr;
            const cgltf_accessor* weights = nullptr;
            for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai)
            {
                if (prim.attributes[ai].type == cgltf_attribute_type_joints)
                    joints = prim.attributes[ai].data;
                else if (prim.attributes[ai].type == cgltf_attribute_type_weights)
                    weights = prim.attributes[ai].data;
            }
            if (joints && weights)
                return true;
        }
    }
    return false;
}

static void LoadGltfPrimitives(const cgltf_data* data,
                               bool useSkinning,
                               std::vector<Vertex3D_PBR>& staticVertices,
                               std::vector<Vertex3D_Skinned>& skinnedVertices,
                               std::vector<uint32_t>& indices,
                               std::vector<SubMesh>& subMeshes,
                               const std::unordered_map<const cgltf_material*, int>& materialMap)
{
    for (cgltf_size mi = 0; mi < data->meshes_count; ++mi)
    {
        const cgltf_mesh& mesh = data->meshes[mi];
        for (cgltf_size pi = 0; pi < mesh.primitives_count; ++pi)
        {
            const cgltf_primitive& prim = mesh.primitives[pi];
            if (prim.type != cgltf_primitive_type_triangles)
                continue;

            SubMesh sub;
            uint32_t baseVertex = useSkinning
                ? static_cast<uint32_t>(skinnedVertices.size())
                : static_cast<uint32_t>(staticVertices.size());
            sub.vertexOffset = baseVertex;
            sub.indexOffset  = static_cast<uint32_t>(indices.size());

            if (prim.material)
            {
                auto it = materialMap.find(prim.material);
                if (it != materialMap.end())
                    sub.materialHandle = it->second;
            }

            const cgltf_accessor* posAccessor = nullptr;
            const cgltf_accessor* normalAccessor = nullptr;
            const cgltf_accessor* uvAccessor = nullptr;
            const cgltf_accessor* tangentAccessor = nullptr;
            const cgltf_accessor* jointsAccessor = nullptr;
            const cgltf_accessor* weightsAccessor = nullptr;

            for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai)
            {
                switch (prim.attributes[ai].type)
                {
                case cgltf_attribute_type_position: posAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_normal:   normalAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_texcoord: uvAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_tangent:  tangentAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_joints:   jointsAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_weights:  weightsAccessor = prim.attributes[ai].data; break;
                default: break;
                }
            }

            if (!posAccessor)
                continue;

            cgltf_size vertexCount = posAccessor->count;

            for (cgltf_size vi = 0; vi < vertexCount; ++vi)
            {
                float values[4] = { 0, 0, 0, 1 };

                cgltf_accessor_read_float(posAccessor, vi, values, 3);
                XMFLOAT3 position = { values[0], values[1], values[2] };

                XMFLOAT3 normal = { 0, 1, 0 };
                if (normalAccessor)
                {
                    cgltf_accessor_read_float(normalAccessor, vi, values, 3);
                    normal = { values[0], values[1], values[2] };
                }

                XMFLOAT2 uv = { 0, 0 };
                if (uvAccessor)
                {
                    cgltf_accessor_read_float(uvAccessor, vi, values, 2);
                    uv = { values[0], values[1] };
                }

                XMFLOAT4 tangent = { 1, 0, 0, 1 };
                if (tangentAccessor)
                {
                    cgltf_accessor_read_float(tangentAccessor, vi, values, 4);
                    tangent = { values[0], values[1], values[2], values[3] };
                }

                if (useSkinning)
                {
                    Vertex3D_Skinned v = {};
                    v.position = position;
                    v.normal = normal;
                    v.texcoord = uv;
                    v.tangent = tangent;

                    uint32_t joints[4] = { 0, 0, 0, 0 };
                    float weights[4] = { 1.0f, 0, 0, 0 };
                    if (jointsAccessor && weightsAccessor)
                    {
                        cgltf_accessor_read_uint(jointsAccessor, vi, joints, 4);
                        cgltf_accessor_read_float(weightsAccessor, vi, weights, 4);
                        float wsum = weights[0] + weights[1] + weights[2] + weights[3];
                        if (wsum > 0.0001f)
                        {
                            for (int k = 0; k < 4; ++k)
                                weights[k] /= wsum;
                        }
                    }
                    v.joints = XMUINT4(joints[0], joints[1], joints[2], joints[3]);
                    v.weights = XMFLOAT4(weights[0], weights[1], weights[2], weights[3]);
                    skinnedVertices.push_back(v);
                }
                else
                {
                    Vertex3D_PBR v = {};
                    v.position = position;
                    v.normal = normal;
                    v.texcoord = uv;
                    v.tangent = tangent;
                    staticVertices.push_back(v);
                }
            }

            if (prim.indices)
            {
                for (cgltf_size ii = 0; ii < prim.indices->count; ++ii)
                {
                    uint32_t idx = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, ii));
                    indices.push_back(baseVertex + idx);
                }
                sub.indexCount = static_cast<uint32_t>(prim.indices->count);
            }
            else
            {
                for (uint32_t ii = 0; ii < static_cast<uint32_t>(vertexCount); ++ii)
                    indices.push_back(baseVertex + ii);
                sub.indexCount = static_cast<uint32_t>(vertexCount);
            }

            subMeshes.push_back(sub);
        }
    }
}

static std::unique_ptr<Model> LoadFromGLTF(const std::wstring& filePath,
                                           ID3D12Device* device,
                                           TextureManager& texManager,
                                           MaterialManager& matManager)
{
    std::string path = ToUtf8(filePath);

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success)
    {
        GX_LOG_ERROR("ModelLoader: Failed to parse glTF file (error: %d)", result);
        return nullptr;
    }

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success)
    {
        GX_LOG_ERROR("ModelLoader: Failed to load glTF buffers (error: %d)", result);
        cgltf_free(data);
        return nullptr;
    }

    auto model = std::make_unique<Model>();

    std::wstring baseDir = GetBaseDir(filePath);
    auto materialMap = LoadGltfMaterials(data, baseDir, texManager, matManager);

    bool useSkinning = (data->skins_count > 0) && GltfHasSkinningAttributes(data);

    std::vector<Vertex3D_PBR> staticVertices;
    std::vector<Vertex3D_Skinned> skinnedVertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> subMeshes;
    LoadGltfPrimitives(data, useSkinning, staticVertices, skinnedVertices, indices, subMeshes, materialMap);

    if (useSkinning && skinnedVertices.empty())
        useSkinning = false;

    if (!useSkinning && staticVertices.empty())
    {
        GX_LOG_ERROR("ModelLoader: No vertices found in glTF file");
        cgltf_free(data);
        return nullptr;
    }

    if (useSkinning)
    {
        model->SetVertexType(MeshVertexType::SkinnedPBR);
        uint32_t vbSize = static_cast<uint32_t>(skinnedVertices.size() * sizeof(Vertex3D_Skinned));
        model->GetMesh().CreateVertexBuffer(device, skinnedVertices.data(), vbSize, sizeof(Vertex3D_Skinned));
    }
    else
    {
        model->SetVertexType(MeshVertexType::PBR);
        uint32_t vbSize = static_cast<uint32_t>(staticVertices.size() * sizeof(Vertex3D_PBR));
        model->GetMesh().CreateVertexBuffer(device, staticVertices.data(), vbSize, sizeof(Vertex3D_PBR));
    }

    uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
    model->GetMesh().CreateIndexBuffer(device, indices.data(), ibSize, DXGI_FORMAT_R32_UINT);

    for (const auto& sub : subMeshes)
        model->GetMesh().AddSubMesh(sub);

    for (const auto& [mat, handle] : materialMap)
        model->AddMaterial(handle);

    if (useSkinning)
        model->SetSkeleton(LoadGltfSkeleton(data));
    if (useSkinning)
        LoadGltfAnimations(data, *model);

    MeshCPUData cpu;
    if (useSkinning)
        cpu.skinnedVertices = std::move(skinnedVertices);
    else
        cpu.staticVertices = std::move(staticVertices);
    cpu.indices = std::move(indices);
    model->SetCPUData(std::move(cpu));

    GX_LOG_INFO("ModelLoader: Loaded glTF model (submeshes=%zu, animations=%u)",
        subMeshes.size(), model->GetAnimationCount());

    cgltf_free(data);
    return model;
}

// -----------------------------------------------------------------------------
// FBX/OBJローダー（FBX SDK）
// 初学者向け: FBX読み込みは外部SDKに依存するため、ビルド設定で有効/無効が変わります。
// -----------------------------------------------------------------------------

#if defined(FBXSDK_SHARED)

static XMFLOAT4X4 ToXMFLOAT4X4(const FbxAMatrix& m)
{
    XMFLOAT4X4 out;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out.m[r][c] = static_cast<float>(m.Get(r, c));
    return out;
}

static std::wstring ResolveFbxTexturePath(FbxFileTexture* tex, const std::wstring& baseDir)
{
    if (!tex) return L"";
    const char* fileName = tex->GetFileName();
    const char* relName = tex->GetRelativeFileName();
    std::wstring path = ToWString(fileName && *fileName ? fileName : (relName ? relName : ""));
    if (path.empty())
        return L"";

    std::filesystem::path p(path);
    if (p.is_absolute())
        return path;
    return (std::filesystem::path(baseDir) / p).wstring();
}

static int LoadFbxTextureFromProperty(FbxProperty prop,
                                      TextureManager& texManager,
                                      const std::wstring& baseDir)
{
    if (!prop.IsValid())
        return -1;

    int texCount = prop.GetSrcObjectCount<FbxTexture>();
    for (int i = 0; i < texCount; ++i)
    {
        FbxTexture* tex = prop.GetSrcObject<FbxTexture>(i);
        if (!tex) continue;
        FbxFileTexture* fileTex = FbxCast<FbxFileTexture>(tex);
        if (!fileTex) continue;
        std::wstring texPath = ResolveFbxTexturePath(fileTex, baseDir);
        if (texPath.empty()) continue;
        int handle = texManager.LoadTexture(texPath);
        if (handle >= 0)
            return handle;
    }
    return -1;
}

static std::unordered_map<FbxSurfaceMaterial*, int>
LoadFbxMaterials(FbxScene* scene,
                 TextureManager& texManager,
                 MaterialManager& matManager,
                 const std::wstring& baseDir)
{
    std::unordered_map<FbxSurfaceMaterial*, int> materialMap;
    if (!scene) return materialMap;

    const int matCount = scene->GetMaterialCount();
    for (int i = 0; i < matCount; ++i)
    {
        FbxSurfaceMaterial* matSrc = scene->GetMaterial(i);
        if (!matSrc) continue;

        Material mat;

        FbxSurfaceLambert* lambert = FbxCast<FbxSurfaceLambert>(matSrc);
        FbxSurfacePhong* phong = FbxCast<FbxSurfacePhong>(matSrc);

        if (lambert)
        {
            FbxDouble3 diff = lambert->Diffuse.Get();
            double diffFactor = lambert->DiffuseFactor.Get();
            mat.constants.albedoFactor = {
                static_cast<float>(diff[0] * diffFactor),
                static_cast<float>(diff[1] * diffFactor),
                static_cast<float>(diff[2] * diffFactor),
                1.0f
            };

            FbxDouble3 emis = lambert->Emissive.Get();
            double emisFactor = lambert->EmissiveFactor.Get();
            mat.constants.emissiveFactor = {
                static_cast<float>(emis[0] * emisFactor),
                static_cast<float>(emis[1] * emisFactor),
                static_cast<float>(emis[2] * emisFactor)
            };
            if (emisFactor > 0.0)
                mat.constants.emissiveStrength = 1.0f;
        }

        if (phong)
        {
            double shininess = phong->Shininess.Get();
            float roughness = 1.0f - static_cast<float>(std::min(std::max(shininess / 100.0, 0.0), 1.0));
            mat.constants.roughnessFactor = (std::max)(roughness, 0.04f);
        }

        int albedoHandle = LoadFbxTextureFromProperty(
            matSrc->FindProperty(FbxSurfaceMaterial::sDiffuse),
            texManager, baseDir);
        if (albedoHandle >= 0)
        {
            mat.albedoMapHandle = albedoHandle;
            mat.constants.flags |= MaterialFlags::HasAlbedoMap;
        }

        int normalHandle = LoadFbxTextureFromProperty(
            matSrc->FindProperty(FbxSurfaceMaterial::sNormalMap),
            texManager, baseDir);
        if (normalHandle < 0)
        {
            normalHandle = LoadFbxTextureFromProperty(
                matSrc->FindProperty(FbxSurfaceMaterial::sBump),
                texManager, baseDir);
        }
        if (normalHandle >= 0)
        {
            mat.normalMapHandle = normalHandle;
            mat.constants.flags |= MaterialFlags::HasNormalMap;
        }

        int emissiveHandle = LoadFbxTextureFromProperty(
            matSrc->FindProperty(FbxSurfaceMaterial::sEmissive),
            texManager, baseDir);
        if (emissiveHandle >= 0)
        {
            mat.emissiveMapHandle = emissiveHandle;
            mat.constants.flags |= MaterialFlags::HasEmissiveMap;
            mat.constants.emissiveStrength = 1.0f;
        }

        int handle = matManager.CreateMaterial(mat);
        materialMap[matSrc] = handle;
    }
    return materialMap;
}

static void CollectFbxSkeletonNodes(FbxNode* node,
                                    std::vector<FbxNode*>& joints,
                                    std::unordered_map<FbxNode*, int>& jointMap)
{
    if (!node) return;
    FbxNodeAttribute* attr = node->GetNodeAttribute();
    if (attr && attr->GetAttributeType() == FbxNodeAttribute::eSkeleton)
    {
        if (jointMap.find(node) == jointMap.end())
        {
            jointMap[node] = static_cast<int>(joints.size());
            joints.push_back(node);
        }
    }

    for (int i = 0; i < node->GetChildCount(); ++i)
        CollectFbxSkeletonNodes(node->GetChild(i), joints, jointMap);
}

static std::unique_ptr<Skeleton> BuildFbxSkeleton(const std::vector<FbxNode*>& joints,
                                                  const std::unordered_map<FbxNode*, int>& jointMap,
                                                  const std::unordered_map<FbxNode*, XMFLOAT4X4>& invBindMap)
{
    auto skeleton = std::make_unique<Skeleton>();
    FbxTime zeroTime;

    for (FbxNode* node : joints)
    {
        Joint j;
        j.name = node->GetName() ? node->GetName() : "";

        int parentIndex = -1;
        FbxNode* parent = node->GetParent();
        while (parent)
        {
            auto it = jointMap.find(parent);
            if (it != jointMap.end())
            {
                parentIndex = it->second;
                break;
            }
            parent = parent->GetParent();
        }
        j.parentIndex = parentIndex;

        FbxAMatrix local = node->EvaluateLocalTransform(zeroTime);
        j.localTransform = ToXMFLOAT4X4(local);

        auto itInv = invBindMap.find(node);
        if (itInv != invBindMap.end())
            j.inverseBindMatrix = itInv->second;
        else
            XMStoreFloat4x4(&j.inverseBindMatrix, XMMatrixIdentity());

        skeleton->AddJoint(j);
    }
    return skeleton;
}

static void CollectAnimTimes(FbxAnimCurve* curve, std::vector<FbxTime>& out)
{
    if (!curve) return;
    const int keyCount = curve->KeyGetCount();
    for (int i = 0; i < keyCount; ++i)
        out.push_back(curve->KeyGetTime(i));
}

static void LoadFbxAnimations(FbxScene* scene,
                              const std::unordered_map<FbxNode*, int>& jointMap,
                              Model& model)
{
    if (!scene || jointMap.empty())
        return;

    const int stackCount = scene->GetSrcObjectCount<FbxAnimStack>();
    for (int si = 0; si < stackCount; ++si)
    {
        FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(si);
        if (!stack) continue;

        scene->SetCurrentAnimationStack(stack);

        FbxTakeInfo* take = scene->GetTakeInfo(stack->GetName());
        FbxTimeSpan span = take ? take->mLocalTimeSpan : stack->GetLocalTimeSpan();
        FbxTime start = span.GetStart();
        FbxTime end = span.GetStop();
        double duration = span.GetDuration().GetSecondDouble();

        AnimationClip clip;
        clip.SetName(stack->GetName() ? stack->GetName() : "");
        clip.SetDuration(static_cast<float>(duration));

        FbxAnimLayer* layer = stack->GetMember<FbxAnimLayer>(0);
        if (!layer)
        {
            model.AddAnimation(std::move(clip));
            continue;
        }

        for (const auto& [node, jointIndex] : jointMap)
        {
            std::vector<FbxTime> times;

            CollectAnimTimes(node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X), times);
            CollectAnimTimes(node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y), times);
            CollectAnimTimes(node->LclTranslation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z), times);
            CollectAnimTimes(node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X), times);
            CollectAnimTimes(node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y), times);
            CollectAnimTimes(node->LclRotation.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z), times);
            CollectAnimTimes(node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_X), times);
            CollectAnimTimes(node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Y), times);
            CollectAnimTimes(node->LclScaling.GetCurve(layer, FBXSDK_CURVENODE_COMPONENT_Z), times);

            if (times.empty())
                continue;

            std::sort(times.begin(), times.end());
            times.erase(std::unique(times.begin(), times.end(),
                [](const FbxTime& a, const FbxTime& b) { return a == b; }), times.end());

            AnimationChannel channel;
            channel.jointIndex = jointIndex;

            for (const auto& t : times)
            {
                FbxTime localTime = t;
                if (localTime < start) localTime = start;
                if (localTime > end) localTime = end;

                FbxAMatrix m = node->EvaluateLocalTransform(localTime);
                FbxVector4 pos = m.GetT();
                FbxQuaternion q = m.GetQ();
                FbxVector4 sca = m.GetS();

                float timeSec = static_cast<float>((localTime - start).GetSecondDouble());
                channel.translationKeys.push_back({ timeSec, { static_cast<float>(pos[0]),
                                                              static_cast<float>(pos[1]),
                                                              static_cast<float>(pos[2]) } });
                channel.rotationKeys.push_back({ timeSec, { static_cast<float>(q[0]),
                                                           static_cast<float>(q[1]),
                                                           static_cast<float>(q[2]),
                                                           static_cast<float>(q[3]) } });
                channel.scaleKeys.push_back({ timeSec, { static_cast<float>(sca[0]),
                                                        static_cast<float>(sca[1]),
                                                        static_cast<float>(sca[2]) } });
            }

            clip.AddChannel(channel);
        }

        model.AddAnimation(std::move(clip));
    }
}

static bool GetPolygonVertexTangent(FbxMesh* mesh, int polyIndex, int vertIndex, FbxVector4& outTangent)
{
    if (!mesh) return false;
    FbxGeometryElementTangent* tangents = mesh->GetElementTangent(0);
    if (!tangents) return false;

    int index = 0;
    if (tangents->GetMappingMode() == FbxGeometryElement::eByControlPoint)
    {
        int cpIndex = mesh->GetPolygonVertex(polyIndex, vertIndex);
        index = cpIndex;
    }
    else if (tangents->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
    {
        index = mesh->GetPolygonVertexIndex(polyIndex) + vertIndex;
    }
    else
    {
        return false;
    }

    if (tangents->GetReferenceMode() == FbxGeometryElement::eDirect)
    {
        outTangent = tangents->GetDirectArray().GetAt(index);
    }
    else if (tangents->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
    {
        int di = tangents->GetIndexArray().GetAt(index);
        outTangent = tangents->GetDirectArray().GetAt(di);
    }
    else
    {
        return false;
    }

    return true;
}

static void BuildFbxSkinWeights(FbxMesh* mesh,
                                const std::unordered_map<FbxNode*, int>& jointMap,
                                std::vector<XMUINT4>& outJoints,
                                std::vector<XMFLOAT4>& outWeights,
                                std::unordered_map<FbxNode*, XMFLOAT4X4>& outInvBind)
{
    const int cpCount = mesh->GetControlPointsCount();
    std::vector<std::vector<std::pair<int, float>>> influences(cpCount);

    const int skinCount = mesh->GetDeformerCount(FbxDeformer::eSkin);
    for (int si = 0; si < skinCount; ++si)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(mesh->GetDeformer(si, FbxDeformer::eSkin));
        if (!skin) continue;
        const int clusterCount = skin->GetClusterCount();
        for (int ci = 0; ci < clusterCount; ++ci)
        {
            FbxCluster* cluster = skin->GetCluster(ci);
            if (!cluster || !cluster->GetLink()) continue;
            FbxNode* link = cluster->GetLink();

            auto it = jointMap.find(link);
            if (it == jointMap.end()) continue;
            const int jointIndex = it->second;

            int indexCount = cluster->GetControlPointIndicesCount();
            int* indices = cluster->GetControlPointIndices();
            double* weights = cluster->GetControlPointWeights();
            for (int i = 0; i < indexCount; ++i)
            {
                int cpIndex = indices[i];
                if (cpIndex < 0 || cpIndex >= cpCount) continue;
                influences[cpIndex].push_back({ jointIndex, static_cast<float>(weights[i]) });
            }

            if (outInvBind.find(link) == outInvBind.end())
            {
                FbxAMatrix meshBind;
                FbxAMatrix linkBind;
                cluster->GetTransformMatrix(meshBind);
                cluster->GetTransformLinkMatrix(linkBind);
                FbxAMatrix invBind = linkBind.Inverse() * meshBind;
                outInvBind[link] = ToXMFLOAT4X4(invBind);
            }
        }
    }

    outJoints.resize(cpCount);
    outWeights.resize(cpCount);
    for (int i = 0; i < cpCount; ++i)
    {
        auto& inf = influences[i];
        std::sort(inf.begin(), inf.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        uint32_t joints[4] = { 0, 0, 0, 0 };
        float weights[4] = { 1.0f, 0, 0, 0 };

        if (!inf.empty())
        {
            const int count = (std::min)(static_cast<int>(inf.size()), 4);
            float wsum = 0.0f;
            for (int k = 0; k < count; ++k)
            {
                joints[k] = static_cast<uint32_t>(inf[k].first);
                weights[k] = inf[k].second;
                wsum += weights[k];
            }
            if (wsum > 0.0001f)
            {
                for (int k = 0; k < count; ++k)
                    weights[k] /= wsum;
            }
        }

        outJoints[i] = XMUINT4(joints[0], joints[1], joints[2], joints[3]);
        outWeights[i] = XMFLOAT4(weights[0], weights[1], weights[2], weights[3]);
    }
}

static std::unique_ptr<Model> LoadFromFBX(const std::wstring& filePath,
                                          ID3D12Device* device,
                                          TextureManager& texManager,
                                          MaterialManager& matManager)
{
    std::string path = ToUtf8(filePath);
    std::wstring baseDir = GetBaseDir(filePath);

    FbxManager* manager = FbxManager::Create();
    if (!manager)
    {
        GX_LOG_ERROR("ModelLoader: FBX SDK manager creation failed");
        return nullptr;
    }

    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(path.c_str(), -1, manager->GetIOSettings()))
    {
        GX_LOG_ERROR("ModelLoader: FBX import init failed: %s", importer->GetStatus().GetErrorString());
        importer->Destroy();
        manager->Destroy();
        return nullptr;
    }

    FbxScene* scene = FbxScene::Create(manager, "scene");
    if (!importer->Import(scene))
    {
        GX_LOG_ERROR("ModelLoader: FBX import failed: %s", importer->GetStatus().GetErrorString());
        importer->Destroy();
        manager->Destroy();
        return nullptr;
    }
    importer->Destroy();

    // 座標系と単位系を変換
    // 初学者向け: FBXはツールごとに軸向きや単位が違うため、DirectX基準に揃えます。
    FbxAxisSystem::DirectX.ConvertScene(scene);
    FbxSystemUnit::m.ConvertScene(scene);

    FbxGeometryConverter geomConv(manager);
    geomConv.Triangulate(scene, true);

    auto materialMap = LoadFbxMaterials(scene, texManager, matManager, baseDir);

    // メッシュを収集
    struct MeshItem { FbxNode* node; FbxMesh* mesh; };
    std::vector<MeshItem> meshItems;
    std::function<void(FbxNode*)> collectMeshes = [&](FbxNode* node)
    {
        if (!node) return;
        if (FbxMesh* mesh = node->GetMesh())
            meshItems.push_back({ node, mesh });
        for (int i = 0; i < node->GetChildCount(); ++i)
            collectMeshes(node->GetChild(i));
    };
    collectMeshes(scene->GetRootNode());

    if (meshItems.empty())
    {
        GX_LOG_ERROR("ModelLoader: No mesh found in FBX/OBJ");
        manager->Destroy();
        return nullptr;
    }

    bool modelSkinned = false;
    for (const auto& item : meshItems)
    {
        if (item.mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
        {
            modelSkinned = true;
            break;
        }
    }

    std::vector<FbxNode*> jointNodes;
    std::unordered_map<FbxNode*, int> jointMap;
    if (modelSkinned)
    {
        CollectFbxSkeletonNodes(scene->GetRootNode(), jointNodes, jointMap);

        // クラスター参照の関節が漏れないように追加
        // 初学者向け: スキニングは「クラスター」と呼ばれる関節参照を使うため、未登録を補います。
        for (const auto& item : meshItems)
        {
            const int skinCount = item.mesh->GetDeformerCount(FbxDeformer::eSkin);
            for (int si = 0; si < skinCount; ++si)
            {
                FbxSkin* skin = static_cast<FbxSkin*>(item.mesh->GetDeformer(si, FbxDeformer::eSkin));
                if (!skin) continue;
                const int clusterCount = skin->GetClusterCount();
                for (int ci = 0; ci < clusterCount; ++ci)
                {
                    FbxCluster* cluster = skin->GetCluster(ci);
                    if (!cluster || !cluster->GetLink()) continue;
                    FbxNode* link = cluster->GetLink();
                    if (jointMap.find(link) == jointMap.end())
                    {
                        jointMap[link] = static_cast<int>(jointNodes.size());
                        jointNodes.push_back(link);
                    }
                }
            }
        }
    }

    std::unordered_map<FbxNode*, XMFLOAT4X4> invBindMap;
    if (modelSkinned)
    {
        for (const auto& item : meshItems)
        {
            std::vector<XMUINT4> tmpJoints;
            std::vector<XMFLOAT4> tmpWeights;
            BuildFbxSkinWeights(item.mesh, jointMap, tmpJoints, tmpWeights, invBindMap);
        }
    }

    auto model = std::make_unique<Model>();
    if (modelSkinned)
        model->SetVertexType(MeshVertexType::SkinnedPBR);

    std::unique_ptr<Skeleton> skeleton;
    if (modelSkinned)
        skeleton = BuildFbxSkeleton(jointNodes, jointMap, invBindMap);

    std::vector<Vertex3D_PBR> staticVertices;
    std::vector<Vertex3D_Skinned> skinnedVertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> subMeshes;

    std::unordered_map<int, std::vector<uint32_t>> submeshIndexMap;

    for (const auto& item : meshItems)
    {
        FbxMesh* mesh = item.mesh;
        if (!mesh) continue;

        const int cpCount = mesh->GetControlPointsCount();
        const FbxVector4* controlPoints = mesh->GetControlPoints();

        std::vector<XMUINT4> cpJoints;
        std::vector<XMFLOAT4> cpWeights;
        if (modelSkinned && mesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
        {
            BuildFbxSkinWeights(mesh, jointMap, cpJoints, cpWeights, invBindMap);
        }
        else if (modelSkinned)
        {
            cpJoints.assign(cpCount, XMUINT4(0, 0, 0, 0));
            cpWeights.assign(cpCount, XMFLOAT4(1, 0, 0, 0));
        }

        // 法線・接線を用意
        // 初学者向け: ライティングや法線マップに必要なベクトルを確保します。
        // GenerateNormals(false): 既存法線を保持（trueだとConvertScene後のLHクロス積で
        // RH頂点から法線を再計算してしまい、全成分が反転する）
        mesh->GenerateNormals(false);
        mesh->GenerateTangentsData();

        FbxLayerElementMaterial* matElem = mesh->GetElementMaterial();

        FbxStringList uvNames;
        mesh->GetUVSetNames(uvNames);
        const char* uvName = uvNames.GetCount() > 0 ? uvNames[0] : nullptr;

        const int polyCount = mesh->GetPolygonCount();
        for (int p = 0; p < polyCount; ++p)
        {
            int matIndex = 0;
            if (matElem && matElem->GetMappingMode() == FbxGeometryElement::eByPolygon)
                matIndex = matElem->GetIndexArray().GetAt(p);

            int matHandle = -1;
            if (item.node && item.node->GetMaterialCount() > 0)
            {
                if (matIndex < item.node->GetMaterialCount())
                {
                    FbxSurfaceMaterial* fbxMat = item.node->GetMaterial(matIndex);
                    auto it = materialMap.find(fbxMat);
                    if (it != materialMap.end())
                        matHandle = it->second;
                }
            }

            int polySize = mesh->GetPolygonSize(p);
            if (polySize < 3)
                continue;

            // 既に三角化済みだが、念のためファン分割にも対応
            // 初学者向け: 多角形が来た場合も三角形に分解して描画します。
            for (int v = 0; v < polySize; ++v)
            {
                int cpIndex = mesh->GetPolygonVertex(p, v);
                FbxVector4 pos = controlPoints[cpIndex];
                FbxVector4 normal;
                if (!mesh->GetPolygonVertexNormal(p, v, normal))
                    normal = FbxVector4(0, 1, 0, 0);

                FbxVector2 uv(0, 0);
                bool unmapped = false;
                if (uvName)
                    mesh->GetPolygonVertexUV(p, v, uvName, uv, unmapped);

                FbxVector4 tangent(1, 0, 0, 1);
                GetPolygonVertexTangent(mesh, p, v, tangent);

                if (modelSkinned)
                {
                    Vertex3D_Skinned vtx = {};
                    vtx.position = { static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]) };
                    vtx.normal = { static_cast<float>(normal[0]), static_cast<float>(normal[1]), static_cast<float>(normal[2]) };
                    vtx.texcoord = { static_cast<float>(uv[0]), static_cast<float>(uv[1]) };
                    vtx.tangent = { static_cast<float>(tangent[0]), static_cast<float>(tangent[1]),
                                    static_cast<float>(tangent[2]), static_cast<float>(tangent[3]) };
                    if (cpIndex >= 0 && cpIndex < static_cast<int>(cpJoints.size()))
                    {
                        vtx.joints = cpJoints[cpIndex];
                        vtx.weights = cpWeights[cpIndex];
                    }
                    else
                    {
                        vtx.joints = XMUINT4(0, 0, 0, 0);
                        vtx.weights = XMFLOAT4(1, 0, 0, 0);
                    }
                    skinnedVertices.push_back(vtx);
                }
                else
                {
                    Vertex3D_PBR vtx = {};
                    vtx.position = { static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2]) };
                    vtx.normal = { static_cast<float>(normal[0]), static_cast<float>(normal[1]), static_cast<float>(normal[2]) };
                    vtx.texcoord = { static_cast<float>(uv[0]), static_cast<float>(uv[1]) };
                    vtx.tangent = { static_cast<float>(tangent[0]), static_cast<float>(tangent[1]),
                                    static_cast<float>(tangent[2]), static_cast<float>(tangent[3]) };
                    staticVertices.push_back(vtx);
                }

                uint32_t idx = modelSkinned
                    ? static_cast<uint32_t>(skinnedVertices.size() - 1)
                    : static_cast<uint32_t>(staticVertices.size() - 1);
                submeshIndexMap[matHandle].push_back(idx);
            }
        }
    }

    if (modelSkinned)
    {
        uint32_t vbSize = static_cast<uint32_t>(skinnedVertices.size() * sizeof(Vertex3D_Skinned));
        model->GetMesh().CreateVertexBuffer(device, skinnedVertices.data(), vbSize, sizeof(Vertex3D_Skinned));
    }
    else
    {
        uint32_t vbSize = static_cast<uint32_t>(staticVertices.size() * sizeof(Vertex3D_PBR));
        model->GetMesh().CreateVertexBuffer(device, staticVertices.data(), vbSize, sizeof(Vertex3D_PBR));
    }

    // インデックスバッファとサブメッシュを構築
    std::vector<int> materialHandles;
    materialHandles.reserve(submeshIndexMap.size());
    for (const auto& kv : submeshIndexMap)
        materialHandles.push_back(kv.first);
    std::sort(materialHandles.begin(), materialHandles.end());

    for (int handle : materialHandles)
    {
        const auto& subIdx = submeshIndexMap[handle];
        if (subIdx.empty())
            continue;
        SubMesh sub;
        sub.materialHandle = handle;
        sub.indexOffset = static_cast<uint32_t>(indices.size());
        sub.indexCount = static_cast<uint32_t>(subIdx.size());
        sub.vertexOffset = 0;
        indices.insert(indices.end(), subIdx.begin(), subIdx.end());
        subMeshes.push_back(sub);
    }

    uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
    model->GetMesh().CreateIndexBuffer(device, indices.data(), ibSize, DXGI_FORMAT_R32_UINT);
    for (const auto& sub : subMeshes)
        model->GetMesh().AddSubMesh(sub);

    for (const auto& [mat, handle] : materialMap)
        model->AddMaterial(handle);

    if (modelSkinned && skeleton)
        model->SetSkeleton(std::move(skeleton));

    if (modelSkinned && model->HasSkeleton())
        LoadFbxAnimations(scene, jointMap, *model);

    MeshCPUData cpu;
    if (modelSkinned)
        cpu.skinnedVertices = std::move(skinnedVertices);
    else
        cpu.staticVertices = std::move(staticVertices);
    cpu.indices = std::move(indices);
    model->SetCPUData(std::move(cpu));

    GX_LOG_INFO("ModelLoader: Loaded FBX/OBJ model (submeshes=%zu, animations=%u)",
        subMeshes.size(), model->GetAnimationCount());

    manager->Destroy();
    return model;
}

#endif // FBXSDK_SHARED

} // namespace

// -----------------------------------------------------------------------------
// 公開API
// -----------------------------------------------------------------------------

std::unique_ptr<Model> ModelLoader::LoadFromFile(const std::wstring& filePath,
                                                 ID3D12Device* device,
                                                 TextureManager& texManager,
                                                 MaterialManager& matManager)
{
    std::wstring ext = GetExtensionLower(filePath);

    if (ext == L".gxmd" || ext == L".gxpak")
    {
        GxmdModelLoader gxmdLoader;
        return gxmdLoader.LoadFromGxmd(filePath, device, texManager, matManager);
    }

    if (ext == L".gltf" || ext == L".glb")
        return LoadFromGLTF(filePath, device, texManager, matManager);

    if (ext == L".fbx" || ext == L".obj")
    {
#if defined(FBXSDK_SHARED)
        return LoadFromFBX(filePath, device, texManager, matManager);
#else
        GX_LOG_ERROR("ModelLoader: FBX SDK is not available for this build");
        return nullptr;
#endif
    }

    GX_LOG_ERROR("ModelLoader: Unsupported model format: %ls", ext.c_str());
    return nullptr;
}

} // namespace GX
