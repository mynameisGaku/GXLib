#pragma once
/// @file LightProbeSystem.h
/// @brief DDGI方式のライトプローブシステム（動的グローバルイルミネーション）
///
/// 間接照明をキャプチャする放射照度プローブの3Dグリッドを管理する。
/// 各プローブは低解像度の放射照度マップと深度マップを格納する。
/// 各プローブからレイを発射するComputeシェーダーにより更新を行う。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Math/Vector3.h"

namespace gx
{

/// @brief プローブグリッドの設定
struct ProbeGridConfig
{
    Vector3 origin = { 0.0f, 0.0f, 0.0f };   ///< グリッド原点（最小角）
    Vector3 spacing = { 4.0f, 4.0f, 4.0f };   ///< プローブ間の距離
    XMUINT3  dimensions = { 8, 4, 8 };          ///< プローブ数によるグリッドサイズ（最大約400個）
};

/// @brief ライトプローブ更新Computeシェーダー用の定数
struct LightProbeUpdateConstants
{
    Vector3 gridOrigin;          ///< グリッド原点（ワールド座標）
    float    _pad0;                ///< パディング
    Vector3 gridSpacing;         ///< プローブ間の距離
    float    hysteresis;           ///< 時間的安定性のヒステリシス係数（0〜1）
    XMUINT3  gridDimensions;       ///< グリッドの各軸プローブ数
    int      raysPerProbe;         ///< プローブあたりのレイ数
    int      probeCount;           ///< プローブ総数
    int      irradianceTexWidth;   ///< 放射照度アトラスの幅（ピクセル）
    int      depthTexWidth;        ///< 深度アトラスの幅（ピクセル）
    float    _pad1;                ///< パディング
};  // 64 bytes

/// @brief DDGI方式のライトプローブシステム（動的グローバルイルミネーション）
class LightProbeSystem
{
public:
    LightProbeSystem() = default;
    ~LightProbeSystem() = default;

    /// @brief グリッド設定でプローブシステムを初期化する
    bool Initialize(ID3D12Device* device, const ProbeGridConfig& config);

    /// @brief Computeシェーダーでプローブの放射照度を更新する
    void Update(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);

    /// @brief ライティングシェーダーでのサンプリング用にプローブデータをSRVとしてバインドする
    void BindForSampling(ID3D12GraphicsCommandList* cmdList, uint32_t rootParamIndex);

    /// @brief デバイスリサイズ処理（プローブは解像度非依存のため何もしない）
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    // --- 有効化 / 無効化 ---
    /// @brief プローブシステムを有効/無効にする
    /// @param v trueで有効化
    void SetEnabled(bool v) { m_enabled = v; }
    /// @brief プローブシステムが有効か判定する
    /// @return 有効ならtrue
    bool IsEnabled() const { return m_enabled; }

    // --- 設定 ---
    /// @brief グリッド設定を更新する
    /// @param config 新しいグリッド設定
    void SetConfig(const ProbeGridConfig& config) { m_config = config; }
    /// @brief 現在のグリッド設定を取得する
    /// @return グリッド設定
    const ProbeGridConfig& GetConfig() const { return m_config; }

    /// @brief グリッド内のプローブ総数を取得する
    uint32_t GetProbeCount() const;

    // --- 更新パラメータ ---
    /// @brief プローブあたりのレイ数を設定する
    /// @param n レイ数（多いほど品質向上、コスト増加）
    void SetRaysPerProbe(int n) { m_raysPerProbe = n; }
    /// @brief プローブあたりのレイ数を取得する
    /// @return レイ数
    int GetRaysPerProbe() const { return m_raysPerProbe; }

    /// @brief ヒステリシス係数を設定する（時間的安定性 vs 応答速度）
    /// @param h ヒステリシス係数（0〜1、高い=安定、低い=即応）
    void SetHysteresis(float h) { m_hysteresis = h; }
    /// @brief ヒステリシス係数を取得する
    /// @return ヒステリシス係数
    float GetHysteresis() const { return m_hysteresis; }

    /// @brief 放射照度アトラステクスチャを取得する（デバッグ/可視化用）
    ID3D12Resource* GetIrradianceAtlas() const { return m_irradianceAtlas.Get(); }

    /// @brief 深度アトラステクスチャを取得する
    ID3D12Resource* GetDepthAtlas() const { return m_depthAtlas.Get(); }

    /// @brief シャットダウンしてリソースを解放する
    void Shutdown();

private:
    bool CreateAtlasTextures(ID3D12Device* device);
    bool CreatePipelines(ID3D12Device* device);

    bool m_enabled = false;        ///< 有効フラグ
    bool m_initialized = false;    ///< 初期化済みフラグ

    ProbeGridConfig m_config;      ///< グリッド設定

    // プローブアトラステクスチャ
    static constexpr int k_IrradianceProbeSize = 6;   ///< プローブあたりの放射照度テクセル数（6x6）
    static constexpr int k_DepthProbeSize = 14;        ///< プローブあたりの深度テクセル数（14x14）
    ComPtr<ID3D12Resource> m_irradianceAtlas;  ///< 放射照度アトラス（R16G16B16A16_FLOAT）
    ComPtr<ID3D12Resource> m_depthAtlas;       ///< 深度アトラス（R16G16_FLOAT）

    // アトラスの寸法（プローブ数から計算）
    int m_irradianceAtlasWidth  = 0;   ///< 放射照度アトラスの幅（ピクセル）
    int m_irradianceAtlasHeight = 0;   ///< 放射照度アトラスの高さ（ピクセル）
    int m_depthAtlasWidth       = 0;   ///< 深度アトラスの幅（ピクセル）
    int m_depthAtlasHeight      = 0;   ///< 深度アトラスの高さ（ピクセル）

    // 更新パラメータ
    int m_raysPerProbe = 64;           ///< プローブあたりのレイ数
    float m_hysteresis = 0.97f;        ///< ヒステリシス係数

    // パイプライン
    Shader m_shader;                              ///< プローブ更新Computeシェーダー
    ComPtr<ID3D12RootSignature> m_updateRS;       ///< 更新用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_updatePSO;      ///< 更新用パイプラインステート
    DynamicBuffer m_updateCB;                     ///< 更新用定数バッファ
    DescriptorHeap m_uavHeap;                     ///< アトラステクスチャ用UAVヒープ
    DescriptorHeap m_srvHeap;                     ///< サンプリング用SRVヒープ

    ID3D12Device* m_device = nullptr;             ///< D3D12デバイス
};

} // namespace gx
/// @}
