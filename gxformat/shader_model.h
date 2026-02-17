#pragma once
/// @file shader_model.h
/// @brief Shader model definitions for gxformat materials

#include <cstdint>
#include <cstring>

namespace gxfmt
{

enum class ShaderModel : uint32_t
{
    Standard   = 0,
    Unlit      = 1,
    Toon       = 2,
    Phong      = 3,
    Subsurface = 4,
    ClearCoat  = 5,
    Custom     = 255,
};

enum class AlphaMode : uint8_t
{
    Opaque = 0,
    Mask   = 1,
    Blend  = 2,
};

/// 256-byte fixed-size shader model parameters block.
/// Layout is a union of all shader model variants.
struct ShaderModelParams
{
    // --- Common fields (offset 0..63) ---
    float    baseColor[4]       = { 1.0f, 1.0f, 1.0f, 1.0f };  // 0
    float    emissiveFactor[3]  = { 0.0f, 0.0f, 0.0f };         // 16
    float    emissiveStrength   = 0.0f;                          // 28
    float    alphaCutoff        = 0.5f;                          // 32
    AlphaMode alphaMode         = AlphaMode::Opaque;             // 36
    uint8_t  doubleSided        = 0;                             // 37
    uint8_t  _pad0[2]           = {};                            // 38
    int32_t  textureNames[8]    = { -1,-1,-1,-1,-1,-1,-1,-1 };   // 40 (StringTable offsets)
    // [0]=albedo/diffuse, [1]=normal, [2]=metalRoughness/specular,
    // [3]=AO, [4]=emissive, [5]=toonRamp, [6]=clearCoatNormal, [7]=reserved

    // --- Standard PBR (offset 72..91) ---
    float    metallic           = 0.0f;                          // 72
    float    roughness          = 0.5f;                          // 76
    float    reflectance        = 0.5f;                          // 80
    float    normalScale        = 1.0f;                          // 84
    float    aoStrength         = 1.0f;                          // 88

    // --- Toon (offset 92..155) ---
    float    shadeColor[4]      = { 0.5f, 0.5f, 0.5f, 1.0f };  // 92
    float    outlineWidth       = 0.0f;                          // 108
    float    outlineColor[4]    = { 0.0f, 0.0f, 0.0f, 1.0f };  // 112
    float    rampThreshold      = 0.5f;                          // 128
    float    rampSmoothing      = 0.1f;                          // 132
    uint32_t rampBands          = 2;                             // 136
    float    rimColor[4]        = { 1.0f, 1.0f, 1.0f, 1.0f };  // 140
    float    rimPower           = 3.0f;                          // 156
    float    rimIntensity       = 0.0f;                          // 160

    // --- Phong (offset 164..179) ---
    float    specularColor[3]   = { 1.0f, 1.0f, 1.0f };         // 164
    float    shininess          = 32.0f;                         // 176

    // --- Subsurface (offset 180..203) ---
    float    subsurfaceColor[3] = { 1.0f, 0.2f, 0.1f };         // 180
    float    subsurfaceRadius   = 1.0f;                          // 192
    float    subsurfaceStrength = 0.0f;                          // 196
    float    thickness          = 0.5f;                          // 200

    // --- ClearCoat (offset 204..211) ---
    float    clearCoatStrength  = 0.0f;                          // 204
    float    clearCoatRoughness = 0.0f;                          // 208

    // --- Padding to 256 bytes ---
    uint8_t  _reserved[44]     = {};                             // 212..255
};

static_assert(sizeof(ShaderModelParams) == 256, "ShaderModelParams must be exactly 256 bytes");

inline ShaderModelParams DefaultShaderModelParams(ShaderModel model)
{
    ShaderModelParams p{};
    switch (model)
    {
    case ShaderModel::Standard:
        p.metallic = 0.0f;
        p.roughness = 0.5f;
        break;
    case ShaderModel::Unlit:
        break;
    case ShaderModel::Toon:
        p.rampBands = 2;
        p.outlineWidth = 0.002f;
        break;
    case ShaderModel::Phong:
        p.shininess = 32.0f;
        break;
    case ShaderModel::Subsurface:
        p.subsurfaceStrength = 0.5f;
        break;
    case ShaderModel::ClearCoat:
        p.clearCoatStrength = 1.0f;
        p.clearCoatRoughness = 0.04f;
        break;
    default:
        break;
    }
    return p;
}

/// Returns a bitmask of which textureNames[] slots are used for the given shader model.
inline uint32_t GetUsedTextureSlots(ShaderModel model)
{
    // bit0=albedo, bit1=normal, bit2=metRough/specular, bit3=AO, bit4=emissive,
    // bit5=toonRamp, bit6=clearCoatNormal
    switch (model)
    {
    case ShaderModel::Standard:   return 0b00011111; // albedo, normal, metRough, AO, emissive
    case ShaderModel::Unlit:      return 0b00000001; // albedo only
    case ShaderModel::Toon:       return 0b00100011; // albedo, normal, toonRamp
    case ShaderModel::Phong:      return 0b00000111; // albedo, normal, specular
    case ShaderModel::Subsurface: return 0b00011111; // albedo, normal, metRough, AO, emissive
    case ShaderModel::ClearCoat:  return 0b01011111; // all + clearCoatNormal
    default:                      return 0b11111111;
    }
}

inline const char* ShaderModelToString(ShaderModel model)
{
    switch (model)
    {
    case ShaderModel::Standard:   return "Standard";
    case ShaderModel::Unlit:      return "Unlit";
    case ShaderModel::Toon:       return "Toon";
    case ShaderModel::Phong:      return "Phong";
    case ShaderModel::Subsurface: return "Subsurface";
    case ShaderModel::ClearCoat:  return "ClearCoat";
    case ShaderModel::Custom:     return "Custom";
    default:                      return "Unknown";
    }
}

inline ShaderModel ShaderModelFromString(const char* str)
{
    if (!str) return ShaderModel::Standard;
    // Case-insensitive first-char check for speed
    auto lower = [](char c) -> char { return (c >= 'A' && c <= 'Z') ? c + 32 : c; };
    if (lower(str[0]) == 'u') return ShaderModel::Unlit;
    if (lower(str[0]) == 't') return ShaderModel::Toon;
    if (lower(str[0]) == 'p') return ShaderModel::Phong;
    if (lower(str[0]) == 'c')
    {
        if (lower(str[1]) == 'l') return ShaderModel::ClearCoat;
        if (lower(str[1]) == 'u') return ShaderModel::Custom;
    }
    if (lower(str[0]) == 's')
    {
        if (lower(str[1]) == 'u') return ShaderModel::Subsurface;
        return ShaderModel::Standard;
    }
    return ShaderModel::Standard;
}

} // namespace gxfmt
