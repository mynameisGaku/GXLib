#pragma once
/// @file Light.h
/// @brief ライトデータ構造体（Directional, Point, Spot）

#include "pch_graphics.h"
#include "Math/Vector3.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief ライトの種類（DxLibの SetLightTypeHandle に相当）
enum class LightType : uint32_t
{
    Directional = 0,  ///< 平行光源（太陽光など、方向のみ指定）
    Point       = 1,  ///< 点光源（電球など、位置と到達距離を指定）
    Spot        = 2,  ///< スポットライト（位置・方向・角度・到達距離を指定）
};

/// @brief GPU転送用ライトデータ（64バイト、cbufferパッキング対応）
struct LightData
{
    Vector3  position;         ///< ライト位置（Point/Spot用）
    float    range;            ///< 到達距離（Point/Spot用）
    Vector3  direction;        ///< ライト方向（Directional/Spot用）
    float    spotAngle;        ///< スポット角度（cos値で格納）
    Vector3  color;            ///< ライト色 (RGB)
    float    intensity;        ///< 強度倍率
    uint32_t type;             ///< LightType 列挙値
    float    padding[3];       ///< 64バイト境界へのパディング
};

/// @brief ライト定数バッファ（b2スロット）
/// クラスタードライティング有効時は最大256灯、従来パスでは16灯
struct LightConstants
{
    static constexpr uint32_t k_MaxLights = 256;
    static constexpr uint32_t k_MaxLightsLegacy = 16;

    LightData lights[k_MaxLights];  ///< ライト配列
    Vector3   ambientColor;         ///< 環境光の色
    uint32_t  numLights;            ///< 有効なライト数
};

/// @brief シェーダー用ライト定数 (HLSL cbuffer b2 と同一レイアウト, 16ライト)
///
/// LightConstants は k_MaxLights=256 (クラスタードライティング用 StructuredBuffer) だが、
/// HLSL cbuffer は gLights[16] 固定。CBV アップロード時はこの構造体を使うこと。
struct ShaderLightConstants
{
    static constexpr uint32_t k_MaxLights = 16;
    LightData lights[k_MaxLights];  ///< 16×64 = 1024B
    Vector3   ambientColor;         ///< 環境光の色 (12B)
    uint32_t  numLights;            ///< 有効なライト数 (4B)
};  // Total: 1040B → 256-align = 1280B

/// @brief ライトデータのファクトリ（DxLibの SetLightDirection / SetLightDifColor に相当）
class Light
{
public:
    /// @brief ディレクショナルライト（平行光源）を作成する
    /// @param direction 光の方向ベクトル（自動正規化）
    /// @param color 光の色 (RGB)
    /// @param intensity 強度倍率
    /// @return 設定済みの LightData
    static LightData CreateDirectional(const Vector3& direction, const Vector3& color, float intensity);

    /// @brief ポイントライト（点光源）を作成する
    /// @param position 光源の位置
    /// @param range 到達距離
    /// @param color 光の色 (RGB)
    /// @param intensity 強度倍率
    /// @return 設定済みの LightData
    static LightData CreatePoint(const Vector3& position, float range, const Vector3& color, float intensity);

    /// @brief スポットライトを作成する
    /// @param position 光源の位置
    /// @param direction 照射方向（自動正規化）
    /// @param range 到達距離
    /// @param spotAngleDeg スポット角度（度数法、内側コーン角の半径）
    /// @param color 光の色 (RGB)
    /// @param intensity 強度倍率
    /// @return 設定済みの LightData
    static LightData CreateSpot(const Vector3& position, const Vector3& direction,
                                 float range, float spotAngleDeg,
                                 const Vector3& color, float intensity);
};

/// @}
} // namespace gx
