#include "pch.h"
#include "Compat/GXEasy.h"
#include "Compat/CompatContext.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
namespace GXEasy
{

int Run(App& app, const AppConfig& config)
{
    app.OnBoot();

    const bool useFrameLimit = (config.targetFps > 0) && !config.vsync;
    if (useFrameLimit)
    {
        // Raise Sleep precision for stable high-FPS limiting.
        timeBeginPeriod(1);
    }

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

    GX_Internal::CompatContext::Instance().SetVSync(config.vsync);

    SetDrawScreen(GX_SCREEN_BACK);

    app.Start();

    LARGE_INTEGER qpcFreq;
    QueryPerformanceFrequency(&qpcFreq);
    const double qpcToSec = 1.0 / static_cast<double>(qpcFreq.QuadPart);
    auto QpcSeconds = [qpcToSec]() {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return static_cast<double>(now.QuadPart) * qpcToSec;
    };
    const double targetSec = useFrameLimit ? (1.0 / static_cast<double>(config.targetFps)) : 0.0;

    double lastTime = QpcSeconds();

    while (ProcessMessage() == 0)
    {
        if (config.allowEscapeExit && CheckHitKey(KEY_INPUT_ESCAPE))
            break;

        double frameStart = QpcSeconds();
        float dt = static_cast<float>(frameStart - lastTime);
        if (dt > config.maxDeltaTime) dt = config.maxDeltaTime;
        lastTime = frameStart;

        app.Update(dt);

        if (config.autoClear)
            ClearDrawScreen();

        app.Draw();

        if (config.autoPresent)
            ScreenFlip();

        if (useFrameLimit)
        {
            double elapsed = QpcSeconds() - frameStart;
            double remaining = targetSec - elapsed;
            if (remaining > 0.0)
            {
                const double sleepPadding = 0.001;
                if (remaining > sleepPadding)
                {
                    DWORD sleepMs = static_cast<DWORD>((remaining - sleepPadding) * 1000.0);
                    if (sleepMs > 0)
                        Sleep(sleepMs);
                }

                while ((QpcSeconds() - frameStart) < targetSec)
                    Sleep(0);
            }
        }
    }

    if (useFrameLimit)
        timeEndPeriod(1);

    app.Release();
    GX_End();
    return 0;
}

} // namespace GXEasy
