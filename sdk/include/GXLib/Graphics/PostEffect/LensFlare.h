#pragma once
/// @file LensFlare.h
/// @brief 物理ベースレンズフレア — 光束追跡（Ray Bundle Tracing）
///
/// カメラレンズを複数の球面/平面インターフェースとしてモデル化し、
/// 各ゴーストパス（2回反射）をコンピュートシェーダーでレイトレースする。
/// Sellmeier分散による波長依存屈折率で自然な色収差を再現。
///
/// パイプライン:
/// Pass 1 [CS]: 光束追跡 → GhostVertex UAV 生成
/// Pass 2 [VS/PS]: ゴーストクアッド描画（加算ブレンド → フレアRT）
/// Pass 3 [Fullscreen PS]: シーン + フレアRT 合成

#include "pch_graphics.h"
#include "Graphics/Resource/RenderTarget.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/3D/Camera3D.h"

namespace gx
{
/// @addtogroup grp_gfx_postfx
/// @{

/// @brief レンズフレアエフェクトの設定パラメータ
struct LensFlareSettings
{
    bool  enabled           = false; ///< エフェクト有効フラグ
    float intensity         = 1.0f;  ///< 全体強度
    float threshold         = 1.5f;  ///< (予約: 将来の自動輝度抽出用)
    int   gridSize          = 32;    ///< グリッド解像度 per ghost
    float apertureBlades    = 6.0f;  ///< 絞り羽根数（スターバースト形状）
    float starburstStrength = 0.3f;  ///< スターバースト強度
};

/// @brief 物理ベースレンズフレアエフェクト
///
/// 光束追跡（Ray Bundle Tracing）でカメラレンズ系を通るゴーストパスを
/// コンピュートシェーダーでトレースし、加算ブレンドで描画する。
class LensFlare
{
public:
    LensFlare() = default;
    ~LensFlare() = default;

    /// @brief 初期化。パイプライン・バッファ・レンズ系データを作成する
    /// @param device D3D12デバイス (nullptrの場合はCPUのみ初期化)
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief リソースを解放する
    void Shutdown();

    /// @brief レンズフレアを生成してHDRシーンに合成する
    /// @param cmdList コマンドリスト
    /// @param hdrInput 入力HDRシーン
    /// @param outputRT 出力先レンダーターゲット
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param camera カメラ (太陽スクリーン座標の計算に使う)
    void Execute(ID3D12GraphicsCommandList* cmdList,
                 RenderTarget& hdrInput, RenderTarget& outputRT,
                 uint32_t frameIndex, const Camera3D& camera);

    /// @brief 画面リサイズ時にサイズを更新する
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    // --- 太陽情報（VolumetricLight パターン踏襲）---
    void SetLightDirection(const Vector3& dir) { m_lightDirection = dir; }
    const Vector3& GetLightDirection() const { return m_lightDirection; }
    void SetLightColor(const Vector3& color) { m_lightColor = color; }
    const Vector3& GetLightColor() const { return m_lightColor; }

    // --- 設定 ---
    void SetSettings(const LensFlareSettings& s) { m_settings = s; }
    const LensFlareSettings& GetSettings() const { return m_settings; }

    bool IsEnabled() const { return m_settings.enabled; }
    void SetEnabled(bool v) { m_settings.enabled = v; }
    void SetIntensity(float v) { m_settings.intensity = v; }
    float GetIntensity() const { return m_settings.intensity; }
    void SetThreshold(float v) { m_settings.threshold = v; }
    float GetThreshold() const { return m_settings.threshold; }

private:
    /// レンズインターフェースデータ（CPU側、GPU転送用）
    struct LensInterfaceData
    {
        float centerZ;
        float radius;
        float apertureRadius;
        float n1, n2;
        float coatingN, coatingD;
        int   isAperture;
    };

    /// 太陽のスクリーン座標と可視性を更新する
    void UpdateSunScreenPos(const Camera3D& camera);

    /// レンズ系を初期化する（7インターフェース定義）
    void InitializeLensSystem();

    /// ゴーストペアを列挙する（全 (i,j) 反射ペア）
    void EnumerateGhosts();

    /// 各パイプラインを作成する
    bool CreateComputePipeline(ID3D12Device* device);
    bool CreateGhostRenderPipeline(ID3D12Device* device);
    bool CreateCompositePipeline(ID3D12Device* device);

    /// GPUバッファ（レンズデータ、ゴーストペア、ゴースト頂点）を作成する
    bool CreateGPUBuffers(ID3D12Device* device);

    // --- 設定 ---
    LensFlareSettings m_settings;

    // --- デバイス ---
    ID3D12Device* m_device = nullptr;
    bool m_initialized = false;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;

    // --- レンズ系データ (CPU) ---
    static constexpr int k_MaxInterfaces = 8;
    static constexpr int k_MaxGhosts = 28; // 8*7/2
    gx::Vector<LensInterfaceData> m_lensInterfaces;
    gx::Vector<uint32_t> m_ghostPairsA; // 反射面A (シーン側)
    gx::Vector<uint32_t> m_ghostPairsB; // 反射面B (センサー側)
    int m_numInterfaces = 0;
    int m_numGhosts = 0;

    // --- GPU バッファ ---
    Buffer m_lensInterfaceBuffer;    ///< StructuredBuffer<LensInterface> (Upload)
    Buffer m_ghostPairBuffer;        ///< StructuredBuffer<uint2> (Upload)
    Buffer m_ghostVertexBuffer;      ///< RWStructuredBuffer<GhostVertex> (Default, UAV)
    DynamicBuffer m_constantBuffer;  ///< 定数バッファ

    // --- デスクリプタヒープ ---
    DescriptorHeap m_srvUavHeap;     ///< SRV/UAV ヒープ (shader-visible, 8スロット)

    // --- 中間 RT ---
    RenderTarget m_flareRT;          ///< ゴースト描画先 (R16G16B16A16_FLOAT)

    // --- パイプライン ---
    Shader m_shader;

    // Pass 1: Compute (光束追跡)
    ComPtr<ID3D12RootSignature> m_computeRS;
    ComPtr<ID3D12PipelineState> m_computePSO;

    // Pass 2: Ghost Quad Rendering (加算ブレンド)
    ComPtr<ID3D12RootSignature> m_ghostRS;
    ComPtr<ID3D12PipelineState> m_ghostPSO;

    // Pass 3: Composite (フルスクリーン合成)
    ComPtr<ID3D12RootSignature> m_compositeRS;
    ComPtr<ID3D12PipelineState> m_compositePSO;

    // --- 太陽情報 ---
    Vector3 m_lightDirection = { 0.3f, -1.0f, 0.4f };
    Vector3 m_lightColor     = { 1.0f, 0.98f, 0.95f };
    Vector2 m_sunScreenUV    = { 0.5f, 0.5f };
    float   m_sunVisible     = 0.0f;
};

/// @}
} // namespace gx
