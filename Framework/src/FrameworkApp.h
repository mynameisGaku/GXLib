#pragma once
/// @file FrameworkApp.h
/// @brief シーンだけ実装すればよい簡易アプリラッパー。初期化とループの手間を隠して学習を楽にする。

#include "pch.h"
#include "Core/Application.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Device/CommandQueue.h"
#include "Graphics/Device/CommandList.h"
#include "Graphics/Device/SwapChain.h"
#include "Graphics/3D/Renderer3D.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/PostEffect/PostEffectPipeline.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/PrimitiveBatch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Rendering/TextRenderer.h"
#include "Input/InputManager.h"
#include "Physics/PhysicsWorld3D.h"
#include "SceneManager.h"

namespace GXFW
{

struct AppConfig
{
    std::wstring title = L"GXLib Framework";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool enableDebug = false;
};

struct SceneContext
{
    GX::Application* app = nullptr;
    GX::GraphicsDevice* graphics = nullptr;
    GX::CommandQueue* commandQueue = nullptr;
    GX::CommandList* commandList = nullptr;
    GX::SwapChain* swapChain = nullptr;
    GX::Renderer3D* renderer = nullptr;
    GX::Camera3D* camera = nullptr;
    GX::PostEffectPipeline* postFX = nullptr;
    GX::InputManager* input = nullptr;
    GX::PhysicsWorld3D* physics = nullptr;
    GX::SpriteBatch* spriteBatch = nullptr;
    GX::PrimitiveBatch* primitiveBatch = nullptr;
    GX::TextRenderer* textRenderer = nullptr;
    GX::FontManager* fontManager = nullptr;

    ID3D12GraphicsCommandList* cmd = nullptr;
    uint32_t frameIndex = 0;
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    int defaultFont = -1;

    // 2Dバッチのヘルパー（Sprite/Primitive）。呼び出し順の管理を簡単にする。
    enum class Active2DBatch
    {
        None,
        Sprite,
        Primitive
    };

    void Begin2D()
    {
        frame2DActive = true;
        active2D = Active2DBatch::None;
    }

    void End2D()
    {
        Flush2D();
        frame2DActive = false;
    }

    void Flush2D()
    {
        if (!frame2DActive)
            return;
        if (active2D == Active2DBatch::Sprite && spriteBatch)
            spriteBatch->End();
        else if (active2D == Active2DBatch::Primitive && primitiveBatch)
            primitiveBatch->End();
        active2D = Active2DBatch::None;
    }

    void EnsureSpriteBatch()
    {
        if (!frame2DActive || !spriteBatch || !cmd)
            return;
        if (active2D == Active2DBatch::Primitive && primitiveBatch)
            primitiveBatch->End();
        if (active2D != Active2DBatch::Sprite)
            spriteBatch->Begin(cmd, frameIndex);
        active2D = Active2DBatch::Sprite;
    }

    void EnsurePrimitiveBatch()
    {
        if (!frame2DActive || !primitiveBatch || !cmd)
            return;
        if (active2D == Active2DBatch::Sprite && spriteBatch)
            spriteBatch->End();
        if (active2D != Active2DBatch::Primitive)
            primitiveBatch->Begin(cmd, frameIndex);
        active2D = Active2DBatch::Primitive;
    }

    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               static_cast<uint32_t>(b);
    }

    void DrawLine(float x1, float y1, float x2, float y2, uint32_t color, int thickness = 1)
    {
        if (!primitiveBatch)
            return;
        EnsurePrimitiveBatch();
        primitiveBatch->DrawLine(x1, y1, x2, y2, color, thickness);
    }

    void DrawBox(float x1, float y1, float x2, float y2, uint32_t color, bool fill)
    {
        if (!primitiveBatch)
            return;
        EnsurePrimitiveBatch();
        primitiveBatch->DrawBox(x1, y1, x2, y2, color, fill);
    }

    void DrawCircle(float cx, float cy, float r, uint32_t color, bool fill, int segments = 32)
    {
        if (!primitiveBatch)
            return;
        EnsurePrimitiveBatch();
        primitiveBatch->DrawCircle(cx, cy, r, color, fill, segments);
    }

    void DrawString(float x, float y, const std::wstring& text, uint32_t color = 0xFFFFFFFF)
    {
        if (!textRenderer || defaultFont < 0)
            return;
        EnsureSpriteBatch();
        textRenderer->DrawString(defaultFont, x, y, text, color);
    }

    void DrawString(int fontHandle, float x, float y, const std::wstring& text, uint32_t color = 0xFFFFFFFF)
    {
        if (!textRenderer)
            return;
        EnsureSpriteBatch();
        textRenderer->DrawString(fontHandle, x, y, text, color);
    }

private:
    Active2DBatch active2D = Active2DBatch::None;
    bool frame2DActive = false;
};

class FrameworkApp
{
public:
    FrameworkApp() = default;
    ~FrameworkApp() = default;

    bool Initialize(const AppConfig& config);
    void SetScene(std::unique_ptr<Scene> scene);
    void Run();
    void Shutdown();

    SceneContext& GetContext() { return m_context; }

private:
    void RenderFrame(float dt);
    void OnResize(uint32_t width, uint32_t height);

    GX::Application m_app;
    GX::GraphicsDevice m_device;
    GX::CommandQueue m_commandQueue;
    GX::CommandList m_commandList;
    GX::SwapChain m_swapChain;
    GX::Renderer3D m_renderer;
    GX::Camera3D m_camera;
    GX::PostEffectPipeline m_postFX;
    GX::InputManager m_input;
    GX::PhysicsWorld3D m_physics;
    GX::SpriteBatch m_spriteBatch;
    GX::PrimitiveBatch m_primitiveBatch;
    GX::FontManager m_fontManager;
    GX::TextRenderer m_textRenderer;

    SceneManager m_sceneManager;
    SceneContext m_context;

    uint64_t m_fenceValues[GX::SwapChain::k_BufferCount] = {};
    uint32_t m_frameIndex = 0;
    float m_totalTime = 0.0f;
    int m_defaultFont = -1;
};

} // namespace GXFW
