#pragma once
/// @file IBL.h
/// @brief イメージベースドライティング（IBL）リソース管理
///
/// 環境マップ（キューブマップ）から拡散照射マップ・鏡面プリフィルタマップ・
/// BRDF積分LUTを事前計算し、PBRの間接照明に使用する。
/// Skyboxの色パラメータからプロシージャルキューブマップを生成する方式のため、
/// 外部HDRIファイルは不要。

#include "pch.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

class CommandQueue;

/// @brief IBLリソース（環境マップから事前計算された間接照明テクスチャ群）
///
/// 3種類のテクスチャを保持する:
///   - 拡散照射マップ (Irradiance): 球面畳み込みで全方向の拡散照明を格納
///   - 鏡面プリフィルタマップ (Prefiltered): roughnessレベル別にミップチェーンで格納
///   - BRDF LUT: NdotVとroughnessの2次元テーブルで環境BRDF積分を近似
class IBL
{
public:
    IBL() = default;
    ~IBL() = default;

    /// @brief IBLリソースを初期化する（BRDF LUTのみ事前生成）
    /// @param device D3D12デバイス
    /// @param cmdQueue コマンドキュー（GPU計算用）
    /// @param srvHeap SRVヒープ（3スロット使用: irradiance, prefiltered, brdfLUT）
    /// @return 成功時true
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    DescriptorHeap& srvHeap);

    /// @brief Skyboxのプロシージャルパラメータからキューブマップを生成してIBLを更新する
    /// @param topColor 天頂色
    /// @param bottomColor 地平色
    /// @param sunDirection 太陽方向
    /// @param sunIntensity 太陽輝度
    void UpdateFromSkybox(const XMFLOAT3& topColor, const XMFLOAT3& bottomColor,
                          const XMFLOAT3& sunDirection, float sunIntensity);

    /// @brief IBLが有効かどうか
    /// @return 初期化済みでテクスチャが利用可能ならtrue
    bool IsReady() const { return m_ready; }

    /// @brief 拡散照射キューブマップのSRV GPUハンドルを返す
    D3D12_GPU_DESCRIPTOR_HANDLE GetIrradianceSRV() const;

    /// @brief 鏡面プリフィルタキューブマップのSRV GPUハンドルを返す
    D3D12_GPU_DESCRIPTOR_HANDLE GetPrefilteredSRV() const;

    /// @brief BRDF LUTのSRV GPUハンドルを返す
    D3D12_GPU_DESCRIPTOR_HANDLE GetBRDFLUTSRV() const;

    /// @brief IBLの強度を設定する
    /// @param intensity 乗算係数（デフォルト1.0）
    void SetIntensity(float intensity) { m_intensity = intensity; }

    /// @brief IBLの強度を取得する
    float GetIntensity() const { return m_intensity; }

    void Shutdown();

private:
    /// @brief BRDF積分LUTを生成する（512x512, R16G16_FLOAT）
    bool GenerateBRDFLUT(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

    /// @brief プロシージャルSkyboxから6面キューブマップを生成する
    bool GenerateEnvironmentCubemap(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

    /// @brief 拡散照射マップを生成する（環境キューブマップからコサイン畳み込み）
    bool GenerateIrradianceMap(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

    /// @brief 鏡面プリフィルタマップを生成する（roughnessごとにミップチェーン）
    bool GeneratePrefilteredMap(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

    ID3D12Device* m_device = nullptr;
    ID3D12CommandQueue* m_cmdQueue = nullptr;
    DescriptorHeap* m_srvHeap = nullptr;

    // 生成元の環境キューブマップ
    ComPtr<ID3D12Resource> m_envCubemap;
    uint32_t m_envCubemapSRVSlot = 0;

    // IBLテクスチャリソース
    ComPtr<ID3D12Resource> m_irradianceMap;     ///< 32x32x6 キューブマップ
    ComPtr<ID3D12Resource> m_prefilteredMap;     ///< 128x128x6, 5ミップ
    ComPtr<ID3D12Resource> m_brdfLUT;            ///< 512x512 R16G16_FLOAT

    // SRVスロット
    uint32_t m_irradianceSRVSlot  = 0;
    uint32_t m_prefilteredSRVSlot = 0;
    uint32_t m_brdfLUTSRVSlot     = 0;

    // 生成用パイプライン
    Shader m_shaderCompiler;
    ComPtr<ID3D12RootSignature> m_genRootSig;    ///< IBL生成用ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_brdfLUTPSO;    ///< BRDF LUT生成PSO
    ComPtr<ID3D12PipelineState> m_envCapturePSO; ///< 環境キューブマップキャプチャPSO
    ComPtr<ID3D12PipelineState> m_irradiancePSO; ///< 拡散照射マップ生成PSO
    ComPtr<ID3D12PipelineState> m_prefilteredPSO;///< 鏡面プリフィルタ生成PSO

    // 生成用中間リソース
    ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_cmdList;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 0;
    HANDLE m_fenceEvent = nullptr;

    // Skyboxパラメータ（前回の値と比較して再生成を判断）
    XMFLOAT3 m_topColor     = { 0.3f, 0.5f, 0.9f };
    XMFLOAT3 m_bottomColor  = { 0.7f, 0.8f, 0.95f };
    XMFLOAT3 m_sunDirection = { 0.3f, -1.0f, 0.5f };
    float    m_sunIntensity = 5.0f;

    float m_intensity = 1.0f;
    bool m_ready = false;

    // キューブマップ解像度
    static constexpr uint32_t k_EnvMapSize = 128;
    static constexpr uint32_t k_IrradianceSize = 32;
    static constexpr uint32_t k_PrefilteredSize = 128;
    static constexpr uint32_t k_PrefilteredMipLevels = 5;
    static constexpr uint32_t k_BRDFLUTSize = 512;

    /// @brief GPUコマンドの実行完了を待機する
    void FlushGPU();
};

} // namespace GX
