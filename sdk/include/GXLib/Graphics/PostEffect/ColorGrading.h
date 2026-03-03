#pragma once
/// @file ColorGrading.h
/// @brief Color Grading (カラーグレーディング) ポストエフェクト
///
/// シーン全体の色調を補正するエフェクト。映画やゲームのルックを決定する重要な工程で、
/// コントラスト・彩度・明度・色温度・ティント・ガンマ・露出の7パラメータを一括適用する。
///
/// 処理順序:
/// 1. 露出 (EV単位の明るさ補正)
/// 2. 色温度・ティント (ホワイトバランス調整)
/// 3. コントラスト (中間グレー基準の S カーブ)
/// 4. 彩度 (グレースケールとの線形補間)
/// 5. 明度 (リニア加算)
/// 6. ガンマ補正 (pow関数)
/// @addtogroup grp_gfx_postfx/// @{

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{

/// Color Grading 定数バッファ (7パラメータ + パディング)
struct ColorGradingConstants
{
    float contrast;     ///< コントラスト [-1, 1] (0 = 変化なし)
    float saturation;   ///< 彩度 [-1, 1] (-1 = モノクロ, 0 = 変化なし)
    float brightness;   ///< 明度 [-1, 1] (リニア加算)
    float temperature;  ///< 色温度 [-1, 1] (負=クール青寄り, 正=ウォーム黄寄り)
    float tint;         ///< ティント [-1, 1] (負=緑寄り, 正=マゼンタ寄り)
    float gamma;        ///< ガンマ [0.1, 5.0] (1.0 = リニア)
    float exposure;     ///< 露出 [-5, 5] EV (1EV = 2倍の明るさ変化)
    float _pad;         ///< 16バイトアライメント用パディング
};

/// @brief シーン全体のカラーグレーディングを行うポストエフェクト
///
/// 7つのパラメータでシーンの色調を調整する。映画的なルック作成や
/// 環境の雰囲気表現 (寒い場所は青寄り、夕暮れは暖色寄り等) に使用する。
class ColorGrading
{
public:
    ColorGrading() = default;
    ~ColorGrading() = default;

    /// @brief 初期化。PSO・定数バッファを作成する
    /// @param device D3D12デバイス (nullptrの場合はCPUのみ初期化)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief カラーグレーディングを適用する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param sceneRT 入力シーン (SRV状態)
    /// @param destRT 出力先。色調補正されたシーンが書き込まれる
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& sceneRT, RenderTarget& destRT);

    /// @brief コントラストを設定する [-1, 1]
    void SetContrast(float v) { m_contrast = v; }
    float GetContrast() const { return m_contrast; }

    /// @brief 彩度を設定する [-1, 1]
    void SetSaturation(float v) { m_saturation = v; }
    float GetSaturation() const { return m_saturation; }

    /// @brief 明度を設定する [-1, 1]
    void SetBrightness(float v) { m_brightness = v; }
    float GetBrightness() const { return m_brightness; }

    /// @brief 色温度を設定する [-1, 1]
    void SetTemperature(float v) { m_temperature = v; }
    float GetTemperature() const { return m_temperature; }

    /// @brief ティントを設定する [-1, 1]
    void SetTint(float v) { m_tint = v; }
    float GetTint() const { return m_tint; }

    /// @brief ガンマを設定する [0.1, 5.0]
    void SetGamma(float v) { m_gamma = v; }
    float GetGamma() const { return m_gamma; }

    /// @brief 露出を設定する (EV単位) [-5, 5]
    void SetExposure(float v) { m_exposure = v; }
    float GetExposure() const { return m_exposure; }

    void SetEnabled(bool v) { m_enabled = v; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief 画面リサイズ時の再初期化
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

private:
    bool CreatePipelines(ID3D12Device* device);

    ID3D12Device* m_device = nullptr;

    float m_contrast    = 0.0f;
    float m_saturation  = 0.0f;
    float m_brightness  = 0.0f;
    float m_temperature = 0.0f;
    float m_tint        = 0.0f;
    float m_gamma       = 1.0f;
    float m_exposure    = 0.0f;
    bool  m_enabled     = false;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader                      m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer               m_constantBuffer;
};

} // namespace gx
/// @}
