/// @file BootScene.cpp
/// @brief 既定のブートシーン。起動直後の表示用に使う。

#include "scenes/BootScene.h"
#include "FrameworkApp.h"

namespace GXFW
{

void BootScene::OnSceneEnter(SceneContext& ctx)
{
    (void)ctx;
    m_time = 0.0f;
}

void BootScene::OnSceneUpdate(SceneContext& ctx, float dt)
{
    (void)ctx;
    m_time += dt;
}

void BootScene::OnSceneRenderUI(SceneContext& ctx)
{
    if (!ctx.textRenderer)
        return;

    const int font = ctx.defaultFont;
    const uint32_t white = 0xFFFFFFFF;
    const uint32_t gray = 0xFFB0B0B0;

    ctx.textRenderer->DrawString(font, 24.0f, 20.0f, L"BootScene", white);
    ctx.textRenderer->DrawString(font, 24.0f, 44.0f, L"Ready to run your first scene.", gray);
    ctx.textRenderer->DrawString(font, 24.0f, 68.0f, L"Tip: Create a scene and call app.SetScene(...) in main.cpp", gray);

    // ループが動いていることを示す簡易点滅。動作確認を兼ねる。
    if (static_cast<int>(m_time * 2.0f) % 2 == 0)
    {
        ctx.textRenderer->DrawString(font, 24.0f, 108.0f, L"Press ESC to quit.", gray);
    }
}

} // namespace GXFW
