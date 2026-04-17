/// @file main.cpp
/// @brief 13-custom-widget — Widget 派生カスタムウィジェット (ADR-0017 L2)
///
/// Layer 2 拡張ポイント: gx::GUI::Widget を派生して独自ウィジェットを定義し、
/// UIContext のウィジェットツリーに組み込む。
///
/// 学習ポイント / Learning points:
///   - Widget を派生して RenderSelf / OnEvent / GetType をオーバーライド
///   - layoutRect で位置・サイズ指定
///   - UIEvent の MouseDown + MouseMove で値変更
///   - GetUIContext() で UIContext に組み込み

#include "GXLib.h"
#include "GUI/UIContext.h"
#include "GUI/Widget.h"
#include "GUI/UIRenderer.h"
#include "Input/InputManager.h"
#include <cmath>

// =========================================================================
// カスタムウィジェット: CircularGauge
// =========================================================================
class CircularGauge : public gx::GUI::Widget
{
public:
    gx::GUI::WidgetType GetType() const override { return gx::GUI::WidgetType::Canvas; }
    float GetIntrinsicWidth() const override { return 120.0f; }
    float GetIntrinsicHeight() const override { return 120.0f; }

    void RenderSelf(gx::GUI::UIRenderer& /*renderer*/) override
    {
        int cx = static_cast<int>(layoutRect.x + layoutRect.width * 0.5f);
        int cy = static_cast<int>(layoutRect.y + layoutRect.height * 0.5f);
        int r  = static_cast<int>(layoutRect.width * 0.45f);

        DrawCircle(cx, cy, r, GetColor(40, 40, 60), TRUE);

        float angle = 6.2831f * m_value;
        constexpr int SEG = 48;
        for (int i = 0; i < SEG; ++i)
        {
            float a0 = (angle * static_cast<float>(i)) / SEG - 1.5708f;
            float a1 = (angle * static_cast<float>(i + 1)) / SEG - 1.5708f;
            int x0 = cx + static_cast<int>(std::cos(a0) * r);
            int y0 = cy + static_cast<int>(std::sin(a0) * r);
            int x1 = cx + static_cast<int>(std::cos(a1) * r);
            int y1 = cy + static_cast<int>(std::sin(a1) * r);
            DrawTriangle(cx, cy, x0, y0, x1, y1, GetColor(100, 200, 255), TRUE);
        }

        DrawCircle(cx, cy, r, GetColor(200, 200, 220), FALSE);
    }

    bool OnEvent(const gx::GUI::UIEvent& event) override
    {
        if (event.type == gx::GUI::UIEventType::MouseDown ||
            (event.type == gx::GUI::UIEventType::MouseMove && pressed))
        {
            float cx = layoutRect.x + layoutRect.width * 0.5f;
            float cy = layoutRect.y + layoutRect.height * 0.5f;
            float dx = event.mouseX - cx;
            float dy = event.mouseY - cy;
            float angle = std::atan2(dy, dx) + 1.5708f;
            if (angle < 0.0f) angle += 6.2831f;
            m_value = angle / 6.2831f;
            return true;
        }
        return Widget::OnEvent(event);
    }

    float GetValue() const { return m_value; }
    void SetValue(float v) { m_value = v; }

private:
    float m_value = 0.5f;
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("13 - Custom Widget (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    auto& ui = gx::GetUIContext();

    auto gauge = std::make_unique<CircularGauge>();
    auto* gaugeRaw = gauge.get();
    gauge->layoutRect.x = 580; gauge->layoutRect.y = 300;
    gauge->layoutRect.width = 120; gauge->layoutRect.height = 120;

    ui.SetRoot(std::move(gauge));

    unsigned int white = GetColor(255, 255, 255);
    unsigned int cyan  = GetColor(100, 220, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        ui.Update(dt, gx::GetInputManager());
        ui.Render();

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 30, cyan,
                         "CircularGauge value : %.2f  (click or drag the circle)",
                         gaugeRaw->GetValue());
        DrawString(10, 50, "Custom widget derived from gx::GUI::Widget. ESC to quit.", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
