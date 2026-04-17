/// @file main.cpp
/// @brief 12-hello-gui — UIContext + Widget ツリーによる GUI 構築 (ADR-0017 L1.5)
///
/// 学習ポイント / Learning points:
///   - gx::GetUIContext() で UIContext を取得 (lazy init)
///   - Panel / Button / TextWidget をコードで組み立てる
///   - layoutRect で位置・サイズ指定
///   - onClick コールバックでカウンタ操作
///   - UIContext::Update(dt, inputManager) + Render() で毎フレーム駆動

#include "GXLib.h"
#include "GUI/UIContext.h"
#include "GUI/Widget.h"
#include "GUI/Widgets/Panel.h"
#include "GUI/Widgets/Button.h"
#include "GUI/Widgets/TextWidget.h"
#include "Input/InputManager.h"
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("12 - Hello GUI");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    auto& ui = gx::GetUIContext();

    // =========================================================================
    // ウィジェットツリー構築
    //   Panel (root)
    //     ├─ TextWidget (title)
    //     ├─ TextWidget (counter display)
    //     ├─ Button (+1)
    //     └─ Button (-1)
    // =========================================================================
    auto root = std::make_unique<gx::GUI::Panel>();
    root->layoutRect.x = 440; root->layoutRect.y = 200;
    root->layoutRect.width = 400; root->layoutRect.height = 300;

    int counter = 0;

    auto title = std::make_unique<gx::GUI::TextWidget>();
    title->SetText(L"Click the buttons!");
    title->layoutRect.x = 20; title->layoutRect.y = 20;
    root->AddChild(std::move(title));

    auto display = std::make_unique<gx::GUI::TextWidget>();
    auto* displayRaw = display.get();
    display->SetText(L"Counter: 0");
    display->layoutRect.x = 20; display->layoutRect.y = 70;
    root->AddChild(std::move(display));

    auto btnPlus = std::make_unique<gx::GUI::Button>();
    btnPlus->SetText(L"  +1  ");
    btnPlus->layoutRect.x = 20; btnPlus->layoutRect.y = 150;
    btnPlus->layoutRect.width = 100; btnPlus->layoutRect.height = 40;
    btnPlus->onClick = [&counter, displayRaw]() {
        ++counter;
        wchar_t buf[32];
        std::swprintf(buf, 32, L"Counter: %d", counter);
        displayRaw->SetText(buf);
    };
    root->AddChild(std::move(btnPlus));

    auto btnMinus = std::make_unique<gx::GUI::Button>();
    btnMinus->SetText(L"  -1  ");
    btnMinus->layoutRect.x = 140; btnMinus->layoutRect.y = 150;
    btnMinus->layoutRect.width = 100; btnMinus->layoutRect.height = 40;
    btnMinus->onClick = [&counter, displayRaw]() {
        --counter;
        wchar_t buf[32];
        std::swprintf(buf, 32, L"Counter: %d", counter);
        displayRaw->SetText(buf);
    };
    root->AddChild(std::move(btnMinus));

    ui.SetRoot(std::move(root));

    unsigned int white = GetColor(255, 255, 255);

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        ui.Update(dt, gx::GetInputManager());
        ui.Render();

        DrawFormatString(10, 10, white, "FPS: %.1f   Counter: %d", GetFPS(), counter);
        DrawString(10, 30, "Click the buttons! ESC to quit.", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
