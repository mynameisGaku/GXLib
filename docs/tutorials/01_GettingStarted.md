# 01 - Getting Started

GXLib は DirectX 12 ベースの 2D/3D ゲームエンジンです。
このチュートリアルでは、プロジェクトのビルドからウィンドウ表示までを解説します。

## このチュートリアルで学ぶこと

- GXLib プロジェクトのビルド方法
- 最小構成のウィンドウ表示プログラム (Compat API)
- ネイティブ API を使った Hello Window
- プロジェクト構成の概要

## 前提知識

- C++ の基本構文（変数、関数、if/for）
- コマンドプロンプトの基本操作
- Visual Studio 2022 のインストール

→ 不安な方は [00_Prerequisites.md](00_Prerequisites.md) を先に読んでください

## 必要環境

| 項目 | 要件 | 補足 |
|------|------|------|
| OS | Windows 10/11 (64-bit) | |
| IDE | Visual Studio 2022 (v143 ツールセット) | C++ デスクトップ開発ワークロードが必要 |
| CMake | 3.24 以上 | [cmake.org](https://cmake.org/download/) からダウンロード |
| Windows SDK | 10.0.19041.0 以上 | VS インストーラーで追加可能 |

## ビルド手順

```bash
# 1. リポジトリをクローン（ダウンロード）
git clone <repository-url> GXLib
cd GXLib

# 2. CMake でビルドファイルを生成
#    -B build: buildフォルダにビルド設定を出力
#    -S .    : 現在のフォルダをソースとして指定
cmake -B build -S .

# 3. Debug（デバッグ）ビルドを実行
cmake --build build --config Debug

# 4. テスト実行（オプション）
ctest --test-dir build --build-config Debug
```

> **ヒント:** Visual Studio で開く場合は `build/GXLib.sln` をダブルクリックしてください。
> ソリューションエクスプローラーにすべてのプロジェクトが表示されます。

## Hello Window（簡易API / Compat API）

最も簡単なウィンドウ表示プログラムです。DXLib 互換の API を使っており、
`Compat/GXLib.h` をインクルードするだけで使えます。

```cpp
#include "Compat/GXLib.h"

// WinMain: Windows アプリケーションのエントリーポイント（開始点）
// コンソールアプリの main() に相当します
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);             // TRUE: ウィンドウモードで起動（FALSE だとフルスクリーン）
    SetGraphMode(1280, 960, 32);        // 画面サイズ: 幅1280px, 高さ960px, 色深度32bit
    SetMainWindowText("Hello GXLib");   // ウィンドウのタイトルバーに表示されるテキスト

    if (GX_Init() != 0) return -1;     // エンジンの初期化（失敗時は -1 を返して終了）
    SetDrawScreen(GX_SCREEN_BACK);     // 描画先を裏画面に設定（下記「なぜ？」参照）

    // メインループ: ウィンドウが閉じられるまで繰り返す
    // ProcessMessage() はウィンドウメッセージを処理し、
    // 閉じるボタンが押されたら 0 以外を返します
    while (ProcessMessage() == 0)
    {
        ClearDrawScreen();              // 画面を黒でクリア（前フレームの描画を消す）

        DrawString(
            100, 100,                   // 描画位置 (x=100, y=100)（左上が原点）
            "Hello, GXLib!",            // 表示するテキスト
            GetColor(255, 255, 255)     // 文字色 (R=255, G=255, B=255 = 白)
        );

        ScreenFlip();                   // 裏画面の内容を表画面に表示（画面更新）
    }

    GX_End();                           // エンジンの終了処理（リソース解放）
    return 0;
}
```

> **なぜ裏画面 (Back Buffer) に描画するのか？**
>
> 画面に直接描画すると、描画途中の不完全な状態がモニターに映り、
> ちらつき（フリッカー）が発生します。
> そこで、見えない「裏画面」に完成画像を作ってから
> `ScreenFlip()` で表画面と切り替えます。
> この手法を **ダブルバッファリング** と呼び、
> ほぼすべてのゲームで使われている基本技術です。

## Hello Window（ネイティブAPI）

より高度な制御が必要な場合は、GXLib のネイティブ API を直接使用します。
Compat API の裏側で動いている仕組みが見えるので、エンジンの理解が深まります。

```cpp
#include "pch.h"                          // プリコンパイル済みヘッダー（ビルド高速化用）
#include "Core/Application.h"             // ウィンドウ管理・メッセージループ
#include "Graphics/Device/GraphicsDevice.h"  // GPU デバイスの管理
#include "Graphics/Device/CommandQueue.h"    // GPU への描画命令送信
#include "Graphics/Device/CommandList.h"     // 描画命令の記録
#include "Graphics/Device/SwapChain.h"       // ダブルバッファリングの管理

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // --- 初期化フェーズ ---

    GX::Application app;
    app.Initialize("Hello GXLib", 1280, 960);
    // ウィンドウを作成。タイトル "Hello GXLib", サイズ 1280x960

    GX::GraphicsDevice device;
    device.Initialize(app.GetWindow());
    // GPU デバイスを初期化。DirectX 12 の機能を使う準備をします

    GX::CommandQueue cmdQueue;
    cmdQueue.Initialize(device);
    // コマンドキュー: 記録した描画命令を GPU に送信する窓口

    GX::CommandList cmdList;
    cmdList.Initialize(device);
    // コマンドリスト: 「何を描画するか」を記録するリスト

    GX::SwapChain swapChain;
    swapChain.Initialize(device, cmdQueue, app.GetWindow());
    // スワップチェーン: 表画面と裏画面の切り替えを管理

    // --- メインループ ---

    while (app.Update())  // ウィンドウメッセージ処理（閉じられたら false）
    {
        auto frameIndex = swapChain.GetCurrentBackBufferIndex();
        // 現在の裏画面番号を取得（0 または 1、ダブルバッファリング）

        cmdList.Begin(frameIndex);
        // 描画命令の記録を開始

        auto rtv = swapChain.GetCurrentRTV();
        // RTV (Render Target View): 「ここに描画してね」という描画先の指定

        float clearColor[] = { 0.1f, 0.1f, 0.2f, 1.0f };
        // クリア色 (R=0.1, G=0.1, B=0.2, A=1.0) — 暗い紺色

        cmdList.Get()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        // 画面を指定色でクリア

        cmdList.End();
        // 描画命令の記録を終了

        cmdQueue.Execute(cmdList);
        // 記録した命令を GPU に送信して実行

        swapChain.Present();
        // 裏画面を表画面に表示（ScreenFlip と同じ役割）

        cmdQueue.WaitForGPU();
        // GPU の処理完了を待つ（CPU と GPU の同期）
    }

    return 0;
}
```

> **Compat API とネイティブ API の違い**
>
> Compat API は内部でネイティブ API を呼び出しています。
> 少ないコードで手軽に使えますが、カスタマイズの幅は限られます。
> ネイティブ API は記述量が増えますが、描画パイプラインを細かく制御できます。
> 最初は Compat API で始めて、慣れたらネイティブ API に移行するのがおすすめです。

## プロジェクト構成

```
GXLib/
├── GXLib/          # エンジンライブラリ (.lib) — ゲームの「部品箱」
│   ├── Core/       # アプリケーション管理、ウィンドウ、タイマー
│   ├── Graphics/   # 描画 (2D/3D/ポストエフェクト/レイヤー/レイトレーシング)
│   ├── Input/      # 入力 (キーボード/マウス/ゲームパッド)
│   ├── Audio/      # サウンド (効果音/BGM)
│   ├── GUI/        # UIシステム (XML+CSSで画面メニュー等を構築)
│   ├── IO/         # ファイル読み書き/ネットワーク通信
│   ├── Math/       # ベクトル/行列/衝突判定 (数学ユーティリティ)
│   ├── Physics/    # 2D/3D物理演算 (重力・衝突応答)
│   └── Compat/     # DXLib互換レイヤー (簡易API)
├── gxformat/       # バイナリ形式定義 (GXMD/GXAN/GXPAK)
├── gxconv/         # モデルコンバーター (FBX/OBJ/glTF → .gxmd/.gxan)
├── gxloader/       # ランタイムローダー (静的ライブラリ)
├── GXModelViewer/  # ImGui ベースの 3D モデルビューア
├── Sandbox/        # テストアプリケーション
├── Samples/        # サンプルプロジェクト (すぐ動かせるゲーム例)
├── Shaders/        # HLSL シェーダー (GPUで動くプログラム)
├── Assets/         # アセット (画像・音声・UIファイル等)
└── Tests/          # ユニットテスト
```

## よくある問題

### ビルドが失敗する

**症状**: `cmake -B build -S .` でエラーが出る

**対処法**:
1. Visual Studio 2022 の「C++ によるデスクトップ開発」ワークロードがインストールされているか確認
2. Windows SDK のバージョンが 10.0.19041.0 以上か確認（VS Installer → 個別のコンポーネント）
3. CMake のバージョンが 3.24 以上か確認: `cmake --version`

### 実行時にシェーダーエラーが出る

**症状**: 実行するとシェーダーファイルが見つからないエラー

**対処法**:
- `dxcompiler.dll` と `dxil.dll` が exe と同じフォルダにあるか確認してください
- Visual Studio から実行する場合、「デバッグ作業ディレクトリ」が exe の出力先と一致しているか確認してください
- `Shaders/` フォルダが exe と同じ階層にコピーされているか確認してください

### 画面が真っ黒のまま

**症状**: ウィンドウは表示されるが何も描画されない

**対処法**:
- `SetDrawScreen(GX_SCREEN_BACK)` を `GX_Init()` の直後に呼んでいるか確認
- `ClearDrawScreen()` → 描画処理 → `ScreenFlip()` の順序が正しいか確認
- GPU ドライバーが最新か確認（NVIDIA / AMD / Intel の公式サイトからアップデート）

## 次のステップ

- [00_Prerequisites.md](00_Prerequisites.md) — 前提知識の確認
- [02_Drawing2D.md](02_Drawing2D.md) — スプライト、図形、テキスト描画
- [03_Game2D.md](03_Game2D.md) — 入力、サウンド、2Dゲーム制作
- [04_Rendering3D.md](04_Rendering3D.md) — 3D描画、PBR、ライティング
- [05_GUI.md](05_GUI.md) — XML+CSS によるGUI構築

用語がわからない場合 → [用語集 (Glossary)](../Glossary.md)
