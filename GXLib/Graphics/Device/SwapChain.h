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

/// @brief スワップチェーンの設定
struct SwapChainDesc
{
    HWND     hwnd        = nullptr;
    uint32_t width       = 1280;
    uint32_t height      = 720;
    bool     vsync       = false;
};

/// @brief スワップチェーンを管理するクラス
class SwapChain
{
public:
    static constexpr uint32_t k_BufferCount = 2;

    SwapChain() = default;
    ~SwapChain() = default;

    /// スワップチェーンを作成
    bool Initialize(IDXGIFactory6* factory,
                    ID3D12Device* device,
                    ID3D12CommandQueue* queue,
                    const SwapChainDesc& desc);

    /// 画面に表示（バッファを交換）
    void Present(bool vsync);

    /// ウィンドウサイズ変更時にバッファをリサイズ
    bool Resize(ID3D12Device* device, uint32_t width, uint32_t height);

    /// 現在のバックバッファを取得
    ID3D12Resource* GetCurrentBackBuffer() const;

    /// 現在のバックバッファのRTVハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const;

    /// 現在のバックバッファインデックスを取得
    uint32_t GetCurrentBackBufferIndex() const;

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
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
