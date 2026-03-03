#pragma once
/// @file FilmGrain.h
/// @brief Film Grain (フィルムグレイン) ポストエフェクト
///
/// 映画フィルムの粒状感を再現するエフェクト。シーンの輝度に応じてノイズ量を変化させ、
/// 暗部に多くグレインを乗せることでフィルム的な質感を出す。
/// ルミナンス重みが高いほど暗い部分にグレインが集中し、
/// 0にすると画面全体に均一なグレインがかかる。
/// @addtogroup grp_gfx_postfx/// @{

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{

/// Film Grain 定数バッファ (強度・サイズ・時間シード・ルミナンス重み)
struct FilmGrainConstants
{
    float intensity;        ///< グレインの強さ [0, 1]
    float size;             ///< グレインの粒サイズ (1.0 = 標準)
    float time;             ///< アニメーション用ノイズシード (毎フレーム更新)
    float luminanceWeight;  ///< ルミナンスによるグレイン量の変化度 [0, 1]
};

/// @brief 映画フィルムの粒状ノイズを再現するエフェクト
///
/// シーンにホワイトノイズを輝度に応じて合成し、フィルム撮影の質感を加える。
/// 時間パラメータを毎フレーム更新することでノイズがアニメーションする。
class FilmGrain
{
public:
    FilmGrain() = default;
    ~FilmGrain() = default;

    /// @brief 初期化。PSO・定数バッファを作成する
    /// @param device D3D12デバイス (nullptrの場合はCPUのみ初期化)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief グレインノイズを適用する
    /// @param cmdList コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param sceneRT 入力シーン (SRV状態)
    /// @param destRT 出力先。sceneRTの内容にグレインが合成されて書き込まれる
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& sceneRT, RenderTarget& destRT);

    /// @brief グレイン強度を設定する [0, 1]
    void SetIntensity(float v) { m_intensity = v; }
    float GetIntensity() const { return m_intensity; }

    /// @brief グレインの粒サイズを設定する (1.0 = 標準)
    void SetSize(float v) { m_size = v; }
    float GetSize() const { return m_size; }

    /// @brief ルミナンスによるグレイン量の変化度を設定する [0, 1]
    void SetLuminanceWeight(float v) { m_luminanceWeight = v; }
    float GetLuminanceWeight() const { return m_luminanceWeight; }

    void SetEnabled(bool v) { m_enabled = v; }
    bool IsEnabled() const { return m_enabled; }

    /// @brief 画面リサイズ時の再初期化
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief ノイズアニメーション用時間を更新する
    /// @param dt 経過時間 (秒)
    void UpdateTime(float dt);

private:
    bool CreatePipelines(ID3D12Device* device);

    ID3D12Device* m_device = nullptr;

    float m_intensity       = 0.05f;
    float m_size            = 1.0f;
    float m_luminanceWeight = 0.5f;
    float m_time            = 0.0f;
    bool  m_enabled         = false;

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
