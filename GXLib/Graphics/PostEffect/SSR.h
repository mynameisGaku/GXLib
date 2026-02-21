#pragma once
/// @file SSR.h
/// @brief Screen Space Reflections (スクリーン空間反射)
///
/// DxLibには無い機能。深度バッファ上でレイマーチングし、画面内の映り込みを再現する。
/// DXRレイトレ反射と排他的に動作する (RTが使えない環境のフォールバック)。
/// GBufferを持たないForward+環境でも動作する (法線は深度勾配から再構成)。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// SSR 定数バッファ (行列3種+レイマーチパラメータ)
struct SSRConstants
{
    XMFLOAT4X4 projection;       ///< 射影行列 (row-major transposed)
    XMFLOAT4X4 invProjection;    ///< 逆射影行列
    XMFLOAT4X4 view;             ///< ビュー行列
    float maxDistance;            ///< レイの最大飛距離 (ビュー空間)
    float stepSize;              ///< 1ステップの移動量
    int   maxSteps;              ///< レイマーチの最大ステップ数
    float thickness;             ///< 深度ヒット判定の厚み
    float intensity;             ///< 反射の強度
    float screenWidth;
    float screenHeight;
    float nearZ;
};  // 224B → 256-align

/// @brief スクリーン空間でレイマーチングして反射を生成するSSRエフェクト
///
/// HDRシーン+深度バッファ+法線RTの3テクスチャを入力とし、
/// ビュー空間でレイを飛ばして画面内の映り込みを計算する。
/// DXR反射と排他: RT有効時はSSRが自動的に無効になる。
class SSR
{
public:
    SSR() = default;
    ~SSR() = default;

    /// @brief 初期化。PSO・SRVヒープ・定数バッファを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief SSRを実行し、反射を含むHDR画像をdestHDRに出力する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT
    /// @param depth 深度バッファ (レイマーチのヒット判定に使う)
    /// @param normalRT 法線RT (反射方向の決定に使う)
    /// @param camera カメラ
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, RenderTarget& normalRT, const Camera3D& camera);

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief レイの最大飛距離 (ビュー空間単位)
    void SetMaxDistance(float d) { m_maxDistance = d; }
    float GetMaxDistance() const { return m_maxDistance; }

    /// @brief 1ステップの移動量。小さいほど精度が高いが遅い
    void SetStepSize(float s) { m_stepSize = s; }
    float GetStepSize() const { return m_stepSize; }

    /// @brief レイマーチの最大ステップ数
    void SetMaxSteps(int n) { m_maxSteps = n; }
    int GetMaxSteps() const { return m_maxSteps; }

    /// @brief 深度ヒット判定の厚み。小さいほど精度が高いが見逃しやすい
    void SetThickness(float t) { m_thickness = t; }
    float GetThickness() const { return m_thickness; }

    /// @brief 反射の強度
    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_maxDistance = 30.0f;
    float m_stepSize = 0.15f;
    int m_maxSteps = 200;
    float m_thickness = 0.15f;    // ビュー空間: ≈1ステップ分
    float m_intensity = 1.0f;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // 3テクスチャ(scene + depth + normal)用専用SRVヒープ (3スロット × 2フレーム = 6)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth,
                       RenderTarget& normalRT, uint32_t frameIndex);
};

} // namespace GX
