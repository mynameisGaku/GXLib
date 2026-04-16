#pragma once
/// @file HiZBuffer.h
/// @brief Hi-Z（Hierarchical Z-Buffer）オクルージョンカリング
///
/// 深度バッファの階層的ミップマップを生成し、遮蔽テストにより
/// 不可視オブジェクトをGPU側で事前にカリングする。

#include "pch_graphics.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Device/DescriptorHeap.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"

namespace gx
{
/// @addtogroup grp_gfx_3d
/// @{

/// @brief Hi-Z カリング用のオブジェクトバウンド（GPU構造体）
struct HiZObjectBounds
{
    Vector3 center;
    float   radius;
};

/// @brief Hi-Z バッファ（深度の階層ミップマップ + オクルージョンカリング）
class HiZBuffer
{
public:
    HiZBuffer() = default;
    ~HiZBuffer() = default;

    /// @brief 初期化
    /// @param device D3D12デバイス
    /// @param width 深度バッファの幅
    /// @param height 深度バッファの高さ
    /// @return 成功時true
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief 深度バッファからHi-Zミップチェーンを生成する
    /// @param cmdList コマンドリスト
    /// @param depthSRV 深度バッファのSRV GPUハンドル
    /// @param depthResource 深度バッファリソース（バリア用）
    /// @param frameIndex フレームインデックス
    void GenerateHiZ(ID3D12GraphicsCommandList* cmdList,
                     D3D12_GPU_DESCRIPTOR_HANDLE depthSRV,
                     uint32_t frameIndex);

    /// @brief オブジェクト配列をHi-Zでカリングする
    /// @param cmdList コマンドリスト
    /// @param bounds オブジェクトバウンドの配列
    /// @param objectCount オブジェクト数
    /// @param viewProjection ビュー射影行列
    /// @param frameIndex フレームインデックス
    void CullObjects(ID3D12GraphicsCommandList* cmdList,
                     const HiZObjectBounds* bounds, uint32_t objectCount,
                     const Matrix4x4& viewProjection, uint32_t frameIndex);

    /// @brief カリング結果の可視フラグを取得する（CPU読み取り用）
    /// @param outFlags 結果格納先（objectCount個）
    /// @return 読み取り成功時true
    bool ReadVisibilityFlags(uint32_t* outFlags, uint32_t objectCount) const;

    /// @brief Hi-Z有効かどうか
    bool IsReady() const { return m_ready; }

    /// @brief 画面サイズ変更時の再作成
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

private:
    bool CreateHiZTexture(ID3D12Device* device);
    bool CreatePipelines(ID3D12Device* device);

    ID3D12Device* m_device = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_mipLevels = 0;
    bool m_ready = false;

    // シェーダーコンパイラ
    Shader m_shader;

    // Hi-Z テクスチャ（R32_FLOAT + ミップチェーン）
    ComPtr<ID3D12Resource> m_hiZTexture;

    // ディスクリプタヒープ（UAV + SRV for each mip）
    DescriptorHeap m_srvUavHeap;
    gx::Vector<uint32_t> m_mipUAVSlots;  // 各ミップのUAVインデックス
    gx::Vector<uint32_t> m_mipSRVSlots;  // 各ミップのSRVインデックス
    uint32_t m_depthSRVSlot = 0;          // 入力深度のSRVコピー先
    uint32_t m_boundsSRVSlot = 0;         // カリング用バウンドSRV
    uint32_t m_flagsUAVSlot = 0;          // カリング用可視フラグUAV

    // Hi-Z生成 Compute Pipeline
    ComPtr<ID3D12RootSignature> m_generateRS;
    ComPtr<ID3D12PipelineState> m_generatePSO;
    DynamicBuffer m_generateCB;

    // カリング Compute Pipeline
    ComPtr<ID3D12RootSignature> m_cullRS;
    ComPtr<ID3D12PipelineState> m_cullPSO;
    DynamicBuffer m_cullCB;

    // カリング用バッファ
    ComPtr<ID3D12Resource> m_boundsBuffer;     // 入力: オブジェクトバウンド (UPLOAD)
    ComPtr<ID3D12Resource> m_visibilityBuffer; // 出力: 可視フラグ (UAV)
    ComPtr<ID3D12Resource> m_readbackBuffer;   // CPU読み取り用 (READBACK)
    uint32_t m_maxObjects = 0;

    static constexpr uint32_t k_MaxCullObjects = 4096;
};

/// @}
} // namespace gx
