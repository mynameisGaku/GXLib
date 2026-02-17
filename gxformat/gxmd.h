#pragma once
/// @file gxmd.h
/// @brief GXMD binary model format definition

#include "types.h"
#include "shader_model.h"

namespace gxfmt
{

// ============================================================
// Constants
// ============================================================

static constexpr uint32_t k_GxmdMagic   = 0x444D5847; // 'GXMD'
static constexpr uint32_t k_GxmdVersion = 2;

// ============================================================
// Vertex format flags
// ============================================================

enum VertexFormat : uint32_t
{
    VF_Position  = 1 << 0,
    VF_Normal    = 1 << 1,
    VF_UV0       = 1 << 2,
    VF_Tangent   = 1 << 3,
    VF_Joints    = 1 << 4,
    VF_Weights   = 1 << 5,
    VF_UV1       = 1 << 6,
    VF_Color     = 1 << 7,

    VF_Standard  = VF_Position | VF_Normal | VF_UV0 | VF_Tangent,
    VF_Skinned   = VF_Standard | VF_Joints | VF_Weights,
};

enum class IndexFormat : uint8_t
{
    UInt16 = 0,
    UInt32 = 1,
};

enum class PrimitiveTopology : uint8_t
{
    TriangleList = 0,
    TriangleStrip = 1,
};

// ============================================================
// Vertices
// ============================================================

/// Standard vertex (48 bytes) - matches GX::Vertex3D_PBR layout
struct VertexStandard
{
    float    position[3];
    float    normal[3];
    float    uv0[2];
    float    tangent[4]; // w = bitangent sign
};

static_assert(sizeof(VertexStandard) == 48, "VertexStandard must be 48 bytes");

/// Skinned vertex (80 bytes) - matches GX::Vertex3D_Skinned layout
struct VertexSkinned
{
    float    position[3];
    float    normal[3];
    float    uv0[2];
    float    tangent[4];
    uint32_t joints[4];   // bone indices
    float    weights[4];  // bone weights
};

static_assert(sizeof(VertexSkinned) == 80, "VertexSkinned must be 80 bytes");

// ============================================================
// File Header (128 bytes)
// ============================================================

struct FileHeader
{
    uint32_t magic;               // 0x444D5847
    uint32_t version;             // 2
    uint32_t flags;               // reserved
    uint32_t meshCount;
    uint32_t materialCount;
    uint32_t boneCount;
    uint32_t animationCount;
    uint32_t blendShapeCount;

    uint64_t stringTableOffset;   // from file start
    uint64_t meshChunkOffset;
    uint64_t materialChunkOffset;
    uint64_t vertexDataOffset;
    uint64_t indexDataOffset;
    uint64_t boneDataOffset;
    uint64_t animationDataOffset;
    uint64_t blendShapeDataOffset;

    uint32_t stringTableSize;     // in bytes
    uint32_t vertexDataSize;      // in bytes
    uint32_t indexDataSize;       // in bytes
    uint8_t  _reserved[20];
};

static_assert(sizeof(FileHeader) == 128, "FileHeader must be 128 bytes");

// ============================================================
// Chunks
// ============================================================

struct MeshChunk
{
    uint32_t nameIndex;           // StringTable byte offset
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t materialIndex;
    uint32_t vertexFormatFlags;   // VertexFormat bitmask
    uint32_t vertexStride;        // bytes per vertex
    uint64_t vertexOffset;        // offset within vertex data block
    uint64_t indexOffset;         // offset within index data block
    IndexFormat     indexFormat;
    PrimitiveTopology topology;
    uint8_t  _pad[2];
    float    aabbMin[3];
    float    aabbMax[3];
};

struct MaterialChunk
{
    uint32_t          nameIndex;     // StringTable byte offset
    ShaderModel       shaderModel;
    ShaderModelParams params;        // 256 bytes
};

// ============================================================
// Skeleton
// ============================================================

struct BoneData
{
    uint32_t nameIndex;              // StringTable byte offset
    int32_t  parentIndex;            // -1 = root
    float    inverseBindMatrix[16];  // row-major (DirectXMath compatible)
    float    localTranslation[3];
    float    localRotation[4];       // quaternion (x,y,z,w)
    float    localScale[3];
};

// ============================================================
// Animation (embedded in GXMD)
// ============================================================

enum class AnimChannelTarget : uint8_t
{
    Translation = 0,
    Rotation    = 1,
    Scale       = 2,
};

struct AnimationChunk
{
    uint32_t nameIndex;         // StringTable byte offset
    float    duration;          // seconds
    uint32_t channelCount;
    uint32_t _pad;
};

struct AnimationChannelDesc
{
    uint32_t        boneIndex;      // index into BoneData array
    AnimChannelTarget target;
    uint8_t         interpolation;  // 0=Linear, 1=Step, 2=CubicSpline
    uint8_t         _pad[2];
    uint32_t        keyCount;
    uint32_t        dataOffset;     // byte offset to key data from anim data start
};

struct VectorKey
{
    float time;
    float value[3];
};

struct QuatKey
{
    float time;
    float value[4]; // x,y,z,w
};

// ============================================================
// Blend Shapes
// ============================================================

struct BlendShapeTarget
{
    uint32_t nameIndex;         // StringTable byte offset
    uint32_t meshIndex;
    uint32_t deltaCount;
    uint32_t deltaOffset;       // byte offset from blend shape data start
};

struct BlendShapeDelta
{
    uint32_t vertexIndex;
    float    positionDelta[3];
    float    normalDelta[3];
};

// ============================================================
// String Table helper
// ============================================================

/// String table: uint32_t byteCount followed by null-terminated UTF-8 strings.
/// Each nameIndex is a byte offset from the first string byte (after byteCount).
static constexpr uint32_t k_InvalidStringIndex = 0xFFFFFFFF;

// ============================================================
// Binary layout:
//   [FileHeader 128B]
//   [StringTable: uint32_t byteCount + UTF-8 strings]
//   [MeshChunk x meshCount]
//   [MaterialChunk x materialCount]
//   [VertexData (contiguous)]
//   [IndexData (uint16 or uint32)]
//   [BoneData x boneCount]        (if boneCount > 0)
//   [AnimationData]                (if animationCount > 0)
//   [BlendShapeData]               (if blendShapeCount > 0)
// ============================================================

} // namespace gxfmt
