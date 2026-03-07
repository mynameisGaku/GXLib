#pragma once
/// @file VolumetricClouds.h
/// @brief UE5スタイル ボリュメトリッククラウド (ray-marching cloud layer)
///
/// マルチオクターブ散乱、Beer-Powder効果、Dual-Lobe HG位相関数、
/// 大気遠近法を含むフルスクリーンレイマーチングパス。

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DepthBuffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include <thread>
#include <atomic>

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief GPU定数バッファレイアウト（VolumetricClouds.hlslと一致が必須）
struct CloudConstants
{
    Matrix4x4 invViewProjection;  ///< 逆ビュープロジェクション行列
    Vector3   cameraPosition;    ///< カメラのワールド座標
    float      time;               ///< 経過時間（秒）
    Vector3   sunDirection;       ///< 太陽の方向ベクトル
    float      cloudBottom;        ///< 雲層の下端高度（ワールドY座標）
    Vector3   sunColor;           ///< 太陽の色（リニアRGB）
    float      cloudTop;           ///< 雲層の上端高度（ワールドY座標）
    float      coverage;           ///< 雲の被覆率（0〜1）
    float      densityMul;         ///< 密度乗数
    float      windSpeed;          ///< 風速
    float      silverLining;       ///< シルバーライニング強度（太陽周辺の輝き）
    Vector3   windDirection;      ///< 風の方向ベクトル
    int        marchSteps;         ///< 雲のレイマーチステップ数
    int        lightSteps;         ///< ライトレイのステップ数
    Vector2   screenDimensions;   ///< スクリーン解像度（幅, 高さ）
    float      _pad0;              ///< パディング
    // --- マルチオクターブ散乱パラメータ ---
    int        msOctaves;          ///< マルチオクターブ散乱のオクターブ数
    float      msAttenuation;      ///< オクターブ毎の光学的深度減衰率
    float      msContribution;     ///< オクターブ毎の寄与減衰率
    float      msEccentricity;     ///< オクターブ毎の位相関数偏心減衰率
    float      powderAmount;       ///< Beer-Powderエフェクト強度
    float      ambientBottom;      ///< 雲底のアンビエント光強度
    float      ambientTop;         ///< 雲頂のアンビエント光強度
    float      atmosphereDensity;  ///< 大気遠近法の密度係数
    int        useNoiseTextures;  ///< 3Dノイズテクスチャを使用するか (0=プロシージャル, 1=テクスチャ)
    float      _pad1[3];          ///< パディング（208B → 256B alignment）
};  // 208B < 256B alignment

/// @brief UE5スタイル レイマーチング ボリュメトリッククラウド
class VolumetricClouds
{
public:
    VolumetricClouds() = default;
    ~VolumetricClouds();

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
    void SetEnabled(bool v) { m_enabled = v; }
    bool IsEnabled() const { return m_enabled; }

    // --- 雲層高度（ワールド空間） ---
    void  SetCloudBottom(float v)  { m_cloudBottom = v; }
    float GetCloudBottom() const   { return m_cloudBottom; }
    void  SetCloudTop(float v)     { m_cloudTop = v; }
    float GetCloudTop() const      { return m_cloudTop; }

    // --- 被覆率（0〜1） ---
    void  SetCoverage(float v)     { m_coverage = v; }
    float GetCoverage() const      { return m_coverage; }

    // --- 密度乗数 ---
    void  SetDensity(float v)      { m_density = v; }
    float GetDensity() const       { return m_density; }

    // --- 風 ---
    void  SetWindSpeed(float v)    { m_windSpeed = v; }
    float GetWindSpeed() const     { return m_windSpeed; }
    void  SetWindDirection(const Vector3& d) { m_windDirection = d; }
    const Vector3& GetWindDirection() const  { return m_windDirection; }

    // --- 品質 ---
    void SetMarchSteps(int n)      { m_marchSteps = n; }
    int  GetMarchSteps() const     { return m_marchSteps; }
    void SetLightSteps(int n)      { m_lightSteps = n; }
    int  GetLightSteps() const     { return m_lightSteps; }

    // --- シルバーライニング強度 ---
    void  SetSilverLiningIntensity(float v) { m_silverLining = v; }
    float GetSilverLiningIntensity() const  { return m_silverLining; }

    // --- 太陽（シーンライトと共有） ---
    void  SetSunDirection(const Vector3& d) { m_sunDirection = d; }
    const Vector3& GetSunDirection() const  { return m_sunDirection; }
    void  SetSunColor(const Vector3& c)     { m_sunColor = c; }

    // --- マルチオクターブ散乱パラメータ ---
    void  SetMSOctaves(int n)             { m_msOctaves = n; }
    int   GetMSOctaves() const            { return m_msOctaves; }
    void  SetMSAttenuation(float v)       { m_msAttenuation = v; }
    float GetMSAttenuation() const        { return m_msAttenuation; }
    void  SetMSContribution(float v)      { m_msContribution = v; }
    float GetMSContribution() const       { return m_msContribution; }
    void  SetMSEccentricity(float v)      { m_msEccentricity = v; }
    float GetMSEccentricity() const       { return m_msEccentricity; }

    // --- Beer-Powder ---
    void  SetPowderAmount(float v)        { m_powderAmount = v; }
    float GetPowderAmount() const         { return m_powderAmount; }

