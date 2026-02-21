#pragma once
/// @file DepthOfField.h
/// @brief Depth of Field (被写界深度/ピントぼけ)
///
/// DxLibには無い機能。カメラのフォーカス距離から離れた部分をぼかす。
/// 被写体にピントを合わせて背景をぼかす映画的な表現ができる。
///
/// 処理の流れ:
/// 1. CoC生成: 深度→ビュー空間Z復元→フォーカス距離との差でCircle of Confusion算出
/// 2. ブラー: CoC加重ガウシアンブラー (H/V分離, half-res)
/// 3. 合成: CoC値に基づいてシャープ画像とブラー画像をlerp

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// CoC生成定数バッファ
struct DoFCoCConstants
{
    XMFLOAT4X4 invProjection;  // 64B
    float focalDistance;        // フォーカス距離 (ビュー空間Z)
    float focalRange;           // フォーカス鮮明範囲
    float cocScale;             // CoC最大ピクセル数制御
    float screenWidth;
    float screenHeight;
    float nearZ;
    float farZ;
    float padding;
};  // 96B → 256-align

/// ブラー定数バッファ
struct DoFBlurConstants
{
    float texelSizeX;
    float texelSizeY;
    float padding[2];
};  // 16B → 256-align

/// 合成定数バッファ
struct DoFCompositeConstants
{
    float dummy;
    float padding[3];
};  // 16B → 256-align

/// @brief フォーカス距離に基づくピントぼけを再現する被写界深度エフェクト
///
/// CoC (Circle of Confusion) マップを生成し、半解像度のガウシアンブラーと
/// フル解像度合成でピント外をぼかす。3パス構成 (CoC生成→ブラー→合成)。
class DepthOfField
{
public:
    DepthOfField() = default;
    ~DepthOfField() = default;

    /// @brief 初期化。CoC RT・ブラーRT・PSO・定数バッファを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief DoFの全パスを実行する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン
    /// @param destHDR 出力先HDR RT (ピントぼけ適用後)
    /// @param depth 深度バッファ (CoC計算に使う)
    /// @param camera カメラ (逆射影行列の取得に使う)
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    /// @brief 画面リサイズ対応
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief フォーカス距離 (ビュー空間Z)。この距離の物体がピントが合う
    void SetFocalDistance(float d) { m_focalDistance = d; }
    float GetFocalDistance() const { return m_focalDistance; }

    /// @brief フォーカス鮮明範囲。この幅の中は完全にシャープ
    void SetFocalRange(float r) { m_focalRange = r; }
    float GetFocalRange() const { return m_focalRange; }

    /// @brief ぼけの最大半径 (ピクセル)
    void SetBokehRadius(float r) { m_bokehRadius = r; }
    float GetBokehRadius() const { return m_bokehRadius; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_focalDistance = 10.0f;
    float m_focalRange   = 5.0f;
    float m_bokehRadius  = 8.0f;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // CoC map (R16_FLOAT, full-res)
    RenderTarget m_cocRT;
    // ブラー中間 (HDR, half-res)
    RenderTarget m_blurTempRT;
    // ブラー結果 (HDR, half-res)
    RenderTarget m_blurRT;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_cocRS;        // b0 + t0(depth) + s0
    ComPtr<ID3D12RootSignature> m_blurRS;       // b0 + t0(scene) + t1(coc) + s0
    ComPtr<ID3D12RootSignature> m_compositeRS;  // b0 + t0(sharp) + t1(blurred) + t2(coc) + s0
    ComPtr<ID3D12PipelineState> m_cocPSO;
    ComPtr<ID3D12PipelineState> m_blurHPSO;
    ComPtr<ID3D12PipelineState> m_blurVPSO;
    ComPtr<ID3D12PipelineState> m_compositePSO;

    // 定数バッファ
    DynamicBuffer m_cocCB;
    DynamicBuffer m_blurCB;
    DynamicBuffer m_compositeCB;

    // 合成パス用: 3テクスチャを1ヒープにまとめるSRVヒープ
    // [0]=sharp(srcHDR), [1]=blurred(blurRT), [2]=CoC
    DescriptorHeap m_compositeSRVHeap;
    ID3D12Device*  m_device = nullptr;

    void UpdateCompositeSRVHeap(RenderTarget& srcHDR, uint32_t frameIndex);
};

} // namespace GX
