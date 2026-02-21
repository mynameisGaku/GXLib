#pragma once
/// @file Mesh.h
/// @brief GPUメッシュ（頂点バッファ / インデックスバッファ + サブメッシュ管理）

#include "pch.h"
#include "Graphics/Resource/Buffer.h"

namespace GX
{

/// @brief サブメッシュ情報（1つのメッシュ内でマテリアルが異なる範囲を表す）
struct SubMesh
{
    uint32_t indexCount   = 0;    ///< 描画するインデックス数
    uint32_t indexOffset  = 0;    ///< インデックスバッファ内の開始位置
    uint32_t vertexOffset = 0;    ///< 頂点バッファ内のベースオフセット
    int      materialHandle = -1; ///< マテリアルハンドル（-1 = デフォルト）
    int      shaderHandle   = -1; ///< カスタムシェーダーハンドル（-1 = PBR標準）
};

/// @brief メッシュ頂点レイアウト種別
enum class MeshVertexType
{
    PBR,        ///< Vertex3D_PBR（静的メッシュ、48B）
    SkinnedPBR  ///< Vertex3D_Skinned（スキニングメッシュ、80B）
};

/// @brief GPUメッシュリソース（単一VB/IB + 複数サブメッシュ）
/// Model内に保持され、Renderer3Dが描画時に参照する。
/// DxLibではMV1LoadModel内部で自動生成されるGPUリソースに相当する
class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    /// @brief 頂点バッファをGPU上に作成する
    /// @param device D3D12デバイス
    /// @param data 頂点データのポインタ
    /// @param size データサイズ（バイト）
    /// @param stride 1頂点あたりのバイト数
    /// @return 成功した場合true
    bool CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride);

    /// @brief インデックスバッファをGPU上に作成する
    /// @param device D3D12デバイス
    /// @param data インデックスデータのポインタ
    /// @param size データサイズ（バイト）
    /// @param format インデックスフォーマット（R32_UINT または R16_UINT）
    /// @return 成功した場合true
    bool CreateIndexBuffer(ID3D12Device* device, const void* data, uint32_t size,
                           DXGI_FORMAT format = DXGI_FORMAT_R32_UINT);

    /// @brief サブメッシュを追加する
    /// @param subMesh サブメッシュ情報
    void AddSubMesh(const SubMesh& subMesh) { m_subMeshes.push_back(subMesh); }

    /// @brief 頂点バッファを取得する
    /// @return 頂点 Buffer への const 参照
    const Buffer& GetVertexBuffer() const { return m_vertexBuffer; }

    /// @brief インデックスバッファを取得する
    /// @return インデックス Buffer への const 参照
    const Buffer& GetIndexBuffer() const { return m_indexBuffer; }

    /// @brief サブメッシュ配列を取得する（const）
    /// @return サブメッシュ配列への const 参照
    const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }

    /// @brief サブメッシュ配列を取得する（変更可）
    /// @return サブメッシュ配列への参照
    std::vector<SubMesh>& GetSubMeshes() { return m_subMeshes; }

    /// @brief 頂点タイプを設定する
    /// @param type PBR または SkinnedPBR
    void SetVertexType(MeshVertexType type) { m_vertexType = type; }

    /// @brief 頂点タイプを取得する
    /// @return 現在の頂点タイプ
    MeshVertexType GetVertexType() const { return m_vertexType; }

    /// @brief スキニングメッシュかどうかを判定する
    /// @return SkinnedPBR の場合true
    bool IsSkinned() const { return m_vertexType == MeshVertexType::SkinnedPBR; }

    /// @brief Toonアウトライン用スムース法線バッファを作成する
    /// @param device D3D12デバイス
    /// @param data スムース法線データ配列
    /// @param vertexCount 頂点数
    /// @return 成功した場合true
    bool CreateSmoothNormalBuffer(ID3D12Device* device, const XMFLOAT3* data, uint32_t vertexCount);

    /// @brief スムース法線バッファのVBVを取得する（slot 1 にバインドして使う）
    /// @return 頂点バッファビュー
    const D3D12_VERTEX_BUFFER_VIEW& GetSmoothNormalBufferView() const { return m_smoothNormalBuffer.GetVertexBufferView(); }

    /// @brief スムース法線バッファが存在するかチェックする
    /// @return バッファが作成済みならtrue
    bool HasSmoothNormals() const { return m_hasSmoothNormals; }

private:
    Buffer m_vertexBuffer;          ///< GPU頂点バッファ
    Buffer m_indexBuffer;           ///< GPUインデックスバッファ
    Buffer m_smoothNormalBuffer;    ///< スムース法線バッファ（Toonアウトライン用）
    std::vector<SubMesh> m_subMeshes;  ///< サブメッシュ配列
    MeshVertexType m_vertexType = MeshVertexType::PBR;
    bool m_hasSmoothNormals = false;
};

/// @brief 同一位置の頂点法線を平均化してスムース法線を計算する
/// @param positions 頂点位置配列
/// @param normals 頂点法線配列
/// @param vertexCount 頂点数
/// @return スムース法線配列
std::vector<XMFLOAT3> ComputeSmoothNormals(const XMFLOAT3* positions, const XMFLOAT3* normals, uint32_t vertexCount);

} // namespace GX
