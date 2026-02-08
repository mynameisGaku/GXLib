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

    /// デバイスを初期化
    bool Initialize(bool enableDebugLayer = true);

    /// D3D12デバイスを取得
    ID3D12Device* GetDevice() const { return m_device.Get(); }

    /// DXGIファクトリを取得
    IDXGIFactory6* GetFactory() const { return m_factory.Get(); }

    /// DXGIアダプタを取得
    IDXGIAdapter1* GetAdapter() const { return m_adapter.Get(); }

private:
    /// デバッグレイヤーを有効化
    void EnableDebugLayer();

    /// 最適なGPUアダプタを選択
    bool SelectAdapter();

    ComPtr<IDXGIFactory6>  m_factory;
    ComPtr<IDXGIAdapter1>  m_adapter;
    ComPtr<ID3D12Device>   m_device;
};

} // namespace GX
