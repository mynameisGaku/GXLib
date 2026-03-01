#pragma once
/// @file shader_model.h
/// @brief シェーダーモデル定義とマテリアルパラメータ構造体
///
/// 全シェーダーモデル共通の256Bパラメータブロック(ShaderModelParams)を定義する。
/// GXMDバイナリのMaterialChunkや、ランタイムのcbuffer(b3)で使用される。

#include <cstdint>
#include <cstring>

namespace gxfmt
{

/// @brief シェーダーモデルの種類
/// @details マテリアルの描画手法を決定する。ShaderRegistryでPSO選択に使われる。
enum class ShaderModel : uint32_t
{
    Standard   = 0,   ///< PBR (metallic-roughness)
    Unlit      = 1,   ///< ライティングなし
    Toon       = 2,   ///< UTS2ベースのセルシェーディング
    Phong      = 3,   ///< Blinn-Phongスペキュラ
    Subsurface = 4,   ///< サブサーフェス散乱
    ClearCoat  = 5,   ///< クリアコート付きPBR
    Custom     = 255, ///< カスタムシェーダー
};

/// @brief アルファブレンドモード
enum class AlphaMode : uint8_t
{
    Opaque = 0,  ///< 不透明
    Mask   = 1,  ///< alphaCutoffで切り抜き
    Blend  = 2,  ///< 半透明ブレンド
};

/// @brief 全シェーダーモデル共通の256Bパラメータブロック
/// @details 各シェーダーモデルが使うフィールドは排他的に領域を共有する。
///          cbuffer(b3)に直接マップされるため、サイズは厳密に256B固定。
struct ShaderModelParams
{
    // ===== 共通フィールド (offset 0..71) =====
    float    baseColor[4]       = { 1.0f, 1.0f, 1.0f, 1.0f };  // 0:  ベースカラー (RGBA)
    float    emissiveFactor[3]  = { 0.0f, 0.0f, 0.0f };         // 16: 発光色 (RGB)
    float    emissiveStrength   = 0.0f;                          // 28: 発光強度
    float    alphaCutoff        = 0.5f;                          // 32: アルファマスク閾値
    AlphaMode alphaMode         = AlphaMode::Opaque;             // 36: アルファモード
    uint8_t  doubleSided        = 0;                             // 37: 両面描画フラグ
    uint8_t  _pad0[2]           = {};                            // 38: パディング
    int32_t  textureNames[8]    = { -1,-1,-1,-1,-1,-1,-1,-1 };   // 40: 文字列テーブルオフセット (-1=未使用)
    // [0]=albedo/diffuse, [1]=normal, [2]=metalRoughness/specular,
    // [3]=AO, [4]=emissive, [5]=toonRamp, [6]=clearCoatNormal, [7]=reserved

    // ===== Standard PBR (offset 72..91) =====
    float    metallic           = 0.0f;                          // 72: 金属度 (0=非金属, 1=金属)
    float    roughness          = 0.5f;                          // 76: 粗さ (0=鏡面, 1=拡散)
    float    reflectance        = 0.5f;                          // 80: 反射率 (F0制御)
    float    normalScale        = 1.0f;                          // 84: 法線マップ強度
    float    aoStrength         = 1.0f;                          // 88: AO適用強度

    // ===== Toon UTS2 (offset 92..163) =====
    float    shadeColor[4]      = { 0.7f, 0.7f, 0.7f, 1.0f };  // 92:  1stシェードカラー
    float    shade2ndColor[4]   = { 0.3f, 0.3f, 0.3f, 1.0f };  // 108: 2ndシェードカラー
    float    baseColorStep      = 0.5f;                          // 124: ベース→1stシェード閾値
    float    baseShadeFeather   = 0.1f;                          // 128: ベース→1stシェードぼかし幅
    float    shadeColorStep     = 0.2f;                          // 132: 1st→2ndシェード閾値
    float    shade1st2ndFeather = 0.05f;                         // 136: 1st→2ndシェードぼかし幅
    float    rimColor[4]        = { 1.0f, 1.0f, 1.0f, 1.0f };  // 140: リムライト色 (RGBA)
    float    rimPower           = 3.0f;                          // 156: リムライトの鋭さ
    float    rimIntensity       = 0.0f;                          // 160: リムライト強度

    // ===== Phong (offset 164..179) — Toonと排他使用 =====
    float    specularColor[3]   = { 1.0f, 1.0f, 1.0f };         // 164: スペキュラ色
    float    shininess          = 32.0f;                         // 176: スペキュラ指数

