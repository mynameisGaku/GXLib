#pragma once
/// @file DynamicResolution.h
/// @brief 動的解像度スケーリング (DRS)
///
/// フレームレートが目標を下回ったとき、自動的に描画解像度を下げてGPU負荷を減らす。
/// 解像度はネイティブの50〜100%の範囲で毎フレーム調整され、
/// フレームレートが回復すると徐々にネイティブ解像度に戻る。
/// @addtogroup grp_gfx_postfx/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief DRS設定
struct DynamicResolutionConfig
{
    float minScale          = 0.5f;   ///< 最小スケール
    float maxScale          = 1.0f;   ///< 最大スケール
    float targetFrameTimeMs = 16.67f; ///< 目標フレーム時間 (ms)
    float adaptationSpeed   = 0.05f;  ///< 適応速度
    bool  enabled           = true;   ///< 有効フラグ
};

/// @brief 動的解像度スケーリングシステム
class DynamicResolution
{
public:
    DynamicResolution() = default;
    ~DynamicResolution() = default;

    /// @brief 初期化
    bool Initialize(uint32_t nativeWidth, uint32_t nativeHeight,
                    const DynamicResolutionConfig& config = {});

    /// @brief フレーム時間に基づいてスケールを更新
    void UpdateScale(float gpuTimeMs);

    /// @brief 現在のスケール値を取得 (0.5~1.0)
    float GetCurrentScale() const { return m_currentScale; }

    /// @brief 現在のレンダリング幅
    uint32_t GetRenderWidth() const;

    /// @brief 現在のレンダリング高さ
    uint32_t GetRenderHeight() const;

    /// @brief ネイティブ解像度幅
    uint32_t GetNativeWidth() const { return m_nativeWidth; }

    /// @brief ネイティブ解像度高さ
    uint32_t GetNativeHeight() const { return m_nativeHeight; }

    /// @brief 設定を取得
    const DynamicResolutionConfig& GetConfig() const { return m_config; }

    /// @brief 設定を更新
    void SetConfig(const DynamicResolutionConfig& config) { m_config = config; }

    /// @brief DRSが有効か
    bool IsEnabled() const { return m_config.enabled; }

    /// @brief スケールを手動でリセット
    void ResetScale() { m_currentScale = m_config.maxScale; }

    /// @brief CASアップスケール実行
    void ApplyUpscale(ID3D12GraphicsCommandList* cmdList);

private:
    DynamicResolutionConfig m_config;    ///< DRS設定
    uint32_t m_nativeWidth  = 0;         ///< ネイティブ解像度幅
    uint32_t m_nativeHeight = 0;         ///< ネイティブ解像度高さ
    float m_currentScale    = 1.0f;      ///< 現在の解像度スケール（0.5〜1.0）
    bool m_initialized      = false;     ///< 初期化済みフラグ
};

} // namespace gx
/// @}
