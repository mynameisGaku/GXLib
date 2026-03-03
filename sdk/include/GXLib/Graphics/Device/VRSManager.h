#pragma once
/// @file VRSManager.h
/// @brief Variable Rate Shading (VRS) 管理
///
/// VRS (可変レートシェーディング) はピクセルシェーダの実行頻度を領域ごとに変えることで、
/// 画質を維持しつつGPU負荷を下げる技術。
/// Tier 1: DrawCall単位で全画面に均一なシェーディングレートを指定
/// Tier 2: Tier 1に加えて、タイル単位のシェーディングレート画像を使って領域ごとに制御可能
///
/// 例えばUI部分や画面端は2x2、中央の注視領域は1x1といった使い分けができる。
/// @addtogroup grp_gfx_device/// @{

#include "pch_graphics.h"
#include "Graphics/Pipeline/Shader.h"

namespace gx
{

/// @brief VRS対応レベル
enum class VRSTier : uint32_t
{
    None = 0,  ///< VRS非対応
    Tier1,     ///< DrawCall単位のシェーディングレート指定
    Tier2,     ///< Tier1 + タイル単位のシェーディングレート画像
};

/// @brief VRS動作モード
enum class VRSMode : uint32_t
{
    Off = 0,    ///< VRS無効
    PerDraw,    ///< DrawCall単位 (Tier 1)
    Image,      ///< シェーディングレート画像 (Tier 2)
};

/// @brief VRS (Variable Rate Shading) を管理するクラス
///
/// GPUのVRS対応レベルを検出し、シェーディングレートの設定やSRI画像の管理を行う。
class VRSManager
{
public:
    /// @brief VRS機能の初期化とGPU対応レベルの検出
    /// @param device D3D12デバイス
    /// @param width 画面幅 (SRI画像サイズの算出に使用)
    /// @param height 画面高さ
    /// @return 初期化成功でtrue (VRS非対応でもtrue)
    bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief サポートされているVRSティアを取得
    VRSTier GetSupportedTier() const { return m_tier; }

    /// @brief DrawCall単位のシェーディングレートを設定する
    /// @param cmdList コマンドリスト (ID3D12GraphicsCommandList5)
    /// @param rate シェーディングレート (1x1, 1x2, 2x1, 2x2, 2x4, 4x2, 4x4)
    void SetShadingRate(ID3D12GraphicsCommandList5* cmdList, D3D12_SHADING_RATE rate);

    /// @brief シェーディングレートの合成方法を設定する
    /// @param cmdList コマンドリスト
    /// @param combiner0 頂点シェーダとDrawCall設定の合成方法
    /// @param combiner1 DrawCall設定とSRI画像の合成方法
    void SetShadingRateCombiner(ID3D12GraphicsCommandList5* cmdList,
                                D3D12_SHADING_RATE_COMBINER combiner0,
                                D3D12_SHADING_RATE_COMBINER combiner1);

    /// @brief シェーディングレート画像 (SRI) を作成する (Tier 2)
    /// @param device D3D12デバイス
    /// @param width 画面幅
    /// @param height 画面高さ
    /// @return 成功でtrue
    bool CreateShadingRateImage(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief SRI画像を生成する（輝度分散や速度ベクトルに基づく）
    /// @param cmdList コマンドリスト
    /// @param hdrRT HDRレンダーターゲット（輝度分析用）
    /// @param velocityRT 速度バッファ（動き分析用、nullptrなら輝度のみ）
    void GenerateShadingRateImage(ID3D12GraphicsCommandList* cmdList,
                                  ID3D12Resource* hdrRT, ID3D12Resource* velocityRT);

    /// @brief SRI画像をパイプラインにバインドする
    /// @param cmdList コマンドリスト
    void BindShadingRateImage(ID3D12GraphicsCommandList5* cmdList);

    /// @brief 画面サイズ変更時にSRI画像を再作成する
    /// @param device D3D12デバイス
    /// @param width 新しい幅
    /// @param height 新しい高さ
    void OnResize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief SRI画像リソースを取得する
    /// @return ID3D12Resourceポインタ (Tier2非対応の場合nullptr)
    ID3D12Resource* GetShadingRateImage() const { return m_shadingRateImage.Get(); }

private:
    VRSTier  m_tier     = VRSTier::None;  ///< GPU対応レベル
    uint32_t m_tileSize = 16;             ///< SRIタイルサイズ（ピクセル単位）
    ComPtr<ID3D12Resource> m_shadingRateImage; ///< シェーディングレート画像
    uint32_t m_sriWidth  = 0;             ///< SRI画像幅（タイル数）
    uint32_t m_sriHeight = 0;             ///< SRI画像高さ（タイル数）

    Shader m_shader;                                      ///< シェーダコンパイラ
    ComPtr<ID3D12RootSignature> m_computeRootSig;         ///< コンピュートルートシグネチャ
    ComPtr<ID3D12PipelineState> m_computePSO;             ///< コンピュートPSO
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;        ///< SRV/UAVディスクリプタヒープ
    uint32_t m_screenWidth = 0, m_screenHeight = 0;       ///< 画面サイズ
    bool m_computeReady = false;                          ///< コンピュートパイプライン構築済みフラグ
};

} // namespace gx
/// @}
