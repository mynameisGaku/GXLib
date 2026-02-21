#pragma once
/// @file MotionBlur.h
/// @brief カメラベース Motion Blur (動きぼかし)
///
/// DxLibには無い機能。カメラが動いた方向に画面をぼかし、動きの勢いを表現する。
/// 深度→ワールド座標復元→前フレームVP再投影で速度ベクトルを求め、
/// その方向にサンプリングしてブラーを生成する。オブジェクト単位ではなくカメラ全体が対象。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// Motion Blur 定数バッファ
struct MotionBlurConstants
{
    XMFLOAT4X4 invViewProjection;       // 現フレーム逆VP (row-major transposed)
    XMFLOAT4X4 previousViewProjection;  // 前フレームVP (row-major transposed)
    float intensity;
    int   sampleCount;
    float padding[2];
};  // 144B → 256-align

/// @brief カメラ移動による動きぼかしを再現するモーションブラーエフェクト
///
/// 前フレームと現フレームのViewProjection行列の差から速度ベクトルを求め、
/// HDRシーンを速度方向にサンプリングしてブラーする。
class MotionBlur
{
public:
    MotionBlur() = default;
    ~MotionBlur() = default;

    /// @brief 初期化。PSO・SRVヒープ・定数バッファを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief モーションブラーを実行する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT
    /// @param depth 深度バッファ (ワールド座標復元に使う)
    /// @param camera カメラ (現フレームの逆VP行列取得に使う)
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief ブラー強度。大きいほど動きの軌跡が長くなる
    void SetIntensity(float i) { m_intensity = i; }
    float GetIntensity() const { return m_intensity; }

    /// @brief 速度方向のサンプリング数。多いほど滑らかだが重い
    void SetSampleCount(int n) { m_sampleCount = n; }
    int GetSampleCount() const { return m_sampleCount; }

    /// @brief 前フレームのVP行列を保存する。Executeの後に呼ぶこと
    void UpdatePreviousVP(const Camera3D& camera);

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_intensity = 1.0f;
    int m_sampleCount = 16;

    XMFLOAT4X4 m_previousVP;       // 前フレームのViewProjection
    bool m_hasPreviousVP = false;   // 初回フレームはスキップ

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

    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);
};

} // namespace GX
