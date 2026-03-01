/// @file gltf_importer.cpp
/// @brief glTF/GLBインポーターの実装 (cgltf使用)

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "gltf_importer.h"
#include "intermediate/scene.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>
#include <vector>
#include <functional>

namespace gxconv
{

static std::string GetDirectory(const std::string& path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return ".";
    return path.substr(0, pos);
}

static std::string ResolveUri(const std::string& baseDir, const char* uri)
{
    if (!uri || !uri[0]) return "";
    if (uri[0] == '/' || (uri[1] == ':')) return uri; // absolute
    return baseDir + "/" + uri;
}

// --- cgltfアクセサヘルパー ---

/// アクセサから指定要素の指定コンポーネントをfloatで読み込む
static float ReadFloat(const cgltf_accessor* acc, cgltf_size index, cgltf_size component)
{
    float val = 0.0f;
    cgltf_accessor_read_float(acc, index, &val + 0, 1);
    // 全コンポーネントを一括読みして該当コンポーネントを返す
    float buf[16] = {};
    cgltf_accessor_read_float(acc, index, buf, acc->type == cgltf_type_scalar ? 1 :
        (acc->type == cgltf_type_vec2 ? 2 : (acc->type == cgltf_type_vec3 ? 3 :
        (acc->type == cgltf_type_vec4 ? 4 : (acc->type == cgltf_type_mat4 ? 16 : 1)))));
    return buf[component];
}

/// アクセサからn個のfloatを読み込む
static void ReadFloatN(const cgltf_accessor* acc, cgltf_size index, float* out, int n)
{
    cgltf_accessor_read_float(acc, index, out, n);
}

/// アクセサから1つのuint32を読み込む
static uint32_t ReadUint(const cgltf_accessor* acc, cgltf_size index)
{
    cgltf_uint val = 0;
    cgltf_accessor_read_uint(acc, index, &val, 1);
    return val;
}

/// アクセサからn個のuint32を読み込む
static void ReadUintN(const cgltf_accessor* acc, cgltf_size index, uint32_t* out, int n)
{
    cgltf_uint buf[4] = {};
    cgltf_accessor_read_uint(acc, index, buf, n);
    for (int i = 0; i < n; ++i) out[i] = buf[i];
}

/// glTFマテリアルからシェーダーモデルを推定する
static gxfmt::ShaderModel DetectShaderModelFromGltf(const cgltf_material* mat)
{
    if (!mat) return gxfmt::ShaderModel::Standard;
    if (mat->unlit) return gxfmt::ShaderModel::Unlit;
    if (mat->has_pbr_metallic_roughness) return gxfmt::ShaderModel::Standard;
    return gxfmt::ShaderModel::Standard;
}

bool GltfImporter::Import(const std::string& filePath, Scene& outScene)
{
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, filePath.c_str(), &data);
    if (result != cgltf_result_success)
    {
        fprintf(stderr, "glTF error: failed to parse %s\n", filePath.c_str());
        return false;
    }

    result = cgltf_load_buffers(&options, data, filePath.c_str());
    if (result != cgltf_result_success)
    {
        fprintf(stderr, "glTF error: failed to load buffers\n");
        cgltf_free(data);
        return false;
    }

    std::string baseDir = GetDirectory(filePath);

    // スケルトン用: ノード→ジョイントインデックスのマップ
    std::map<const cgltf_node*, uint32_t> nodeToJointIndex;