    // --- 高度依存アンビエント ---
    void  SetAmbientBottom(float v)       { m_ambientBottom = v; }
    float GetAmbientBottom() const        { return m_ambientBottom; }
    void  SetAmbientTop(float v)          { m_ambientTop = v; }
    float GetAmbientTop() const           { return m_ambientTop; }

    // --- 大気遠近法 ---
    void  SetAtmosphereDensity(float v)   { m_atmosphereDensity = v; }
    float GetAtmosphereDensity() const    { return m_atmosphereDensity; }

    // --- テンポラルリプロジェクション ---
    void  SetTemporalEnabled(bool v)      { m_temporalEnabled = v; }
    bool  IsTemporalEnabled() const       { return m_temporalEnabled; }
    void  SetTemporalAlpha(float v)       { m_temporalAlpha = v; }
    float GetTemporalAlpha() const        { return m_temporalAlpha; }

private:
    bool CreatePipelines(ID3D12Device* device);
    void UpdateSRVHeap(RenderTarget& srcHDR, DepthBuffer& depth, uint32_t frameIndex);
    void GenerateNoiseData();
    void CreateNoiseResources(ID3D12Device* device);
    void UploadNoiseTextures(ID3D12GraphicsCommandList* cmdList);
    void CreateTemporalPipeline(ID3D12Device* device);
    void ExecuteCloudOnly(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                          DepthBuffer& depth, const Camera3D& camera, float elapsedTime);
    void ExecuteTemporalResolve(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex,
                                RenderTarget& srcHDR, RenderTarget& destHDR,
                                DepthBuffer& depth, const Camera3D& camera);

    bool m_enabled = false;

    // 雲パラメータ
    float    m_cloudBottom  = 1500.0f;
    float    m_cloudTop     = 3000.0f;
    float    m_coverage     = 0.5f;
    float    m_density      = 0.05f;
    float    m_windSpeed    = 10.0f;
    Vector3 m_windDirection = { 1.0f, 0.0f, 0.0f };
    int      m_marchSteps   = 32;
    int      m_lightSteps   = 6;
    float    m_silverLining  = 0.5f;

    // マルチオクターブ散乱
    int   m_msOctaves        = 6;
    float m_msAttenuation    = 0.3f;
    float m_msContribution   = 0.7f;
    float m_msEccentricity   = 0.5f;
    float m_powderAmount     = 0.8f;
    float m_ambientBottom    = 0.20f;
    float m_ambientTop       = 0.55f;
    float m_atmosphereDensity = 0.00003f;

    // 太陽（外部から設定）
    Vector3 m_sunDirection = { 0.3f, -1.0f, 0.5f };
    Vector3 m_sunColor     = { 1.0f, 0.98f, 0.95f };

    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // パイプライン
    Shader m_shader;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pso;
    DynamicBuffer m_cb;

    // SRVヒープ（シーン+深度+baseNoise+detailNoise、4スロット×2フレーム=8）
    DescriptorHeap m_srvHeap;
    ID3D12Device* m_device = nullptr;

    // --- 3Dノイズテクスチャ ---
    ComPtr<ID3D12Resource> m_baseNoiseTexture;    ///< 128³ DEFAULT heap
    ComPtr<ID3D12Resource> m_detailNoiseTexture;  ///< 32³ DEFAULT heap
    ComPtr<ID3D12Resource> m_baseNoiseUpload;     ///< Upload buffer
    ComPtr<ID3D12Resource> m_detailNoiseUpload;   ///< Upload buffer
    bool m_noiseUploaded = false;

    // --- 非同期ノイズ生成 ---
    std::thread m_noiseThread;
    std::atomic<bool> m_noiseDataReady{false};
    gx::Vector<uint8_t> m_baseNoiseData;     ///< 128³×4 bytes
    gx::Vector<uint8_t> m_detailNoiseData;   ///< 32³×4 bytes

    // --- テンポラルリプロジェクション ---
    /// @brief テンポラルリゾルブ定数バッファ
    struct CloudTemporalConstants
    {
        Matrix4x4 prevViewProjection;  ///< 前フレームのVP行列
        Matrix4x4 invViewProjection;   ///< 現フレームの逆VP行列
        float alpha;                   ///< テンポラルブレンド比率
        float screenWidth;
        float screenHeight;
        float hasHistory;              ///< 0.0 or 1.0
        float halfWidth;
        float halfHeight;
        float depthThreshold;          ///< バイラテラル閾値
        float _pad;
    };  // 160B

    RenderTarget m_cloudRT;                       ///< 半解像度 cloud-only
    RenderTarget m_historyRT[2];                  ///< フル解像度ダブルバッファ
    uint32_t m_historyWriteIdx = 0;
    ComPtr<ID3D12PipelineState> m_cloudOnlyPSO;
    ComPtr<ID3D12RootSignature> m_temporalRS;
    ComPtr<ID3D12PipelineState> m_temporalPSO;
    DynamicBuffer m_temporalCB;
    DescriptorHeap m_temporalHeap;                ///< 4 SRV × 2 frames = 8
    Matrix4x4 m_previousVP = {};
    bool m_hasPreviousVP = false;
    bool m_hasHistory = false;
    bool m_temporalEnabled = true;
    float m_temporalAlpha = 0.05f;
};

/// @}
} // namespace gx
