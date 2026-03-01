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

/// @brief 中間表現の頂点データ（インポーター→エクスポーター間の共通形式）
struct IntermediateVertex
{
    float position[3] = {};    ///< 頂点位置 (x, y, z)
    float normal[3]   = {};    ///< 法線ベクトル (x, y, z)
    float texcoord[2] = {};    ///< テクスチャ座標 (u, v)
    float tangent[4]  = {};    ///< 接線ベクトル (x, y, z, w)。w はバイタンジェントの符号
    uint32_t joints[4] = {};   ///< スキニング用ボーンインデックス（最大4本）
    float weights[4]  = {};    ///< スキニング用ボーンウェイト（最大4本、合計1.0）
};

/// @brief 中間表現のメッシュデータ（頂点配列＋インデックス配列）
struct IntermediateMesh
{
    std::string name;                          ///< メッシュ名
    std::vector<IntermediateVertex> vertices;   ///< 頂点配列
    std::vector<uint32_t> indices;             ///< インデックス配列（三角形リスト）
    uint32_t materialIndex = 0;                ///< 使用するマテリアルのインデックス
    bool hasSkinning = false;                  ///< スキニングデータを持つか
};

/// @brief 中間表現のマテリアルデータ
struct IntermediateMaterial
{
    std::string name;              ///< マテリアル名
    gxfmt::ShaderModel shaderModel = gxfmt::ShaderModel::Standard; ///< シェーダーモデル種別
    gxfmt::ShaderModelParams params{};  ///< シェーダーモデル固有パラメータ
    std::string texturePaths[8];   ///< テクスチャファイルパス（ShaderModelParams::textureNamesと同スロット順）
};

/// @brief 中間表現のスケルトンジョイント（ボーン）
struct IntermediateJoint
{
    std::string name;              ///< ジョイント名
    int32_t parentIndex = -1;      ///< 親ジョイントのインデックス（-1=ルート）
    float inverseBindMatrix[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };                             ///< 逆バインド行列（4x4、行優先）
    float localTranslation[3] = {};    ///< ローカル位置 (x, y, z)
    float localRotation[4]    = { 0, 0, 0, 1 }; ///< ローカル回転クォータニオン (x, y, z, w)
    float localScale[3]       = { 1, 1, 1 };    ///< ローカルスケール (x, y, z)
};

/// @brief Vec3キーフレーム（位置・スケール用）
struct IntermediateKeyframeVec3
{
    float time;       ///< キーフレーム時刻（秒）
    float value[3];   ///< 値 (x, y, z)
};

/// @brief クォータニオンキーフレーム（回転用）
struct IntermediateKeyframeQuat
{
    float time;       ///< キーフレーム時刻（秒）
    float value[4];   ///< クォータニオン (x, y, z, w)
};

/// @brief アニメーションチャンネル（1ジョイントの1プロパティに対応）
struct IntermediateAnimChannel
{
    uint32_t jointIndex = 0;       ///< 対象ジョイントのインデックス
    std::string boneName;          ///< ボーン名（GXAN名前ベース出力用）
    uint8_t target = 0;            ///< 対象プロパティ（0=Translation, 1=Rotation, 2=Scale）
    uint8_t interpolation = 0;     ///< 補間モード（0=Linear, 1=Step, 2=CubicSpline）
    std::vector<IntermediateKeyframeVec3> vecKeys;   ///< Vec3キーフレーム配列（Translation/Scale用）
    std::vector<IntermediateKeyframeQuat> quatKeys;  ///< クォータニオンキーフレーム配列（Rotation用）
};

/// @brief アニメーションクリップ（複数チャンネルの集合）
struct IntermediateAnimation
{
    std::string name;              ///< アニメーション名
    float duration = 0.0f;         ///< 総再生時間（秒）
    std::vector<IntermediateAnimChannel> channels; ///< チャンネル配列
};

/// @brief 中間表現のシーン全体（インポート結果をまとめるコンテナ）
struct Scene
{
    std::vector<IntermediateMesh>      meshes;      ///< メッシュ配列
    std::vector<IntermediateMaterial>  materials;    ///< マテリアル配列
    std::vector<IntermediateJoint>     skeleton;     ///< スケルトンジョイント配列
    std::vector<IntermediateAnimation> animations;   ///< アニメーション配列
    bool hasSkeleton = false;                        ///< スケルトンを持つか
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
