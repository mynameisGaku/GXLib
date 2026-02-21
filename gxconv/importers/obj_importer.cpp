/// @file obj_importer.cpp
/// @brief OBJ/MTLインポーターの実装

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

#include "obj_importer.h"
#include "intermediate/scene.h"

#include <cstdio>
#include <map>
#include <algorithm>

namespace gxconv
{

/// MTLの照明モデル(illum)とPBRプロパティからシェーダーモデルを推定する
static gxfmt::ShaderModel DetectShaderModel(const tinyobj::material_t& mat)
{
    // PBR拡張があればStandard (metallic-roughness)
    if (mat.roughness > 0.0f || mat.metallic > 0.0f)
        return gxfmt::ShaderModel::Standard;

    // illumモデルでの判定: 0=Unlit, 1-2=Phong系
    switch (mat.illum)
    {
    case 0:
        return gxfmt::ShaderModel::Unlit;
    case 1:
    case 2:
        return gxfmt::ShaderModel::Phong;
    default:
        return gxfmt::ShaderModel::Standard;
    }
}

/// テクスチャパスをOBJファイルディレクトリからの相対パスとして解決する
static std::string ResolveTexturePath(const std::string& objDir, const std::string& texName)
{
    if (texName.empty()) return "";
    // If already absolute, return as-is
    if (texName.size() > 1 && (texName[1] == ':' || texName[0] == '/'))
        return texName;
    return objDir + "/" + texName;
}

static std::string GetDirectory(const std::string& filePath)
{
    size_t pos = filePath.find_last_of("/\\");
    if (pos == std::string::npos) return ".";
    return filePath.substr(0, pos);
}

bool ObjImporter::Import(const std::string& filePath, Scene& outScene)
{
    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config;
    config.triangulate = true;
    config.vertex_color = false;

    if (!reader.ParseFromFile(filePath, config))
    {
        if (!reader.Error().empty())
            fprintf(stderr, "OBJ error: %s\n", reader.Error().c_str());
        return false;
    }

    if (!reader.Warning().empty())
        fprintf(stderr, "OBJ warning: %s\n", reader.Warning().c_str());

    const auto& attrib = reader.GetAttrib();
    const auto& shapes = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    std::string objDir = GetDirectory(filePath);

    // Convert materials
    for (size_t i = 0; i < materials.size(); ++i)
    {
        const auto& srcMat = materials[i];
        IntermediateMaterial dstMat;
        dstMat.name = srcMat.name;
        dstMat.shaderModel = DetectShaderModel(srcMat);
        dstMat.params = gxfmt::DefaultShaderModelParams(dstMat.shaderModel);

        // ベースカラー: MTLのdiffuse色を使用
        dstMat.params.baseColor[0] = srcMat.diffuse[0];
        dstMat.params.baseColor[1] = srcMat.diffuse[1];
        dstMat.params.baseColor[2] = srcMat.diffuse[2];
        dstMat.params.baseColor[3] = 1.0f - srcMat.dissolve;
        if (srcMat.dissolve < 1.0f)
            dstMat.params.baseColor[3] = srcMat.dissolve; // dissolve = 不透明度

        // Alpha mode
        if (srcMat.dissolve < 1.0f)
            dstMat.params.alphaMode = gxfmt::AlphaMode::Blend;

        // Emissive
        dstMat.params.emissiveFactor[0] = srcMat.emission[0];
        dstMat.params.emissiveFactor[1] = srcMat.emission[1];
        dstMat.params.emissiveFactor[2] = srcMat.emission[2];
        if (srcMat.emission[0] > 0 || srcMat.emission[1] > 0 || srcMat.emission[2] > 0)
            dstMat.params.emissiveStrength = 1.0f;

        // Shader-specific params
        if (dstMat.shaderModel == gxfmt::ShaderModel::Phong)
        {
            dstMat.params.specularColor[0] = srcMat.specular[0];
            dstMat.params.specularColor[1] = srcMat.specular[1];
            dstMat.params.specularColor[2] = srcMat.specular[2];
            dstMat.params.shininess = srcMat.shininess;
        }
        else if (dstMat.shaderModel == gxfmt::ShaderModel::Standard)
        {
            dstMat.params.metallic = srcMat.metallic;
            dstMat.params.roughness = srcMat.roughness > 0.0f ? srcMat.roughness : 0.5f;
        }

        // Textures
        dstMat.texturePaths[0] = ResolveTexturePath(objDir, srcMat.diffuse_texname);
        dstMat.texturePaths[1] = ResolveTexturePath(objDir, srcMat.normal_texname.empty()
            ? srcMat.bump_texname : srcMat.normal_texname);
        dstMat.texturePaths[2] = ResolveTexturePath(objDir, srcMat.specular_texname);
        // slot 3 = AO (ambient_texname)
        dstMat.texturePaths[3] = ResolveTexturePath(objDir, srcMat.ambient_texname);

        outScene.materials.push_back(std::move(dstMat));
    }

    // Default material if none
    if (outScene.materials.empty())
    {
        IntermediateMaterial defaultMat;
        defaultMat.name = "Default";
        defaultMat.shaderModel = gxfmt::ShaderModel::Standard;
        defaultMat.params = gxfmt::DefaultShaderModelParams(gxfmt::ShaderModel::Standard);
        outScene.materials.push_back(std::move(defaultMat));
    }

    // 各shapeをマテリアル別にサブメッシュに分割して変換
    for (size_t si = 0; si < shapes.size(); ++si)
    {
        const auto& shape = shapes[si];

        // Group faces by material index
        std::map<int, std::vector<size_t>> matFaces; // materialId -> face indices
        for (size_t fi = 0; fi < shape.mesh.num_face_vertices.size(); ++fi)
        {
            int matId = shape.mesh.material_ids[fi];
            if (matId < 0) matId = 0;
            matFaces[matId].push_back(fi);
        }

        for (auto& [matId, faceIndices] : matFaces)
        {
            IntermediateMesh mesh;
            mesh.name = shape.name;
            if (matFaces.size() > 1)
                mesh.name += "_mat" + std::to_string(matId);
            mesh.materialIndex = static_cast<uint32_t>(matId);

            // 頂点重複排除マップ: (位置idx, 法線idx, UVidx) → 出力頂点インデックス
            std::map<std::tuple<int,int,int>, uint32_t> vertexMap;

            for (size_t fi : faceIndices)
            {
                size_t fv = shape.mesh.num_face_vertices[fi];
                size_t indexOffset = 0;
                for (size_t f = 0; f < fi; ++f)
                    indexOffset += shape.mesh.num_face_vertices[f];

                for (size_t v = 0; v < fv; ++v)
                {
                    tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                    auto key = std::make_tuple(idx.vertex_index, idx.normal_index, idx.texcoord_index);

                    auto it = vertexMap.find(key);
                    if (it != vertexMap.end())
                    {
                        mesh.indices.push_back(it->second);
                        continue;
                    }

                    IntermediateVertex vert{};

                    if (idx.vertex_index >= 0)
                    {
                        vert.position[0] = attrib.vertices[3 * idx.vertex_index + 0];
                        vert.position[1] = attrib.vertices[3 * idx.vertex_index + 1];
                        vert.position[2] = attrib.vertices[3 * idx.vertex_index + 2];
                    }

                    if (idx.normal_index >= 0)
                    {
                        vert.normal[0] = attrib.normals[3 * idx.normal_index + 0];
                        vert.normal[1] = attrib.normals[3 * idx.normal_index + 1];
                        vert.normal[2] = attrib.normals[3 * idx.normal_index + 2];
                    }

                    if (idx.texcoord_index >= 0)
                    {
                        vert.texcoord[0] = attrib.texcoords[2 * idx.texcoord_index + 0];
                        vert.texcoord[1] = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]; // V座標を反転 (OBJ→DX)
                    }

                    uint32_t newIdx = static_cast<uint32_t>(mesh.vertices.size());
                    vertexMap[key] = newIdx;
                    mesh.vertices.push_back(vert);
                    mesh.indices.push_back(newIdx);
                }
            }

            // Compute tangents
            ComputeTangents(mesh);

            outScene.meshes.push_back(std::move(mesh));
        }
    }

    outScene.hasSkeleton = false;
    return true;
}

} // namespace gxconv
