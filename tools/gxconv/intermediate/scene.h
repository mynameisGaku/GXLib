#pragma once
/// @file scene.h
/// @brief Intermediate scene representation for gxconv

#include <cstdint>
#include <string>
#include <vector>
#include <cmath>
#include "shader_model.h"

namespace gxconv
{

struct IntermediateVertex
{
    float position[3] = {};
    float normal[3]   = {};
    float texcoord[2] = {};
    float tangent[4]  = {}; // w = bitangent sign
    uint32_t joints[4] = {};
    float weights[4]  = {};
};

struct IntermediateMesh
{
    std::string name;
    std::vector<IntermediateVertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t materialIndex = 0;
    bool hasSkinning = false;
};

struct IntermediateMaterial
{
    std::string name;
    gxfmt::ShaderModel shaderModel = gxfmt::ShaderModel::Standard;
    gxfmt::ShaderModelParams params{};
    std::string texturePaths[8]; // same slots as ShaderModelParams::textureNames
};

struct IntermediateJoint
{
    std::string name;
    int32_t parentIndex = -1;
    float inverseBindMatrix[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    float localTranslation[3] = {};
    float localRotation[4]    = { 0, 0, 0, 1 }; // quaternion x,y,z,w
    float localScale[3]       = { 1, 1, 1 };
};

struct IntermediateKeyframeVec3
{
    float time;
    float value[3];
};

struct IntermediateKeyframeQuat
{
    float time;
    float value[4]; // x,y,z,w
};

struct IntermediateAnimChannel
{
    uint32_t jointIndex = 0;
    std::string boneName; // for GXAN name-based output
    uint8_t target = 0;   // 0=Translation, 1=Rotation, 2=Scale
    uint8_t interpolation = 0; // 0=Linear, 1=Step, 2=CubicSpline
    std::vector<IntermediateKeyframeVec3> vecKeys;
    std::vector<IntermediateKeyframeQuat> quatKeys;
};

struct IntermediateAnimation
{
    std::string name;
    float duration = 0.0f;
    std::vector<IntermediateAnimChannel> channels;
};

struct Scene
{
    std::vector<IntermediateMesh>      meshes;
    std::vector<IntermediateMaterial>  materials;
    std::vector<IntermediateJoint>     skeleton;
    std::vector<IntermediateAnimation> animations;
    bool hasSkeleton = false;
};

/// Compute tangent vectors for a mesh using triangle differential method
inline void ComputeTangents(IntermediateMesh& mesh)
{
    if (mesh.indices.empty() || mesh.vertices.empty()) return;

    const size_t vertCount = mesh.vertices.size();
    std::vector<float> tan1(vertCount * 3, 0.0f);
    std::vector<float> tan2(vertCount * 3, 0.0f);

    for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
    {
        uint32_t i0 = mesh.indices[i + 0];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];

        const auto& v0 = mesh.vertices[i0];
        const auto& v1 = mesh.vertices[i1];
        const auto& v2 = mesh.vertices[i2];

        float dx1 = v1.position[0] - v0.position[0];
        float dy1 = v1.position[1] - v0.position[1];
        float dz1 = v1.position[2] - v0.position[2];
        float dx2 = v2.position[0] - v0.position[0];
        float dy2 = v2.position[1] - v0.position[1];
        float dz2 = v2.position[2] - v0.position[2];

        float du1 = v1.texcoord[0] - v0.texcoord[0];
        float dv1 = v1.texcoord[1] - v0.texcoord[1];
        float du2 = v2.texcoord[0] - v0.texcoord[0];
        float dv2 = v2.texcoord[1] - v0.texcoord[1];

        float r = du1 * dv2 - du2 * dv1;
        if (std::abs(r) < 1e-8f) r = 1.0f;
        r = 1.0f / r;

        float sx = (dv2 * dx1 - dv1 * dx2) * r;
        float sy = (dv2 * dy1 - dv1 * dy2) * r;
        float sz = (dv2 * dz1 - dv1 * dz2) * r;
        float tx = (du1 * dx2 - du2 * dx1) * r;
        float ty = (du1 * dy2 - du2 * dy1) * r;
        float tz = (du1 * dz2 - du2 * dz1) * r;

        for (uint32_t idx : { i0, i1, i2 })
        {
            tan1[idx * 3 + 0] += sx;
            tan1[idx * 3 + 1] += sy;
            tan1[idx * 3 + 2] += sz;
            tan2[idx * 3 + 0] += tx;
            tan2[idx * 3 + 1] += ty;
            tan2[idx * 3 + 2] += tz;
        }
    }

    for (size_t i = 0; i < vertCount; ++i)
    {
        auto& v = mesh.vertices[i];
        float nx = v.normal[0], ny = v.normal[1], nz = v.normal[2];
        float t1x = tan1[i * 3], t1y = tan1[i * 3 + 1], t1z = tan1[i * 3 + 2];

        // Gram-Schmidt orthogonalize
        float dot = nx * t1x + ny * t1y + nz * t1z;
        float rx = t1x - nx * dot;
        float ry = t1y - ny * dot;
        float rz = t1z - nz * dot;
        float len = std::sqrt(rx * rx + ry * ry + rz * rz);
        if (len > 1e-8f) { rx /= len; ry /= len; rz /= len; }
        else { rx = 1; ry = 0; rz = 0; }

        // Handedness
        float cx = ny * t1z - nz * t1y;
        float cy = nz * t1x - nx * t1z;
        float cz = nx * t1y - ny * t1x;
        float t2x = tan2[i * 3], t2y = tan2[i * 3 + 1], t2z = tan2[i * 3 + 2];
        float hand = (cx * t2x + cy * t2y + cz * t2z) < 0.0f ? -1.0f : 1.0f;

        v.tangent[0] = rx;
        v.tangent[1] = ry;
        v.tangent[2] = rz;
        v.tangent[3] = hand;
    }
}

} // namespace gxconv
