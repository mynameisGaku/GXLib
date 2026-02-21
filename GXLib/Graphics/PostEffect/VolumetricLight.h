#pragma once
/// @file VolumetricLight.h
/// @brief ボリューメトリックライト (ゴッドレイ/光の筋)
///
/// DxLibには無い機能。太陽光が大気中の粒子に散乱して見える「光の筋」を再現する。
/// GPU Gems 3のスクリーン空間ラディアルブラー方式を採用。
/// 太陽のスクリーン座標に向かって放射状にサンプリングすることで光条を生成する。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// VolumetricLight 定数バッファ
struct VolumetricLightConstants
{
    XMFLOAT2 sunScreenPos;    // offset 0: UV空間の太陽位置
    float    density;          // offset 8: 散乱密度
    float    decay;            // offset 12: 減衰
    float    weight;           // offset 16: サンプルウェイト
    float    exposure;         // offset 20: 露出
    int      numSamples;       // offset 24: サンプル数
    float    intensity;        // offset 28: 全体強度
    XMFLOAT3 lightColor;      // offset 32: 光の色
    float    sunVisible;       // offset 44: 太陽可視性 (0-1)
};  // 48B → 256-align

/// @brief 太陽光の放射状散乱を再現するゴッドレイエフェクト
///
/// ライト方向からスクリーン上の太陽位置を計算し、各ピクセルから太陽に向かう
/// ラディアルブラーで光の筋を生成する。深度バッファで遮蔽判定も行う。
class VolumetricLight
{
public:
    VolumetricLight() = default;
    ~VolumetricLight() = default;

    /// @brief 初期化。PSO・SRVヒープ・定数バッファを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief ゴッドレイを生成してHDRシーンに合成する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT
    /// @param depth 深度バッファ (遮蔽判定に使う)
    /// @param camera カメラ (太陽スクリーン座標の計算に使う)
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    // --- 有効/無効 ---
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    // --- ライト方向 (ワールド空間) ---
    void SetLightDirection(const XMFLOAT3& dir) { m_lightDirection = dir; }
    const XMFLOAT3& GetLightDirection() const { return m_lightDirection; }

    // --- ライト色 ---
    void SetLightColor(const XMFLOAT3& color) { m_lightColor = color; }
    const XMFLOAT3& GetLightColor() const { return m_lightColor; }

    // --- パラメータ ---
    void SetDensity(float v) { m_density = v; }
    float GetDensity() const { return m_density; }

    void SetDecay(float v) { m_decay = v; }
    float GetDecay() const { return m_decay; }

    void SetWeight(float v) { m_weight = v; }
    float GetWeight() const { return m_weight; }

    void SetExposure(float v) { m_exposure = v; }
    float GetExposure() const { return m_exposure; }

    void SetNumSamples(int n) { m_numSamples = n; }
    int GetNumSamples() const { return m_numSamples; }

    void SetIntensity(float v) { m_intensity = v; }
    float GetIntensity() const { return m_intensity; }

    /// @brief 太陽のスクリーン座標と可視性を更新する (毎フレーム、enabled無関係に呼ぶ)
    void UpdateSunInfo(const Camera3D& camera);

    /// @brief 最後に計算した太陽の可視性 (0〜1)
    float GetLastSunVisible() const { return m_lastSunVisible; }
    /// @brief 最後に計算した太陽のスクリーンUV座標
    XMFLOAT2 GetLastSunScreenPos() const { return m_lastSunScreenPos; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;

    // ライトパラメータ
    XMFLOAT3 m_lightDirection = { 0.3f, -1.0f, 0.5f };
    XMFLOAT3 m_lightColor     = { 1.0f, 0.98f, 0.95f };

    // エフェクトパラメータ
    float m_density   = 1.0f;
    float m_decay     = 0.97f;
    float m_weight    = 0.04f;
    float m_exposure  = 0.35f;
    float m_intensity = 1.0f;
    int   m_numSamples = 96;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // 2テクスチャ(scene + depth)用専用SRVヒープ (2スロット × 2フレーム = 4)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    // デバッグ用: 最後のフレームの計算値
    float m_lastSunVisible = 0.0f;
    XMFLOAT2 m_lastSunScreenPos = { 0.0f, 0.0f };

    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);
};

} // namespace GX
