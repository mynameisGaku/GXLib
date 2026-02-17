#pragma once
/// @file RTAccelerationStructure.h
/// @brief DXR アクセラレーション構造 (BLAS/TLAS) 管理クラス

#include "pch.h"
#include "Graphics/Resource/Buffer.h"

namespace GX
{

/// @brief DXR アクセラレーション構造を管理するクラス
/// BLASの構築・キャッシュとTLASのフレーム毎の再構築を担当
class RTAccelerationStructure
{
public:
    static constexpr uint32_t k_MaxInstances = 512;

    RTAccelerationStructure() = default;
    ~RTAccelerationStructure() = default;

    /// @brief 初期化
    bool Initialize(ID3D12Device5* device);

    /// @brief BLASを構築してキャッシュに追加
    /// @param cmdList DXR対応コマンドリスト
    /// @param vb 頂点バッファ
    /// @param vertexCount 頂点数
    /// @param vertexStride 頂点ストライド (bytes)
    /// @param ib インデックスバッファ
    /// @param indexCount インデックス数
    /// @param indexFormat インデックスフォーマット
    /// @return BLASインデックス (-1で失敗)
    int BuildBLAS(ID3D12GraphicsCommandList4* cmdList,
                  ID3D12Resource* vb, uint32_t vertexCount, uint32_t vertexStride,
                  ID3D12Resource* ib, uint32_t indexCount,
                  DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);

    /// @brief フレーム開始時にインスタンスリストをクリア
    void BeginFrame();

    /// @brief TLASインスタンスを追加
    /// @param blasIndex BuildBLASで取得したインデックス
    /// @param worldMatrix ワールド変換行列 (row-major)
    /// @param instanceID シェーダー側で参照するインスタンスID
    /// @param mask インスタンスマスク
    void AddInstance(int blasIndex, const XMMATRIX& worldMatrix,
                     uint32_t instanceID = 0, uint8_t mask = 0xFF,
                     uint32_t instanceFlags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE);

    /// @brief TLASを構築する
    /// @param cmdList DXR対応コマンドリスト
    /// @param frameIndex ダブルバッファ用フレームインデックス
    void BuildTLAS(ID3D12GraphicsCommandList4* cmdList, uint32_t frameIndex);

    /// @brief TLAS の GPU アドレスを取得
    D3D12_GPU_VIRTUAL_ADDRESS GetTLASAddress() const;

private:
    ID3D12Device5* m_device = nullptr;

    // BLAS キャッシュ
    struct BLASEntry
    {
        Buffer result;      // BLAS 結果バッファ (DEFAULT heap)
        Buffer scratch;     // スクラッチバッファ (ビルド後も保持 — update 用)
    };
    std::vector<BLASEntry> m_blasCache;

    // TLAS (ダブルバッファ)
    static constexpr uint32_t k_BufferCount = 2;
    Buffer m_tlasResult[k_BufferCount];
    Buffer m_tlasScratch[k_BufferCount];
    Buffer m_instanceDescBuffer[k_BufferCount]; // UPLOAD heap

    // フレームごとのインスタンスリスト
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instances;

    // 最後にBuildTLASしたバッファインデックス
    uint32_t m_lastBuiltBufIdx = 0;
};

} // namespace GX
