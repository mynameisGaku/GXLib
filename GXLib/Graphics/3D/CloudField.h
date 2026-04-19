#pragma once
/// @file CloudField.h
/// @brief CloudField — N 層 CloudLayer + WeatherMap + AtmosphereLUT aggregate (ADR-0021).
///
/// ADR-0021 §Decision 1 foundation aggregator。
/// `AtmosphereLUT*` は Phase D.0 で実体化、Phase B 時点では nullptr 保持。
/// `SetEnabled(false)` で shipping-build 時に clouds を完全無効化 (§8 disable path)。
///
/// @addtogroup grp_gfx_3d
/// @{

#include "pch_common.h"
#include "Graphics/3D/CloudLayer.h"
#include "Graphics/3D/WeatherMap.h"

namespace gx
{

/// @brief Atmosphere LUT interface (Phase D.0 で実装、Phase B では forward decl のみ)
class IAtmosphereLUT;

/// @brief Cloud field aggregate (ADR-0021 §Decision 1)
class CloudField
{
public:
    /// @brief Layer 最大数 (ADR-0021 MED tier 3、HIGH tier も 3、LOW tier 2。将来 ULTRA は +1 程度)
    static constexpr size_t k_MaxLayers = 8;

    CloudField() = default;
    ~CloudField() = default;
    CloudField(const CloudField&) = delete;
    CloudField& operator=(const CloudField&) = delete;

    /// @brief Layer を追加 (full なら false)
    bool AddLayer(const CloudLayer& layer)
    {
        if (m_layers.size() >= k_MaxLayers) return false;
        m_layers.push_back(layer);
        return true;
    }

    /// @brief Layer 取得 (index bounds out は first layer を返す、空なら undefined)
    CloudLayer& GetLayer(size_t index)
    {
        if (index >= m_layers.size())
        {
            static CloudLayer fallback;
            return fallback;
        }
        return m_layers[index];
    }

    const CloudLayer& GetLayer(size_t index) const
    {
        if (index >= m_layers.size())
        {
            static const CloudLayer fallback;
            return fallback;
        }
        return m_layers[index];
    }

    /// @brief Layer 数
    size_t GetLayerCount() const { return m_layers.size(); }

    /// @brief 全 layer 削除
    void ClearLayers() { m_layers.clear(); }

    /// @brief WeatherMap アクセス
    WeatherMap& GetWeatherMap() { return m_weatherMap; }
    const WeatherMap& GetWeatherMap() const { return m_weatherMap; }

    /// @brief Atmosphere LUT 設定 (Phase D.0 で実体化)
    void SetAtmosphereLUT(IAtmosphereLUT* lut) { m_atmosphereLUT = lut; }
    IAtmosphereLUT* GetAtmosphereLUT() const { return m_atmosphereLUT; }

    /// @brief Enabled flag (ADR-0021 §8 disable path — 全 5 pass を FrameGraph から skip 可能)
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief Flight-combat デフォルト 3 層を生成
    /// Low stratus (800-1500m) + Mid cumulus (2000-4000m) + High cirrus (7000-11000m)
    void InitializeDefaultLayers()
    {
        m_layers.clear();

        // Low layer: stratus
        CloudLayer low;
        low.altitudeBottom = 800.0f;
        low.altitudeTop    = 1500.0f;
        low.dominantClass  = CloudClass::Stratus;
        low.curve          = HeightDensityCurve::Default(CloudClass::Stratus);
        low.animationSpeed = 1.0f;
        m_layers.push_back(low);

        // Mid layer: cumulus
        CloudLayer mid;
        mid.altitudeBottom = 2000.0f;
        mid.altitudeTop    = 4000.0f;
        mid.dominantClass  = CloudClass::Cumulus;
        mid.curve          = HeightDensityCurve::Default(CloudClass::Cumulus);
        mid.animationSpeed = 1.0f;
        m_layers.push_back(mid);

        // High layer: cirrus
        CloudLayer high;
        high.altitudeBottom = 7000.0f;
        high.altitudeTop    = 11000.0f;
        high.dominantClass  = CloudClass::Cirrus;
        high.curve          = HeightDensityCurve::Default(CloudClass::Cirrus);
        high.animationSpeed = 0.3f;  // 高高度 cirrus は遅い
        m_layers.push_back(high);
    }

private:
    gx::Vector<CloudLayer> m_layers;
    WeatherMap m_weatherMap;
    IAtmosphereLUT* m_atmosphereLUT = nullptr;  ///< Phase D.0 で実体化
    bool m_enabled = true;
};

} // namespace gx
/// @}
