#pragma once
/// @file CloudLayer.h
/// @brief Cloud layer / class / height-density curve (ADR-0021 Phase B foundation).
///
/// ADR-0021 §Decision 1 — multi-layer CloudField の構成要素。
/// 新 enum `CloudClass` は既存 `VolumetricClouds.h` の float API
/// (SetCloudType/GetCloudType) と名前衝突を避けるため別名。
///
/// @addtogroup grp_gfx_3d
/// @{

#include "pch_common.h"

namespace gx
{

/// @brief 雲の種別 (ADR-0021 TD review R3 — 既存 `SetCloudType(float)` と共存)
///
/// 物理的な雲分類 (meteorological cloud genera の簡略化)。
/// `HeightDensityCurve::Default(CloudClass)` で典型的な高度密度プロファイルを得る。
enum class CloudClass
{
    Stratus,          ///< 低高度、flat sheet、overcast skies
    Stratocumulus,    ///< 低高度、puffy patches
    Cumulus,          ///< 中高度、classical "fair weather" cotton puffs
    Cumulonimbus,     ///< 高背、anvil top、thunderstorm
    Cirrus,           ///< 高高度、wispy ice clouds
    Cirrocumulus,     ///< 高高度、fine-grained patches (mackerel sky)
    Altocumulus       ///< 中高度、sheet + roll patches
};

/// @brief 高度正規化 (0=layer底、1=layer頂) → density multiplier (0..1) curve
///
/// 現 Phase B 実装は analytic curve (3 params)。Phase D 以降、より複雑な
/// per-class profile が必要になれば table lookup に拡張する可能性。
///
/// Default() が各 CloudClass に対応する AC7-inspired デフォルトカーブを返す。
struct HeightDensityCurve
{
    /// @brief layer 底からのフェードイン距離 (normalized 0..1)
    /// 0 = 底から即最大密度、0.3 = 底から 30% で最大到達
    float bottomFadeHeight = 0.1f;

    /// @brief layer 頂へのフェードアウト開始距離 (normalized 0..1)
    /// 1.0 = 頂まで密度維持、0.7 = 70% から減衰開始
    float topFadeHeight = 0.9f;

    /// @brief 中間域の形状指数 (1 = linear、>1 = 底重 cumulus 型、<1 = 頂重 anvil 型)
    float shapeExponent = 1.0f;

    /// @brief normalizedHeight (0..1) で density multiplier (0..1) を返す
    /// @param normalizedHeight 0 = layer 底、1 = layer 頂
    /// @return density 倍率 (0..1)、底上頂下の平滑 bell
    float Sample(float normalizedHeight) const
    {
        if (normalizedHeight <= 0.0f || normalizedHeight >= 1.0f) return 0.0f;

        // Fade-in from bottom
        float fadeIn = normalizedHeight < bottomFadeHeight
            ? normalizedHeight / bottomFadeHeight
            : 1.0f;

        // Fade-out toward top
        float fadeOut = normalizedHeight > topFadeHeight
            ? (1.0f - normalizedHeight) / (1.0f - topFadeHeight)
            : 1.0f;

        // Combined + shape exponent (bias mid-density profile)
        float combined = fadeIn * fadeOut;
        if (shapeExponent != 1.0f)
            combined = std::pow(combined, shapeExponent);

        return std::clamp(combined, 0.0f, 1.0f);
    }

    /// @brief 指定 CloudClass の典型的プロファイルを返す (ADR-0021 AC7-inspired default)
    static HeightDensityCurve Default(CloudClass klass)
    {
        HeightDensityCurve c;
        switch (klass)
        {
        case CloudClass::Stratus:
            // 平坦 sheet: 全域フラット
            c.bottomFadeHeight = 0.05f;
            c.topFadeHeight = 0.95f;
            c.shapeExponent = 1.0f;
            break;
        case CloudClass::Stratocumulus:
            // 塊 sheet: やや底重
            c.bottomFadeHeight = 0.1f;
            c.topFadeHeight = 0.85f;
            c.shapeExponent = 1.2f;
            break;
        case CloudClass::Cumulus:
            // 古典 cotton puff: 底重で膨らむ
            c.bottomFadeHeight = 0.15f;
            c.topFadeHeight = 0.75f;
            c.shapeExponent = 1.5f;
            break;
        case CloudClass::Cumulonimbus:
            // 雷雲 tower: 底重 + 頂 anvil
            c.bottomFadeHeight = 0.05f;
            c.topFadeHeight = 0.95f;
            c.shapeExponent = 0.8f;  // flatter, reaches top
            break;
        case CloudClass::Cirrus:
            // 薄い wispy: 薄めのプロファイル
            c.bottomFadeHeight = 0.2f;
            c.topFadeHeight = 0.8f;
            c.shapeExponent = 0.7f;
            break;
        case CloudClass::Cirrocumulus:
            c.bottomFadeHeight = 0.15f;
            c.topFadeHeight = 0.85f;
            c.shapeExponent = 0.9f;
            break;
        case CloudClass::Altocumulus:
            c.bottomFadeHeight = 0.1f;
            c.topFadeHeight = 0.9f;
            c.shapeExponent = 1.1f;
            break;
        }
        return c;
    }
};

/// @brief 高度帯別 cloud layer (ADR-0021 §Decision 1)
///
/// CloudField 内に N 本登録。1 本の統合 ray march (§Decision 2) が
/// altitude でこの layer を sample する。
struct CloudLayer
{
    /// @brief layer 底の世界高度 (m)
    float altitudeBottom = 1000.0f;

    /// @brief layer 頂の世界高度 (m)
    float altitudeTop = 2000.0f;

    /// @brief 支配的な雲種別 (shader は class 別 density profile を選択)
    CloudClass dominantClass = CloudClass::Cumulus;

    /// @brief 高度密度 curve (class 別 AC7 デフォルトは `HeightDensityCurve::Default`)
    HeightDensityCurve curve;

    /// @brief wind 倍率 (1.0 = layer ごとの wind speed そのまま、0.3 = 高高度 cirrus の遅いドリフト)
    float animationSpeed = 1.0f;

    /// @brief per-layer detail softening (0.5-1.0、TA review Issue 16 追加)
    float diffusivity = 0.8f;

    /// @brief per-layer detail fade 開始距離 (m)
    float detailFadeDistance = 500.0f;

    /// @brief per-layer 密度倍率 (overall multiplier)
    float densityMul = 1.0f;
};

} // namespace gx
/// @}
