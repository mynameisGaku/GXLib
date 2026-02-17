/// @file gxmd_exporter.cpp
/// @brief GXMD binary model exporter implementation

#include "gxmd_exporter.h"
#include "intermediate/scene.h"
#include "gxmd.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <array>
#include <map>
#include <algorithm>

namespace gxconv
{

// ============================================================
// String Table Builder
// ============================================================

class StringTable
{
public:
    uint32_t Add(const std::string& str)
    {
        if (str.empty())
            return gxfmt::k_InvalidStringIndex;

        // Check if already exists
        auto it = m_map.find(str);
        if (it != m_map.end())
            return it->second;

        uint32_t offset = static_cast<uint32_t>(m_data.size());
        m_map[str] = offset;
        m_data.insert(m_data.end(), str.begin(), str.end());
        m_data.push_back('\0');
        return offset;
    }

    const std::vector<uint8_t>& Data() const { return m_data; }
    uint32_t Size() const { return static_cast<uint32_t>(m_data.size()); }

private:
    std::vector<uint8_t> m_data;
    std::map<std::string, uint32_t> m_map;
};

// ============================================================
// File writer helper
// ============================================================

class BinaryWriter
{
public:
    bool Open(const std::string& path)
    {
        m_file = fopen(path.c_str(), "wb");
        return m_file != nullptr;
    }

    ~BinaryWriter()
    {
        if (m_file) fclose(m_file);
    }

    void Write(const void* data, size_t size)
    {
        fwrite(data, 1, size, m_file);
        m_offset += size;
    }

    template<typename T>
    void Write(const T& value)
    {
        Write(&value, sizeof(T));
    }

    void WriteZeros(size_t count)
    {
        std::vector<uint8_t> zeros(count, 0);
        Write(zeros.data(), count);
    }

    void Seek(uint64_t offset)
    {
        _fseeki64(m_file, static_cast<long long>(offset), SEEK_SET);
        m_offset = offset;
    }

