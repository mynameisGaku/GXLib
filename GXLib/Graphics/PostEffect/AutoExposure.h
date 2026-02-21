#pragma once
/// @file AutoExposure.h
/// @brief 自動露出 (明暗順応)
///
/// DxLibには無い機能。HDRシーンの平均輝度を計算し、明るい場所では暗く、
/// 暗い場所では明るく露出を自動調整する。トンネルに入ったときの目の順応と同じ。
/// ピクセルシェーダーベースの対数輝度ダウンサンプル方式 (CSインフラ不要)。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{

/// @brief シーン輝度に基づいて露出を自動調整する明暗順応エフェクト
///
/// 対数輝度の段階的ダウンサンプル (256→64→16→4→1) で平均輝度を算出し、
/// CPUリードバック後に時間的に滑らかに露出を適応させる。
/// ComputeExposureの戻り値をトーンマッピングの露出値として使う。
class AutoExposure
{
public:
    AutoExposure() = default;
    ~AutoExposure() = default;

    /// @brief 初期化。輝度ダウンサンプルRT・PSO・リードバックバッファを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief 平均輝度を計算し、適応済みの露出値を返す
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param hdrScene 入力HDRシーン
    /// @param deltaTime フレーム間秒数 (適応速度の計算に使う)
    /// @return トーンマッピングに渡す露出値
    float ComputeExposure(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                          RenderTarget& hdrScene, float deltaTime);

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief 順応速度。大きいほど素早く新しい明るさに合わせる
    void SetAdaptationSpeed(float s) { m_adaptationSpeed = s; }
    float GetAdaptationSpeed() const { return m_adaptationSpeed; }

    /// @brief 露出の下限値
    void SetMinExposure(float v) { m_minExposure = v; }
    float GetMinExposure() const { return m_minExposure; }

    /// @brief 露出の上限値
    void SetMaxExposure(float v) { m_maxExposure = v; }
    float GetMaxExposure() const { return m_maxExposure; }

    /// @brief 目標中間灰 (Key Value)。シーンの「基準の明るさ」を決める
    void SetKeyValue(float v) { m_keyValue = v; }
    float GetKeyValue() const { return m_keyValue; }

    /// @brief 現在の露出値を取得
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
