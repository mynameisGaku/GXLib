#pragma once
/// @file GPUDrivenRenderer.h
/// @brief GPUドリブンレンダリング（メッシュシェーダベース）
///
/// メッシュレット単位でジオメトリを管理し、メッシュシェーダで描画する。
/// 従来のCPU側のDrawCallループを排し、GPU上でメッシュレットの選別と描画を一括実行する。
///
/// メッシュレット: メッシュを最大64頂点・最大126プリミティブの小ブロックに分割したもの。
/// Amplification Shaderがフラスタムカリングを行い、可視メッシュレットだけをMesh Shaderに渡す。
/// @addtogroup grp_gfx_3d/// @{

#include "pch_graphics.h"
#include "Graphics/Pipeline/MeshPipeline.h"
#include "Graphics/Pipeline/Shader.h"
#include "Graphics/Resource/Buffer.h"

namespace gx
{

/// @brief メッシュレットの記述子（GPUバッファに格納される）
struct MeshletData
{
    uint32_t vertexCount;      ///< このメッシュレットの頂点数（最大64）
    uint32_t vertexOffset;     ///< 頂点バッファ内のオフセット
    uint32_t primitiveCount;   ///< プリミティブ数（最大126）
    uint32_t primitiveOffset;  ///< プリミティブインデックスバッファ内のオフセット
};

/// @brief GPUドリブンレンダラー
///
/// メッシュシェーダ対応GPUでのみ動作する。非対応GPUでは IsAvailable()==false。
/// 使い方:
///   1. Initialize() でGPU対応確認
///   2. UploadMeshlets() でメッシュレットデータをGPUにアップロード
///   3. Render() でメッシュシェーダを使った描画を実行
class GPUDrivenRenderer
{
public:
    /// @brief 初期化。メッシュシェーダの対応レベルを確認する
    /// @param device D3D12デバイス
    /// @param device2 D3D12Device2 (ストリームベースPSO作成用)
    /// @return 初期化成功でtrue（メッシュシェーダ非対応でもtrueを返す）
    bool Initialize(ID3D12Device* device, ID3D12Device2* device2);

    /// @brief メッシュシェーダベースの描画が利用可能か
    /// @return メッシュシェーダ対応GPUならtrue
    bool IsAvailable() const { return m_available; }

    /// @brief メッシュレットデータをGPUにアップロードする
    /// @param device D3D12デバイス
    /// @param data メッシュレット配列の先頭ポインタ
    /// @param count メッシュレット数
    void UploadMeshlets(ID3D12Device* device, const MeshletData* data, uint32_t count);

    /// @brief メッシュシェーダによる描画を実行する
    /// @param cmdList コマンドリスト (ID3D12GraphicsCommandList6)
    /// @param frameIndex ダブルバッファ用フレームインデックス
    /// @param viewProj ビュープロジェクション行列（フラスタムカリング用）
    /// @param meshletCount 描画するメッシュレット数
    void Render(ID3D12GraphicsCommandList6* cmdList, uint32_t frameIndex,
                const DirectX::XMFLOAT4X4& viewProj, uint32_t meshletCount);

    /// @brief アップロード済みメッシュレット数を取得する
    uint32_t GetMeshletCount() const { return m_meshletCount; }

    // === GPU-Driven Indirect Draw ===

    /// @brief GPU間接描画コマンド
    struct GPUDrawCommand {
        D3D12_DRAW_INDEXED_ARGUMENTS drawArgs;  ///< 間接描画引数
        uint32_t meshletIndex;                   ///< メッシュレットインデックス
        uint32_t objectIndex;                    ///< オブジェクトインデックス
    };

    /// @brief メッシュレットバウンディング球
    struct MeshletBounds {
        XMFLOAT3 center;   ///< 球の中心座標
        float radius;       ///< 球の半径
    };

    /// @brief 間接描画パイプラインを作成
    void CreateIndirectPipeline(ID3D12Device* device);

    /// @brief メッシュレットバウンズをアップロード
    void UploadMeshletBounds(ID3D12Device* device, const MeshletBounds* bounds, uint32_t count);

    /// @brief フラスタムカリングして間接描画を実行
    void CullAndDraw(ID3D12GraphicsCommandList* cmdList, const XMFLOAT4X4& viewProj, uint32_t objectCount);

    /// @brief 可視オブジェクト数を取得
    uint32_t GetVisibleCount() const { return m_visibleCount; }

    /// @brief 間接描画パイプラインが準備済みか
    bool IsIndirectReady() const { return m_indirectReady; }

private:
    bool            m_available    = false;              ///< メッシュシェーダ利用可能フラグ
    MeshShaderTier  m_tier         = MeshShaderTier::None; ///< メッシュシェーダ対応レベル
    Buffer          m_meshletBuffer;                      ///< メッシュレットデータ用GPUバッファ
    uint32_t        m_meshletCount = 0;                  ///< メッシュレット数

    // 間接描画メンバー
    ComPtr<ID3D12Resource> m_indirectArgBuffer;        ///< 間接描画引数バッファ
    ComPtr<ID3D12CommandSignature> m_commandSignature; ///< コマンドシグネチャ
    ComPtr<ID3D12PipelineState> m_cullPSO;             ///< カリング用ComputePSO
    ComPtr<ID3D12RootSignature> m_cullRootSignature;   ///< カリング用ルートシグネチャ
    Buffer m_boundsBuffer;                              ///< バウンディング球バッファ
    uint32_t m_boundsCount = 0;                        ///< バウンディング球数
    uint32_t m_visibleCount = 0;                       ///< 可視オブジェクト数
    bool m_indirectReady = false;                      ///< 間接描画パイプライン準備済みフラグ

    Shader m_shader;                                      ///< シェーダコンパイラ
    ComPtr<ID3D12RootSignature> m_rootSignature;          ///< ルートシグネチャ
    ComPtr<ID3D12PipelineState> m_meshPSO;                ///< メッシュシェーダPSO
    ID3D12Device2* m_device2 = nullptr;                   ///< Device2 (ストリームPSO用)
    MeshPipeline m_meshPipeline;                          ///< メッシュパイプラインビルダー
    ComPtr<ID3D12Resource> m_sceneCBUpload;               ///< シーン定数バッファ (UPLOAD)
    bool m_psoReady = false;                              ///< PSO構築済みフラグ
};

} // namespace gx
/// @}
