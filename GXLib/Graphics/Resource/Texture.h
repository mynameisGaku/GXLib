#pragma once
/// @file Texture.h
/// @brief GPUテクスチャリソースの読み込みと管理
///
/// DxLibの LoadGraph() で読み込む画像に相当するクラス。
/// 画像ファイルをstb_imageでデコードした後、以下の手順でGPUに転送する:
///   1. UPLOADヒープにステージングバッファを作成
///   2. DEFAULTヒープ（GPU専用メモリ）にテクスチャリソースを作成
///   3. CopyTextureRegionでステージング→テクスチャにコピー
///   4. SRV（Shader Resource View）を作成してシェーダーからアクセス可能にする
///
/// テクスチャの行ピッチは256バイトアライメントが必須（D3D12仕様）。

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief GPUテクスチャリソースを管理するクラス
///
/// 画像ファイルまたはCPUメモリのピクセルデータからGPUテクスチャを作成する。
/// SRVを通じてシェーダーからサンプリング可能。
class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    /// @brief 画像ファイルからテクスチャを読み込む
    /// @param device D3D12デバイス
    /// @param cmdQueue コピーコマンドの実行に使用するコマンドキュー
    /// @param filePath 画像ファイルのパス（PNG/JPG/BMP/TGA等、stb_image対応形式）
    /// @param srvHeap SRVを作成するディスクリプタヒープ
    /// @param srvIndex SRVの書き込み先インデックス
    /// @return 成功時true
    bool LoadFromFile(ID3D12Device* device,
                      ID3D12CommandQueue* cmdQueue,
                      const std::wstring& filePath,
                      DescriptorHeap* srvHeap,
                      uint32_t srvIndex);

    /// @brief CPUメモリのピクセルデータからテクスチャを作成する
    /// @param device D3D12デバイス
    /// @param cmdQueue コピーコマンドの実行に使用するコマンドキュー
    /// @param pixels RGBAピクセルデータ（1ピクセル4バイト）
    /// @param width テクスチャの幅（ピクセル）
    /// @param height テクスチャの高さ（ピクセル）
    /// @param srvHeap SRVを作成するディスクリプタヒープ
    /// @param srvIndex SRVの書き込み先インデックス
    /// @return 成功時true
    bool CreateFromMemory(ID3D12Device* device,
                          ID3D12CommandQueue* cmdQueue,
                          const void* pixels,
                          uint32_t width, uint32_t height,
                          DescriptorHeap* srvHeap,
                          uint32_t srvIndex);

    /// @brief SRVのGPUディスクリプタハンドルを取得する
    /// @return シェーダーにバインドするためのGPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandle; }

    /// @brief 既存テクスチャのピクセルデータを差し替える
    ///
    /// リソースとSRVはそのまま維持し、内容だけを更新する。
    /// サイズは元のテクスチャと一致する必要がある。
    /// @param device D3D12デバイス
    /// @param cmdQueue コピーコマンドの実行に使用するコマンドキュー
    /// @param pixels 新しいRGBAピクセルデータ
    /// @param width テクスチャの幅（元と同じ値を指定）
    /// @param height テクスチャの高さ（元と同じ値を指定）
    /// @return 成功時true
    bool UpdatePixels(ID3D12Device* device,
                      ID3D12CommandQueue* cmdQueue,
                      const void* pixels,
                      uint32_t width, uint32_t height);

    /// @brief テクスチャの幅を取得する
    /// @return ピクセル単位の幅
    uint32_t GetWidth() const { return m_width; }

    /// @brief テクスチャの高さを取得する
    /// @return ピクセル単位の高さ
    uint32_t GetHeight() const { return m_height; }

    /// @brief テクスチャフォーマットを取得する
    /// @return DXGI_FORMAT（通常はR8G8B8A8_UNORM）
    DXGI_FORMAT GetFormat() const { return m_format; }

    /// @brief 内部のID3D12Resourceを取得する
    /// @return リソースポインタ（未作成時はnullptr）
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

private:
    /// GPU側テクスチャリソースを作成し、ピクセルデータをアップロードする（内部実装）
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
    uint32_t    m_srvIndex = 0;   ///< SRVのヒープ内インデックス
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

} // namespace GX
