#pragma once
/// @file RenderLayer.h
/// @brief 描画レイヤークラス
///
/// 各レイヤーは独自のLDR RenderTarget (RGBA8) を保持し、
/// 合成時にZ-order順にバックバッファに描画される。

#include "pch.h"
#include "Graphics/Resource/RenderTarget.h"

namespace GX
{

/// @brief レイヤーのブレンドモード
enum class LayerBlendMode { Alpha, Add, Sub, Mul, Screen, None };

/// @brief 描画レイヤー
class RenderLayer
{
public:
    RenderLayer() = default;
    ~RenderLayer() = default;

    /// レイヤーを作成
    bool Create(ID3D12Device* device, const std::string& name,
                int32_t zOrder, uint32_t width, uint32_t height);

    /// レイヤーへの描画開始（RT遷移 + OMSet + Viewport/Scissor）
    void Begin(ID3D12GraphicsCommandList* cmdList);

    /// レイヤーへの描画終了（RT → SRV遷移）
    void End();

    /// レイヤーをクリア（a=0で透明クリア）
    void Clear(ID3D12GraphicsCommandList* cmdList,
               float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f);

    /// リサイズ
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

    // --- マスク ---
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
