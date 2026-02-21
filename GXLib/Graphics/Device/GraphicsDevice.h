#pragma once
/// @file GraphicsDevice.h
/// @brief D3D12デバイス初期化・管理クラス
///
/// DxLibではGPUデバイスは内部で自動的に作られるが、DX12では自分で作って管理する。
/// DXGIファクトリでGPUを探し、D3D12Deviceを作成してリソース生成の起点にする。
/// DXR(レイトレーシング)対応GPUならID3D12Device5も取得する。

#include "pch.h"

namespace GX
{

/// @brief GPU本体を表すクラス（DxLibでは内部で自動管理される）
///
/// DX12ではテクスチャやバッファなど、すべてのリソース作成にデバイスが必要。
/// デバッグビルドではデバッグレイヤーを有効にすると、API誤用を検出してくれる。
class GraphicsDevice
{
public:
    GraphicsDevice() = default;
    ~GraphicsDevice() = default;

    /// @brief GPUデバイスを初期化する
    /// @param enableDebugLayer trueでデバッグレイヤーを有効化（API誤用を検出する開発用機能）
    /// @param enableGPUValidation trueでGPUベース検証を有効化（非常に遅いが詳細なエラー検出が可能）
    /// @return 成功なら true
    bool Initialize(bool enableDebugLayer = true, bool enableGPUValidation = false);

    /// @brief D3D12デバイスを取得する
    /// @return ID3D12Deviceへのポインタ
    ID3D12Device* GetDevice() const { return m_device.Get(); }

    /// @brief DXGIファクトリを取得する（GPU列挙やSwapChain作成に使う）
    /// @return IDXGIFactory6へのポインタ
    IDXGIFactory6* GetFactory() const { return m_factory.Get(); }

    /// @brief 選択されたGPUアダプタを取得する
    /// @return IDXGIAdapter1へのポインタ
    IDXGIAdapter1* GetAdapter() const { return m_adapter.Get(); }

    /// @brief DXR対応のDevice5インターフェースを取得する
    /// @return ID3D12Device5へのポインタ（DXR非対応GPUではnullptr）
    ID3D12Device5* GetDevice5() const { return m_device5.Get(); }

    /// @brief レイトレーシング(DXR)が使えるかどうか
    /// @return DXR Tier 1.0以上に対応していれば true
    bool SupportsRaytracing() const { return m_supportsRaytracing; }

    /// @brief 解放漏れしたDXGIオブジェクトをOutputDebugStringに出力する
    ///
    /// アプリ終了時に呼ぶと、リーク箇所の特定に役立つ。
    static void ReportLiveObjects();

private:
    /// デバッグレイヤーを有効化（D3D12Debug + オプションでGPUベース検証）
    void EnableDebugLayer(bool gpuValidation);

    /// InfoQueueのメッセージフィルタを設定し、重大エラーでブレークさせる
    void ConfigureInfoQueue();

    /// 高性能GPUを自動選択する（ソフトウェアアダプタはスキップ）
    bool SelectAdapter();

    ComPtr<IDXGIFactory6>  m_factory;   ///< GPU列挙・SwapChain作成用ファクトリ
    ComPtr<IDXGIAdapter1>  m_adapter;   ///< 選択されたGPU
    ComPtr<ID3D12Device>   m_device;    ///< 標準デバイスインターフェース
    ComPtr<ID3D12Device5>  m_device5;   ///< DXR用拡張インターフェース
    bool m_supportsRaytracing = false;  ///< DXR対応フラグ
};

} // namespace GX
