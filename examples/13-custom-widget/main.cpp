/// @file main.cpp
/// @brief 13-custom-widget — カスタムウィジェット派生 (ADR-0017 L2)
///
/// Layer 2 拡張ポイント: gx::GUI::Widget を派生して独自ウィジェットを定義し、
/// UIContext のウィジェットツリーに組み込む。
///
/// 学習ポイント / Learning points:
///   - Widget を派生して RenderSelf / OnEvent / GetType をオーバーライド
///   - ビルトインでは提供されないウィジェット (円形ゲージ、カスタム描画) を追加
///   - Widget.onClick / onHover 等の共通コールバックは自動で使える
///
/// このサンプルでは「CircularGauge」を作り、マウスドラッグで値を変更する。

#include "GXLib.h"
#include "GUI/UIContext.h"
#include "GUI/Widget.h"
#include "GUI/UIRenderer.h"
#include <cmath>

// =========================================================================
// カスタムウィジェット: CircularGauge
// =========================================================================
class CircularGauge : public gx::GUI::Widget
{
public:
    // Widget 契約を満たす必須オーバーライド
    gx::GUI::WidgetType GetType() const override {
        return gx::GUI::WidgetType::Canvas;   // 既存 enum から最も近いものを選択
    }

    float GetIntrinsicWidth()  const override { return 120.0f; }
    float GetIntrinsicHeight() const override { return 120.0f; }

    // 描画: UIRenderer の低レベル API で円ゲージを描く
    // (UIRenderer の API は実装に依存するため、ここでは概念のみ)
    void RenderSelf(gx::GUI::UIRenderer& renderer) override
    {
        (void)renderer;
        // UIRenderer::FillCircle / StrokeArc が存在しない場合は
        // DrawCircle (Compat layer) を使うこともできる:
        int cx = static_cast<int>(layout.x + layout.width  * 0.5f);
        int cy = static_cast<int>(layout.y + layout.height * 0.5f);
        int r  = static_cast<int>(layout.width * 0.45f);

        // 背景
        DrawCircle(cx, cy, r, GetColor(40, 40, 60), TRUE);
        // 値に応じた扇型 (360度のうち value*360)
        float angle = 6.2831f * m_value;
        const int SEG = 48;
        for (int i = 0; i < SEG; ++i)
        {
            float a0 = (angle * i) / SEG - 1.5708f;   // -90 deg から時計回り
            float a1 = (angle * (i + 1)) / SEG - 1.5708f;
            int x0 = cx + static_cast<int>(std::cosf(a0) * r);
            int y0 = cy + static_cast<int>(std::sinf(a0) * r);
            int x1 = cx + static_cast<int>(std::cosf(a1) * r);
            int y1 = cy + static_cast<int>(std::sinf(a1) * r);
            DrawTriangle(cx, cy, x0, y0, x1, y1, GetColor(100, 200, 255), TRUE);
        }
        // 縁
        DrawCircle(cx, cy, r, GetColor(200, 200, 220), FALSE);
    }

    // イベント: マウスドラッグで値を変更
    bool OnEvent(const gx::GUI::UIEvent& event) override
    {
        if (event.type == gx::GUI::UIEventType::MouseDrag ||
            event.type == gx::GUI::UIEventType::MouseDown)
        {
            // クリック位置から中心へのベクトル → 角度 → 0..1 に正規化
            float cx = layout.x + layout.width  * 0.5f;
            float cy = layout.y + layout.height * 0.5f;
            float dx = event.mouseX - cx;
            float dy = event.mouseY - cy;
            float angle = std::atan2f(dy, dx) + 1.5708f;   // -90deg 基準
            if (angle < 0.0f) angle += 6.2831f;
            m_value = angle / 6.2831f;
            if (onValueChanged) onValueChanged(gx::String(""));   // 汎用通知
            return true;   // consumed
        }
        return Widget::OnEvent(event);
    }

    float GetValue() const { return m_value; }
    void  SetValue(float v) { m_value = v; }

private:
    float m_value = 0.5f;   // 0..1
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("13 - Custom Widget (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    auto& ui = GetUIContext();
    ui.OnResize(1280, 720);

    auto gauge = std::make_unique<CircularGauge>();
    auto* gaugeRaw = gauge.get();
    gauge->layout.x = 580; gauge->layout.y = 300;
    gauge->layout.width = 120; gauge->layout.height = 120;

    ui.SetRoot(std::move(gauge));

    unsigned int white = GetColor(255, 255, 255);
    unsigned int cyan  = GetColor(100, 220, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        ui.Update(dt);
        ui.Render();

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 30, cyan,
                         "CircularGauge value : %.2f  (click or drag the circle)",
                         gaugeRaw->GetValue());
        DrawString(10, 50, white, "Custom widget derived from gx::GUI::Widget. ESC to quit.");

        ScreenFlip();
    }

    GX_End();
    return 0;
}
