/// @file Samples/EasyHello/main.cpp
/// @brief GXFrameworkのシーン流れを使ったEasyHelloサンプル。最小構成の書き方が分かる。

#include "FrameworkApp.h"
#include "GameScene.h"

namespace
{

class EasyHelloScene : public GXFW::GameScene
{
public:
    const char* GetName() const override { return "EasyHelloScene"; }

protected:
    void OnSceneEnter(GXFW::SceneContext& ctx) override
    {
        (void)ctx;
        m_x = 200.0f;
        m_y = 180.0f;
    }

    void OnSceneUpdate(GXFW::SceneContext& ctx, float dt) override
    {
        const float speed = 220.0f;
        if (ctx.input->CheckHitKey(VK_LEFT))  m_x -= speed * dt;
        if (ctx.input->CheckHitKey(VK_RIGHT)) m_x += speed * dt;
        if (ctx.input->CheckHitKey(VK_UP))    m_y -= speed * dt;
        if (ctx.input->CheckHitKey(VK_DOWN))  m_y += speed * dt;
    }

    void OnSceneRenderUI(GXFW::SceneContext& ctx) override
    {
        ctx.DrawString(20.0f, 20.0f, L"EasyHello: use arrow keys", GXFW::SceneContext::Color(255, 255, 255));
        ctx.DrawCircle(m_x, m_y, 30.0f, GXFW::SceneContext::Color(255, 200, 80), true);
        ctx.DrawString(20.0f, 50.0f, L"ESC: quit", GXFW::SceneContext::Color(180, 220, 255));
    }

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
};

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    GXFW::FrameworkApp app;
    GXFW::AppConfig config;
    config.title = L"GXLib Sample: EasyHello";
    config.width = 1280;
    config.height = 720;
    config.enableDebug = true;

    if (!app.Initialize(config))
        return -1;

    app.SetScene(std::make_unique<EasyHelloScene>());
    app.Run();
    app.Shutdown();
    return 0;
}
