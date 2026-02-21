#pragma once
/// @file TAA.h
/// @brief Temporal Anti-Aliasing (時間的アンチエイリアシング)
///
/// DxLibには無い機能。毎フレームカメラをサブピクセル単位でずらし(ジッター)、
/// 前フレームの結果と合成することでMSAA不要のアンチエイリアシングを実現する。
/// Halton(2,3)数列でジッターパターンを生成し、近傍クランプでゴーストを防ぐ。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace GX
{

/// TAA 定数バッファ
struct TAAConstants
{
    XMFLOAT4X4 invViewProjection;       // 64B: 現フレーム逆VP (非ジッター)
    XMFLOAT4X4 previousViewProjection;  // 64B: 前フレームVP (非ジッター)
    XMFLOAT2   jitterOffset;            // 8B: 現フレームジッター (NDC)
    float      blendFactor;             // 4B: 履歴ウェイト (0.9)
    float      screenWidth;             // 4B
    float      screenHeight;            // 4B
    float      padding[3];             // 12B
};  // 160B → 256-align

/// @brief ジッター+履歴合成でジャギーを消すTAAエフェクト
///
/// 現フレーム(ジッター適用済み)と前フレーム履歴をリプロジェクション+
/// 近傍クランプ付きでブレンドする。PostEffectPipeline::BeginSceneでジッターが
/// カメラに自動適用される。
class TAA
{
public:
    TAA() = default;
    ~TAA() = default;

    /// @brief 初期化。履歴RT・PSO・SRVヒープ・定数バッファを作成する
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief TAAを実行する (srcHDR→destHDR、destHDR→historyRTへコピー)
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param srcHDR 入力HDRシーン (ジッター適用済み)
    /// @param destHDR 出力先HDR RT (AA適用後)
    /// @param depth 深度バッファ (リプロジェクションに使う)
    /// @param camera カメラ
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera);

    /// @brief 画面リサイズ時に履歴RTを再生成する
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief 履歴のブレンド比率 (0〜1)。大きいほど前フレームの影響が強い
    void SetBlendFactor(float f) { m_blendFactor = f; }
    float GetBlendFactor() const { return m_blendFactor; }

    /// @brief 現フレームのジッターオフセットを取得 (NDC空間)
    XMFLOAT2 GetCurrentJitter() const;

    /// @brief 前フレームのVP行列を保存する。Executeの後に呼ぶこと
    void UpdatePreviousVP(const Camera3D& camera);

    /// @brief フレームカウントを進める (ジッターパターンの更新)
    void AdvanceFrame() { m_frameCount++; }

    uint32_t GetFrameCount() const { return m_frameCount; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;
    float m_blendFactor = 0.9f;

    RenderTarget m_historyRT;       // R16G16B16A16_FLOAT (前フレームTAA出力)
    bool m_hasHistory = false;

    XMFLOAT4X4 m_previousVP;
    bool m_hasPreviousVP = false;
    uint32_t m_frameCount = 0;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // 3テクスチャ(scene + history + depth)用専用SRVヒープ (3スロット × 2フレーム = 6)
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);

    /// Halton数列 (base=2,3) ジッター生成
    static float Halton(int index, int base);
};

} // namespace GX
