/// @file ModelLoader.cpp
/// @brief glTF 2.0モデルローダーの実装
///
/// cgltf.hを使用してglTF 2.0のメッシュ、マテリアル、テクスチャ、
/// スケルトン、アニメーションを読み込む。

#include "pch.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "Graphics/3D/ModelLoader.h"
#include "Graphics/3D/Vertex3D.h"
#include "Core/Logger.h"
#include <filesystem>

namespace GX
{

// ============================================================================
// ヘルパー関数
// ============================================================================

static std::wstring ToWString(const std::string& str)
{
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

static const void* GetBufferData(const cgltf_accessor* accessor, cgltf_size index)
{
    const cgltf_buffer_view* view = accessor->buffer_view;
    const uint8_t* data = static_cast<const uint8_t*>(view->buffer->data);
    return data + view->offset + accessor->offset + index * accessor->stride;
}

static float ReadFloat(const cgltf_accessor* accessor, cgltf_size index, int component)
{
    float out = 0;
    cgltf_accessor_read_float(accessor, index, &out + 0, 1);
    // Use the full cgltf API for proper reading
    float values[16];
    cgltf_accessor_read_float(accessor, index, values, cgltf_num_components(accessor->type));
    return values[component];
}

// ============================================================================
// メッシュ読み込み
// ============================================================================

static void LoadMeshPrimitives(const cgltf_data* data,
                                 std::vector<Vertex3D_PBR>& vertices,
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
            sub.vertexOffset = static_cast<uint32_t>(vertices.size());
            sub.indexOffset  = static_cast<uint32_t>(indices.size());

            // マテリアル
            if (prim.material)
            {
                auto it = materialMap.find(prim.material);
                if (it != materialMap.end())
                    sub.materialHandle = it->second;
            }

            // アクセサを検索
            const cgltf_accessor* posAccessor = nullptr;
            const cgltf_accessor* normalAccessor = nullptr;
            const cgltf_accessor* uvAccessor = nullptr;
            const cgltf_accessor* tangentAccessor = nullptr;

            for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai)
            {
                switch (prim.attributes[ai].type)
                {
                case cgltf_attribute_type_position: posAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_normal:   normalAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_texcoord: uvAccessor = prim.attributes[ai].data; break;
                case cgltf_attribute_type_tangent:  tangentAccessor = prim.attributes[ai].data; break;
                default: break;
                }
            }

            if (!posAccessor)
                continue;

            cgltf_size vertexCount = posAccessor->count;

            // 頂点データ読み込み
            for (cgltf_size vi = 0; vi < vertexCount; ++vi)
            {
                Vertex3D_PBR v = {};
                float values[4];

                // Position
                cgltf_accessor_read_float(posAccessor, vi, values, 3);
                v.position = { values[0], values[1], values[2] };

                // Normal
                if (normalAccessor)
                {
                    cgltf_accessor_read_float(normalAccessor, vi, values, 3);
                    v.normal = { values[0], values[1], values[2] };
                }
                else
                {
                    v.normal = { 0, 1, 0 };
                }

                // UV
                if (uvAccessor)
                {
                    cgltf_accessor_read_float(uvAccessor, vi, values, 2);
                    v.texcoord = { values[0], values[1] };
                }

                // Tangent
                if (tangentAccessor)
                {
                    cgltf_accessor_read_float(tangentAccessor, vi, values, 4);
                    v.tangent = { values[0], values[1], values[2], values[3] };
                }
                else
                {
                    v.tangent = { 1, 0, 0, 1 };
                }

                vertices.push_back(v);
            }

            // インデックスデータ
            if (prim.indices)
            {
                for (cgltf_size ii = 0; ii < prim.indices->count; ++ii)
                {
                    uint32_t idx = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, ii));
                    indices.push_back(sub.vertexOffset + idx);
                }
                sub.indexCount = static_cast<uint32_t>(prim.indices->count);
            }
            else
            {
                // インデックスなし: 連番生成
                for (uint32_t ii = 0; ii < vertexCount; ++ii)
                    indices.push_back(sub.vertexOffset + ii);
                sub.indexCount = static_cast<uint32_t>(vertexCount);
            }

            subMeshes.push_back(sub);
        }
    }
}

// ============================================================================
// マテリアル読み込み
// ============================================================================

