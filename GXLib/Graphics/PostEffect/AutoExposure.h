#pragma once
/// @file AutoExposure.h
/// @brief 自動露出 (Eye Adaptation) ポストエフェクト
///
/// HDRシーンの平均輝度を計算し、時間的に滑らかに露出を適応させる。
/// ピクセルシェーダーベースの対数輝度ダウンサンプル方式（CSインフラ不要）。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief 自動露出エフェクト
class AutoExposure
{
public:
    AutoExposure() = default;
    ~AutoExposure() = default;

    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// 平均輝度を計算し、露出を返す
    float ComputeExposure(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                          RenderTarget& hdrScene, float deltaTime);

    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetAdaptationSpeed(float s) { m_adaptationSpeed = s; }
    float GetAdaptationSpeed() const { return m_adaptationSpeed; }

    void SetMinExposure(float v) { m_minExposure = v; }
    float GetMinExposure() const { return m_minExposure; }

    void SetMaxExposure(float v) { m_maxExposure = v; }
    float GetMaxExposure() const { return m_maxExposure; }

    void SetKeyValue(float v) { m_keyValue = v; }
    float GetKeyValue() const { return m_keyValue; }

    float GetCurrentExposure() const { return m_currentExposure; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_adaptationSpeed = 1.5f;
    float m_minExposure = 0.1f;
    float m_maxExposure = 10.0f;
    float m_keyValue = 0.18f;           // 目標中間灰
    float m_currentExposure = 1.0f;

    // ダウンサンプルチェーン: R16_FLOAT
    // 256→64→16→4→1 (5レベル、4パス)
    static constexpr uint32_t k_NumLevels = 5;
    static constexpr uint32_t k_Sizes[k_NumLevels] = { 256, 64, 16, 4, 1 };
    RenderTarget m_luminanceRT[k_NumLevels];

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_commonRS;
    ComPtr<ID3D12PipelineState> m_luminancePSO;
    ComPtr<ID3D12PipelineState> m_downsamplePSO;
    DynamicBuffer m_cb;

    // CPUリードバック (2フレームリングバッファでストール回避)
    ComPtr<ID3D12Resource> m_readbackBuffer[2];
    float m_lastAvgLuminance = 0.5f;
    uint32_t m_readbackFrameCount = 0;
};

} // namespace GX