    // --- Materials ---
    for (cgltf_size mi = 0; mi < data->materials_count; ++mi)
    {
        const cgltf_material* mat = &data->materials[mi];
        IntermediateMaterial dstMat;
        dstMat.name = mat->name ? mat->name : "Material_" + std::to_string(mi);
        dstMat.shaderModel = DetectShaderModelFromGltf(mat);
        dstMat.params = gxfmt::DefaultShaderModelParams(dstMat.shaderModel);

        if (mat->has_pbr_metallic_roughness)
        {
            const auto& pbr = mat->pbr_metallic_roughness;
            dstMat.params.baseColor[0] = pbr.base_color_factor[0];
            dstMat.params.baseColor[1] = pbr.base_color_factor[1];
            dstMat.params.baseColor[2] = pbr.base_color_factor[2];
            dstMat.params.baseColor[3] = pbr.base_color_factor[3];
            dstMat.params.metallic = pbr.metallic_factor;
            dstMat.params.roughness = pbr.roughness_factor;

            if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image)
                dstMat.texturePaths[0] = ResolveUri(baseDir, pbr.base_color_texture.texture->image->uri);
            if (pbr.metallic_roughness_texture.texture && pbr.metallic_roughness_texture.texture->image)
                dstMat.texturePaths[2] = ResolveUri(baseDir, pbr.metallic_roughness_texture.texture->image->uri);
        }

        if (mat->normal_texture.texture && mat->normal_texture.texture->image)
        {
            dstMat.texturePaths[1] = ResolveUri(baseDir, mat->normal_texture.texture->image->uri);
            dstMat.params.normalScale = mat->normal_texture.scale;
        }

        if (mat->occlusion_texture.texture && mat->occlusion_texture.texture->image)
        {
            dstMat.texturePaths[3] = ResolveUri(baseDir, mat->occlusion_texture.texture->image->uri);
            dstMat.params.aoStrength = mat->occlusion_texture.scale;
        }

        if (mat->emissive_texture.texture && mat->emissive_texture.texture->image)
            dstMat.texturePaths[4] = ResolveUri(baseDir, mat->emissive_texture.texture->image->uri);

        dstMat.params.emissiveFactor[0] = mat->emissive_factor[0];
        dstMat.params.emissiveFactor[1] = mat->emissive_factor[1];
        dstMat.params.emissiveFactor[2] = mat->emissive_factor[2];
        if (mat->emissive_factor[0] > 0 || mat->emissive_factor[1] > 0 || mat->emissive_factor[2] > 0)
            dstMat.params.emissiveStrength = 1.0f;

        // Alpha mode
        if (mat->alpha_mode == cgltf_alpha_mode_mask)
        {
            dstMat.params.alphaMode = gxfmt::AlphaMode::Mask;
            dstMat.params.alphaCutoff = mat->alpha_cutoff;
        }
        else if (mat->alpha_mode == cgltf_alpha_mode_blend)
        {
            dstMat.params.alphaMode = gxfmt::AlphaMode::Blend;
        }

        dstMat.params.doubleSided = mat->double_sided ? 1 : 0;

