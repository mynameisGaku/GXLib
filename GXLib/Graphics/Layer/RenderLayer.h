#pragma once
/// @file RenderLayer.h
/// @brief 描画レイヤー (個別の描画面)
///
/// DxLibのMakeScreen()に相当する機能。各レイヤーは独自のLDR RenderTarget (RGBA8) を保持し、
/// Begin/Endで描画先を切り替えて独立した画面に描画できる。
/// Z-order順でLayerCompositorが合成する。ブレンドモード・不透明度・マスク対応。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"

namespace GX
{

/// @brief レイヤー合成時のブレンドモード
enum class LayerBlendMode { Alpha, Add, Sub, Mul, Screen, None };

/// @brief 個別の描画面を管理する描画レイヤー
///
/// DxLibのMakeScreen()+SetDrawScreen()に相当する機能。
/// LDR RenderTargetを保持し、Begin/Endの間に描画した内容がこのレイヤーに蓄積される。
class RenderLayer
{
public:
    RenderLayer() = default;
    ~RenderLayer() = default;

    /// @brief レイヤーを作成する
    /// @param device D3D12デバイス
    /// @param name レイヤーの識別名
    /// @param zOrder 合成順序 (小さいほど先=奥に描画)
    /// @param width レイヤー幅
    /// @param height レイヤー高さ
    /// @return 成功でtrue
    bool Create(ID3D12Device* device, const std::string& name,
                int32_t zOrder, uint32_t width, uint32_t height);

    /// @brief このレイヤーへの描画を開始する (RTをRTV状態に遷移、OMSetを発行)
    /// @param cmdList コマンドリスト
    void Begin(ID3D12GraphicsCommandList* cmdList);

    /// @brief このレイヤーへの描画を終了する (RTをSRV状態に遷移)
    void End();

    /// @brief レイヤーをクリアする (a=0で完全透明)
    void Clear(ID3D12GraphicsCommandList* cmdList,
               float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f);

    /// @brief 画面リサイズ時にRTを再生成する
    void OnResize(ID3D12Device* device, uint32_t w, uint32_t h);

    // --- プロパティ ---
    const std::string& GetName() const { return m_name; }
    int32_t GetZOrder() const { return m_zOrder; }
    void SetZOrder(int32_t z) { m_zOrder = z; }
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool v) { m_visible = v; }
    float GetOpacity() const { return m_opacity; }
    void SetOpacity(float o) { m_opacity = o; }
    LayerBlendMode GetBlendMode() const { return m_blendMode; }
    void SetBlendMode(LayerBlendMode mode) { m_blendMode = mode; }
    bool IsPostFXEnabled() const { return m_postFXEnabled; }
    void SetPostFXEnabled(bool e) { m_postFXEnabled = e; }

    // --- マスク (DxLibのCreateMaskScreenに相当) ---
    /// @brief マスクレイヤーを設定する。マスクの白部分のみが表示される
    void SetMask(RenderLayer* mask) { m_mask = mask; }
    RenderLayer* GetMask() const { return m_mask; }
    bool HasMask() const { return m_mask != nullptr; }

    // --- リソースアクセス ---
    RenderTarget& GetRenderTarget() { return m_renderTarget; }
    const RenderTarget& GetRenderTarget() const { return m_renderTarget; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return m_renderTarget.GetRTVHandle(); }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

private:
    std::string m_name;
    int32_t m_zOrder = 0;
    bool m_visible = true;
    float m_opacity = 1.0f;
    LayerBlendMode m_blendMode = LayerBlendMode::Alpha;
    bool m_postFXEnabled = false;
    RenderLayer* m_mask = nullptr;
    RenderTarget m_renderTarget;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    ID3D12GraphicsCommandList* m_cmdList = nullptr;
};

} // namespace GX
