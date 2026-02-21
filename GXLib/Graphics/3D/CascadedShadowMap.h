#pragma once
/// @file CascadedShadowMap.h
/// @brief カスケードシャドウマップ（CSM）
/// DxLibの SetShadowMapDrawArea / SetUseShadowMap に相当する影の描画システム。
/// カメラの視錐台を4分割し、近距離ほど高解像度の影を割り当てる(PSSM方式)。

#include "pch.h"
#include "Graphics/3D/ShadowMap.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief シャドウ描画用の定数バッファ構造体
/// 4カスケード分のライト空間VP行列と分割距離をシェーダーに渡す。
struct ShadowConstants
{
    static constexpr uint32_t k_NumCascades = 4;

    XMFLOAT4X4 lightVP[k_NumCascades];      ///< カスケードごとのライト空間VP行列（転置済み）
    float      cascadeSplits[k_NumCascades]; ///< カスケード分割距離（ビュー空間Z）
    float      shadowMapSize;                ///< シャドウマップの解像度（ピクセル）
    float      padding[3];
};

/// @brief カスケードシャドウマップ管理クラス（4カスケード、4096x4096）
/// カメラのビュー空間を対数/線形ブレンドで分割し、各カスケードに正射影を割り当てる。
class CascadedShadowMap
{
public:
    static constexpr uint32_t k_NumCascades   = 4;     ///< カスケード段数
    static constexpr uint32_t k_ShadowMapSize = 4096;  ///< 各カスケードの解像度

    CascadedShadowMap() = default;
    ~CascadedShadowMap() = default;

    /// @brief CSMを初期化する（SRVは外部のシェーダー可視ヒープに配置）
    /// @param device D3D12デバイス
    /// @param srvHeap SRVを配置するヒープ
    /// @param srvStartIndex SRVの開始インデックス（連続k_NumCascades個使用する）
    /// @return 成功ならtrue
    bool Initialize(ID3D12Device* device, DescriptorHeap* srvHeap, uint32_t srvStartIndex);

    /// @brief カスケードの分割比率を設定する（各値は0~1の範囲）
    /// @param s0 カスケード0の終端比率
    /// @param s1 カスケード1の終端比率
    /// @param s2 カスケード2の終端比率
    /// @param s3 カスケード3の終端比率（通常1.0）
    void SetCascadeSplits(float s0, float s1, float s2, float s3);

    /// @brief カメラとライト方向からカスケードごとのライト空間VP行列を更新する
    /// @param camera 現在のカメラ
    /// @param lightDirection ディレクショナルライトの方向
    void Update(const Camera3D& camera, const XMFLOAT3& lightDirection);

    /// @brief 指定カスケードのシャドウマップを取得する
    /// @param cascade カスケードインデックス (0~3)
    /// @return シャドウマップ参照
    ShadowMap& GetShadowMap(uint32_t cascade) { return m_shadowMaps[cascade]; }

    /// @brief 指定カスケードのシャドウマップを取得する（const版）
    /// @param cascade カスケードインデックス (0~3)
    /// @return シャドウマップconst参照
    const ShadowMap& GetShadowMap(uint32_t cascade) const { return m_shadowMaps[cascade]; }

    /// @brief シャドウ定数バッファを取得する（シェーダーへの転送用）
    /// @return シャドウ定数
    const ShadowConstants& GetShadowConstants() const { return m_constants; }

    /// @brief 先頭カスケードのSRV GPUハンドルを取得する（ディスクリプタテーブルバインド用）
    /// @return GPUディスクリプタハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandleStart; }

private:
    /// 指定カスケードのライト空間正射影VP行列を計算する
    void ComputeCascadeLightVP(uint32_t cascade, const Camera3D& camera,
                                const XMFLOAT3& lightDirection,
                                float nearZ, float farZ);

    std::array<ShadowMap, k_NumCascades>  m_shadowMaps;
    ShadowConstants                       m_constants = {};
    D3D12_GPU_DESCRIPTOR_HANDLE           m_srvGPUHandleStart = {};

    /// カスケード分割比率（nearZ~farZの割合）
    std::array<float, k_NumCascades> m_cascadeRatios = { 0.05f, 0.15f, 0.4f, 1.0f };
};

} // namespace GX
