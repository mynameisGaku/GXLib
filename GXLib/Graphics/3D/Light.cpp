/// @file Light.cpp
/// @brief ライト作成ヘルパーの実装
#include "pch.h"
#include "Graphics/3D/Light.h"

namespace GX
{

LightData Light::CreateDirectional(const XMFLOAT3& direction, const XMFLOAT3& color, float intensity)
{
    LightData light = {};
    light.type      = static_cast<uint32_t>(LightType::Directional);
    light.color     = color;
    light.intensity = intensity;

    // 方向を正規化
    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&direction));
    XMStoreFloat3(&light.direction, dir);

    return light;
}

LightData Light::CreatePoint(const XMFLOAT3& position, float range, const XMFLOAT3& color, float intensity)
{
    LightData light = {};
    light.type      = static_cast<uint32_t>(LightType::Point);
    light.position  = position;
    light.range     = range;
    light.color     = color;
    light.intensity = intensity;

    return light;
}

LightData Light::CreateSpot(const XMFLOAT3& position, const XMFLOAT3& direction,
                              float range, float spotAngleDeg,
                              const XMFLOAT3& color, float intensity)
{
    LightData light = {};
    light.type      = static_cast<uint32_t>(LightType::Spot);
    light.position  = position;
    light.range     = range;
    light.color     = color;
    light.intensity = intensity;
    light.spotAngle = cosf(XMConvertToRadians(spotAngleDeg * 0.5f));

    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&direction));
    XMStoreFloat3(&light.direction, dir);

    return light;
}

} // namespace GX
