#pragma once
/// @file DebugOverlay.h
/// @brief ImGui ベースインタラクティブデバッグオーバーレイ
///
/// Compat API (ToggleDebugOverlay) および GameFramework (GameBase::ToggleDebugOverlay)
/// の両方から使用できる。ImGui を使用したインタラクティブなデバッグウィンドウ。

namespace gx
{

class Renderer3D;
class PostEffectPipeline;
class AudioManager;
class QualitySettings;

/// @brief DebugOverlay::Draw に渡す描画コンテキスト
struct DebugOverlayContext
{
    Renderer3D*         renderer3D;
    PostEffectPipeline* postEffect;
    AudioManager*       audioManager;
    QualitySettings*    quality;
    float    deltaTime;
    float    fps;
    uint32_t screenWidth;
    uint32_t screenHeight;
};

/// @brief ImGui ベースインタラクティブデバッグオーバーレイ
class DebugOverlay
{
public:
    void Toggle() { m_visible = !m_visible; }
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool v) { m_visible = v; }

    /// @brief オーバーレイを描画する
    ///
    /// ImGui::NewFrame 後、ImGui::Render 前に呼ぶ。
    void Draw(const DebugOverlayContext& ctx);

private:
    bool  m_visible = false;
    float m_fpsHistory[120] = {};
    int   m_fpsHistoryOffset = 0;

    void DrawSystemTab(const DebugOverlayContext& ctx);
    void DrawRendererTab(const DebugOverlayContext& ctx);
    void DrawPostFXTab(const DebugOverlayContext& ctx);
    void DrawAudioTab(const DebugOverlayContext& ctx);
    void DrawQualityTab(const DebugOverlayContext& ctx);
};

} // namespace gx
