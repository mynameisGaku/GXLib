/// @file main.cpp
/// @brief 13-custom-widget — カスタムウィジェット概念 (ADR-0017 L2)
///
/// Layer 2 拡張ポイント: gx::GUI::Widget を派生して独自ウィジェットを定義する。
///
/// 学習ポイント / Learning points:
///   - Widget を派生して RenderSelf / OnEvent / GetType をオーバーライド
///   - カスタムウィジェットの描画 + インタラクションパターン
///   - このサンプルは UIContext 依存を避けるため Compat ��画で自己完��
///     本番では UIContext ツリーに組み込んで使う (ADR-0012)
///
/// 「CircularGauge」��マウスクリック/ドラッグで値変更するデモ。

#include "GXLib.h"
#include <cmath>
#include <cstdio>

// =========================================================================
// カスタムウィジェット概念: CircularGauge (Compat 描画ベース)
// 本番では gx::GUI::Widget 派生 + RenderSelf + OnEvent オーバーライド
// =========================================================================
class CircularGauge
{
public:
    float cx, cy, radius;
    float value = 0.5f;

    void Draw() const
    {
        int icx = static_cast<int>(cx);
        int icy = static_cast<int>(cy);
        int ir  = static_cast<int>(radius);

        DrawCircle(icx, icy, ir, GetColor(40, 40, 60), TRUE);

        float angle = 6.2831f * value;
        constexpr int SEG = 48;
        for (int i = 0; i < SEG; ++i)
        {
            float a0 = (angle * static_cast<float>(i)) / SEG - 1.5708f;
            float a1 = (angle * static_cast<float>(i + 1)) / SEG - 1.5708f;
            int x0 = icx + static_cast<int>(std::cos(a0) * radius);
            int y0 = icy + static_cast<int>(std::sin(a0) * radius);
            int x1 = icx + static_cast<int>(std::cos(a1) * radius);
            int y1 = icy + static_cast<int>(std::sin(a1) * radius);
            DrawTriangle(icx, icy, x0, y0, x1, y1, GetColor(100, 200, 255), TRUE);
        }

        DrawCircle(icx, icy, ir, GetColor(200, 200, 220), FALSE);
    }

    bool HandleInput(int mx, int my, bool mouseDown)
    {
        float dx = static_cast<float>(mx) - cx;
        float dy = static_cast<float>(my) - cy;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (!mouseDown || dist > radius) return false;

        float angle = std::atan2(dy, dx) + 1.5708f;
        if (angle < 0.0f) angle += 6.2831f;
        value = angle / 6.2831f;
        return true;
    }
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("13 - Custom Widget (Layer 2)");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    CircularGauge gauge;
    gauge.cx = 640.0f;
    gauge.cy = 360.0f;
    gauge.radius = 80.0f;

    unsigned int white = GetColor(255, 255, 255);
    unsigned int cyan  = GetColor(100, 220, 255);
    unsigned int gray  = GetColor(120, 120, 120);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        bool mouseDown = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;

        gauge.HandleInput(mx, my, mouseDown);
        gauge.Draw();

        DrawFormatString(10, 10, white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 30, cyan,
                         "CircularGauge value : %.2f  (click/drag the circle)",
                         gauge.value);
        DrawString(10, 50, "Custom widget concept -- derived from gx::GUI::Widget in production.", white);
        DrawString(10, 680, "NOTE: Real custom widgets override Widget::RenderSelf + OnEvent (see ADR-0012)", gray);
        DrawString(10, 700, "ESC: quit", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
