#pragma once
/// @file TAA.h
/// @brief Temporal Anti-Aliasing (時間的アンチエイリアシング)
///
/// DxLibには無い機能。毎フレームカメラをサブピクセル単位でずらし(ジッター)、
/// 前フレームの結果と合成することでMSAA不要のアンチエイリアシングを実現する。
/// Halton(2,3)数列でジッターパターンを生成し、近傍クランプでゴーストを防ぐ。

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace gx
{
/// @addtogroup grp_gfx_postfx
/// @{

/// @brief TAA 定数バッファ
struct TAAConstants
{
    XMFLOAT4X4 invViewProjection;       ///< 現フレーム逆VP行列（非ジッター）
    XMFLOAT4X4 previousViewProjection;  ///< 前フレームVP行列（非ジッター）
    XMFLOAT2   jitterOffset;            ///< 現フレームジッターオフセット（NDC空間）
    float      blendFactor;             ///< 履歴ブレンド比率（0〜1）
    float      screenWidth;             ///< スクリーン幅（ピクセル）
    float      screenHeight;            ///< スクリーン高さ（ピクセル）
    float      padding[3];              ///< パディング
};  // 160B → 256-align

/// シャープニング定数バッファ
struct SharpeningConstants
{
    float sharpness;     ///< シャープニング強度 (0=無効, 1=最大)
    float screenWidth;   ///< スクリーン幅
    float screenHeight;  ///< スクリーン高さ
    float padding;
};

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

    /// @brief シャープニング強度 (0=無効, 1=最大)
    void SetSharpness(float s) { m_sharpness = s; }
    float GetSharpness() const { return m_sharpness; }

    /// @brief 内部解像度スケール (0.5〜1.0, 1.0=フル解像度)
    /// @note PostEffectPipeline側でHDR RTサイズを調整する必要がある
    void SetResolutionScale(float scale) { m_resolutionScale = (std::max)(0.5f, (std::min)(1.0f, scale)); }
    float GetResolutionScale() const { return m_resolutionScale; }

    /// @brief 現フレームのジッターオフセットを取得 (NDC空間)
    XMFLOAT2 GetCurrentJitter() const;

    /// @brief 前フレームのVP行列を保存する。Executeの後に呼ぶこと
    void UpdatePreviousVP(const Camera3D& camera);

    /// @brief フレームカウントを進める (ジッターパターンの更新)
    void AdvanceFrame() { m_frameCount++; }

    uint32_t GetFrameCount() const { return m_frameCount; }

private:
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;               ///< 有効フラグ
    float m_blendFactor = 0.9f;           ///< 履歴ブレンド比率
    float m_sharpness = 0.0f;             ///< CASシャープニング強度
    float m_resolutionScale = 1.0f;       ///< 内部解像度スケール

    RenderTarget m_historyRT;             ///< 履歴レンダーターゲット（R16G16B16A16_FLOAT）
    bool m_hasHistory = false;            ///< 履歴RTに有効なデータがあるか

    XMFLOAT4X4 m_previousVP;             ///< 前フレームのVP行列
    bool m_hasPreviousVP = false;         ///< 前フレームVP行列が有効か
    uint32_t m_frameCount = 0;            ///< フレームカウンター（ジッター生成用）

    uint32_t m_width  = 0;                ///< 画面幅（ピクセル）
    uint32_t m_height = 0;                ///< 画面高さ（ピクセル）

    // パイプライン
    Shader m_shader;                              ///< TAAシェーダー
    ComPtr<ID3D12RootSignature> m_rootSignature;  ///< ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_pso;            ///< パイプラインステート
    DynamicBuffer m_cb;                           ///< 定数バッファ

    // SRVヒープ（scene + history + depth, 3スロット×2フレーム=6）
    DescriptorHeap m_srvHeap;                     ///< SRVデスクリプタヒープ
    ID3D12Device* m_device = nullptr;             ///< D3D12デバイス

    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);

    // シャープニング (CAS)
    ComPtr<ID3D12RootSignature> m_sharpenRS;
    ComPtr<ID3D12PipelineState> m_sharpenPSO;
    DynamicBuffer m_sharpenCB;
    RenderTarget m_sharpenTempRT;  ///< シャープニング用一時RT
    bool CreateSharpenPipeline(ID3D12Device* device);

    /// Halton数列 (base=2,3) ジッター生成
    static float Halton(int index, int base);
};

/// @}
} // namespace gx