        outScene.materials.push_back(std::move(dstMat));
    }

    if (outScene.materials.empty())
    {
        IntermediateMaterial defaultMat;
        defaultMat.name = "Default";
        defaultMat.shaderModel = gxfmt::ShaderModel::Standard;
        defaultMat.params = gxfmt::DefaultShaderModelParams(gxfmt::ShaderModel::Standard);
        outScene.materials.push_back(std::move(defaultMat));
    }

    // --- Skeleton ---
    if (data->skins_count > 0)
    {
        const cgltf_skin* skin = &data->skins[0];
        outScene.hasSkeleton = true;
        outScene.skeleton.resize(skin->joints_count);

        for (cgltf_size ji = 0; ji < skin->joints_count; ++ji)
        {
            const cgltf_node* node = skin->joints[ji];
            nodeToJointIndex[node] = static_cast<uint32_t>(ji);
        }

        for (cgltf_size ji = 0; ji < skin->joints_count; ++ji)
        {
            const cgltf_node* node = skin->joints[ji];
            auto& joint = outScene.skeleton[ji];
            joint.name = node->name ? node->name : "Joint_" + std::to_string(ji);

            // Parent index
            joint.parentIndex = -1;
            if (node->parent)
            {
                auto it = nodeToJointIndex.find(node->parent);
                if (it != nodeToJointIndex.end())
                    joint.parentIndex = static_cast<int32_t>(it->second);
            }

            // 逆バインド行列: cgltfは列メジャー → 行メジャーに転置
            if (skin->inverse_bind_matrices)
            {
                float mat[16];
                ReadFloatN(skin->inverse_bind_matrices, ji, mat, 16);
                for (int r = 0; r < 4; ++r)
                    for (int c = 0; c < 4; ++c)
                        joint.inverseBindMatrix[r * 4 + c] = mat[c * 4 + r];
            }

            // Local transform
            if (node->has_translation)
            {
                joint.localTranslation[0] = node->translation[0];
                joint.localTranslation[1] = node->translation[1];
                joint.localTranslation[2] = node->translation[2];
            }
            if (node->has_rotation)
            {
                joint.localRotation[0] = node->rotation[0];
                joint.localRotation[1] = node->rotation[1];
                joint.localRotation[2] = node->rotation[2];
                joint.localRotation[3] = node->rotation[3];
            }
            if (node->has_scale)
            {
                joint.localScale[0] = node->scale[0];
                joint.localScale[1] = node->scale[1];
                joint.localScale[2] = node->scale[2];
            }
        }
    }

    // --- Meshes ---
    for (cgltf_size ni = 0; ni < data->nodes_count; ++ni)
    {
        const cgltf_node* node = &data->nodes[ni];
        if (!node->mesh) continue;
        const cgltf_mesh* mesh = node->mesh;

        for (cgltf_size pi = 0; pi < mesh->primitives_count; ++pi)
        {
            const cgltf_primitive* prim = &mesh->primitives[pi];
            if (prim->type != cgltf_primitive_type_triangles) continue;

            IntermediateMesh dstMesh;
            dstMesh.name = mesh->name ? mesh->name : "Mesh_" + std::to_string(ni);
            if (mesh->primitives_count > 1)
                dstMesh.name += "_prim" + std::to_string(pi);

            // Material index
            if (prim->material)
                dstMesh.materialIndex = static_cast<uint32_t>(prim->material - data->materials);

            // プリミティブの頂点属性アクセサを検索
            const cgltf_accessor* posAcc = nullptr;
            const cgltf_accessor* normAcc = nullptr;
            const cgltf_accessor* uvAcc = nullptr;
            const cgltf_accessor* tanAcc = nullptr;
            const cgltf_accessor* jointsAcc = nullptr;
            const cgltf_accessor* weightsAcc = nullptr;

            for (cgltf_size ai = 0; ai < prim->attributes_count; ++ai)
            {
                const cgltf_attribute* attr = &prim->attributes[ai];
                if (attr->type == cgltf_attribute_type_position)   posAcc = attr->data;
                else if (attr->type == cgltf_attribute_type_normal)   normAcc = attr->data;
                else if (attr->type == cgltf_attribute_type_texcoord && attr->index == 0) uvAcc = attr->data;
                else if (attr->type == cgltf_attribute_type_tangent)  tanAcc = attr->data;
                else if (attr->type == cgltf_attribute_type_joints && attr->index == 0)   jointsAcc = attr->data;
                else if (attr->type == cgltf_attribute_type_weights && attr->index == 0)  weightsAcc = attr->data;
            }

            if (!posAcc) continue;

            dstMesh.hasSkinning = (jointsAcc != nullptr && weightsAcc != nullptr);

            // Read vertices
            dstMesh.vertices.resize(posAcc->count);
            for (cgltf_size vi = 0; vi < posAcc->count; ++vi)
            {
                auto& v = dstMesh.vertices[vi];
                ReadFloatN(posAcc, vi, v.position, 3);
                if (normAcc) ReadFloatN(normAcc, vi, v.normal, 3);
                if (uvAcc) ReadFloatN(uvAcc, vi, v.texcoord, 2);
                if (tanAcc) ReadFloatN(tanAcc, vi, v.tangent, 4);
                if (jointsAcc) ReadUintN(jointsAcc, vi, v.joints, 4);
                if (weightsAcc) ReadFloatN(weightsAcc, vi, v.weights, 4);
            }

            // Read indices
            if (prim->indices)
            {
                dstMesh.indices.resize(prim->indices->count);
                for (cgltf_size ii = 0; ii < prim->indices->count; ++ii)
                    dstMesh.indices[ii] = static_cast<uint32_t>(cgltf_accessor_read_index(prim->indices, ii));
            }
            else
            {
                // Non-indexed: generate sequential indices
                dstMesh.indices.resize(posAcc->count);
                for (uint32_t ii = 0; ii < posAcc->count; ++ii)
                    dstMesh.indices[ii] = ii;
            }

            // Compute tangents if not provided
            if (!tanAcc)
                ComputeTangents(dstMesh);

            outScene.meshes.push_back(std::move(dstMesh));
        }
    }

    // --- Animations ---
    for (cgltf_size ai = 0; ai < data->animations_count; ++ai)
    {
        const cgltf_animation* anim = &data->animations[ai];
        IntermediateAnimation dstAnim;
        dstAnim.name = anim->name ? anim->name : "Animation_" + std::to_string(ai);
        dstAnim.duration = 0.0f;

        for (cgltf_size ci = 0; ci < anim->channels_count; ++ci)
        {
            const cgltf_animation_channel* ch = &anim->channels[ci];
            if (!ch->target_node || !ch->sampler) continue;

            // Find joint index
            auto it = nodeToJointIndex.find(ch->target_node);
            if (it == nodeToJointIndex.end()) continue;

            const cgltf_animation_sampler* sampler = ch->sampler;

            IntermediateAnimChannel dstCh;
            dstCh.jointIndex = it->second;
            dstCh.boneName = ch->target_node->name ? ch->target_node->name : "";

            // Interpolation
            if (sampler->interpolation == cgltf_interpolation_type_step)
                dstCh.interpolation = 1;
            else if (sampler->interpolation == cgltf_interpolation_type_cubic_spline)
                dstCh.interpolation = 2;

            // Target
            if (ch->target_path == cgltf_animation_path_type_translation)
            {
                dstCh.target = 0;
                for (cgltf_size ki = 0; ki < sampler->input->count; ++ki)
                {
                    IntermediateKeyframeVec3 kf;
                    ReadFloatN(sampler->input, ki, &kf.time, 1);
                    ReadFloatN(sampler->output, ki, kf.value, 3);
                    dstCh.vecKeys.push_back(kf);
                    if (kf.time > dstAnim.duration) dstAnim.duration = kf.time;
                }
            }
            else if (ch->target_path == cgltf_animation_path_type_rotation)
            {
                dstCh.target = 1;
                for (cgltf_size ki = 0; ki < sampler->input->count; ++ki)
                {
                    IntermediateKeyframeQuat kf;
                    ReadFloatN(sampler->input, ki, &kf.time, 1);
                    ReadFloatN(sampler->output, ki, kf.value, 4);
                    dstCh.quatKeys.push_back(kf);
                    if (kf.time > dstAnim.duration) dstAnim.duration = kf.time;
                }
            }
            else if (ch->target_path == cgltf_animation_path_type_scale)
            {
                dstCh.target = 2;
                for (cgltf_size ki = 0; ki < sampler->input->count; ++ki)
                {
                    IntermediateKeyframeVec3 kf;
                    ReadFloatN(sampler->input, ki, &kf.time, 1);
                    ReadFloatN(sampler->output, ki, kf.value, 3);
                    dstCh.vecKeys.push_back(kf);
                    if (kf.time > dstAnim.duration) dstAnim.duration = kf.time;
                }
            }
            else
            {
                continue;
            }

            anim->channels_count; // suppress
            dstAnim.channels.push_back(std::move(dstCh));
        }

        if (!dstAnim.channels.empty())
            outScene.animations.push_back(std::move(dstAnim));
    }

    cgltf_free(data);
    return true;
}

} // namespace gxconv
