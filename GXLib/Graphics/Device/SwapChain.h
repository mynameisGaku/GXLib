#pragma once
/// @file SwapChain.h
/// @brief スワップチェーン管理クラス
///
/// 【初学者向け解説】
/// スワップチェーンとは、画面に表示する画像を管理する仕組みです。
///
/// 「ダブルバッファリング」という手法を使います：
/// - バッファA: 現在画面に表示中（フロントバッファ）
/// - バッファB: 裏で次のフレームを描画中（バックバッファ）
/// 描画が完了したらA⇔Bを交換（Present）して、ちらつきなく滑らかに表示します。
///
/// イメージとしては、画家が2枚のキャンバスを使い、
/// 1枚を展示しながら別の1枚に次の絵を描いているようなものです。

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief スワップチェーンの設定構造体
struct SwapChainDesc
{
    HWND     hwnd        = nullptr;   ///< 描画先のウィンドウハンドル
    uint32_t width       = 1280;      ///< バックバッファの幅（ピクセル）
    uint32_t height      = 720;       ///< バックバッファの高さ（ピクセル）
    bool     vsync       = false;     ///< 垂直同期を有効にするかどうか
};

/// @brief スワップチェーンを管理するクラス
class SwapChain
{
public:
    /// @brief ダブルバッファリング用のバッファ数
    static constexpr uint32_t k_BufferCount = 2;

    SwapChain() = default;
    ~SwapChain() = default;

    /// @brief スワップチェーンを作成する
    /// @param factory DXGIファクトリへのポインタ
    /// @param device D3D12デバイスへのポインタ
    /// @param queue コマンドキューへのポインタ
    /// @param desc スワップチェーンの設定
    /// @return 作成に成功した場合true
    bool Initialize(IDXGIFactory6* factory,
                    ID3D12Device* device,
                    ID3D12CommandQueue* queue,
                    const SwapChainDesc& desc);

    /// @brief 画面に表示する（フロントバッファとバックバッファを交換）
    /// @param vsync 垂直同期を有効にするかどうか
    void Present(bool vsync);

    /// @brief ウィンドウサイズ変更時にバッファをリサイズする
    /// @param device D3D12デバイスへのポインタ
    /// @param width 新しい幅（ピクセル）
    /// @param height 新しい高さ（ピクセル）
    /// @return リサイズに成功した場合true
    bool Resize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief 現在のバックバッファリソースを取得する
    /// @return ID3D12Resourceへのポインタ
    ID3D12Resource* GetCurrentBackBuffer() const;

    /// @brief 現在のバックバッファのRTVハンドルを取得する
    /// @return RTVのCPUディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const;

    /// @brief 現在のバックバッファインデックスを取得する
    /// @return バックバッファインデックス（0またはk_BufferCount-1）
    uint32_t GetCurrentBackBufferIndex() const;

    /// @brief バックバッファの幅を取得する
    /// @return 幅（ピクセル）
    uint32_t GetWidth() const { return m_width; }

    /// @brief バックバッファの高さを取得する
    /// @return 高さ（ピクセル）
    uint32_t GetHeight() const { return m_height; }

    /// @brief バックバッファのフォーマットを取得する
    /// @return DXGI_FORMAT値
    DXGI_FORMAT GetFormat() const { return m_format; }

private:
    void CreateRenderTargetViews(ID3D12Device* device);

    ComPtr<IDXGISwapChain4>                         m_swapChain;
    std::array<ComPtr<ID3D12Resource>, k_BufferCount> m_backBuffers;
    DescriptorHeap                                   m_rtvHeap;

    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
};

} // namespace GX
