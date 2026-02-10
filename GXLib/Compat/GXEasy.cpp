#include "pch.h"
#include "Compat/GXEasy.h"

namespace GXEasy
{

int Run(App& app, const AppConfig& config)
{
    app.OnBoot();

    ChangeWindowMode(config.windowed ? TRUE : FALSE);
    SetGraphMode(config.width, config.height, config.colorBitNum);
    {
#ifdef UNICODE
        SetMainWindowText(config.title.c_str());
#else
        std::string titleA;
        int len = WideCharToMultiByte(CP_ACP, 0, config.title.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len > 0)
        {
            titleA.resize(len - 1);
            WideCharToMultiByte(CP_ACP, 0, config.title.c_str(), -1, titleA.data(), len, nullptr, nullptr);
            SetMainWindowText(titleA.c_str());
        }
#endif
    }
    SetBackgroundColor(config.bgR, config.bgG, config.bgB);

    if (GX_Init() == -1)
        return -1;

    SetDrawScreen(GX_SCREEN_BACK);

    app.Start();

    int lastTime = GetNowCount();

    while (ProcessMessage() == 0)
    {
        if (config.allowEscapeExit && CheckHitKey(KEY_INPUT_ESCAPE))
            break;

        int now = GetNowCount();
        float dt = (now - lastTime) / 1000.0f;
        if (dt > config.maxDeltaTime) dt = config.maxDeltaTime;
        lastTime = now;

        app.Update(dt);

        if (config.autoClear)
            ClearDrawScreen();

        app.Draw();

        if (config.autoPresent)
            ScreenFlip();
    }

    app.Release();
    GX_End();
    return 0;
}

} // namespace GXEasy
