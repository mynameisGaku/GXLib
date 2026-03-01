#pragma once
/// @file LensFlare.h
/// @brief レンズフレア/グレアエフェクト
///
/// DxLibには無い機能。HDRシーンの明るい部分からゴーストイメージ、ハローリング、
/// スターバースト、色収差シフトを生成し、カメラレンズの光学的特性を再現する。
/// ダストテクスチャによるレンズ汚れ表現にも対応。
///
/// 処理の流れ:
/// 1. 閾値抽出 (明るいピクセルのみ)
/// 2. ゴーストイメージ生成 (反転UV + 複数スケール)
/// 3. ハローリング (周辺部の環状グレア)
/// 4. 色収差シフト (R/Bチャンネルのオフセット)
/// 5. スターバーストパターン (角度ベースの放射状フィルタ)

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{
/// @addtogroup grp_gfx_postfx
/// @{

/// @brief レンズフレアエフェクトの設定パラメータ
struct LensFlareSettings
{
    bool  enabled          = false;  ///< エフェクト有効フラグ
    float intensity        = 1.0f;   ///< 全体強度
    float threshold        = 1.5f;   ///< 輝度閾値 (この値以上のピクセルからフレアを生成)
    int   ghostCount       = 4;      ///< ゴーストイメージの数
    float ghostDispersion  = 0.5f;   ///< ゴーストの分散距離
    float haloRadius       = 0.6f;   ///< ハローリングの半径
    float chromaticShift   = 0.01f;  ///< 色収差シフト量
    bool  dustEnabled      = false;  ///< レンズダストテクスチャ有効フラグ
};

/// @brief カメラレンズの光学的フレアエフェクト
///
/// HDRシーンから閾値以上の明部を抽出し、ゴーストイメージ・ハローリング・
/// スターバースト・色収差を合成してレンズフレアを生成する。
class LensFlare
{
public:
    LensFlare() = default;
    ~LensFlare() = default;

    /// @brief 初期化。PSO・定数バッファを作成する
    /// @param device D3D12デバイス (nullptrの場合はCPUのみ初期化)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief リソースを解放する
    void Shutdown();

    /// @brief 設定パラメータを更新する
    void SetSettings(const LensFlareSettings& settings) { m_settings = settings; }

    /// @brief 現在の設定パラメータを取得する
    const LensFlareSettings& GetSettings() const { return m_settings; }

    /// @brief レンズフレアを生成してHDRシーンに合成する
    /// @param cmdList コマンドリスト
    /// @param hdrInput 入力HDRシーン (SRV状態)
    /// @param outputRT 出力先レンダーターゲット
    /// @param frameIndex ダブルバッファ用フレームインデックス
    void Execute(ID3D12GraphicsCommandList* cmdList,
                 RenderTarget& hdrInput, RenderTarget& outputRT,
                 uint32_t frameIndex);

    /// @brief エフェクトが有効かどうか
    bool IsEnabled() const { return m_settings.enabled; }

private:
    bool CreatePipelines(ID3D12Device* device);

    LensFlareSettings m_settings;                  ///< エフェクト設定

    ID3D12Device* m_device = nullptr;              ///< D3D12デバイス
    bool m_initialized = false;                    ///< 初期化完了フラグ

    uint32_t m_width  = 0;                         ///< 画面幅
    uint32_t m_height = 0;                         ///< 画面高さ

    // パイプライン
    Shader                      m_shader;          ///< レンズフレアシェーダー
    ComPtr<ID3D12RootSignature> m_rootSignature;   ///< ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_pso;             ///< パイプラインステート
    DynamicBuffer               m_constantBuffer;  ///< 定数バッファ
};

/// @}
} // namespace gx
