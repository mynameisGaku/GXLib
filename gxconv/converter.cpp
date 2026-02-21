/// @file converter.cpp
/// @brief 変換処理オーケストレーターの実装

#include "converter.h"
#include "intermediate/scene.h"
#include "importers/obj_importer.h"
#include "exporters/gxmd_exporter.h"
#include "gxmd.h"
#include "gxan.h"

#if defined(GXCONV_HAS_FBX)
#include "importers/fbx_importer.h"
#endif
#if defined(GXCONV_HAS_GLTF)
#include "importers/gltf_importer.h"
#include "exporters/gxan_exporter.h"
#endif

#include <cstdio>
#include <cstring>
#include <algorithm>

namespace gxconv
{

/// パスから小文字化した拡張子を取得する (".obj", ".fbx"等)
static std::string GetExtension(const std::string& path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return "";
    std::string ext = path.substr(dot);
    for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return ext;
}

/// パスの拡張子を差し替える
static std::string ChangeExtension(const std::string& path, const std::string& newExt)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return path + newExt;
    return path.substr(0, dot) + newExt;
}

int Converter::ShowInfo(const std::string& path)
{
    std::string ext = GetExtension(path);

    if (ext == ".gxmd")
    {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { fprintf(stderr, "Error: Cannot open %s\n", path.c_str()); return 1; }

        gxfmt::FileHeader header{};
        fread(&header, sizeof(header), 1, f);

        if (header.magic != gxfmt::k_GxmdMagic)
        {
            fprintf(stderr, "Error: Not a GXMD file\n");
            fclose(f);
            return 1;
        }

        printf("GXMD File: %s\n", path.c_str());
        printf("  Version: %u\n", header.version);
        printf("  Meshes: %u\n", header.meshCount);
        printf("  Materials: %u\n", header.materialCount);
        printf("  Bones: %u\n", header.boneCount);
        printf("  Animations: %u\n", header.animationCount);
        printf("  BlendShapes: %u\n", header.blendShapeCount);
        printf("  StringTable: %u bytes\n", header.stringTableSize);
        printf("  VertexData: %u bytes\n", header.vertexDataSize);
        printf("  IndexData: %u bytes\n", header.indexDataSize);

        // Read string table
        std::vector<uint8_t> stringData(header.stringTableSize);
        fseek(f, static_cast<long>(header.stringTableOffset + 4), SEEK_SET);
        fread(stringData.data(), 1, header.stringTableSize, f);

        auto getString = [&](uint32_t offset) -> const char* {
            if (offset == gxfmt::k_InvalidStringIndex || offset >= stringData.size())
                return "(none)";
            return reinterpret_cast<const char*>(stringData.data() + offset);
        };

        // Read mesh chunks
        fseek(f, static_cast<long>(header.meshChunkOffset), SEEK_SET);
        for (uint32_t i = 0; i < header.meshCount; ++i)
        {
            gxfmt::MeshChunk mc{};
            fread(&mc, sizeof(mc), 1, f);
            printf("  Mesh[%u]: \"%s\" verts=%u idx=%u mat=%u stride=%u\n",
                   i, getString(mc.nameIndex), mc.vertexCount, mc.indexCount,
                   mc.materialIndex, mc.vertexStride);
        }

        // Read material chunks
        fseek(f, static_cast<long>(header.materialChunkOffset), SEEK_SET);
        for (uint32_t i = 0; i < header.materialCount; ++i)
        {
            gxfmt::MaterialChunk mc{};
            fread(&mc, sizeof(mc), 1, f);
            printf("  Material[%u]: \"%s\" shaderModel=%s\n",
                   i, getString(mc.nameIndex),
                   gxfmt::ShaderModelToString(mc.shaderModel));
        }

        // Read bone names
        if (header.boneCount > 0)
        {
            fseek(f, static_cast<long>(header.boneDataOffset), SEEK_SET);
            for (uint32_t i = 0; i < header.boneCount; ++i)
            {
                gxfmt::BoneData bd{};
                fread(&bd, sizeof(bd), 1, f);
                printf("  Bone[%u]: \"%s\" parent=%d\n",
                       i, getString(bd.nameIndex), bd.parentIndex);
            }
        }

        fclose(f);
        return 0;
    }
    else if (ext == ".gxan")
    {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { fprintf(stderr, "Error: Cannot open %s\n", path.c_str()); return 1; }

        gxfmt::GxanHeader header{};
        fread(&header, sizeof(header), 1, f);

        if (header.magic != gxfmt::k_GxanMagic)
        {
            fprintf(stderr, "Error: Not a GXAN file\n");
            fclose(f);
            return 1;
        }

        printf("GXAN File: %s\n", path.c_str());
        printf("  Version: %u\n", header.version);
        printf("  Duration: %.3f sec\n", header.duration);
        printf("  Channels: %u\n", header.channelCount);

        fclose(f);
        return 0;
    }

    fprintf(stderr, "Error: --info only supports .gxmd and .gxan files\n");
    return 1;
}

int Converter::Run(const ConvertOptions& options)
{
    if (options.infoOnly)
        return ShowInfo(options.inputPath);

    std::string ext = GetExtension(options.inputPath);
    std::string outputPath = options.outputPath;

    // Determine output extension
    if (outputPath.empty())
    {
        if (options.animOnly)
            outputPath = ChangeExtension(options.inputPath, ".gxan");
        else
            outputPath = ChangeExtension(options.inputPath, ".gxmd");
    }

    // Import
    Scene scene;
    bool imported = false;

    if (ext == ".obj")
    {
        ObjImporter importer;
        imported = importer.Import(options.inputPath, scene);
    }
#if defined(GXCONV_HAS_FBX)
    else if (ext == ".fbx")
    {
        FbxImporter importer;
        imported = importer.Import(options.inputPath, scene);
    }
#endif
#if defined(GXCONV_HAS_GLTF)
    else if (ext == ".gltf" || ext == ".glb")
    {
        GltfImporter importer;
        imported = importer.Import(options.inputPath, scene);
    }
#endif
    else
    {
        fprintf(stderr, "Error: Unsupported input format: %s\n", ext.c_str());
        return 1;
    }

    if (!imported)
    {
        fprintf(stderr, "Error: Failed to import %s\n", options.inputPath.c_str());
        return 1;
    }

    // CLIオプションによるシェーダーモデル上書き
    if (options.hasShaderModelOverride)
    {
        for (auto& mat : scene.materials)
        {
            mat.shaderModel = options.shaderModelOverride;
            mat.params = gxfmt::DefaultShaderModelParams(mat.shaderModel);
        }
    }

    if (options.toonOutlineWidth > 0.0f)
    {
        for (auto& mat : scene.materials)
            mat.params.outlineWidth = options.toonOutlineWidth;
    }

    // Export
#if defined(GXCONV_HAS_GLTF)
    if (options.animOnly)
    {
        // Export as GXAN
        GxanExporter exporter;
        if (!exporter.Export(scene, outputPath))
        {
            fprintf(stderr, "Error: Failed to export %s\n", outputPath.c_str());
            return 1;
        }
        return 0;
    }
#endif

    ExportOptions exportOpts;
    exportOpts.useIndex16 = options.useIndex16;
    exportOpts.excludeAnimations = options.excludeAnimations;

    GxmdExporter exporter;
    if (!exporter.Export(scene, outputPath, exportOpts))
    {
        fprintf(stderr, "Error: Failed to export %s\n", outputPath.c_str());
        return 1;
    }

    return 0;
}

} // namespace gxconv
