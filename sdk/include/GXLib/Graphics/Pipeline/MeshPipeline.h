#pragma once
/// @file MeshPipeline.h
/// @brief メッシュシェーダパイプライン管理
///
/// メッシュシェーダはDX12 Ultimateで導入された新しいジオメトリ処理パイプライン。
/// 従来のVertex/Index Buffer + Input Assemblerを、
/// Amplification Shader (AS) + Mesh Shader (MS) に置き換える。
///
/// AS: メッシュレット単位でフラスタムカリング等を行い、可視メッシュレットだけをMSに渡す
/// MS: メッシュレットから頂点/プリミティブを出力する（従来のVS+GS相当）
///
/// GPUドリブンレンダリングとの組み合わせで、CPU→GPU間のDrawCallボトルネックを解消できる。
/// @addtogroup grp_gfx_pipeline/// @{

#include "pch_graphics.h"

namespace gx
{

/// @brief メッシュシェーダ対応レベル
enum class MeshShaderTier : uint32_t
{
    None = 0,  ///< メッシュシェーダ非対応
    Tier1,     ///< メッシュシェーダ対応 (Tier 1.0)
};

/// @brief メッシュシェーダPSO作成に必要な記述子
struct MeshPipelineDesc
{
    const void* asBlob     = nullptr;  ///< Amplification Shader バイトコード
    uint32_t    asBlobSize = 0;        ///< AS バイトコードサイズ
    const void* msBlob     = nullptr;  ///< Mesh Shader バイトコード
    uint32_t    msBlobSize = 0;        ///< MS バイトコードサイズ
    const void* psBlob     = nullptr;  ///< Pixel Shader バイトコード
    uint32_t    psBlobSize = 0;        ///< PS バイトコードサイズ
    ID3D12RootSignature* rootSignature = nullptr; ///< ルートシグネチャ
    DXGI_FORMAT rtvFormat    = DXGI_FORMAT_R16G16B16A16_FLOAT; ///< レンダーターゲットフォーマット
    DXGI_FORMAT dsvFormat    = DXGI_FORMAT_D32_FLOAT;          ///< 深度バッファフォーマット
    uint32_t    numRenderTargets = 1;  ///< レンダーターゲット数
};

/// @brief メッシュシェーダパイプラインの作成とディスパッチを行うクラス
///
/// GPU対応レベルを検出し、ストリームベースPSO (D3D12_PIPELINE_STATE_STREAM_DESC) で
/// メッシュシェーダPSOを作成する。DispatchMeshでメッシュレットを処理する。
class MeshPipeline
{
public:
    /// @brief GPUのメッシュシェーダ対応レベルを検出する
    /// @param device D3D12デバイス
    /// @return 対応レベル
    static MeshShaderTier DetectTier(ID3D12Device* device);

    /// @brief メッシュシェーダPSOを作成する
    /// @param device D3D12Device2 (ストリームベースPSO作成に必要)
    /// @param desc パイプライン記述子
    /// @return 作成されたPSOのComPtr (失敗時はnull)
    ComPtr<ID3D12PipelineState> Build(ID3D12Device2* device, const MeshPipelineDesc& desc);

    /// @brief メッシュシェーダをディスパッチする
    /// @param cmdList コマンドリスト (ID3D12GraphicsCommandList6)
    /// @param x スレッドグループ数X
    /// @param y スレッドグループ数Y
    /// @param z スレッドグループ数Z
    static void DispatchMesh(ID3D12GraphicsCommandList6* cmdList,
                             uint32_t x, uint32_t y, uint32_t z);

private:
    MeshShaderTier m_tier = MeshShaderTier::None;
};

} // namespace gx
/// @}
