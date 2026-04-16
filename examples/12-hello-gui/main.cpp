/// @file main.cpp
/// @brief 12-hello-gui — 基本ウィジェットレイアウト概念 (ADR-0017 L1.5)
///
/// 学習ポイント / Learning points:
///   - gx::GUI::Widget / Panel / Button / TextWidget の構造
///   - layoutRect で位置・サイズを指定
///   - onClick コールバックでインタラクション
///   - UIContext はレンダラー統合が必要なため、このサンプルで���
///     Compat 描画関数で疑似的な GUI を実装して概念を示す
///
/// 本番 GUI 実装では UIContext::Initialize(renderer, w, h) + SetRoot + Update + Render を使用。

#include "GXLib.h"
#include <cstdio>

struct SimpleButton
{
    int x, y, w, h;
    const char* label;
    bool hovered = false;
    bool pressed = false;
};

static bool UpdateButton(SimpleButton& btn, int mx, int my, bool mouseDown)
{
    btn.hovered = (mx >= btn.x && mx <= btn.x + btn.w &&
                   my >= btn.y && my <= btn.y + btn.h);
    bool clicked = false;
    if (btn.hovered && mouseDown && !btn.pressed)
        clicked = true;
    btn.pressed = btn.hovered && mouseDown;
    return clicked;
}

static void DrawButton(const SimpleButton& btn, unsigned int normalColor,
                       unsigned int hoverColor, unsigned int textColor)
{
    unsigned int bg = btn.hovered ? hoverColor : normalColor;
    DrawBox(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h, bg, TRUE);
    DrawBox(btn.x, btn.y, btn.x + btn.w, btn.y + btn.h, textColor, FALSE);
    DrawString(btn.x + 10, btn.y + 10, btn.label, textColor);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("12 - Hello GUI");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // 疑似 GUI: Panel + Button + テキスト表示
    // 本来は UIContext + Widget ツリーで構築するが、
    // Compat 層には UIContext へのアクセスポイントがないため DrawBox/DrawString で代替
    // =========================================================================
    int counter = 0;
    SimpleButton btnPlus  = { 460, 350, 120, 40, "  +1  " };
    SimpleButton btnMinus = { 600, 350, 120, 40, "  -1  " };

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int gray   = GetColor(60, 60, 60);
    unsigned int hover  = GetColor(80, 80, 120);
    unsigned int green  = GetColor(80, 220, 80);
    unsigned int cyan   = GetColor(100, 220, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;

        ClearDrawScreen();

        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        bool mouseDown = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;

        bool plusClicked  = UpdateButton(btnPlus, mx, my, mouseDown);
        bool minusClicked = UpdateButton(btnMinus, mx, my, mouseDown);

        if (plusClicked)  ++counter;
        if (minusClicked) --counter;

        // Panel background
        DrawBox(440, 200, 840, 500, gray, TRUE);
        DrawBox(440, 200, 840, 500, white, FALSE);

        // Title
        DrawString(460, 220, "Click the buttons!", cyan);

        // Counter display
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Counter: %d", counter);
        DrawString(460, 280, buf, green);

        // Buttons
        DrawButton(btnPlus,  gray, hover, white);
        DrawButton(btnMinus, gray, hover, white);

        // Info
        DrawFormatString(10, 10, white, "FPS: %.1f   Counter: %d", GetFPS(), counter);
        DrawString(10, 30, "Click the buttons! ESC to quit.", white);
        DrawString(10, 680, "NOTE: Real GUI uses gx::GUI::UIContext + Widget tree (see ADR-0012)", gray);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
