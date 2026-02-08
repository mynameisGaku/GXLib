#pragma once
/// @file Material.h
/// @brief PBRマテリアル定義

#include "pch.h"

namespace GX
{

/// マテリアル定数バッファ（b3）
struct MaterialConstants
{
    XMFLOAT4 albedoFactor;       // ベースカラー (RGBA)
    float    metallicFactor;     // 金属性 0-1
    float    roughnessFactor;    // 粗さ 0-1
    float    aoStrength;         // AO強度
    float    emissiveStrength;   // エミッシブ倍率
    XMFLOAT3 emissiveFactor;     // エミッシブ色
    uint32_t flags;              // テクスチャ有無ビットフラグ
};

/// テクスチャフラグ
namespace MaterialFlags
{
    constexpr uint32_t HasAlbedoMap    = 1 << 0;
    constexpr uint32_t HasNormalMap    = 1 << 1;
    constexpr uint32_t HasMetRoughMap  = 1 << 2;
    constexpr uint32_t HasAOMap        = 1 << 3;
    constexpr uint32_t HasEmissiveMap  = 1 << 4;
}

/// @brief PBRマテリアルデータ
struct Material
{
    MaterialConstants constants = {
        { 1.0f, 1.0f, 1.0f, 1.0f },  // albedoFactor: 白
        0.0f,                          // metallic: 非金属
        0.5f,                          // roughness: 中程度
        1.0f,                          // aoStrength
        0.0f,                          // emissiveStrength
        { 0.0f, 0.0f, 0.0f },         // emissiveFactor
        0                              // flags: テクスチャ無し
    };

    // テクスチャハンドル（TextureManager）
    int albedoMapHandle    = -1;
    int normalMapHandle    = -1;
    int metRoughMapHandle  = -1;
    int aoMapHandle        = -1;
    int emissiveMapHandle  = -1;
};

/// @brief マテリアルマネージャー（ハンドル+freelist）
class MaterialManager
{
public:
    static constexpr uint32_t k_MaxMaterials = 256;

    MaterialManager() = default;
    ~MaterialManager() = default;

    /// マテリアルを作成してハンドルを返す
    int CreateMaterial(const Material& material);

    /// ハンドルからマテリアルを取得
    Material* GetMaterial(int handle);

    /// マテリアルを解放
    void ReleaseMaterial(int handle);

private:
    int AllocateHandle();

    struct MaterialEntry
    {
        Material material;
        bool     active = false;
    };

    std::vector<MaterialEntry> m_entries;
    std::vector<int>           m_freeHandles;
    int                        m_nextHandle = 0;
};

} // namespace GX
