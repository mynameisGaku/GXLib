/// @file main.cpp
/// @brief 02-hello-sound — SE再生とBGMループ (ADR-0017 L1)
///
/// 学習ポイント / Learning points:
///   - LoadSoundMem / PlaySoundMem でSE / SE playback
///   - PlayMusic / StopMusic でBGMループ / BGM loop
///   - ChangeVolumeSoundMem で音量制御 / Volume control
///   - アセット不在時のフォールバック / Asset-missing fallback

#include "GXLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("02 - Hello Sound");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    int se = LoadSoundMem("Assets/audio/se_decide.wav");
    const bool hasSE = (se != -1);
    const bool hasBGM = (PlayMusic("Assets/audio/bgm_main.ogg", GX_PLAYTYPE_LOOP) == 0);

    int volume = 180;
    if (hasSE) ChangeVolumeSoundMem(volume, se);

    unsigned int white = GetColor(255, 255, 255);
    unsigned int gray  = GetColor(160, 160, 160);
    unsigned int red   = GetColor(255, 100, 100);

    float seCooldown = 0.0f;

    while (ProcessMessage() == 0)
    {
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        float dt = GetDeltaTime();
        if (seCooldown > 0.0f) seCooldown -= dt;

        ClearDrawScreen();

        // SPACE で SE 再生 (cooldown で多重起動防止)
        if (hasSE && CheckHitKey(KEY_INPUT_SPACE) && seCooldown <= 0.0f)
        {
            PlaySoundMem(se, GX_PLAYTYPE_BACK, TRUE);
            seCooldown = 0.3f;
        }

        // UP/DOWN で音量調整
        if (hasSE)
        {
            if (CheckHitKey(KEY_INPUT_UP))   { volume = (volume < 253) ? volume + 2 : 255; ChangeVolumeSoundMem(volume, se); }
            if (CheckHitKey(KEY_INPUT_DOWN)) { volume = (volume > 2)   ? volume - 2 : 0;   ChangeVolumeSoundMem(volume, se); }
        }

        DrawFormatString(10, 10,  white, "FPS: %.1f", GetFPS());
        DrawString(10, 40,  "SPACE  : Play SE", white);
        DrawString(10, 60,  "UP/DOWN: Adjust SE volume", white);
        DrawFormatString(10, 80,  white, "Volume : %d / 255", volume);
        DrawFormatString(10, 110, hasBGM ? white : red, "BGM    : %s", hasBGM ? "playing (loop)" : "[NOT FOUND] check Assets/audio/bgm_main.ogg");
        DrawFormatString(10, 130, hasSE  ? white : red, "SE     : %s", hasSE  ? "ready"           : "[NOT FOUND] check Assets/audio/se_decide.wav");
        DrawString(10, 160, "ESC: quit", gray);

        ScreenFlip();
    }

    StopMusic();
    if (hasSE) DeleteSoundMem(se);
    GX_End();
    return 0;
}
