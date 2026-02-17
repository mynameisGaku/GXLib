#pragma once
/// @file model_loader.h
/// @brief GXMD runtime loader (GPU-independent, CPU raw data)

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "gxmd.h"
#include "shader_model.h"

namespace gxloader
{

struct LoadedSubMesh
{
    uint32_t vertexOffset = 0;
    uint32_t vertexCount  = 0;
    uint32_t indexOffset   = 0;
    uint32_t indexCount    = 0;
    uint32_t materialIndex = 0;
    float    aabbMin[3]    = {};
    float    aabbMax[3]    = {};
};

struct LoadedMaterial
{
    std::string            name;
    gxfmt::ShaderModel     shaderModel = gxfmt::ShaderModel::Standard;
    gxfmt::ShaderModelParams params{};
    std::string            texturePaths[8]; // resolved from string table
};

struct LoadedJoint
{
    std::string name;
    int32_t     parentIndex = -1;
    float       inverseBindMatrix[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    float       localTranslation[3] = {};
    float       localRotation[4]    = { 0, 0, 0, 1 };
    float       localScale[3]       = { 1, 1, 1 };
};

struct LoadedAnimChannel
{
    uint32_t jointIndex = 0;
    uint8_t  target     = 0; // 0=T, 1=R, 2=S
    uint8_t  interpolation = 0;
    std::vector<gxfmt::VectorKey> vecKeys;
    std::vector<gxfmt::QuatKey>   quatKeys;
};

struct LoadedAnimation
{
    std::string name;
    float       duration = 0.0f;
    std::vector<LoadedAnimChannel> channels;
};

struct LoadedModel
{
    // Vertex data (one of the two will be filled)
    std::vector<gxfmt::VertexStandard> standardVertices;
    std::vector<gxfmt::VertexSkinned>  skinnedVertices;
    bool isSkinned = false;

    // Index data
    std::vector<uint16_t> indices16;
    std::vector<uint32_t> indices32;
    bool uses16BitIndices = false;

    // Sub-meshes
    std::vector<LoadedSubMesh> subMeshes;

    // Materials
    std::vector<LoadedMaterial> materials;

    // Skeleton
    std::vector<LoadedJoint> joints;

    // Animations
    std::vector<LoadedAnimation> animations;

    // File version
    uint32_t version = 0;
};

/// Load a GXMD file from disk
std::unique_ptr<LoadedModel> LoadGxmd(const std::string& filePath);

/// Load a GXMD from memory buffer
std::unique_ptr<LoadedModel> LoadGxmdFromMemory(const uint8_t* data, size_t size);

} // namespace gxloader
