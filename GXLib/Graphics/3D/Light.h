#pragma once
/// @file Light.h
/// @brief ライトデータ構造体（Directional, Point, Spot）

#include "pch.h"

namespace GX
{

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
    XMFLOAT3 position;         ///< ライト位置（Point/Spot用）
    float    range;            ///< 到達距離（Point/Spot用）
    XMFLOAT3 direction;        ///< ライト方向（Directional/Spot用）
    float    spotAngle;        ///< スポット角度（cos値で格納）
    XMFLOAT3 color;            ///< ライト色 (RGB)
    float    intensity;        ///< 強度倍率
    uint32_t type;             ///< LightType 列挙値
    float    padding[3];       ///< 64バイト境界へのパディング
};

/// @brief ライト定数バッファ（b2スロット、最大16灯＋アンビエント）
struct LightConstants
{
    static constexpr uint32_t k_MaxLights = 16;

    LightData lights[k_MaxLights];  ///< ライト配列
    XMFLOAT3  ambientColor;         ///< 環境光の色
    uint32_t  numLights;            ///< 有効なライト数
};

/// @brief ライトデータのファクトリ（DxLibの SetLightDirection / SetLightDifColor に相当）
class Light
{
public:
    /// @brief ディレクショナルライト（平行光源）を作成する
    /// @param direction 光の方向ベクトル（自動正規化）
    /// @param color 光の色 (RGB)
    /// @param intensity 強度倍率
    /// @return 設定済みの LightData
    static LightData CreateDirectional(const XMFLOAT3& direction, const XMFLOAT3& color, float intensity);

    /// @brief ポイントライト（点光源）を作成する
    /// @param position 光源の位置
    /// @param range 到達距離
    /// @param color 光の色 (RGB)
    /// @param intensity 強度倍率
    /// @return 設定済みの LightData
    static LightData CreatePoint(const XMFLOAT3& position, float range, const XMFLOAT3& color, float intensity);

    /// @brief スポットライトを作成する
    /// @param position 光源の位置
    /// @param direction 照射方向（自動正規化）
    /// @param range 到達距離
    /// @param spotAngleDeg スポット角度（度数法、内側コーン角の半径）
    /// @param color 光の色 (RGB)
    /// @param intensity 強度倍率
    /// @return 設定済みの LightData
    static LightData CreateSpot(const XMFLOAT3& position, const XMFLOAT3& direction,
                                 float range, float spotAngleDeg,
                                 const XMFLOAT3& color, float intensity);
};

} // namespace GX
