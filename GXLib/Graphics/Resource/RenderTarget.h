#pragma once
/// @file RenderTarget.h
/// @brief オフスクリーンレンダーターゲット
///
/// DxLibの MakeScreen() で作成する描画先に相当するクラス。
/// 画面に直接描画するのではなく、一旦テクスチャに描画し、
/// その結果をポストエフェクトやミニマップなどに使う。
///
/// HDRパイプラインではR16G16B16A16_FLOATフォーマットを使用する。
/// リソースステート管理のため、TransitionTo()を通じてバリアを発行すること。

#include "pch.h"
#include "Graphics/Device/DescriptorHeap.h"

namespace GX
{

/// @brief オフスクリーンレンダーターゲット
///
/// RTV（書き込み用ビュー）とSRV（読み取り用ビュー）の両方を内部に持つ。
/// 描画先として使った後、テクスチャとしてシェーダーから参照できる。
class RenderTarget
{
public:
    RenderTarget() = default;
    ~RenderTarget() = default;

    /// @brief レンダーターゲットを作成する
    /// @param device D3D12デバイス
    /// @param width テクスチャの幅（ピクセル）
    /// @param height テクスチャの高さ（ピクセル）
    /// @param format テクスチャフォーマット（HDR用にR16G16B16A16_FLOATを指定可能）
    /// @return 成功時true
    bool Create(ID3D12Device* device,
                uint32_t width, uint32_t height,
                DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

    /// @brief RTV（Render Target View）のCPUハンドルを取得する
    /// @return 描画先として設定するためのCPUディスクリプタハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const;

    /// @brief SRV（Shader Resource View）のGPUハンドルを取得する
    /// @return テクスチャとしてシェーダーにバインドするためのGPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const;

    /// @brief 内部のID3D12Resourceを取得する
    /// @return リソースポインタ
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    /// @brief テクスチャの幅を取得する
    /// @return ピクセル単位の幅
    uint32_t GetWidth() const { return m_width; }

    /// @brief テクスチャの高さを取得する
    /// @return ピクセル単位の高さ
    uint32_t GetHeight() const { return m_height; }

    /// @brief テクスチャフォーマットを取得する
    /// @return DXGI_FORMAT値
    DXGI_FORMAT GetFormat() const { return m_format; }

    /// @brief SRV用のディスクリプタヒープを取得する
    /// @return SRVヒープへの参照
    DescriptorHeap& GetSRVHeap() { return m_srvHeap; }

    /// @brief 現在のリソースステートを取得する
    /// @return D3D12のリソースステート
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

    /// @brief リソースステートを外部から設定する（外部でバリアを発行した場合に同期用）
    /// @param state 新しいリソースステート
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

    /// @brief リソースバリアを発行してステートを遷移させる
    ///
    /// 直接D3D12バリアを発行するとm_currentStateとの不整合が起きるため、
    /// ステート遷移には必ずこのメソッドを使うこと。
    /// @param cmdList コマンドリスト
    /// @param newState 遷移先のリソースステート
    void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);

private:
    ComPtr<ID3D12Resource> m_resource;
    DescriptorHeap         m_rtvHeap;  ///< RTV用ヒープ（1スロット）
    DescriptorHeap         m_srvHeap;  ///< SRV用shader-visibleヒープ（1スロット）
    uint32_t    m_width  = 0;
    uint32_t    m_height = 0;
    DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
};

} // namespace GX
