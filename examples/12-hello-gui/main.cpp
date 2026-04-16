/// @file main.cpp
/// @brief 12-hello-gui — 基本ウィジェットとレイアウト (ADR-0017 L1.5)
///
/// 学習ポイント / Learning points:
///   - gx::GUI::UIContext の初期化と毎フレーム Update / Render
///   - Panel / Button / TextWidget をコードで組み立てる
///   - onClick コールバックでカウンタを操作
///   - XML/CSS ローダーを使わない pure-C++ 構築パターン
///
/// より宣言的なスタイルは GUILoader + XML/CSS を使用可能 (ADR-0012 参照)。

#include "GXLib.h"
#include "GUI/UIContext.h"
#include "GUI/Widgets/Panel.h"
#include "GUI/Widgets/Button.h"
#include "GUI/Widgets/TextWidget.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("12 - Hello GUI");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // =========================================================================
    // UIContext を取得 (GX_Init で自動生成される)
    // =========================================================================
    auto& ui = GetUIContext();   // gx::GUI::UIContext& を返す想定
    ui.OnResize(1280, 720);

    // =========================================================================
    // ウィジェットツリーを構築
    //   Panel (root)
    //     ├─ TextWidget (title)
    //     ├─ TextWidget (counter display)
    //     ├─ Button (+)
    //     └─ Button (-)
    // =========================================================================
    auto root = std::make_unique<gx::GUI::Panel>();
    root->layout.x = 440; root->layout.y = 200;
    root->layout.width = 400; root->layout.height = 300;

    int counter = 0;

    auto title = std::make_unique<gx::GUI::TextWidget>();
    title->SetText(L"Click the buttons!");
    title->layout.x = 20; title->layout.y = 20;
    root->AddChild(std::move(title));

    auto display = std::make_unique<gx::GUI::TextWidget>();
    auto* displayRaw = display.get();
    display->SetText(L"Counter: 0");
    display->layout.x = 20; display->layout.y = 70;
    root->AddChild(std::move(display));

    auto btnPlus = std::make_unique<gx::GUI::Button>();
    btnPlus->SetText(L"  +1  ");
    btnPlus->layout.x = 20; btnPlus->layout.y = 150;
    btnPlus->layout.width = 100; btnPlus->layout.height = 40;
    btnPlus->onClick = [&counter, displayRaw]() {
        ++counter;
        wchar_t buf[32];
        std::swprintf(buf, 32, L"Counter: %d", counter);
        displayRaw->SetText(buf);
    };
    root->AddChild(std::move(btnPlus));

    auto btnMinus = std::make_unique<gx::GUI::Button>();
    btnMinus->SetText(L"  -1  ");
    btnMinus->layout.x = 140; btnMinus->layout.y = 150;
    btnMinus->layout.width = 100; btnMinus->layout.height = 40;
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

        // UIContext::Update がマウス/キー入力処理・イベント伝播・レイアウト再計算を行う
        ui.Update(dt);
        // Render はウィジェットツリーを順に描画する
        ui.Render();

        DrawFormatString(10, 10, white, "FPS: %.1f   Counter: %d", GetFPS(), counter);
        DrawString(10, 30, "Click the buttons! ESC to quit.", white);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
