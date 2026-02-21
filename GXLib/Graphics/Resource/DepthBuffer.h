#pragma once
/// @file DepthBuffer.h
/// @brief 深度バッファ（Zバッファ）の管理
///
/// DxLibで SetUseZBuffer3D(TRUE) として有効化するZバッファに相当する。
/// 3D描画で手前のオブジェクトが奥のオブジェクトを正しく遮蔽するために使う。
///
/// 用途に応じて3種類の作成方法がある:
/// - Create(): DSVのみ（通常の深度テスト用）
/// - CreateWithSRV(): DSV + 外部ヒープにSRV（シャドウマップ用）
/// - CreateWithOwnSRV(): DSV + 自前ヒープにSRV（SSAO用）

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief 深度バッファ管理クラス
///
/// 深度テスト用のDSV（Depth Stencil View）を提供する。
/// シャドウマップやSSAOなど、深度値をシェーダーから読む用途では
/// SRV付きの作成メソッドを使う。
class DepthBuffer
{
public:
    DepthBuffer() = default;
    ~DepthBuffer() = default;

    /// @brief 深度バッファを作成する（DSVのみ）
    /// @param device D3D12デバイス
    /// @param width バッファの幅（ピクセル）
    /// @param height バッファの高さ（ピクセル）
    /// @param format 深度フォーマット
    /// @return 成功時true
    bool Create(ID3D12Device* device, uint32_t width, uint32_t height,
                DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT);

    /// @brief DSV + SRV付きで深度バッファを作成する（シャドウマップ用）
    ///
    /// SRVは外部から渡されたヒープに作成する。
    /// リソースフォーマットにR32_TYPELESSを使い、DSVにはD32_FLOAT、SRVにはR32_FLOATで解釈する。
    /// @param device D3D12デバイス
    /// @param width バッファの幅
    /// @param height バッファの高さ
    /// @param srvHeap SRVの作成先ヒープ
    /// @param srvIndex SRVのインデックス
    /// @return 成功時true
    bool CreateWithSRV(ID3D12Device* device, uint32_t width, uint32_t height,
                        DescriptorHeap* srvHeap, uint32_t srvIndex);

    /// @brief DSVハンドルを取得する
    /// @return 深度テスト設定に使うCPUディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const;

    /// @brief DSV + 自前shader-visible SRV付きで深度バッファを作成する（SSAO用）
    ///
    /// 専用のshader-visibleヒープを内部に持つので、
    /// ポストエフェクトパス内で独立してバインドできる。
    /// @param device D3D12デバイス
    /// @param width バッファの幅
    /// @param height バッファの高さ
    /// @return 成功時true
    bool CreateWithOwnSRV(ID3D12Device* device, uint32_t width, uint32_t height);

    /// @brief SRVのGPUハンドルを取得する
    /// @return シェーダーにバインドするためのGPUディスクリプタハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const { return m_srvGPUHandle; }

    /// @brief 内部のID3D12Resourceを取得する
    /// @return リソースポインタ
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    /// @brief バッファの幅を取得する
    /// @return ピクセル単位の幅
    uint32_t GetWidth() const { return m_width; }

    /// @brief バッファの高さを取得する
    /// @return ピクセル単位の高さ
    uint32_t GetHeight() const { return m_height; }

    /// @brief 深度フォーマットを取得する
    /// @return DXGI_FORMAT値
    DXGI_FORMAT GetFormat() const { return m_format; }

    /// @brief リソースバリアを発行してステートを遷移させる
    /// @param cmdList コマンドリスト
    /// @param newState 遷移先のリソースステート
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

    /// @brief 現在のリソースステートを取得する
    /// @return D3D12のリソースステート
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

    /// @brief 自前SRVヒープを取得する（SSAO用）
    /// @return SRVヒープへの参照
    DescriptorHeap& GetOwnSRVHeap() { return m_ownSrvHeap; }

    /// @brief 自前SRVヒープを持っているかどうか
    /// @return CreateWithOwnSRV()で作成した場合true
    bool HasOwnSRV() const { return m_hasOwnSRV; }

private:
    ComPtr<ID3D12Resource> m_resource;
    DescriptorHeap         m_dsvHeap;       ///< DSV用ヒープ（1スロット）
    DescriptorHeap         m_ownSrvHeap;    ///< 自前shader-visible SRVヒープ（SSAO用）
    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGPUHandle = {};
    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_D32_FLOAT;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    bool m_hasOwnSRV = false;
};

} // namespace GX
