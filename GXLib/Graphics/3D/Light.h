#pragma once
/// @file Light.h
/// @brief ライトデータ構造体（Directional, Point, Spot）

#include "pch.h"

namespace GX
{

/// ライトタイプ
enum class LightType : uint32_t
{
    Directional = 0,
    Point       = 1,
    Spot        = 2,
};

/// GPU転送用ライトデータ（64バイトアラインメント）
struct LightData
{
    XMFLOAT3 position;         // Point/Spot用
    float    range;            // Point/Spot用
    XMFLOAT3 direction;        // Directional/Spot用
    float    spotAngle;        // Spot用（cosθ）
    XMFLOAT3 color;            // ライト色
    float    intensity;        // 強度
    uint32_t type;             // LightType
    float    padding[3];
};

/// ライト定数バッファ（b2）
struct LightConstants
{
    static constexpr uint32_t k_MaxLights = 16;

    LightData lights[k_MaxLights];
    XMFLOAT3  ambientColor;
    uint32_t  numLights;
};

/// @brief ライト管理ヘルパー
class Light
{
public:
    /// Directional Lightを作成
    static LightData CreateDirectional(const XMFLOAT3& direction, const XMFLOAT3& color, float intensity);

    /// Point Lightを作成
    static LightData CreatePoint(const XMFLOAT3& position, float range, const XMFLOAT3& color, float intensity);

    /// Spot Lightを作成
    static LightData CreateSpot(const XMFLOAT3& position, const XMFLOAT3& direction,
                                 float range, float spotAngleDeg,
                                 const XMFLOAT3& color, float intensity);
};

} // namespace GX
