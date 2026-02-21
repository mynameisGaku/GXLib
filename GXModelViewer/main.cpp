/// @file GXModelViewer/main.cpp
/// @brief GXModelViewerアプリケーションのエントリポイント
///
/// Win32 WinMain関数。アプリケーションの初期化→メインループ→終了処理を行う。

#include "GXModelViewerApp.h"

/// @brief Win32エントリポイント。ウィンドウ1600x900で起動し、メインループを実行する。
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
