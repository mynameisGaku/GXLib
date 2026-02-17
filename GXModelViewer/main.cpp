/// @file GXModelViewer/main.cpp
/// @brief GXModelViewer entry point

#include "GXModelViewerApp.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    GXModelViewerApp app;
    if (!app.Initialize(hInstance, 1600, 900, L"GXModelViewer"))
        return 1;

    int result = app.Run();
    app.Shutdown();
    return result;
}
