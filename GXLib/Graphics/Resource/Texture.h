#pragma once
/// @file Texture.h
/// @brief テクスチャリソース管理クラス
///
/// 【初学者向け解説】
/// テクスチャとは、3Dモデルや2Dスプライトに貼り付ける画像のことです。
/// GPUがテクスチャを読めるようにするには：
/// 1. stb_imageで画像ファイルをCPUメモリに読み込む
/// 2. UPLOADヒープにステージングバッファを作成
/// 3. DEFAULTヒープ（GPU専用メモリ）にテクスチャリソースを作成
/// 4. CopyTextureRegionでステージング→テクスチャにコピー
/// 5. SRV（Shader Resource View）を作成してシェーダーからアクセス可能にする

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief GPUテクスチャリソースを管理するクラス
class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    /// 画像ファイルからテクスチャを読み込み
    /// @param device D3D12デバイス
    /// @param cmdQueue コピーコマンド実行用キュー
    /// @param filePath 画像ファイルパス
    /// @param srvHeap SRV用ディスクリプタヒープ
    /// @param srvIndex SRVを書き込むインデックス
    bool LoadFromFile(ID3D12Device* device,
                      ID3D12CommandQueue* cmdQueue,
                      const std::wstring& filePath,
                      DescriptorHeap* srvHeap,
                      uint32_t srvIndex);

    /// CPUメモリのピクセルデータからテクスチャを作成
    bool CreateFromMemory(ID3D12Device* device,
                          ID3D12CommandQueue* cmdQueue,
                          const void* pixels,
                          uint32_t width, uint32_t height,
                          DescriptorHeap* srvHeap,
                          uint32_t srvIndex);

    /// SRVのGPUハンドルを取得
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandle; }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    DXGI_FORMAT GetFormat() const { return m_format; }
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

private:
    /// GPU側テクスチャリソースを作成してデータをコピー
    bool UploadToGPU(ID3D12Device* device,
                     ID3D12CommandQueue* cmdQueue,
                     const void* pixels,
                     uint32_t width, uint32_t height,
                     DescriptorHeap* srvHeap,
                     uint32_t srvIndex);

    ComPtr<ID3D12Resource>       m_resource;
    D3D12_GPU_DESCRIPTOR_HANDLE  m_srvGPUHandle = {};
    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    uint32_t    m_srvIndex = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

} // namespace GX
