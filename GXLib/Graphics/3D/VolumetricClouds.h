#pragma once
/// @file VolumetricClouds.h
/// @brief Volumetric Clouds (ray-marching cloud layer)
///
/// Fullscreen pixel-shader pass that ray-marches through a
/// horizontal cloud slab defined by bottom/top altitude.
/// FBM noise drives density; Beer-Lambert governs scattering.

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief GPU定数バッファレイアウト（VolumetricClouds.hlslと一致が必須）
struct CloudConstants
{
    XMFLOAT4X4 invViewProjection;  ///< 逆ビュープロジェクション行列
    XMFLOAT3   cameraPosition;    ///< カメラのワールド座標
    float      time;               ///< 経過時間（秒）
    XMFLOAT3   sunDirection;       ///< 太陽の方向ベクトル
    float      cloudBottom;        ///< 雲層の下端高度（ワールドY座標）
    XMFLOAT3   sunColor;           ///< 太陽の色（リニアRGB）
    float      cloudTop;           ///< 雲層の上端高度（ワールドY座標）
    float      coverage;           ///< 雲の被覆率（0〜1）
    float      densityMul;         ///< 密度乗数
    float      windSpeed;          ///< 風速
    float      silverLining;       ///< シルバーライニング強度（太陽周辺の輝き）
    XMFLOAT3   windDirection;      ///< 風の方向ベクトル
    int        marchSteps;         ///< 雲のレイマーチステップ数
    int        lightSteps;         ///< ライトレイのステップ数
    XMFLOAT2   screenDimensions;   ///< スクリーン解像度（幅, 高さ）
    float      _pad[1];            ///< パディング（160バイトアライメント用）
};  // 160B < 256B alignment

/// @brief Ray-marched volumetric cloud layer
class VolumetricClouds
{
public:
    VolumetricClouds() = default;
    ~VolumetricClouds() = default;

    /// @brief Initialise resources, shaders, PSO
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief Execute the cloud pass (HDR space)
    void Execute(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                 RenderTarget& srcHDR, RenderTarget& destHDR,
                 DepthBuffer& depth, const Camera3D& camera,
                 float elapsedTime);

    /// @brief Handle screen resize
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    // --- 有効化 / 無効化 ---
    /// @brief 雲レンダリングを有効/無効にする
    void SetEnabled(bool v) { m_enabled = v; }
    /// @brief 雲レンダリングが有効か判定する
    bool IsEnabled() const { return m_enabled; }

    // --- 雲層高度（ワールド空間） ---
    /// @brief 雲層の下端高度を設定する
    void  SetCloudBottom(float v)  { m_cloudBottom = v; }
    /// @brief 雲層の下端高度を取得する
    float GetCloudBottom() const   { return m_cloudBottom; }
    /// @brief 雲層の上端高度を設定する
    void  SetCloudTop(float v)     { m_cloudTop = v; }
    /// @brief 雲層の上端高度を取得する
    float GetCloudTop() const      { return m_cloudTop; }

    // --- 被覆率（0〜1） ---
    /// @brief 雲の被覆率を設定する（0=快晴、1=全天曇り）
    void  SetCoverage(float v)     { m_coverage = v; }
    /// @brief 雲の被覆率を取得する
    float GetCoverage() const      { return m_coverage; }

    // --- 密度乗数 ---
    /// @brief 雲の密度乗数を設定する
    void  SetDensity(float v)      { m_density = v; }
    /// @brief 雲の密度乗数を取得する
    float GetDensity() const       { return m_density; }

    // --- 風 ---
    /// @brief 風速を設定する
    void  SetWindSpeed(float v)    { m_windSpeed = v; }
    /// @brief 風速を取得する
    float GetWindSpeed() const     { return m_windSpeed; }
    /// @brief 風の方向ベクトルを設定する
    void  SetWindDirection(const XMFLOAT3& d) { m_windDirection = d; }
    /// @brief 風の方向ベクトルを取得する
    const XMFLOAT3& GetWindDirection() const  { return m_windDirection; }

    // --- 品質 ---
    /// @brief 雲のレイマーチステップ数を設定する
    void SetMarchSteps(int n)      { m_marchSteps = n; }
    /// @brief 雲のレイマーチステップ数を取得する
    int  GetMarchSteps() const     { return m_marchSteps; }
    /// @brief ライトレイのステップ数を設定する
    void SetLightSteps(int n)      { m_lightSteps = n; }
    /// @brief ライトレイのステップ数を取得する
    int  GetLightSteps() const     { return m_lightSteps; }

    // --- シルバーライニング強度 ---
    /// @brief シルバーライニング強度を設定する（太陽周辺の輝き）
    void  SetSilverLiningIntensity(float v) { m_silverLining = v; }
    /// @brief シルバーライニング強度を取得する
    float GetSilverLiningIntensity() const  { return m_silverLining; }

    // --- 太陽（シーンライトと共有） ---
    /// @brief 太陽の方向ベクトルを設定する
    void  SetSunDirection(const XMFLOAT3& d) { m_sunDirection = d; }
    /// @brief 太陽の色を設定する
    void  SetSunColor(const XMFLOAT3& c)     { m_sunColor = c; }

private:
    bool CreatePipelines(ID3D12Device* device);
    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);

    bool m_enabled = false;                        ///< 有効フラグ

    // 雲パラメータ
    float    m_cloudBottom  = 1500.0f;               ///< 雲層下端高度（ワールドY座標）
    float    m_cloudTop     = 3000.0f;               ///< 雲層上端高度（ワールドY座標）
    float    m_coverage     = 0.5f;                  ///< 被覆率（0〜1）
    float    m_density      = 0.05f;                 ///< 密度乗数
    float    m_windSpeed    = 10.0f;                 ///< 風速
    XMFLOAT3 m_windDirection = { 1.0f, 0.0f, 0.0f };///< 風の方向ベクトル
    int      m_marchSteps   = 64;                    ///< レイマーチステップ数
    int      m_lightSteps   = 6;                     ///< ライトレイステップ数
    float    m_silverLining  = 0.5f;                 ///< シルバーライニング強度

    // 太陽（外部から設定）
    XMFLOAT3 m_sunDirection = { 0.3f, -1.0f, 0.5f };///< 太陽の方向
    XMFLOAT3 m_sunColor     = { 1.0f, 0.98f, 0.95f };///< 太陽の色

    uint32_t m_width  = 0;                           ///< 描画幅（ピクセル）
    uint32_t m_height = 0;                           ///< 描画高さ（ピクセル）

    // パイプライン
    Shader m_shader;                                 ///< 雲シェーダー
    ComPtr<ID3D12RootSignature> m_rootSignature;     ///< ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_pso;               ///< パイプラインステート
    DynamicBuffer m_cb;                              ///< 定数バッファ

    // SRVヒープ（シーン+深度、2スロット×2フレーム=4）
    DescriptorHeap m_srvHeap;                        ///< SRVデスクリプタヒープ
    ID3D12Device* m_device = nullptr;                ///< D3D12デバイス
};

/// @}
} // namespace gx
