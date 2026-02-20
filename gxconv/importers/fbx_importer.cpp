/// @file fbx_importer.cpp
/// @brief FBX importer implementation using ufbx

#include "fbx_importer.h"
#include "intermediate/scene.h"

#include "ufbx.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>
#include <vector>

namespace gxconv
{

static gxfmt::ShaderModel DetectShaderModelFromFbx(const ufbx_material* mat)
{
    if (!mat) return gxfmt::ShaderModel::Standard;

    // Check material name for toon/cel hints
    std::string name(mat->name.data, mat->name.length);
    for (auto& c : name) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    if (name.find("toon") != std::string::npos || name.find("cel") != std::string::npos)
        return gxfmt::ShaderModel::Toon;

    if (mat->shader_type == UFBX_SHADER_FBX_PHONG)
        return gxfmt::ShaderModel::Phong;
    if (mat->shader_type == UFBX_SHADER_FBX_LAMBERT)
        return gxfmt::ShaderModel::Phong; // low shininess

    return gxfmt::ShaderModel::Standard;
}

static std::string GetTextureFilePath(const ufbx_material_map& map)
{
    if (!map.texture || !map.texture->filename.length)
    {
        if (map.texture && map.texture->relative_filename.length)
            return std::string(map.texture->relative_filename.data, map.texture->relative_filename.length);
        return "";
    }
    std::string fullPath(map.texture->filename.data, map.texture->filename.length);
    // Strip directory — only store filename (FBX has absolute paths)
    auto slash = fullPath.find_last_of("/\\");
    if (slash != std::string::npos)
        return fullPath.substr(slash + 1);
    return fullPath;
}

static std::string UfbxStr(ufbx_string s)
{
    return std::string(s.data, s.length);
}

bool FbxImporter::Import(const std::string& filePath, Scene& outScene)
{
    ufbx_load_opts opts = {};
    opts.target_axes = ufbx_axes_left_handed_y_up;
    opts.target_unit_meters = 1.0f;
    opts.target_camera_axes = ufbx_axes_left_handed_y_up;
    opts.target_light_axes = ufbx_axes_left_handed_y_up;
    opts.space_conversion = UFBX_SPACE_CONVERSION_ADJUST_TRANSFORMS;

    ufbx_error error = {};
    ufbx_scene* scene = ufbx_load_file(filePath.c_str(), &opts, &error);
    if (!scene)
    {
        fprintf(stderr, "FBX error: %s\n", error.description.data);
        return false;
    }

    // --- Materials ---
    for (size_t mi = 0; mi < scene->materials.count; ++mi)
    {
        const ufbx_material* mat = scene->materials.data[mi];
        IntermediateMaterial dstMat;
        dstMat.name = UfbxStr(mat->name);
        dstMat.shaderModel = DetectShaderModelFromFbx(mat);
        dstMat.params = gxfmt::DefaultShaderModelParams(dstMat.shaderModel);

        // Base color
        ufbx_vec4 bc = mat->pbr.base_color.value_vec4;
        dstMat.params.baseColor[0] = static_cast<float>(bc.x);
        dstMat.params.baseColor[1] = static_cast<float>(bc.y);
        dstMat.params.baseColor[2] = static_cast<float>(bc.z);
        dstMat.params.baseColor[3] = static_cast<float>(bc.w);

        // PBR params
        dstMat.params.metallic = static_cast<float>(mat->pbr.metalness.value_real);
        dstMat.params.roughness = static_cast<float>(mat->pbr.roughness.value_real);
        if (dstMat.params.roughness <= 0.0f) dstMat.params.roughness = 0.5f;

        // Emissive
        ufbx_vec4 em = mat->pbr.emission_color.value_vec4;
        dstMat.params.emissiveFactor[0] = static_cast<float>(em.x);
        dstMat.params.emissiveFactor[1] = static_cast<float>(em.y);
        dstMat.params.emissiveFactor[2] = static_cast<float>(em.z);
        float emFactor = static_cast<float>(mat->pbr.emission_factor.value_real);
        dstMat.params.emissiveStrength = emFactor;

        // Phong shininess (from specular exponent)
        if (dstMat.shaderModel == gxfmt::ShaderModel::Phong)
        {
            ufbx_vec4 spec = mat->pbr.specular_color.value_vec4;
            dstMat.params.specularColor[0] = static_cast<float>(spec.x);
            dstMat.params.specularColor[1] = static_cast<float>(spec.y);
            dstMat.params.specularColor[2] = static_cast<float>(spec.z);
            dstMat.params.shininess = static_cast<float>(mat->fbx.specular_exponent.value_real);
            if (dstMat.params.shininess <= 0.0f) dstMat.params.shininess = 16.0f;
        }

        // Textures
        dstMat.texturePaths[0] = GetTextureFilePath(mat->pbr.base_color);
        dstMat.texturePaths[1] = GetTextureFilePath(mat->pbr.normal_map);
        dstMat.texturePaths[2] = GetTextureFilePath(mat->pbr.metalness);
        dstMat.texturePaths[3] = GetTextureFilePath(mat->pbr.ambient_occlusion);
        dstMat.texturePaths[4] = GetTextureFilePath(mat->pbr.emission_color);

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

    // --- Build skeleton (collect all bone nodes) ---
    std::map<const ufbx_node*, uint32_t> boneNodeToIndex;
    std::vector<const ufbx_node*> boneNodes;

    // Find all bones from skin deformers
    for (size_t si = 0; si < scene->skin_deformers.count; ++si)
    {
        ufbx_skin_deformer* skin = scene->skin_deformers.data[si];
        for (size_t ci = 0; ci < skin->clusters.count; ++ci)
        {
            ufbx_skin_cluster* cluster = skin->clusters.data[ci];
            if (cluster->bone_node && boneNodeToIndex.find(cluster->bone_node) == boneNodeToIndex.end())
            {
                uint32_t idx = static_cast<uint32_t>(boneNodes.size());
                boneNodeToIndex[cluster->bone_node] = idx;
                boneNodes.push_back(cluster->bone_node);
            }
        }
    }

    // Build skeleton joints
    if (!boneNodes.empty())
    {
        outScene.hasSkeleton = true;
        outScene.skeleton.resize(boneNodes.size());

        for (size_t i = 0; i < boneNodes.size(); ++i)
        {
            const ufbx_node* node = boneNodes[i];
            auto& joint = outScene.skeleton[i];
            joint.name = UfbxStr(node->name);

            // Find parent
            joint.parentIndex = -1;
            if (node->parent)
            {
                auto it = boneNodeToIndex.find(node->parent);
                if (it != boneNodeToIndex.end())
                    joint.parentIndex = static_cast<int32_t>(it->second);
            }

            // Local transform (TRS)
            if (joint.parentIndex == -1)
            {
                // Root bone: use node_to_world to include non-bone ancestor
                // (e.g. Armature) axis/unit conversions from ADJUST_TRANSFORMS
                ufbx_transform worldXf = ufbx_matrix_to_transform(&node->node_to_world);
                joint.localTranslation[0] = static_cast<float>(worldXf.translation.x);
                joint.localTranslation[1] = static_cast<float>(worldXf.translation.y);
                joint.localTranslation[2] = static_cast<float>(worldXf.translation.z);
                joint.localRotation[0] = static_cast<float>(worldXf.rotation.x);
                joint.localRotation[1] = static_cast<float>(worldXf.rotation.y);
                joint.localRotation[2] = static_cast<float>(worldXf.rotation.z);
                joint.localRotation[3] = static_cast<float>(worldXf.rotation.w);
                joint.localScale[0] = static_cast<float>(worldXf.scale.x);
                joint.localScale[1] = static_cast<float>(worldXf.scale.y);
                joint.localScale[2] = static_cast<float>(worldXf.scale.z);
            }
            else
            {
                // Child bone: local transform relative to parent bone
                joint.localTranslation[0] = static_cast<float>(node->local_transform.translation.x);
                joint.localTranslation[1] = static_cast<float>(node->local_transform.translation.y);
                joint.localTranslation[2] = static_cast<float>(node->local_transform.translation.z);
                joint.localRotation[0] = static_cast<float>(node->local_transform.rotation.x);
                joint.localRotation[1] = static_cast<float>(node->local_transform.rotation.y);
                joint.localRotation[2] = static_cast<float>(node->local_transform.rotation.z);
                joint.localRotation[3] = static_cast<float>(node->local_transform.rotation.w);
                joint.localScale[0] = static_cast<float>(node->local_transform.scale.x);
                joint.localScale[1] = static_cast<float>(node->local_transform.scale.y);
                joint.localScale[2] = static_cast<float>(node->local_transform.scale.z);
            }
        }

        // Set inverse bind matrices from skin clusters
        for (size_t si = 0; si < scene->skin_deformers.count; ++si)
        {
            ufbx_skin_deformer* skin = scene->skin_deformers.data[si];
            for (size_t ci = 0; ci < skin->clusters.count; ++ci)
            {
                ufbx_skin_cluster* cluster = skin->clusters.data[ci];
                if (!cluster->bone_node) continue;
                auto it = boneNodeToIndex.find(cluster->bone_node);
                if (it == boneNodeToIndex.end()) continue;

                auto& joint = outScene.skeleton[it->second];
                // ufbx geometry_to_bone is already the inverse bind matrix
                const ufbx_matrix& m = cluster->geometry_to_bone;
                // Store as row-major 4x4 (row-vector convention for DirectXMath)
                // ufbx cols are column vectors → transpose to row-vector layout
                joint.inverseBindMatrix[0]  = static_cast<float>(m.cols[0].x);  // m00
                joint.inverseBindMatrix[1]  = static_cast<float>(m.cols[0].y);  // m10
                joint.inverseBindMatrix[2]  = static_cast<float>(m.cols[0].z);  // m20
                joint.inverseBindMatrix[3]  = 0.0f;
                joint.inverseBindMatrix[4]  = static_cast<float>(m.cols[1].x);  // m01
                joint.inverseBindMatrix[5]  = static_cast<float>(m.cols[1].y);  // m11
                joint.inverseBindMatrix[6]  = static_cast<float>(m.cols[1].z);  // m21
                joint.inverseBindMatrix[7]  = 0.0f;
                joint.inverseBindMatrix[8]  = static_cast<float>(m.cols[2].x);  // m02
                joint.inverseBindMatrix[9]  = static_cast<float>(m.cols[2].y);  // m12
                joint.inverseBindMatrix[10] = static_cast<float>(m.cols[2].z);  // m22
                joint.inverseBindMatrix[11] = 0.0f;
                joint.inverseBindMatrix[12] = static_cast<float>(m.cols[3].x);  // tx
                joint.inverseBindMatrix[13] = static_cast<float>(m.cols[3].y);  // ty
                joint.inverseBindMatrix[14] = static_cast<float>(m.cols[3].z);  // tz
                joint.inverseBindMatrix[15] = 1.0f;
            }
        }
    }

    // --- Meshes ---
    for (size_t ni = 0; ni < scene->nodes.count; ++ni)
    {
        const ufbx_node* node = scene->nodes.data[ni];
        if (!node->mesh) continue;
        const ufbx_mesh* mesh = node->mesh;

        // Determine skin weights for this mesh
        struct JointIndices { uint32_t v[4]; };
        struct JointWeights { float v[4]; };
        std::vector<JointIndices> vertJoints(mesh->num_vertices, {{0,0,0,0}});
        std::vector<JointWeights> vertWeights(mesh->num_vertices, {{0,0,0,0}});
        bool meshHasSkin = false;

        if (mesh->skin_deformers.count > 0)
        {
            ufbx_skin_deformer* skin = mesh->skin_deformers.data[0];
            meshHasSkin = true;

            // Build per-vertex influence list
            struct Influence { uint32_t joint; float weight; };
            std::vector<std::vector<Influence>> influences(mesh->num_vertices);

            for (size_t ci = 0; ci < skin->clusters.count; ++ci)
            {
                ufbx_skin_cluster* cluster = skin->clusters.data[ci];
                if (!cluster->bone_node) continue;
                auto it = boneNodeToIndex.find(cluster->bone_node);
                if (it == boneNodeToIndex.end()) continue;

                uint32_t jointIdx = it->second;
                for (size_t vi = 0; vi < cluster->num_weights; ++vi)
                {
                    uint32_t vertIdx = cluster->vertices.data[vi];
                    float w = static_cast<float>(cluster->weights.data[vi]);
                    if (vertIdx < mesh->num_vertices && w > 0.0f)
                        influences[vertIdx].push_back({jointIdx, w});
                }
            }

            // Keep top 4 influences per vertex, normalize
            for (size_t v = 0; v < mesh->num_vertices; ++v)
            {
                auto& inf = influences[v];
                std::sort(inf.begin(), inf.end(), [](auto& a, auto& b) { return a.weight > b.weight; });

                float totalW = 0.0f;
                int count = static_cast<int>(std::min(inf.size(), size_t(4)));
                for (int i = 0; i < count; ++i)
                {
                    vertJoints[v].v[i] = inf[i].joint;
                    vertWeights[v].v[i] = inf[i].weight;
                    totalW += inf[i].weight;
                }
                if (totalW > 0.0f)
                    for (int i = 0; i < 4; ++i) vertWeights[v].v[i] /= totalW;
            }
        }

        // Group faces by material
        std::map<uint32_t, std::vector<uint32_t>> matFaces; // matIdx -> list of face indices
        for (uint32_t fi = 0; fi < mesh->num_faces; ++fi)
        {
            uint32_t matIdx = 0;
            if (mesh->face_material.count > fi)
                matIdx = mesh->face_material.data[fi];
            matFaces[matIdx].push_back(fi);
        }

        for (auto& [matIdx, faceList] : matFaces)
        {
            IntermediateMesh dstMesh;
            dstMesh.name = UfbxStr(node->name);
            if (matFaces.size() > 1)
                dstMesh.name += "_mat" + std::to_string(matIdx);
            dstMesh.materialIndex = matIdx;
            dstMesh.hasSkinning = meshHasSkin;

            // Build per-face vertices
            std::map<std::tuple<uint32_t, uint32_t, uint32_t>, uint32_t> vertexMap;

            for (uint32_t fi : faceList)
            {
                ufbx_face face = mesh->faces.data[fi];
                // Triangulate (fan triangulation for convex polygons)
                uint32_t numTriangles = face.num_indices >= 3 ? face.num_indices - 2 : 0;

                for (uint32_t t = 0; t < numTriangles; ++t)
                {
                    // Fan triangulation: (0, t+1, t+2)
                    uint32_t triIndices[3] = {
                        face.index_begin,
                        face.index_begin + t + 1,
                        face.index_begin + t + 2
                    };

                    // Swap winding for left-handed coordinate system
                    std::swap(triIndices[1], triIndices[2]);

                    for (int vi = 0; vi < 3; ++vi)
                    {
                        uint32_t idx = triIndices[vi];
                        uint32_t posIdx = mesh->vertex_indices.data[idx];

                        // Normal index
                        uint32_t normIdx = 0;
                        if (mesh->vertex_normal.exists)
                            normIdx = mesh->vertex_normal.indices.data[idx];

                        // UV index
                        uint32_t uvIdx = 0;
                        if (mesh->vertex_uv.exists)
                            uvIdx = mesh->vertex_uv.indices.data[idx];

                        auto key = std::make_tuple(posIdx, normIdx, uvIdx);
                        auto it = vertexMap.find(key);
                        if (it != vertexMap.end())
                        {
                            dstMesh.indices.push_back(it->second);
                            continue;
                        }

                        IntermediateVertex vert{};

                        ufbx_vec3 pos = mesh->vertex_position.values.data[posIdx];
                        vert.position[0] = static_cast<float>(pos.x);
                        vert.position[1] = static_cast<float>(pos.y);
                        vert.position[2] = static_cast<float>(pos.z);

                        if (mesh->vertex_normal.exists)
                        {
                            ufbx_vec3 n = mesh->vertex_normal.values.data[normIdx];
                            vert.normal[0] = static_cast<float>(n.x);
                            vert.normal[1] = static_cast<float>(n.y);
                            vert.normal[2] = static_cast<float>(n.z);
                        }

                        if (mesh->vertex_uv.exists)
                        {
                            ufbx_vec2 uv = mesh->vertex_uv.values.data[uvIdx];
                            vert.texcoord[0] = static_cast<float>(uv.x);
                            vert.texcoord[1] = 1.0f - static_cast<float>(uv.y);
                        }

                        if (meshHasSkin)
                        {
                            std::memcpy(vert.joints, vertJoints[posIdx].v, 16);
                            std::memcpy(vert.weights, vertWeights[posIdx].v, 16);
                        }

                        uint32_t newIdx = static_cast<uint32_t>(dstMesh.vertices.size());
                        vertexMap[key] = newIdx;
                        dstMesh.vertices.push_back(vert);
                        dstMesh.indices.push_back(newIdx);
                    }
                }
            }

            ComputeTangents(dstMesh);
            outScene.meshes.push_back(std::move(dstMesh));
        }
    }

    // --- Animations ---
    for (size_t ai = 0; ai < scene->anim_stacks.count; ++ai)
    {
        const ufbx_anim_stack* stack = scene->anim_stacks.data[ai];
        IntermediateAnimation anim;
        anim.name = UfbxStr(stack->name);
        anim.duration = static_cast<float>(stack->time_end - stack->time_begin);
        if (anim.duration <= 0.0f) anim.duration = 1.0f;

        // Sample each bone at regular intervals
        float sampleRate = 30.0f; // 30 fps
        uint32_t numSamples = static_cast<uint32_t>(anim.duration * sampleRate) + 1;
        if (numSamples < 2) numSamples = 2;

        for (auto& [node, jointIdx] : boneNodeToIndex)
        {
            // Determine if this is a root bone (no bone ancestor in skeleton)
            bool isRootBone = true;
            if (node->parent)
            {
                if (boneNodeToIndex.find(node->parent) != boneNodeToIndex.end())
                    isRootBone = false;
            }

            // For root bones, pre-fetch parent's world matrix (static during animation)
            ufbx_matrix parentWorld = {};
            if (isRootBone && node->parent)
                parentWorld = node->parent->node_to_world;

            // Translation channel
            {
                IntermediateAnimChannel ch;
                ch.jointIndex = jointIdx;
                ch.boneName = UfbxStr(node->name);
                ch.target = 0; // Translation
                ch.interpolation = 0; // Linear

                for (uint32_t s = 0; s < numSamples; ++s)
                {
                    double t = stack->time_begin + (double)s / sampleRate;
                    if (t > stack->time_end) t = stack->time_end;

                    ufbx_transform xf = ufbx_evaluate_transform(stack->anim, node, t);
                    if (isRootBone && node->parent)
                    {
                        // Convert local to world: parent_world * local_matrix
                        ufbx_matrix localMat = ufbx_transform_to_matrix(&xf);
                        ufbx_matrix worldMat = ufbx_matrix_mul(&parentWorld, &localMat);
                        xf = ufbx_matrix_to_transform(&worldMat);
                    }
                    IntermediateKeyframeVec3 kf;
                    kf.time = static_cast<float>(t - stack->time_begin);
                    kf.value[0] = static_cast<float>(xf.translation.x);
                    kf.value[1] = static_cast<float>(xf.translation.y);
                    kf.value[2] = static_cast<float>(xf.translation.z);
                    ch.vecKeys.push_back(kf);
                }
                anim.channels.push_back(std::move(ch));
            }

            // Rotation channel
            {
                IntermediateAnimChannel ch;
                ch.jointIndex = jointIdx;
                ch.boneName = UfbxStr(node->name);
                ch.target = 1; // Rotation
                ch.interpolation = 0;

                for (uint32_t s = 0; s < numSamples; ++s)
                {
                    double t = stack->time_begin + (double)s / sampleRate;
                    if (t > stack->time_end) t = stack->time_end;

                    ufbx_transform xf = ufbx_evaluate_transform(stack->anim, node, t);
                    if (isRootBone && node->parent)
                    {
                        ufbx_matrix localMat = ufbx_transform_to_matrix(&xf);
                        ufbx_matrix worldMat = ufbx_matrix_mul(&parentWorld, &localMat);
                        xf = ufbx_matrix_to_transform(&worldMat);
                    }
                    IntermediateKeyframeQuat kf;
                    kf.time = static_cast<float>(t - stack->time_begin);
                    kf.value[0] = static_cast<float>(xf.rotation.x);
                    kf.value[1] = static_cast<float>(xf.rotation.y);
                    kf.value[2] = static_cast<float>(xf.rotation.z);
                    kf.value[3] = static_cast<float>(xf.rotation.w);
                    ch.quatKeys.push_back(kf);
                }
                anim.channels.push_back(std::move(ch));
            }

            // Scale channel
            {
                IntermediateAnimChannel ch;
                ch.jointIndex = jointIdx;
                ch.boneName = UfbxStr(node->name);
                ch.target = 2; // Scale
                ch.interpolation = 0;

                for (uint32_t s = 0; s < numSamples; ++s)
                {
                    double t = stack->time_begin + (double)s / sampleRate;
                    if (t > stack->time_end) t = stack->time_end;

                    ufbx_transform xf = ufbx_evaluate_transform(stack->anim, node, t);
                    if (isRootBone && node->parent)
                    {
                        ufbx_matrix localMat = ufbx_transform_to_matrix(&xf);
                        ufbx_matrix worldMat = ufbx_matrix_mul(&parentWorld, &localMat);
                        xf = ufbx_matrix_to_transform(&worldMat);
                    }
                    IntermediateKeyframeVec3 kf;
                    kf.time = static_cast<float>(t - stack->time_begin);
                    kf.value[0] = static_cast<float>(xf.scale.x);
                    kf.value[1] = static_cast<float>(xf.scale.y);
                    kf.value[2] = static_cast<float>(xf.scale.z);
                    ch.vecKeys.push_back(kf);
                }
                anim.channels.push_back(std::move(ch));
            }
        }

        if (!anim.channels.empty())
            outScene.animations.push_back(std::move(anim));
    }

    ufbx_free_scene(scene);
    return true;
}

} // namespace gxconv
