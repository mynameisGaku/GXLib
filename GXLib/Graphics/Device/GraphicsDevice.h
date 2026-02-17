#pragma once
/// @file GraphicsDevice.h
/// @brief D3D12デバイス初期化クラス
///
/// 【初学者向け解説】
/// 「デバイス」とは、GPU（グラフィックスカード）を操作するためのインターフェースです。
/// DirectX 12でグラフィックスを描画するには、まずこのデバイスを作成する必要があります。
///
/// 初期化の流れ：
/// 1. DXGIFactory作成 — GPUを探すための「工場」
/// 2. アダプタ（GPU）を列挙して最適なものを選択
/// 3. D3D12Deviceを作成 — これがGPUへの命令窓口
/// 4. デバッグビルド時はデバッグレイヤーを有効化
///    （間違った使い方をするとエラーメッセージが出る便利機能）

#include "pch.h"

namespace GX
{

/// @brief D3D12デバイスを管理するクラス
class GraphicsDevice
{
public:
    GraphicsDevice() = default;
    ~GraphicsDevice() = default;

    /// @brief D3D12デバイスを初期化する
    /// @param enableDebugLayer デバッグレイヤーを有効化するかどうか
    /// @param enableGPUValidation GPU-based validationを有効化するかどうか（非常に遅いが詳細なエラー検出が可能）
    /// @return 初期化に成功した場合true
    bool Initialize(bool enableDebugLayer = true, bool enableGPUValidation = false);

    /// @brief D3D12デバイスを取得する
    /// @return ID3D12Deviceへのポインタ
    ID3D12Device* GetDevice() const { return m_device.Get(); }

    /// @brief DXGIファクトリを取得する
    /// @return IDXGIFactory6へのポインタ
    IDXGIFactory6* GetFactory() const { return m_factory.Get(); }

    /// @brief DXGIアダプタ（GPU）を取得する
    /// @return IDXGIAdapter1へのポインタ
    IDXGIAdapter1* GetAdapter() const { return m_adapter.Get(); }

    /// @brief ID3D12Device5を取得する（DXR用）
    /// @return ID3D12Device5へのポインタ（DXR非対応時はnullptr）
    ID3D12Device5* GetDevice5() const { return m_device5.Get(); }

    /// @brief レイトレーシングがサポートされているか
    bool SupportsRaytracing() const { return m_supportsRaytracing; }

    /// @brief DXGI生存オブジェクトをレポートする（シャットダウン後に呼ぶ）
    static void ReportLiveObjects();

private:
    /// デバッグレイヤーを有効化
    void EnableDebugLayer(bool gpuValidation);

    /// InfoQueue メッセージフィルタ設定
    void ConfigureInfoQueue();

    /// 最適なGPUアダプタを選択
    bool SelectAdapter();

    ComPtr<IDXGIFactory6>  m_factory;
    ComPtr<IDXGIAdapter1>  m_adapter;
    ComPtr<ID3D12Device>   m_device;
    ComPtr<ID3D12Device5>  m_device5;
    bool m_supportsRaytracing = false;
};

} // namespace GX
