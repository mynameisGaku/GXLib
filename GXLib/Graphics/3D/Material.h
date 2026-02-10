#pragma once
/// @file Material.h
/// @brief PBRマテリアル
/// 初学者向け: PBRは「現実っぽい光の反射」を目指す方式です。

#include "pch.h"

namespace GX
{

/// @brief マテリアル定数（GPU定数バッファ向けに配置）
struct MaterialConstants
{
    XMFLOAT4 albedoFactor;       // RGBA
    float    metallicFactor;     // 0-1
    float    roughnessFactor;    // 0-1
    float    aoStrength;         // 0-1
    float    emissiveStrength;   // 0-1
    XMFLOAT3 emissiveFactor;     // RGB
    uint32_t flags;              // MaterialFlags
};

/// @brief マテリアルのテクスチャ有無フラグ
namespace MaterialFlags
{
    constexpr uint32_t HasAlbedoMap   = 1 << 0;
    constexpr uint32_t HasNormalMap   = 1 << 1;
    constexpr uint32_t HasMetRoughMap = 1 << 2;
    constexpr uint32_t HasAOMap       = 1 << 3;
    constexpr uint32_t HasEmissiveMap = 1 << 4;
}

/// @brief MaterialManager::SetTextureで使うテクスチャスロット
enum class MaterialTextureSlot
{
    Albedo,
    Normal,
    MetalRoughness,
    AO,
    Emissive
};

/// @brief PBRマテリアルデータ
struct Material
{
    MaterialConstants constants = {
        { 1.0f, 1.0f, 1.0f, 1.0f }, // albedo
        0.0f,                      // metallic
        0.5f,                      // roughness
        1.0f,                      // AO strength
        0.0f,                      // emissive strength
        { 0.0f, 0.0f, 0.0f },       // emissive factor
        0                          // flags
    };

    int albedoMapHandle   = -1;
    int normalMapHandle   = -1;
    int metRoughMapHandle = -1;
    int aoMapHandle       = -1;
    int emissiveMapHandle = -1;

    int shaderHandle      = -1; // 任意のシェーダー上書き
};

/// @brief マテリアル管理（ハンドル + フリーリスト）
class MaterialManager
{
public:
    static constexpr uint32_t k_MaxMaterials = 256;

    MaterialManager() = default;
    ~MaterialManager() = default;

    int CreateMaterial(const Material& material);
    Material* GetMaterial(int handle);
    void ReleaseMaterial(int handle);

    bool SetTexture(int handle, MaterialTextureSlot slot, int textureHandle);
    bool SetShaderHandle(int handle, int shaderHandle);

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
