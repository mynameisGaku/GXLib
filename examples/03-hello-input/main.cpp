/// @file main.cpp
/// @brief 03-hello-input — キーボード/マウス/ゲームパッド + ActionMapping (ADR-0017 L1)
///
/// 学習ポイント / Learning points:
///   - GetHitKeyStateAll / CheckHitKey でキーボード / Keyboard
///   - GetMouseInput / GetMousePoint / GetMouseWheelRotVol でマウス / Mouse
///   - GetJoypadInputState でゲームパッドボタン / Gamepad raw bitmask
///   - SetActionKey/Button + IsActionPressed/Triggered / 論理アクション
///   - WASD と左スティックの両方でボックスを動かす / Dual-input movement

#include "GXLib.h"
#include <algorithm>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("03 - Hello Input");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // 論理アクションを登録 (キーとパッド両方バインド) / Register logical actions
    SetActionKey("jump", KEY_INPUT_SPACE);
    SetActionButton("jump", PAD_INPUT_A);
    SetActionKey("fire", KEY_INPUT_Z);
    SetActionButton("fire", PAD_INPUT_B);

    unsigned int white  = GetColor(255, 255, 255);
    unsigned int green  = GetColor( 80, 220,  80);
    unsigned int red    = GetColor(220,  80,  80);
    unsigned int yellow = GetColor(255, 220,  50);

    float boxX = 640.0f, boxY = 360.0f;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();

        ClearDrawScreen();

        char keys[256] = {};
        GetHitKeyStateAll(keys);
        int keyCount = 0;
        for (int i = 0; i < 256; ++i) if (keys[i]) ++keyCount;

        int mx = 0, my = 0;
        GetMousePoint(&mx, &my);
        int mb = GetMouseInput();
        int wheel = GetMouseWheelRotVol();
        int pad = GetJoypadInputState(GX_INPUT_PAD1);

        bool jump = IsActionPressed("jump") != 0;
        bool fire = IsActionTriggered("fire") != 0;

        // WASD でボックスを動かす / Move box with WASD
        float ax = 0.0f, ay = 0.0f;
        if (keys[KEY_INPUT_A]) ax -= 1.0f;
        if (keys[KEY_INPUT_D]) ax += 1.0f;
        if (keys[KEY_INPUT_W]) ay -= 1.0f;
        if (keys[KEY_INPUT_S]) ay += 1.0f;
        boxX = std::clamp(boxX + ax * 200.0f * dt, 20.0f, 1260.0f);
        boxY = std::clamp(boxY + ay * 200.0f * dt, 20.0f,  700.0f);

        DrawFormatString(10, 10,  white, "FPS: %.1f", GetFPS());
        DrawFormatString(10, 40,  white, "Keys held : %d", keyCount);
        DrawFormatString(10, 60,  white, "Mouse     : (%d, %d)  Buttons: %d  Wheel: %d", mx, my, mb, wheel);
        DrawFormatString(10, 80,  white, "Pad raw   : 0x%08X", pad);
        DrawFormatString(10, 100, jump ? green  : white, "JUMP (Space/A) : %s", jump ? "PRESSED"   : "---");
        DrawFormatString(10, 120, fire ? yellow : white, "FIRE (Z/B)     : %s", fire ? "TRIGGERED" : "---");

        DrawString(10, 155, "WASD : move box   |   ESC : quit", white);
        DrawBox(static_cast<int>(boxX) - 20, static_cast<int>(boxY) - 20,
                static_cast<int>(boxX) + 20, static_cast<int>(boxY) + 20,
                jump ? green : red, TRUE);

        DrawCircle(mx, my, 5, yellow, TRUE);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
