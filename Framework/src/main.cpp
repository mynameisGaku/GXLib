/// @file main.cpp
/// @brief GXLib Framework のエントリーポイント。最小構成で起動する。

#include "FrameworkApp.h"

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
#ifdef _DEBUG
    bool enableDebug = true;
#else
    bool enableDebug = false;
#endif

    GXFW::FrameworkApp app;
    GXFW::AppConfig config;
    config.title = L"GXLib Framework";
    config.width = 1280;
    config.height = 720;
    config.enableDebug = enableDebug;

    if (!app.Initialize(config))
        return -1;

    app.Run();
    app.Shutdown();
    return 0;
}
