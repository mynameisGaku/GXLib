#pragma once
/// @file ShaderModelConstants.h
/// @brief GPU定数バッファ用シェーダーモデルパラメータ（256B, 16Bアライン）

#include "pch.h"
#include <gxformat/shader_model.h>

namespace GX
{

/// @brief GPU定数バッファ用のシェーダーモデルパラメータ（b3スロット、256B）
/// HLSLの ShaderModelCommon.hlsli と完全に一致する必要がある。
struct ShaderModelGPUParams
{
    // --- 共通 (0..63) ---
    XMFLOAT4 baseColor;           //  0: albedo RGBA
    XMFLOAT3 emissiveFactor;      // 16: emissive RGB
    float    emissiveStrength;    // 28
    float    alphaCutoff;         // 32
    uint32_t shaderModelId;       // 36: ShaderModel enum value
    uint32_t flags;               // 40: MaterialFlags
    float    normalScale;         // 44
    float    metallic;            // 48
    float    roughness;           // 52
    float    aoStrength;          // 56
    float    reflectance;         // 60

    // --- Toon (64..127) ---
    XMFLOAT4 shadeColor;          // 64
    float    outlineWidth;        // 80
    XMFLOAT3 outlineColor;        // 84 (packed float3)
    float    rampThreshold;       // 96
    float    rampSmoothing;       // 100
    uint32_t rampBands;           // 104
    float    _toonPad;            // 108
    XMFLOAT4 rimColor;            // 112
    float    rimPower;            // 128
    float    rimIntensity;        // 132
    float    _toonPad2[2];        // 136

    // --- Phong (144..159) ---
    XMFLOAT3 specularColor;       // 144
    float    shininess;           // 156

    // --- Subsurface (160..191) ---
    XMFLOAT3 subsurfaceColor;     // 160
    float    subsurfaceRadius;    // 172
    float    subsurfaceStrength;  // 176
    float    thickness;           // 180
    float    _ssPad[2];           // 184

    // --- ClearCoat (192..207) ---
    float    clearCoatStrength;   // 192
    float    clearCoatRoughness;  // 196
    float    _ccPad[2];           // 200

    // --- Reserved (208..255) ---
    float    _reserved[12];       // 208
};

static_assert(sizeof(ShaderModelGPUParams) == 256, "ShaderModelGPUParams must be exactly 256 bytes");

/// @brief gxfmt::ShaderModelParams → ShaderModelGPUParams 変換
inline ShaderModelGPUParams ConvertToGPUParams(const gxfmt::ShaderModelParams& src,
                                                gxfmt::ShaderModel model,
                                                uint32_t materialFlags)
{
    ShaderModelGPUParams dst = {};

    // 共通
    dst.baseColor       = { src.baseColor[0], src.baseColor[1], src.baseColor[2], src.baseColor[3] };
    dst.emissiveFactor  = { src.emissiveFactor[0], src.emissiveFactor[1], src.emissiveFactor[2] };
    dst.emissiveStrength = src.emissiveStrength;
    dst.alphaCutoff     = src.alphaCutoff;
    dst.shaderModelId   = static_cast<uint32_t>(model);
    dst.flags           = materialFlags;
    dst.normalScale     = src.normalScale;
    dst.metallic        = src.metallic;
    dst.roughness       = src.roughness;
    dst.aoStrength      = src.aoStrength;
    dst.reflectance     = src.reflectance;

    // Toon
    dst.shadeColor      = { src.shadeColor[0], src.shadeColor[1], src.shadeColor[2], src.shadeColor[3] };
    dst.outlineWidth    = src.outlineWidth;
    dst.outlineColor    = { src.outlineColor[0], src.outlineColor[1], src.outlineColor[2] };
    dst.rampThreshold   = src.rampThreshold;
    dst.rampSmoothing   = src.rampSmoothing;
    dst.rampBands       = src.rampBands;
    dst.rimColor        = { src.rimColor[0], src.rimColor[1], src.rimColor[2], src.rimColor[3] };
    dst.rimPower        = src.rimPower;
    dst.rimIntensity    = src.rimIntensity;

    // Phong
    dst.specularColor   = { src.specularColor[0], src.specularColor[1], src.specularColor[2] };
    dst.shininess       = src.shininess;

    // Subsurface
    dst.subsurfaceColor    = { src.subsurfaceColor[0], src.subsurfaceColor[1], src.subsurfaceColor[2] };
    dst.subsurfaceRadius   = src.subsurfaceRadius;
    dst.subsurfaceStrength = src.subsurfaceStrength;
    dst.thickness          = src.thickness;

    // ClearCoat
    dst.clearCoatStrength  = src.clearCoatStrength;
    dst.clearCoatRoughness = src.clearCoatRoughness;

    return dst;
}

/// @brief 後方互換: MaterialConstants → ShaderModelGPUParams 変換（Standard用）
inline ShaderModelGPUParams ConvertFromLegacy(const struct MaterialConstants& legacy)
{
    ShaderModelGPUParams dst = {};

    dst.baseColor       = legacy.albedoFactor;
    dst.emissiveFactor  = legacy.emissiveFactor;
    dst.emissiveStrength = legacy.emissiveStrength;
    dst.shaderModelId   = 0; // Standard
    dst.flags           = legacy.flags;
    dst.normalScale     = 1.0f;
    dst.metallic        = legacy.metallicFactor;
    dst.roughness       = legacy.roughnessFactor;
    dst.aoStrength      = legacy.aoStrength;
    dst.reflectance     = 0.5f;
    dst.alphaCutoff     = 0.5f;

    return dst;
}

} // namespace GX
