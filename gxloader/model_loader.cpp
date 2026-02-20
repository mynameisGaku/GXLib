/// @file model_loader.cpp
/// @brief GXMD runtime loader implementation

#include "model_loader.h"
#include <cstdio>
#include <cstring>

namespace gxloader
{

// Helper: read string from string table
static std::string ReadString(const uint8_t* stringData, uint32_t stringSize, uint32_t offset)
{
    if (offset == gxfmt::k_InvalidStringIndex || offset >= stringSize)
        return "";
    return std::string(reinterpret_cast<const char*>(stringData + offset));
}

std::unique_ptr<LoadedModel> LoadGxmdFromMemory(const uint8_t* data, size_t size)
{
    if (size < sizeof(gxfmt::FileHeader))
        return nullptr;

    const auto* header = reinterpret_cast<const gxfmt::FileHeader*>(data);

    if (header->magic != gxfmt::k_GxmdMagic)
        return nullptr;

    auto model = std::make_unique<LoadedModel>();
    model->version = header->version;

    // --- String Table ---
    const uint8_t* stBase = data + header->stringTableOffset;
    uint32_t stByteCount = 0;
    std::memcpy(&stByteCount, stBase, 4);
    const uint8_t* stringData = stBase + 4;
    uint32_t stringSize = stByteCount;

    // --- Materials ---
    const auto* matChunks = reinterpret_cast<const gxfmt::MaterialChunk*>(data + header->materialChunkOffset);
    model->materials.resize(header->materialCount);
    for (uint32_t i = 0; i < header->materialCount; ++i)
    {
        auto& dst = model->materials[i];
        const auto& src = matChunks[i];
        dst.name = ReadString(stringData, stringSize, src.nameIndex);
        dst.shaderModel = src.shaderModel;
        dst.params = src.params;

        // Resolve texture paths
        for (int t = 0; t < 8; ++t)
        {
            int32_t texIdx = src.params.textureNames[t];
            if (texIdx >= 0)
                dst.texturePaths[t] = ReadString(stringData, stringSize, static_cast<uint32_t>(texIdx));
        }
    }

    // --- Meshes ---
    const auto* meshChunks = reinterpret_cast<const gxfmt::MeshChunk*>(data + header->meshChunkOffset);

    // Determine vertex type from first mesh
    bool isSkinned = false;
    if (header->meshCount > 0)
        isSkinned = (meshChunks[0].vertexFormatFlags & gxfmt::VF_Joints) != 0;
    model->isSkinned = isSkinned;

    // Read vertex data
    const uint8_t* vertexBase = data + header->vertexDataOffset;
    const uint8_t* indexBase = data + header->indexDataOffset;

    // Accumulate all meshes into single vertex/index arrays
    uint32_t globalVertexOffset = 0;
    uint32_t globalIndexOffset = 0;

    // First pass: count totals
    uint32_t totalVertices = 0;
    uint32_t totalIndices = 0;
    for (uint32_t i = 0; i < header->meshCount; ++i)
    {
        totalVertices += meshChunks[i].vertexCount;
        totalIndices += meshChunks[i].indexCount;
    }

    if (isSkinned)
        model->skinnedVertices.resize(totalVertices);
    else
        model->standardVertices.resize(totalVertices);

    // Check if ALL meshes use 16-bit indices; if any uses 32-bit, unify to 32-bit
    bool allUse16 = true;
    for (uint32_t i = 0; i < header->meshCount; ++i)
    {
        if (meshChunks[i].indexFormat != gxfmt::IndexFormat::UInt16)
        {
            allUse16 = false;
            break;
        }
    }
    model->uses16BitIndices = allUse16;
    if (allUse16)
        model->indices16.resize(totalIndices);
    else
        model->indices32.resize(totalIndices);

    for (uint32_t i = 0; i < header->meshCount; ++i)
    {
        const auto& mc = meshChunks[i];

        // Copy vertices
        const uint8_t* vSrc = vertexBase + mc.vertexOffset;
        if (isSkinned)
            std::memcpy(&model->skinnedVertices[globalVertexOffset], vSrc,
                        mc.vertexCount * sizeof(gxfmt::VertexSkinned));
        else
            std::memcpy(&model->standardVertices[globalVertexOffset], vSrc,
                        mc.vertexCount * sizeof(gxfmt::VertexStandard));

        // Copy indices — handle per-mesh index format
        const uint8_t* iSrc = indexBase + mc.indexOffset;
        bool meshUse16 = (mc.indexFormat == gxfmt::IndexFormat::UInt16);

        if (allUse16)
        {
            // All meshes are 16-bit → direct copy
            std::memcpy(&model->indices16[globalIndexOffset], iSrc,
                        mc.indexCount * sizeof(uint16_t));
        }
        else if (meshUse16)
        {
            // This mesh is 16-bit but output is 32-bit → widen
            const uint16_t* src16 = reinterpret_cast<const uint16_t*>(iSrc);
            for (uint32_t j = 0; j < mc.indexCount; ++j)
                model->indices32[globalIndexOffset + j] = src16[j];
        }
        else
        {
            // This mesh is 32-bit → direct copy
            std::memcpy(&model->indices32[globalIndexOffset], iSrc,
                        mc.indexCount * sizeof(uint32_t));
        }

        // SubMesh
        LoadedSubMesh sm;
        sm.vertexOffset = globalVertexOffset;
        sm.vertexCount = mc.vertexCount;
        sm.indexOffset = globalIndexOffset;
        sm.indexCount = mc.indexCount;
        sm.materialIndex = mc.materialIndex;
        std::memcpy(sm.aabbMin, mc.aabbMin, 12);
        std::memcpy(sm.aabbMax, mc.aabbMax, 12);
        model->subMeshes.push_back(sm);

        globalVertexOffset += mc.vertexCount;
        globalIndexOffset += mc.indexCount;
    }

    // --- Skeleton ---
    if (header->boneCount > 0)
    {
        const auto* bones = reinterpret_cast<const gxfmt::BoneData*>(data + header->boneDataOffset);
        model->joints.resize(header->boneCount);
        for (uint32_t i = 0; i < header->boneCount; ++i)
        {
            auto& dst = model->joints[i];
            const auto& src = bones[i];
            dst.name = ReadString(stringData, stringSize, src.nameIndex);
            dst.parentIndex = src.parentIndex;
            std::memcpy(dst.inverseBindMatrix, src.inverseBindMatrix, 64);
            std::memcpy(dst.localTranslation, src.localTranslation, 12);
            std::memcpy(dst.localRotation, src.localRotation, 16);
            std::memcpy(dst.localScale, src.localScale, 12);
        }
    }

    // --- Animations ---
    if (header->animationCount > 0)
    {
        const uint8_t* animPtr = data + header->animationDataOffset;

        for (uint32_t ai = 0; ai < header->animationCount; ++ai)
        {
            const auto* ac = reinterpret_cast<const gxfmt::AnimationChunk*>(animPtr);
            animPtr += sizeof(gxfmt::AnimationChunk);

            LoadedAnimation anim;
            anim.name = ReadString(stringData, stringSize, ac->nameIndex);
            anim.duration = ac->duration;

            // Read channel descriptors
            std::vector<gxfmt::AnimationChannelDesc> descs(ac->channelCount);
            std::memcpy(descs.data(), animPtr, ac->channelCount * sizeof(gxfmt::AnimationChannelDesc));
            animPtr += ac->channelCount * sizeof(gxfmt::AnimationChannelDesc);

            const uint8_t* keyBase = animPtr;

            for (uint32_t ci = 0; ci < ac->channelCount; ++ci)
            {
                const auto& desc = descs[ci];
                LoadedAnimChannel ch;
                ch.jointIndex = desc.boneIndex;
                ch.target = static_cast<uint8_t>(desc.target);
                ch.interpolation = desc.interpolation;

                const uint8_t* keyPtr = keyBase + desc.dataOffset;

                if (desc.target == gxfmt::AnimChannelTarget::Rotation)
                {
                    ch.quatKeys.resize(desc.keyCount);
                    std::memcpy(ch.quatKeys.data(), keyPtr, desc.keyCount * sizeof(gxfmt::QuatKey));
                }
                else
                {
                    ch.vecKeys.resize(desc.keyCount);
                    std::memcpy(ch.vecKeys.data(), keyPtr, desc.keyCount * sizeof(gxfmt::VectorKey));
                }

                anim.channels.push_back(std::move(ch));
            }

            // Advance animPtr past key data
            uint32_t keyDataSize = 0;
            for (auto& desc : descs)
            {
                if (desc.target == gxfmt::AnimChannelTarget::Rotation)
                    keyDataSize += desc.keyCount * sizeof(gxfmt::QuatKey);
                else
                    keyDataSize += desc.keyCount * sizeof(gxfmt::VectorKey);
            }
            animPtr = keyBase + keyDataSize;

            model->animations.push_back(std::move(anim));
        }
    }

    return model;
}

std::unique_ptr<LoadedModel> LoadGxmd(const std::string& filePath)
{
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) return nullptr;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0)
    {
        fclose(f);
        return nullptr;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    fread(buffer.data(), 1, buffer.size(), f);
    fclose(f);

    return LoadGxmdFromMemory(buffer.data(), buffer.size());
}

#ifdef _WIN32
std::unique_ptr<LoadedModel> LoadGxmdW(const std::wstring& filePath)
{
    FILE* f = _wfopen(filePath.c_str(), L"rb");
    if (!f) return nullptr;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize <= 0) { fclose(f); return nullptr; }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    fread(buffer.data(), 1, buffer.size(), f);
    fclose(f);

    return LoadGxmdFromMemory(buffer.data(), buffer.size());
}
#endif

} // namespace gxloader
