#pragma once
/// @file CascadedShadowMap.h
/// @brief カスケードシャドウマップ（CSM）

#include "pch.h"
#include "Graphics/3D/ShadowMap.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// シャドウ定数バッファ（FrameConstantsに追加するデータ）
struct ShadowConstants
{
    static constexpr uint32_t k_NumCascades = 4;

    XMFLOAT4X4 lightVP[k_NumCascades];    // ライト空間のVP行列
    float      cascadeSplits[k_NumCascades]; // カスケード分割距離（ビュー空間Z）
    float      shadowMapSize;               // シャドウマップ解像度
    float      padding[3];
};

/// @brief カスケードシャドウマップ管理クラス
class CascadedShadowMap
{
public:
    static constexpr uint32_t k_NumCascades   = 4;
    static constexpr uint32_t k_ShadowMapSize = 4096;

    CascadedShadowMap() = default;
    ~CascadedShadowMap() = default;

    /// 初期化（SRVは外部ヒープに配置）
    /// @param srvHeap シェーダー可視SRVヒープ（TextureManager等）
    /// @param srvStartIndex SRVの開始インデックス（連続k_NumCascades個使用）
    bool Initialize(ID3D12Device* device, DescriptorHeap* srvHeap, uint32_t srvStartIndex);

    /// カスケードの分割比率を設定（0-1, デフォルト: 0.07, 0.2, 0.5, 1.0）
    void SetCascadeSplits(float s0, float s1, float s2, float s3);

    /// カメラとライト方向からライト空間VP行列を計算
    void Update(const Camera3D& camera, const XMFLOAT3& lightDirection);

    /// 指定カスケードのシャドウマップ取得
    ShadowMap& GetShadowMap(uint32_t cascade) { return m_shadowMaps[cascade]; }
    const ShadowMap& GetShadowMap(uint32_t cascade) const { return m_shadowMaps[cascade]; }

    /// シャドウ定数を取得
    const ShadowConstants& GetShadowConstants() const { return m_constants; }

    /// 先頭カスケードのSRV GPUハンドルを取得（ディスクリプタテーブルバインド用）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandleStart; }

private:
    void ComputeCascadeLightVP(uint32_t cascade, const Camera3D& camera,
                                const XMFLOAT3& lightDirection,
                                float nearZ, float farZ);

    std::array<ShadowMap, k_NumCascades>  m_shadowMaps;
    ShadowConstants                       m_constants = {};
    D3D12_GPU_DESCRIPTOR_HANDLE           m_srvGPUHandleStart = {};

    // カスケード分割比率（nearZ～farZの割合）
    std::array<float, k_NumCascades> m_cascadeRatios = { 0.05f, 0.15f, 0.4f, 1.0f };
};

} // namespace GX
