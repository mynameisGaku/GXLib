#pragma once
/// @file Material.h
/// @brief マテリアルデータとマテリアル管理（DxLibの MV1SetMaterialDifColor 等に相当）

#include "pch.h"
#include <gxformat/shader_model.h>

namespace GX
{

/// @brief マテリアル定数（後方互換用、ShaderModelGPUParams に変換されてGPUに送られる）
struct MaterialConstants
{
    XMFLOAT4 albedoFactor;       ///< アルベド（基本色）RGBA
    float    metallicFactor;     ///< 金属度 (0.0 = 非金属、1.0 = 完全金属)
    float    roughnessFactor;    ///< 粗さ (0.0 = 鏡面、1.0 = 完全拡散)
    float    aoStrength;         ///< アンビエントオクルージョン強度
    float    emissiveStrength;   ///< 自発光の強度
    XMFLOAT3 emissiveFactor;     ///< 自発光色 (RGB)
    uint32_t flags;              ///< MaterialFlags ビットフラグ
};

/// @brief マテリアルのテクスチャスロット有無を示すビットフラグ
/// シェーダー側で各テクスチャの有無を判定するために使う
namespace MaterialFlags
{
    constexpr uint32_t HasAlbedoMap        = 1 << 0;  ///< アルベドマップ (t0)
    constexpr uint32_t HasNormalMap        = 1 << 1;  ///< ノーマルマップ (t1)
    constexpr uint32_t HasMetRoughMap      = 1 << 2;  ///< メタリック/ラフネスマップ (t2)
    constexpr uint32_t HasAOMap            = 1 << 3;  ///< AOマップ (t3)
    constexpr uint32_t HasEmissiveMap      = 1 << 4;  ///< エミッシブマップ (t4)
    constexpr uint32_t HasToonRampMap      = 1 << 5;  ///< Toonランプテクスチャ (t5)
    constexpr uint32_t HasSubsurfaceMap    = 1 << 6;  ///< サブサーフェスマップ (t6)
    constexpr uint32_t HasClearCoatMaskMap = 1 << 7;  ///< クリアコートマスク (t7)
}

/// @brief テクスチャスロット指定用列挙（MaterialManager::SetTexture で使う）
enum class MaterialTextureSlot
{
    Albedo,
    Normal,
    MetalRoughness,
    AO,
    Emissive,
    ToonRamp,
    SubsurfaceMap,
    ClearCoatMask
};

/// @brief マテリアルデータ（DxLibの MV1SetMaterialDifColor 等でアクセスするマテリアル情報に相当）
/// テクスチャハンドル、シェーダーモデル、定数パラメータを保持する。
/// MaterialManager に登録してハンドルで管理する
struct Material
{
    MaterialConstants constants = {
        { 1.0f, 1.0f, 1.0f, 1.0f },  // albedo (白)
        0.0f,                         // metallic (非金属)
        0.5f,                         // roughness (中間)
        1.0f,                         // AO strength
        0.0f,                         // emissive strength
        { 0.0f, 0.0f, 0.0f },        // emissive factor
        0                             // flags
    };

    int albedoMapHandle        = -1;  ///< アルベドテクスチャハンドル
    int normalMapHandle        = -1;  ///< ノーマルマップハンドル
    int metRoughMapHandle      = -1;  ///< メタリック/ラフネスマップハンドル
    int aoMapHandle            = -1;  ///< AOマップハンドル
    int emissiveMapHandle      = -1;  ///< エミッシブマップハンドル
    int toonRampMapHandle      = -1;  ///< Toonランプテクスチャハンドル
    int subsurfaceMapHandle    = -1;  ///< サブサーフェスマップハンドル
    int clearCoatMaskMapHandle = -1;  ///< クリアコートマスクハンドル

    int shaderHandle      = -1;  ///< カスタムシェーダーハンドル（-1 = 既定PSO使用）

    /// シェーダーモデル（Standard/Unlit/Toon/Phong/Subsurface/ClearCoat）
    gxfmt::ShaderModel       shaderModel  = gxfmt::ShaderModel::Standard;
    /// シェーダーモデル固有パラメータ（256B、cbuffer b3 に送られる）
    gxfmt::ShaderModelParams shaderParams = {};
};

/// @brief マテリアル管理（ハンドルベース + フリーリスト方式）
/// TextureManager と同じパターンで、整数ハンドルでマテリアルを参照する
class MaterialManager
{
public:
    static constexpr uint32_t k_MaxMaterials = 256;  ///< 最大マテリアル数

    MaterialManager() = default;
    ~MaterialManager() = default;

    /// @brief マテリアルを登録してハンドルを返す
    /// @param material 登録するマテリアル
    /// @return 割り当てられたハンドル
    int CreateMaterial(const Material& material);

    /// @brief ハンドルからマテリアルを取得する
    /// @param handle マテリアルハンドル
    /// @return マテリアルへのポインタ（無効ハンドルならnullptr）
    Material* GetMaterial(int handle);

    /// @brief マテリアルを解放する（ハンドルはフリーリストに戻る）
    /// @param handle 解放するマテリアルのハンドル
    void ReleaseMaterial(int handle);

    /// @brief マテリアルのテクスチャを差し替える
    /// @param handle マテリアルハンドル
    /// @param slot テクスチャスロット
    /// @param textureHandle 新しいテクスチャハンドル（-1 で解除）
    /// @return 成功した場合true
    bool SetTexture(int handle, MaterialTextureSlot slot, int textureHandle);

    /// @brief マテリアルのカスタムシェーダーを設定する
    /// @param handle マテリアルハンドル
    /// @param shaderHandle シェーダーハンドル（-1 でデフォルトに戻す）
    /// @return 成功した場合true
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