    uint64_t Tell() const { return m_offset; }

private:
    FILE* m_file = nullptr;
    uint64_t m_offset = 0;
};

// ============================================================

bool GxmdExporter::Export(const Scene& scene, const std::string& outputPath,
                          const ExportOptions& options)
{
    // --- Pass 1: Build string table and compute sizes ---

    StringTable strings;

    // Mesh names
    std::vector<uint32_t> meshNameIndices;
    for (auto& mesh : scene.meshes)
        meshNameIndices.push_back(strings.Add(mesh.name));

    // Material names + texture paths
    std::vector<uint32_t> matNameIndices;
    std::vector<std::array<int32_t, 8>> matTexIndices;
    for (auto& mat : scene.materials)
    {
        matNameIndices.push_back(strings.Add(mat.name));
        std::array<int32_t, 8> texIds;
        for (int t = 0; t < 8; ++t)
        {
            if (!mat.texturePaths[t].empty())
                texIds[t] = static_cast<int32_t>(strings.Add(mat.texturePaths[t]));
            else
                texIds[t] = -1;
        }
        matTexIndices.push_back(texIds);
    }

    // Bone names
    std::vector<uint32_t> boneNameIndices;
    for (auto& joint : scene.skeleton)
        boneNameIndices.push_back(strings.Add(joint.name));

    // Animation names
    std::vector<uint32_t> animNameIndices;
    if (!options.excludeAnimations)
    {
        for (auto& anim : scene.animations)
            animNameIndices.push_back(strings.Add(anim.name));
    }

    // Determine vertex format
    bool hasSkinning = scene.hasSkeleton;
    for (auto& mesh : scene.meshes)
        if (mesh.hasSkinning) hasSkinning = true;

    uint32_t vertexStride = hasSkinning ? sizeof(gxfmt::VertexSkinned) : sizeof(gxfmt::VertexStandard);
    uint32_t vertexFlags = hasSkinning ? gxfmt::VF_Skinned : gxfmt::VF_Standard;

    // Compute vertex/index data sizes
    uint64_t totalVertexBytes = 0;
    uint64_t totalIndexBytes = 0;
    std::vector<uint64_t> meshVertexOffsets, meshIndexOffsets;

    for (auto& mesh : scene.meshes)
    {
        meshVertexOffsets.push_back(totalVertexBytes);
        meshIndexOffsets.push_back(totalIndexBytes);
        totalVertexBytes += mesh.vertices.size() * vertexStride;

        bool use16 = options.useIndex16 && mesh.vertices.size() <= 65535;
        gxfmt::IndexFormat idxFmt = use16 ? gxfmt::IndexFormat::UInt16 : gxfmt::IndexFormat::UInt32;
        uint32_t idxSize = (idxFmt == gxfmt::IndexFormat::UInt16) ? 2 : 4;
        totalIndexBytes += mesh.indices.size() * idxSize;
    }

    // Compute animation data size
    uint64_t totalAnimBytes = 0;
    if (!options.excludeAnimations)
    {
        for (auto& anim : scene.animations)
        {
            totalAnimBytes += sizeof(gxfmt::AnimationChunk);
            for (auto& ch : anim.channels)
            {
                totalAnimBytes += sizeof(gxfmt::AnimationChannelDesc);
                if (ch.target == 1) // rotation
                    totalAnimBytes += ch.quatKeys.size() * sizeof(gxfmt::QuatKey);
                else
                    totalAnimBytes += ch.vecKeys.size() * sizeof(gxfmt::VectorKey);
            }
        }
    }

    // --- Pass 2: Compute offsets ---

    uint64_t cursor = sizeof(gxfmt::FileHeader);

    uint64_t stringTableOffset = cursor;
    cursor += 4 + strings.Size(); // uint32 byteCount + data

    uint64_t meshChunkOffset = cursor;
    cursor += scene.meshes.size() * sizeof(gxfmt::MeshChunk);

    uint64_t materialChunkOffset = cursor;
    cursor += scene.materials.size() * sizeof(gxfmt::MaterialChunk);

    uint64_t vertexDataOffset = cursor;
    cursor += totalVertexBytes;

    uint64_t indexDataOffset = cursor;
    cursor += totalIndexBytes;

    uint64_t boneDataOffset = cursor;
    cursor += scene.skeleton.size() * sizeof(gxfmt::BoneData);

    uint64_t animDataOffset = cursor;
    cursor += totalAnimBytes;

    uint64_t blendShapeOffset = cursor;
    // No blend shapes yet

    // --- Pass 3: Write ---

    BinaryWriter writer;
    if (!writer.Open(outputPath))
    {
        fprintf(stderr, "Error: Cannot open output file: %s\n", outputPath.c_str());
        return false;
    }

    // File header
    gxfmt::FileHeader header{};
    header.magic = gxfmt::k_GxmdMagic;
    header.version = gxfmt::k_GxmdVersion;
    header.flags = 0;
    header.meshCount = static_cast<uint32_t>(scene.meshes.size());
    header.materialCount = static_cast<uint32_t>(scene.materials.size());
    header.boneCount = static_cast<uint32_t>(scene.skeleton.size());
    header.animationCount = options.excludeAnimations ? 0 : static_cast<uint32_t>(scene.animations.size());
    header.blendShapeCount = 0;
    header.stringTableOffset = stringTableOffset;
    header.meshChunkOffset = meshChunkOffset;
    header.materialChunkOffset = materialChunkOffset;
    header.vertexDataOffset = vertexDataOffset;
    header.indexDataOffset = indexDataOffset;
    header.boneDataOffset = boneDataOffset;
    header.animationDataOffset = animDataOffset;
    header.blendShapeDataOffset = blendShapeOffset;
    header.stringTableSize = strings.Size();
    header.vertexDataSize = static_cast<uint32_t>(totalVertexBytes);
    header.indexDataSize = static_cast<uint32_t>(totalIndexBytes);
    std::memset(header._reserved, 0, sizeof(header._reserved));
    writer.Write(header);

    // String table
    uint32_t stSize = strings.Size();
    writer.Write(stSize);
    writer.Write(strings.Data().data(), strings.Data().size());

    // Mesh chunks
    for (size_t i = 0; i < scene.meshes.size(); ++i)
    {
        auto& mesh = scene.meshes[i];
        gxfmt::MeshChunk chunk{};
        chunk.nameIndex = meshNameIndices[i];
        chunk.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
        chunk.indexCount = static_cast<uint32_t>(mesh.indices.size());
        chunk.materialIndex = mesh.materialIndex;
        chunk.vertexFormatFlags = vertexFlags;
        chunk.vertexStride = vertexStride;
        chunk.vertexOffset = meshVertexOffsets[i];
        chunk.indexOffset = meshIndexOffsets[i];

        bool use16 = options.useIndex16 && mesh.vertices.size() <= 65535;
        chunk.indexFormat = use16 ? gxfmt::IndexFormat::UInt16 : gxfmt::IndexFormat::UInt32;
        chunk.topology = gxfmt::PrimitiveTopology::TriangleList;

        // Compute AABB
        chunk.aabbMin[0] = chunk.aabbMin[1] = chunk.aabbMin[2] = 1e30f;
        chunk.aabbMax[0] = chunk.aabbMax[1] = chunk.aabbMax[2] = -1e30f;
        for (auto& v : mesh.vertices)
        {
            for (int j = 0; j < 3; ++j)
            {
                if (v.position[j] < chunk.aabbMin[j]) chunk.aabbMin[j] = v.position[j];
                if (v.position[j] > chunk.aabbMax[j]) chunk.aabbMax[j] = v.position[j];
            }
        }

        writer.Write(chunk);
    }

    // Material chunks
    for (size_t i = 0; i < scene.materials.size(); ++i)
    {
        auto& mat = scene.materials[i];
        gxfmt::MaterialChunk chunk{};
        chunk.nameIndex = matNameIndices[i];
        chunk.shaderModel = mat.shaderModel;
        chunk.params = mat.params;

        // Set texture name indices
        for (int t = 0; t < 8; ++t)
            chunk.params.textureNames[t] = matTexIndices[i][t];

        writer.Write(chunk);
    }

    // Vertex data
    for (auto& mesh : scene.meshes)
    {
        if (hasSkinning)
        {
            for (auto& v : mesh.vertices)
            {
                gxfmt::VertexSkinned sv{};
                std::memcpy(sv.position, v.position, 12);
                std::memcpy(sv.normal, v.normal, 12);
                std::memcpy(sv.uv0, v.texcoord, 8);
                std::memcpy(sv.tangent, v.tangent, 16);
                std::memcpy(sv.joints, v.joints, 16);
                std::memcpy(sv.weights, v.weights, 16);
                writer.Write(sv);
            }
        }
        else
        {
            for (auto& v : mesh.vertices)
            {
                gxfmt::VertexStandard sv{};
                std::memcpy(sv.position, v.position, 12);
                std::memcpy(sv.normal, v.normal, 12);
                std::memcpy(sv.uv0, v.texcoord, 8);
                std::memcpy(sv.tangent, v.tangent, 16);
                writer.Write(sv);
            }
        }
    }

    // Index data
    for (auto& mesh : scene.meshes)
    {
        bool use16 = options.useIndex16 && mesh.vertices.size() <= 65535;
        if (use16)
        {
            for (uint32_t idx : mesh.indices)
            {
                uint16_t idx16 = static_cast<uint16_t>(idx);
                writer.Write(idx16);
            }
        }
        else
        {
            writer.Write(mesh.indices.data(), mesh.indices.size() * 4);
        }
    }

    // Bone data
    for (size_t i = 0; i < scene.skeleton.size(); ++i)
    {
        auto& joint = scene.skeleton[i];
        gxfmt::BoneData bd{};
        bd.nameIndex = boneNameIndices[i];
        bd.parentIndex = joint.parentIndex;
        std::memcpy(bd.inverseBindMatrix, joint.inverseBindMatrix, 64);
        std::memcpy(bd.localTranslation, joint.localTranslation, 12);
        std::memcpy(bd.localRotation, joint.localRotation, 16);
        std::memcpy(bd.localScale, joint.localScale, 12);
        writer.Write(bd);
    }

    // Animation data
    if (!options.excludeAnimations)
    {
        uint32_t animKeyDataOffset = 0; // relative to anim data start
        // First pass: compute channel desc + key layout
        for (size_t ai = 0; ai < scene.animations.size(); ++ai)
        {
            auto& anim = scene.animations[ai];
            gxfmt::AnimationChunk achunk{};
            achunk.nameIndex = animNameIndices[ai];
            achunk.duration = anim.duration;
            achunk.channelCount = static_cast<uint32_t>(anim.channels.size());
            achunk._pad = 0;
            writer.Write(achunk);

            // Compute offsets for channel key data
            // Key data comes after all channel descriptors for this animation
            uint32_t localKeyOffset = 0;
            // But we need to write descriptors first, then keys
            // So collect descriptors
            std::vector<gxfmt::AnimationChannelDesc> descs;
            for (auto& ch : anim.channels)
            {
                gxfmt::AnimationChannelDesc desc{};
                desc.boneIndex = ch.jointIndex;
                desc.target = static_cast<gxfmt::AnimChannelTarget>(ch.target);
                desc.interpolation = ch.interpolation;
                desc.dataOffset = localKeyOffset;

                if (ch.target == 1) // rotation
                {
                    desc.keyCount = static_cast<uint32_t>(ch.quatKeys.size());
                    localKeyOffset += desc.keyCount * sizeof(gxfmt::QuatKey);
                }
                else
                {
                    desc.keyCount = static_cast<uint32_t>(ch.vecKeys.size());
                    localKeyOffset += desc.keyCount * sizeof(gxfmt::VectorKey);
                }
                descs.push_back(desc);
            }

            // Write descriptors
            for (auto& d : descs)
                writer.Write(d);

            // Write key data
            for (auto& ch : anim.channels)
            {
                if (ch.target == 1) // rotation
                {
                    for (auto& k : ch.quatKeys)
                    {
                        gxfmt::QuatKey qk;
                        qk.time = k.time;
                        std::memcpy(qk.value, k.value, 16);
                        writer.Write(qk);
                    }
                }
                else
                {
                    for (auto& k : ch.vecKeys)
                    {
                        gxfmt::VectorKey vk;
                        vk.time = k.time;
                        std::memcpy(vk.value, k.value, 12);
                        writer.Write(vk);
                    }
                }
            }
        }
    }

    printf("Exported: %s\n", outputPath.c_str());
    printf("  Meshes: %u, Materials: %u, Bones: %u, Animations: %u\n",
           header.meshCount, header.materialCount, header.boneCount, header.animationCount);

    return true;
}

} // namespace gxconv
