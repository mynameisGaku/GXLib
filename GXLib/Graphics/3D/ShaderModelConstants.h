#pragma once
/// @file ShaderModelConstants.h
/// @brief GPU定数バッファ用シェーダーモデルパラメータ（256B, 16Bアライン）

#include "pch.h"
#include <gxformat/shader_model.h>

namespace GX
{

/// @brief GPU定数バッファ用のシェーダーモデルパラメータ（b3スロット、256B固定）
/// HLSL側の ShaderModelCommon.hlsli の cbuffer レイアウトと完全一致が必須。
/// 6種類のシェーダーモデル（Standard/Unlit/Toon/Phong/Subsurface/ClearCoat）の
/// パラメータを256B内に共存させ、Toon用にPhong/SS/CC領域をエイリアスとして再利用する
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

    // --- Toon (64..143) --- UTS2-style double shade
    XMFLOAT4 shadeColor;          // 64:  1st shade color
    XMFLOAT4 shade2ndColor;       // 80:  2nd shade color
    float    baseColorStep;       // 96:  base->1st threshold
    float    baseShadeFeather;    // 100: base->1st feather
    float    shadeColorStep;      // 104: 1st->2nd threshold
    float    shade1st2ndFeather;  // 108: 1st->2nd feather
    XMFLOAT4 rimColor;            // 112
    float    rimPower;            // 128
    float    rimIntensity;        // 132
    float    highColorPower;      // 136: specular power
    float    highColorIntensity;  // 140: specular intensity

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

    // --- Toon Extended (208..255) ---
    float    outlineWidth;        // 208
    XMFLOAT3 outlineColor;        // 212
    XMFLOAT3 highColor;           // 224: specular color
    float    shadowReceiveLevel;  // 236: CSM shadow influence
    float    rimInsideMask;       // 240: rim shadow mask
    float    _toonReserved[3];    // 244..255
};

static_assert(sizeof(ShaderModelGPUParams) == 256, "ShaderModelGPUParams must be exactly 256 bytes");

/// @brief gxfmt::ShaderModelParams から GPU定数バッファ形式に変換する
/// @param src アセットパイプラインのパラメータ
/// @param model シェーダーモデル種別（Toon時はPhong/SS/CC領域にエイリアスを格納）
/// @param materialFlags テクスチャ有無ビットフラグ
/// @return GPU送信用の256Bパラメータ
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

    // Toon (UTS2 double shade)
    dst.shadeColor        = { src.shadeColor[0], src.shadeColor[1], src.shadeColor[2], src.shadeColor[3] };
    dst.shade2ndColor     = { src.shade2ndColor[0], src.shade2ndColor[1], src.shade2ndColor[2], src.shade2ndColor[3] };
    dst.baseColorStep     = src.baseColorStep;
    dst.baseShadeFeather  = src.baseShadeFeather;
    dst.shadeColorStep    = src.shadeColorStep;
    dst.shade1st2ndFeather = src.shade1st2ndFeather;
    dst.rimColor          = { src.rimColor[0], src.rimColor[1], src.rimColor[2], src.rimColor[3] };
    dst.rimPower          = src.rimPower;
    dst.rimIntensity      = src.rimIntensity;
    dst.highColorPower    = src.highColorPower;
    dst.highColorIntensity = src.highColorIntensity;

    // Toon Extended
    dst.outlineWidth       = src.outlineWidth;
    dst.outlineColor       = { src.outlineColor[0], src.outlineColor[1], src.outlineColor[2] };
    dst.highColor          = { src.highColor[0], src.highColor[1], src.highColor[2] };
    dst.shadowReceiveLevel = src.shadowReceiveLevel;
    dst.rimInsideMask      = src.rimInsideMask;

    // Toon はPhong/SS/CC領域をUTS2拡張パラメータのエイリアスとして再利用する
    // cbuffer 256B制約のためにこの方式を採用している
    if (model == gxfmt::ShaderModel::Toon)
    {
        dst.specularColor   = { src.toonRimLightDirMask(), src.toonRimFeatherOff(), src.toonHighColorBlendAdd() };
        dst.shininess       = src.toonHighColorOnShadow();
        dst.subsurfaceColor = { src.toonOutlineFarDist(), src.toonOutlineNearDist(), src.toonOutlineBlendBaseColor() };
    }
    else
    {
        dst.specularColor   = { src.specularColor[0], src.specularColor[1], src.specularColor[2] };
        dst.shininess       = src.shininess;
        dst.subsurfaceColor = { src.subsurfaceColor[0], src.subsurfaceColor[1], src.subsurfaceColor[2] };
    }
    dst.subsurfaceRadius   = src.subsurfaceRadius;
    dst.subsurfaceStrength = src.subsurfaceStrength;
    dst.thickness          = src.thickness;

    // ClearCoat
    dst.clearCoatStrength  = src.clearCoatStrength;
    dst.clearCoatRoughness = src.clearCoatRoughness;

    return dst;
}

/// @brief 後方互換: 旧MaterialConstants から ShaderModelGPUParams に変換する（Standard用）
/// @param legacy 旧形式のマテリアル定数
/// @return GPU送信用の256Bパラメータ（Standard シェーダーモデル）
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
