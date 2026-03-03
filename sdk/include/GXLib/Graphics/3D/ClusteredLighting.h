#pragma once
/// @file ClusteredLighting.h
/// @brief クラスタードライティング（Froxelベースライトカリング）
///
/// 画面をクラスタ（16x8x24フロクセル）に分割し、各クラスタに影響するライトを
/// 事前計算することで、多数のライト（最大256灯）を効率的に処理する。

#include "pch_graphics.h"
#include "Graphics/3D/Light.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Math/Matrix4x4.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief クラスタ情報（ライトインデックスバッファへのオフセットとカウント）
struct ClusterInfo
{
    uint32_t offset = 0;  ///< ライトインデックスバッファ内のオフセット
    uint32_t count  = 0;  ///< このクラスタに影響するライト数
};

/// @brief クラスタードライティング設定
struct ClusteredLightingSettings
{
    bool     enabled      = false;  ///< 有効/無効
    uint32_t clusterCountX = 16;   ///< X方向クラスタ数
    uint32_t clusterCountY = 8;    ///< Y方向クラスタ数
    uint32_t clusterCountZ = 24;   ///< Z方向（深度）クラスタ数
};

/// @brief クラスタードライティング（Froxelベースライトカリング）
///
/// 画面空間を3Dグリッド（froxels）に分割し、各クラスタに影響するライトリストを
/// コンピュートシェーダーで事前計算する。多数のダイナミックライトを効率処理。
class ClusteredLighting
{
public:
    ClusteredLighting() = default;
    ~ClusteredLighting() { Shutdown(); }

    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @return 成功時 true
    bool Initialize(ID3D12Device* device);

    /// @brief 終了処理
    void Shutdown();

    /// @brief 設定を変更
    void SetSettings(const ClusteredLightingSettings& settings) { m_settings = settings; }

    /// @brief 現在の設定を取得
    const ClusteredLightingSettings& GetSettings() const { return m_settings; }

    /// @brief ライトをクラスタに割り当てる（CSディスパッチ）
    /// @param cmdList   コマンドリスト
    /// @param lights    ライトデータ配列
    /// @param lightCount ライト数
    /// @param viewMatrix ビュー行列
    /// @param projMatrix プロジェクション行列
    /// @param nearZ      ニアクリップ
    /// @param farZ       ファークリップ
    /// @param screenWidth  スクリーン幅
    /// @param screenHeight スクリーン高さ
    /// @param frameIndex   フレームインデックス
    void AssignLights(ID3D12GraphicsCommandList* cmdList,
                      const LightData* lights, uint32_t lightCount,
                      const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix,
                      float nearZ, float farZ,
                      uint32_t screenWidth, uint32_t screenHeight,
                      uint32_t frameIndex);

    /// @brief 有効かどうか
    bool IsEnabled() const { return m_settings.enabled; }

    /// @brief 総クラスタ数を取得
    uint32_t GetTotalClusters() const
    {
        return m_settings.clusterCountX * m_settings.clusterCountY * m_settings.clusterCountZ;
    }

    /// @brief 最大ライト数を取得
    uint32_t GetMaxLights() const { return LightConstants::k_MaxLights; }

    /// @brief クラスタ次元を取得
    /// @param outX X方向クラスタ数
    /// @param outY Y方向クラスタ数
    /// @param outZ Z方向クラスタ数
    void GetClusterDimensions(uint32_t& outX, uint32_t& outY, uint32_t& outZ) const
    {
        outX = m_settings.clusterCountX;
        outY = m_settings.clusterCountY;
        outZ = m_settings.clusterCountZ;
    }

private:
    static constexpr uint32_t k_MaxLightsPerCluster = 64;

    ClusteredLightingSettings m_settings;

    // GPU resources
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_clusterInfoBuffer;     ///< ClusterInfo[totalClusters]
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_lightIndexBuffer;      ///< uint[totalClusters * maxPerCluster]
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_lightDataBuffer;       ///< LightData[maxLights]
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_constantBuffer;        ///< クラスタCB
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;         ///< CS用ルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_assignPSO;             ///< CSパイプラインステート
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;       ///< SRV/UAVヒープ

    ID3D12Device* m_device = nullptr;
    bool m_initialized = false;
};

/// @}
} // namespace gx
