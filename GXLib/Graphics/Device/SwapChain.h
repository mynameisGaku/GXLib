#pragma once
/// @file SwapChain.h
/// @brief ダブルバッファリングによる画面表示管理
///
/// DxLibのScreenFlip()に相当する仕組み。
/// 2枚のバックバッファを交互に使い、片方に描画しつつ片方を画面に表示する。
/// ウィンドウリサイズ時はバッファを再作成する必要がある。

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

class Fence;

/// @brief スワップチェーン作成時の設定
struct SwapChainDesc
{
    HWND     hwnd        = nullptr;   ///< 描画先のウィンドウハンドル
    uint32_t width       = 1280;      ///< バックバッファの幅（ピクセル）
    uint32_t height      = 720;       ///< バックバッファの高さ（ピクセル）
    bool     vsync       = false;     ///< 垂直同期（trueでティアリング防止、ただしフレームレート制限あり）
};

/// @brief 画面表示のダブルバッファリングを管理する（DxLibのScreenFlip相当）
///
/// Present()でフロント/バックバッファを切り替えて画面に反映する。
/// バッファのフォーマットはR8G8B8A8_UNORM（LDR）。HDRパイプラインの最終出力先。
class SwapChain
{
public:
    /// @brief バッファ数（2 = ダブルバッファリング）
    static constexpr uint32_t k_BufferCount = 2;

    SwapChain() = default;
    ~SwapChain() = default;

    /// @brief スワップチェーンを作成する
    /// @param factory DXGIファクトリへのポインタ
    /// @param device D3D12デバイスへのポインタ
    /// @param queue コマンドキューへのポインタ（Present時の送信先）
    /// @param desc ウィンドウハンドル、解像度、vsyncなどの設定
    /// @return 成功なら true
    bool Initialize(IDXGIFactory6* factory,
                    ID3D12Device* device,
                    ID3D12CommandQueue* queue,
                    const SwapChainDesc& desc);

    /// @brief バックバッファを画面に表示する（DxLibのScreenFlip()に相当）
    /// @param vsync trueで垂直同期待ち
    void Present(bool vsync);

    /// @brief ウィンドウリサイズ時にバッファサイズを変更する
    /// @param device D3D12デバイスへのポインタ
    /// @param width 新しい幅（ピクセル）
    /// @param height 新しい高さ（ピクセル）
    /// @param queue GPU同期用（nullptrなら呼び出し元が同期済みと想定）
    /// @param fence GPU同期用（nullptrなら呼び出し元が同期済みと想定）
    /// @return 成功なら true
    bool Resize(ID3D12Device* device, uint32_t width, uint32_t height,
                ID3D12CommandQueue* queue = nullptr, Fence* fence = nullptr);

    /// @brief 現在描画先になっているバックバッファを取得する
    /// @return ID3D12Resourceへのポインタ
    ID3D12Resource* GetCurrentBackBuffer() const;

    /// @brief 現在のバックバッファに対応するRTVハンドルを取得する
    /// @return RTV(RenderTargetView)のCPUディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const;

    /// @brief 現在のバックバッファ番号を取得する（0 or 1）
    /// @return バッファインデックス
    uint32_t GetCurrentBackBufferIndex() const;

    /// @brief バックバッファの幅を取得する
    /// @return 幅（ピクセル）
    uint32_t GetWidth() const { return m_width; }

    /// @brief バックバッファの高さを取得する
    /// @return 高さ（ピクセル）
    uint32_t GetHeight() const { return m_height; }

    /// @brief バックバッファのピクセルフォーマットを取得する
    /// @return DXGI_FORMAT（デフォルトはR8G8B8A8_UNORM）
    DXGI_FORMAT GetFormat() const { return m_format; }

private:
    /// 各バックバッファに対してRTV(描画先ビュー)を作成する
    void CreateRenderTargetViews(ID3D12Device* device);

    ComPtr<IDXGISwapChain4>                           m_swapChain;
    std::array<ComPtr<ID3D12Resource>, k_BufferCount> m_backBuffers; ///< ダブルバッファ本体
    DescriptorHeap                                    m_rtvHeap;     ///< RTV用ディスクリプタヒープ

    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM; ///< LDR最終出力フォーマット
};

} // namespace GX
