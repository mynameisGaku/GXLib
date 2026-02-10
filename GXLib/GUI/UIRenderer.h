#pragma once
// GUI 描画ユーティリティ

#include "pch.h"
#include "GUI/Style.h"
#include "GUI/Widget.h"
#include "Math/Transform2D.h"
#include "Graphics/Resource/DynamicBuffer.h"
#include "Graphics/Resource/Buffer.h"
#include "Graphics/Pipeline/Shader.h"

namespace GX
{
class SpriteBatch;
class TextRenderer;
class FontManager;
}

namespace GX { namespace GUI {

// ---------------------------------------------------------------------------
// Scissor
// ---------------------------------------------------------------------------
struct ScissorRect
{
    float left = 0, top = 0, right = 0, bottom = 0;

    ScissorRect Intersect(const ScissorRect& other) const
    {
        return {
            (std::max)(left,  other.left),
            (std::max)(top,   other.top),
            (std::min)(right, other.right),
            (std::min)(bottom, other.bottom)
        };
    }
};

// ---------------------------------------------------------------------------
// UIRect Batch
// ---------------------------------------------------------------------------
struct UIRectVertex
{
    XMFLOAT2 position;
    XMFLOAT2 localUV;
};

struct UIRectConstants
{
    XMFLOAT4X4 projection;         // 64
    XMFLOAT2   rectSize;           // 8
    float      cornerRadius;       // 4
    float      borderWidth;        // 4
    XMFLOAT4   fillColor;          // 16
    XMFLOAT4   borderColor;        // 16
    XMFLOAT2   shadowOffset;       // 8
    float      shadowBlur;         // 4
    float      shadowAlpha;        // 4
    float      opacity;            // 4
    float      _pad[3];            // 12
    XMFLOAT4   gradientColor;      // 16
    XMFLOAT2   gradientDir;        // 8
    float      gradientEnabled;    // 4
    float      _pad2;              // 4
    XMFLOAT2   effectCenter;       // 8
    float      effectTime;         // 4
    float      effectDuration;     // 4
    float      effectStrength;     // 4
    float      effectWidth;        // 4
    float      effectType;         // 4
    float      _pad3;              // 4
};                                 // 208 bytes

enum class UIRectEffectType
{
    None = 0,
    Ripple = 1
};

struct UIRectEffect
{
    UIRectEffectType type = UIRectEffectType::None;
    float centerX = 0.5f;   // 0..1
    float centerY = 0.5f;   // 0..1
    float time = 0.0f;      // 秒
    float duration = 0.8f;  // 秒
    float strength = 0.4f;  // 0..1
    float width = 0.08f;    // 0..1
};

// ---------------------------------------------------------------------------
// UIRenderer
// ---------------------------------------------------------------------------
class UIRenderer
{
public:
    UIRenderer() = default;
    ~UIRenderer() = default;

    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                    uint32_t screenWidth, uint32_t screenHeight,
                    SpriteBatch* spriteBatch, TextRenderer* textRenderer,
                    FontManager* fontManager);

    void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);
    void End();

    // --- Rect ---
    void DrawRect(const LayoutRect& rect, const Style& style,
                  float opacity = 1.0f,
                  const UIRectEffect* effect = nullptr);
    void DrawSolidRect(float x, float y, float w, float h, const StyleColor& color);
    void DrawGradientRect(float x, float y, float w, float h,
                          const StyleColor& startColor, const StyleColor& endColor,
                          float dirX = 0.0f, float dirY = 1.0f,
                          float cornerRadius = 0.0f,
                          float opacity = 1.0f);

    // --- Text ---
    void DrawText(float x, float y, int fontHandle, const std::wstring& text,
                  const StyleColor& color, float opacity = 1.0f);
    int GetTextWidth(int fontHandle, const std::wstring& text);
    int GetLineHeight(int fontHandle);
    float GetFontCapOffset(int fontHandle);

    // --- Image ---
    void DrawImage(float x, float y, float w, float h, int textureHandle, float opacity = 1.0f);
    void DrawImageUV(float x, float y, float w, float h, int textureHandle,
                     float u0, float v0, float u1, float v1,
                     float opacity = 1.0f);

    // --- Scissor ---
    void PushScissor(const LayoutRect& rect);
    void PopScissor();

    // --- Transform / Opacity ---
    void PushTransform(const Transform2D& local);
    void PopTransform();
    const Transform2D& GetTransform() const { return m_transformStack.back(); }

    void PushOpacity(float opacity);
    void PopOpacity();
    float GetOpacity() const { return m_opacityStack.back(); }

    // --- Deferred ---
    void DeferDraw(std::function<void()> fn);
    void FlushDeferredDraws();

    // --- Resize ---
    void OnResize(uint32_t width, uint32_t height);
    void SetDesignResolution(uint32_t designWidth, uint32_t designHeight);

    float GetGuiScale() const { return m_guiScale; }
    float GetGuiOffsetX() const { return m_guiOffsetX; }
    float GetGuiOffsetY() const { return m_guiOffsetY; }

    FontManager* GetFontManager() const { return m_fontManager; }
    SpriteBatch* GetSpriteBatch() const { return m_spriteBatch; }
    TextRenderer* GetTextRenderer() const { return m_textRenderer; }

private:
    bool CreateRectPipeline(ID3D12Device* device);
    void UpdateProjectionMatrix();

    void DrawRectInternal(float x, float y, float w, float h,
                          const StyleColor& fillColor, float cornerRadius,
                          float borderWidth, const StyleColor& borderColor,
                          float shadowOffsetX, float shadowOffsetY,
                          float shadowBlur, float shadowAlpha,
                          float opacity,
                          const StyleColor* gradientColor,
                          const XMFLOAT2& gradientDir,
                          const UIRectEffect* effect);

    void ApplyScissor();
    void UpdateGuiMetrics();
    void ApplyGuiViewport();

    // Device
    ID3D12Device*              m_device = nullptr;
    ID3D12GraphicsCommandList* m_cmdList = nullptr;
    uint32_t                   m_frameIndex = 0;

    // Renderers
    SpriteBatch*  m_spriteBatch = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    FontManager*  m_fontManager = nullptr;

    // Screen
    uint32_t m_screenWidth  = 1280;
    uint32_t m_screenHeight = 720;
    XMFLOAT4X4 m_projectionMatrix;

    // Design resolution scaling
    uint32_t m_designWidth  = 0;
    uint32_t m_designHeight = 0;
    float    m_guiScale   = 1.0f;
    float    m_guiOffsetX = 0.0f;
    float    m_guiOffsetY = 0.0f;

    // UIRect batch
    Shader                       m_rectShader;
    ComPtr<ID3D12RootSignature>  m_rectRootSignature;
    ComPtr<ID3D12PipelineState>  m_rectPSO;
    DynamicBuffer                m_rectVertexBuffer;
    DynamicBuffer                m_rectConstantBuffer;
    Buffer                       m_rectIndexBuffer;
    uint32_t                     m_rectDrawCount = 0;

    // Scissor
    std::vector<ScissorRect> m_scissorStack;
    ScissorRect m_fullScreen;
    bool m_spriteBatchActive = false;

    // Deferred
    std::vector<std::function<void()>> m_deferredDraws;

    // Transform/Opacity stacks
    std::vector<Transform2D> m_transformStack;
    std::vector<float> m_opacityStack;
};

}} // namespace GX::GUI