    // ===== Subsurface (offset 180..203) — Toonと排他使用 =====
    float    subsurfaceColor[3] = { 1.0f, 0.2f, 0.1f };         // 180: 表面下散乱色
    float    subsurfaceRadius   = 1.0f;                          // 192: 散乱半径
    float    subsurfaceStrength = 0.0f;                          // 196: 散乱強度
    float    thickness          = 0.5f;                          // 200: 厚み

    // ===== ClearCoat (offset 204..211) =====
    float    clearCoatStrength  = 0.0f;                          // 204: クリアコート強度
    float    clearCoatRoughness = 0.0f;                          // 208: クリアコート粗さ

    // ===== Toon拡張 (offset 212..255) =====
    float    outlineWidth       = 0.0f;                          // 212: アウトライン幅
    float    outlineColor[3]    = { 0.0f, 0.0f, 0.0f };         // 216: アウトライン色 (RGB)
    float    highColor[3]       = { 1.0f, 1.0f, 1.0f };         // 228: ハイカラー (Toonスペキュラ色)
    float    highColorPower     = 50.0f;                         // 240: ハイカラーべき指数
    float    highColorIntensity = 0.0f;                          // 244: ハイカラー強度
    float    shadowReceiveLevel = 1.0f;                          // 248: CSMシャドウ影響度
    float    rimInsideMask      = 0.5f;                          // 252: リムライト内側マスク閾値

    // ===== Toon拡張エイリアス (Phong/SS/CC領域を再利用、排他) =====
    inline float& toonRimLightDirMask()       { return specularColor[0]; } ///< リムライト方向制限
    inline float& toonRimFeatherOff()         { return specularColor[1]; } ///< リムフェザーオフ (step切替)
    inline float& toonHighColorBlendAdd()     { return specularColor[2]; } ///< ハイカラー加算ブレンドフラグ
    inline float& toonHighColorOnShadow()     { return shininess; }        ///< シャドウ上のハイカラー有効フラグ
    inline float& toonOutlineFarDist()        { return subsurfaceColor[0]; } ///< アウトライン遠距離
    inline float& toonOutlineNearDist()       { return subsurfaceColor[1]; } ///< アウトライン近距離
    inline float& toonOutlineBlendBaseColor() { return subsurfaceColor[2]; } ///< アウトラインにベースカラー混合する度合
    inline const float& toonRimLightDirMask()       const { return specularColor[0]; }
    inline const float& toonRimFeatherOff()         const { return specularColor[1]; }
    inline const float& toonHighColorBlendAdd()     const { return specularColor[2]; }
    inline const float& toonHighColorOnShadow()     const { return shininess; }
    inline const float& toonOutlineFarDist()        const { return subsurfaceColor[0]; }
    inline const float& toonOutlineNearDist()       const { return subsurfaceColor[1]; }
    inline const float& toonOutlineBlendBaseColor() const { return subsurfaceColor[2]; }
};

static_assert(sizeof(ShaderModelParams) == 256, "ShaderModelParams must be exactly 256 bytes");

/// @brief 指定シェーダーモデルのデフォルトパラメータを返す
/// @param model シェーダーモデルの種類
/// @return モデル固有のデフォルト値が設定されたShaderModelParams
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
        p.outlineWidth = 0.002f;
        p.highColorPower = 50.0f;
        p.highColorIntensity = 0.0f;
        p.shadowReceiveLevel = 1.0f;
        p.rimInsideMask = 0.5f;
        // Toon extended aliases
        p.toonRimLightDirMask()       = 0.0f;
        p.toonRimFeatherOff()         = 0.0f;
        p.toonHighColorBlendAdd()     = 1.0f;
        p.toonHighColorOnShadow()     = 1.0f;
        p.toonOutlineFarDist()        = 100.0f;
        p.toonOutlineNearDist()       = 0.5f;
        p.toonOutlineBlendBaseColor() = 0.0f;
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

/// @brief 指定シェーダーモデルが使用するテクスチャスロットのビットマスクを返す
/// @param model シェーダーモデルの種類
/// @return ビットマスク (bit0=albedo, bit1=normal, bit2=metRough, bit3=AO, bit4=emissive, bit5=toonRamp, bit6=clearCoatNormal)
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

/// @brief ShaderModelを文字列に変換する
/// @param model シェーダーモデルの種類
/// @return 静的文字列ポインタ ("Standard", "Unlit"等)
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

/// @brief 文字列からShaderModelを判定する (大文字小文字を区別しない)
/// @param str シェーダーモデル名 ("standard", "toon"等)
/// @return 対応するShaderModel。不明な場合はStandard
inline ShaderModel ShaderModelFromString(const char* str)
{
    if (!str) return ShaderModel::Standard;
    // 先頭1〜2文字で高速判定
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