static std::unordered_map<const cgltf_material*, int>
LoadMaterials(const cgltf_data* data,
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

            // Albedo map
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

            // Metallic-Roughness map
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

        // Normal map
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

        // AO map
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

        // Emissive
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

// ============================================================================
// スケルトン読み込み
// ============================================================================

static std::unique_ptr<Skeleton> LoadSkeleton(const cgltf_data* data)
{
    if (data->skins_count == 0)
        return nullptr;

    const cgltf_skin& skin = data->skins[0];
    auto skeleton = std::make_unique<Skeleton>();

    // ジョイントを追加
    for (cgltf_size i = 0; i < skin.joints_count; ++i)
    {
        const cgltf_node* node = skin.joints[i];
        Joint joint;
        joint.name = node->name ? node->name : "";

        // 親インデックス
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

        // 逆バインド行列
        XMStoreFloat4x4(&joint.inverseBindMatrix, XMMatrixIdentity());
        if (skin.inverse_bind_matrices)
        {
            float values[16];
            cgltf_accessor_read_float(skin.inverse_bind_matrices, i, values, 16);
            joint.inverseBindMatrix = *reinterpret_cast<XMFLOAT4X4*>(values);
        }

        // ローカルトランスフォーム
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

// ============================================================================
// アニメーション読み込み
// ============================================================================

static void LoadAnimations(const cgltf_data* data, Model& model)
{
    if (!model.HasSkeleton())
        return;

    const Skeleton* skeleton = model.GetSkeleton();
    const cgltf_skin& skin = data->skins[0];

    for (cgltf_size ai = 0; ai < data->animations_count; ++ai)
    {
        const cgltf_animation& gltfAnim = data->animations[ai];
        AnimationClip clip;
        clip.SetName(gltfAnim.name ? gltfAnim.name : "");

        float maxTime = 0.0f;

        // チャンネルをジョイントごとにまとめる
        std::unordered_map<int, AnimationChannel> channelMap;

        for (cgltf_size ci = 0; ci < gltfAnim.channels_count; ++ci)
        {
            const cgltf_animation_channel& gltfChannel = gltfAnim.channels[ci];
            if (!gltfChannel.target_node)
                continue;

            // ジョイントインデックスを特定
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

            // タイムスタンプ読み込み
            std::vector<float> times(sampler.input->count);
            for (cgltf_size ti = 0; ti < sampler.input->count; ++ti)
            {
                float t;
                cgltf_accessor_read_float(sampler.input, ti, &t, 1);
                times[ti] = t;
                if (t > maxTime) maxTime = t;
            }

            // 値読み込み
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

// ============================================================================
// メインローダー
// ============================================================================

std::unique_ptr<Model> ModelLoader::LoadFromFile(const std::wstring& filePath,
                                                   ID3D12Device* device,
                                                   TextureManager& texManager,
                                                   MaterialManager& matManager)
{
    // wstring → string
    std::string path;
    {
        int size = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        path.resize(size - 1);
        WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, path.data(), size, nullptr, nullptr);
    }

    // ベースディレクトリ取得
    std::wstring baseDir = filePath;
    auto lastSlash = baseDir.find_last_of(L"/\\");
    if (lastSlash != std::wstring::npos)
        baseDir = baseDir.substr(0, lastSlash);
    else
        baseDir = L".";

    // cgltfでパース
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success)
    {
        GX_LOG_ERROR("ModelLoader: Failed to parse glTF file (error: %d)", result);
        return nullptr;
    }

    // バッファ読み込み
    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success)
    {
        GX_LOG_ERROR("ModelLoader: Failed to load glTF buffers (error: %d)", result);
        cgltf_free(data);
        return nullptr;
    }

    auto model = std::make_unique<Model>();

    // マテリアル読み込み
    auto materialMap = LoadMaterials(data, baseDir, texManager, matManager);

    // メッシュ読み込み
    std::vector<Vertex3D_PBR> vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh> subMeshes;
    LoadMeshPrimitives(data, vertices, indices, subMeshes, materialMap);

    if (vertices.empty())
    {
        GX_LOG_ERROR("ModelLoader: No vertices found in glTF file");
        cgltf_free(data);
        return nullptr;
    }

    // GPUバッファ作成
    uint32_t vbSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex3D_PBR));
    model->GetMesh().CreateVertexBuffer(device, vertices.data(), vbSize, sizeof(Vertex3D_PBR));

    uint32_t ibSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
    model->GetMesh().CreateIndexBuffer(device, indices.data(), ibSize, DXGI_FORMAT_R32_UINT);

    for (const auto& sub : subMeshes)
        model->GetMesh().AddSubMesh(sub);

    for (const auto& [mat, handle] : materialMap)
        model->AddMaterial(handle);

    // スケルトン読み込み
    model->SetSkeleton(LoadSkeleton(data));

    // アニメーション読み込み
    LoadAnimations(data, *model);

    GX_LOG_INFO("ModelLoader: Loaded model (%zu vertices, %zu indices, %zu submeshes, %u animations)",
        vertices.size(), indices.size(), subMeshes.size(), model->GetAnimationCount());

    cgltf_free(data);
    return model;
}

} // namespace GX
