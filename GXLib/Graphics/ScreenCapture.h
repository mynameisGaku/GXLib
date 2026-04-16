#pragma once
/// @file ScreenCapture.h
/// @brief スクリーンキャプチャ（バックバッファ→CPUメモリ→ファイル保存）
///
/// D3D12のバックバッファからReadbackヒープへコピーし、
/// CPUメモリに読み出して画像ファイルとして保存する。
/// @addtogroup grp_graphics/// @{

#include "pch_graphics.h"

namespace gx
{

class GraphicsDevice;

/// @brief スクリーンキャプチャ
class ScreenCapture
{
public:
    ScreenCapture() = default;
    ~ScreenCapture() = default;

    /// @brief 初期化する
    /// @param device グラフィックスデバイスへのポインタ
    /// @return 初期化成功時 true
    bool Initialize(ID3D12Device* device);

    /// @brief 現在のバックバッファをファイルに保存する
    /// @param cmdList コマンドリスト
    /// @param backBuffer バックバッファリソース
    /// @param width 幅
    /// @param height 高さ
    /// @param filePath 出力ファイルパス（.bmpのみ対応）
    /// @return 成功時 true
    bool CaptureToFile(ID3D12GraphicsCommandList* cmdList,
                       ID3D12Resource* backBuffer,
                       uint32_t width, uint32_t height,
                       const gx::String& filePath);

    /// @brief 現在のバックバッファをメモリに読み出す
    /// @param cmdList コマンドリスト
    /// @param backBuffer バックバッファリソース
    /// @param width 幅
    /// @param height 高さ
    /// @param outPixels 出力ピクセルデータ (RGBA8)
    /// @param outWidth 出力幅
    /// @param outHeight 出力高さ
    /// @return 成功時 true
    bool CaptureToMemory(ID3D12GraphicsCommandList* cmdList,
                         ID3D12Resource* backBuffer,
                         uint32_t width, uint32_t height,
                         gx::Vector<uint8_t>& outPixels,
                         uint32_t& outWidth, uint32_t& outHeight);

    /// @brief シャットダウン
    void Shutdown();

    /// @brief 初期化済みかどうか
    bool IsReady() const { return m_device != nullptr; }

private:
    ID3D12Device* m_device = nullptr;             ///< D3D12デバイス
    ComPtr<ID3D12Resource> m_readbackBuffer;       ///< リードバックバッファ（READBACKヒープ）
    uint32_t m_readbackWidth  = 0;                 ///< リードバックバッファの幅
    uint32_t m_readbackHeight = 0;                 ///< リードバックバッファの高さ
    uint64_t m_readbackRowPitch = 0;               ///< 行ピッチ（バイト）

    bool EnsureReadbackBuffer(uint32_t width, uint32_t height);
};

} // namespace gx
/// @}
