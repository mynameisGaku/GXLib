#pragma once
/// @file ComputeSkinning.h
/// @brief GPU Compute Shader によるスキニング
///
/// 静的メッシュの頂点バッファとボーン行列を入力とし、
/// Compute Shader でスキニング済み頂点をGPU上に出力する。
/// 従来の頂点シェーダーベースのスキニングと切り替え可能。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{

/// @brief Compute Shader スキニング
class ComputeSkinning
{
public:
    ComputeSkinning() = default;
    ~ComputeSkinning() = default;

    /// @brief 初期化する
    /// @param device D3D12デバイス
    /// @return 初期化成功時 true
    bool Initialize(ID3D12Device* device);

    /// @brief スキニングを実行する
    /// @param cmdList コマンドリスト
    /// @param vertexSRV 入力頂点バッファのSRV
    /// @param boneSRV ボーン行列バッファのSRV
    /// @param outputUAV 出力頂点バッファのUAV
    /// @param vertexCount 頂点数
    /// @param bonesPerVertex 頂点あたりのボーン影響数（デフォルト: 4）
    void Dispatch(ID3D12GraphicsCommandList* cmdList,
                  D3D12_GPU_DESCRIPTOR_HANDLE vertexSRV,
                  D3D12_GPU_DESCRIPTOR_HANDLE boneSRV,
                  D3D12_GPU_DESCRIPTOR_HANDLE outputUAV,
                  uint32_t vertexCount,
                  uint32_t bonesPerVertex = 4);

    /// @brief シャットダウン
    void Shutdown();

    /// @brief 初期化済みかどうか
    bool IsReady() const { return m_ready; }

    /// @brief Compute Skinningの有効/無効を設定する
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /// @brief Compute Skinningが有効かどうか
    bool IsEnabled() const { return m_enabled; }

private:
    static constexpr uint32_t k_ThreadGroupSize = 64;  ///< Computeスレッドグループサイズ

    ID3D12Device* m_device = nullptr;  ///< D3D12デバイス
    bool m_ready   = false;            ///< 初期化済みフラグ
    bool m_enabled = false;            ///< 有効フラグ

    Shader m_shader;                              ///< Computeシェーダー
    ComPtr<ID3D12RootSignature> m_rootSignature;  ///< ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_pso;            ///< パイプラインステート
};

} // namespace gx
/// @}
