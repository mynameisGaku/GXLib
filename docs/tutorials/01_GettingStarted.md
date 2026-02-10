# 01 - Getting Started

GXLib は DirectX 12 ベースの 2D/3D ゲームエンジンです。
このチュートリアルでは、プロジェクトのビルドからウィンドウ表示までを解説します。

## 必要環境

- Windows 10/11 (64-bit)
- Visual Studio 2022 (v143 ツールセット)
- CMake 3.24 以上
- Windows SDK 10.0.19041.0 以上

## ビルド手順

```bash
# 1. リポジトリをクローン
git clone <repository-url> GXLib
cd GXLib

# 2. CMake でビルドファイルを生成
cmake -B build -S .

# 3. Debug ビルド
cmake --build build --config Debug

# 4. テスト実行
ctest --test-dir build --build-config Debug
```

Visual Studio で開く場合は `build/GXLib.sln` を開いてください。

## Hello Window（簡易API）

最も簡単なウィンドウ表示プログラムです。`Compat/GXLib.h` をインクルードするだけで使えます。

```cpp
#include "Compat/GXLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);             // ウィンドウモード
    SetGraphMode(1280, 960, 32);        // 解像度設定
    SetMainWindowText("Hello GXLib");   // タイトル

    if (GX_Init() != 0) return -1;      // 初期化
    SetDrawScreen(GX_SCREEN_BACK);      // 裏画面描画

    while (ProcessMessage() == 0)
    {
        ClearDrawScreen();              // 画面クリア
        DrawString(100, 100, "Hello, GXLib!", GetColor(255, 255, 255));
        ScreenFlip();                   // 画面更新
    }

    GX_End();                           // 終了
    return 0;
}
```

## Hello Window（ネイティブAPI）

より高度な制御が必要な場合は、GXLib のネイティブ API を直接使用します。

```cpp
#include "pch.h"
#include "Core/Application.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Device/CommandQueue.h"
#include "Graphics/Device/CommandList.h"
#include "Graphics/Device/SwapChain.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    GX::Application app;
    app.Initialize("Hello GXLib", 1280, 960);

    GX::GraphicsDevice device;
    device.Initialize(app.GetWindow());

    GX::CommandQueue cmdQueue;
    cmdQueue.Initialize(device);

    GX::CommandList cmdList;
    cmdList.Initialize(device);

    GX::SwapChain swapChain;
    swapChain.Initialize(device, cmdQueue, app.GetWindow());

    while (app.Update())
    {
        auto frameIndex = swapChain.GetCurrentBackBufferIndex();
        cmdList.Begin(frameIndex);

        auto rtv = swapChain.GetCurrentRTV();
        float clearColor[] = { 0.1f, 0.1f, 0.2f, 1.0f };
        cmdList.Get()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

        cmdList.End();
        cmdQueue.Execute(cmdList);
        swapChain.Present();
        cmdQueue.WaitForGPU();
    }

    return 0;
}
```

## プロジェクト構成

```
GXLib/
├── GXLib/          # エンジンライブラリ (.lib)
│   ├── Core/       # アプリケーション、ウィンドウ
│   ├── Graphics/   # 描画 (2D/3D/PostEffect/Layer)
│   ├── Input/      # 入力 (キーボード/マウス/ゲームパッド)
│   ├── Audio/      # サウンド (SE/BGM)
│   ├── GUI/        # UIシステム (XML+CSS)
│   ├── IO/         # ファイル/ネットワーク
│   ├── Math/       # ベクトル/行列/衝突判定
│   ├── Physics/    # 2D/3D物理演算
│   └── Compat/     # DXLib互換レイヤー
├── Sandbox/        # テストアプリケーション
├── Samples/        # サンプルプロジェクト
├── Shaders/        # HLSLシェーダー
├── Assets/         # アセット
└── Tests/          # Google Test テスト
```

## 次のステップ

- [02_Drawing2D.md](02_Drawing2D.md) — スプライト、図形、テキスト描画
- [03_Game2D.md](03_Game2D.md) — 入力、サウンド、2Dゲーム制作
- [04_Rendering3D.md](04_Rendering3D.md) — 3D描画、PBR、ライティング
- [05_GUI.md](05_GUI.md) — XML+CSS によるGUI構築
