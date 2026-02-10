# GXLib

DirectX 12 ベースの 2D/3D ゲームエンジン。DXLib 互換 API を備え、C++20 / Windows 環境で動作します。

## 公開ページ (GitHub Pages)
https://mynameisgaku.github.io/GXLib/

## 特徴

- **DirectX 12 ネイティブ** — D3D12 によるローレベル GPU 制御
- **2D 描画** — SpriteBatch / PrimitiveBatch / SpriteSheet / Animation2D / Camera2D
- **3D レンダリング** — PBR シェーダー / glTF・FBX・OBJ モデル / スケルタルアニメーション / アニメーションブレンド / Terrain
- **シャドウ** — Cascaded Shadow Maps (4 分割) / Spot Shadow / Point Shadow (6 面キューブ)
- **ポストエフェクト** — HDR / Bloom / Tonemapping (Reinhard/ACES/Uncharted2) / FXAA / Vignette / ChromaticAberration / ColorGrading / SSAO / DoF / MotionBlur / SSR / Outline / VolumetricLight / TAA / AutoExposure
- **レイヤーシステム** — RenderLayer / LayerStack / LayerCompositor / MaskScreen
- **GUI** — Flexbox レイアウト / CSS スタイルシート / XML 宣言的 UI / 17 種ウィジェット
- **アニメーション** — Animator (クロスフェード) / Humanoid リターゲット
- **テキスト** — DirectWrite ラスタライズ / Unicode フルサポート (日本語対応)
- **入力** — Keyboard / Mouse / Gamepad (XInput)
- **オーディオ** — XAudio2 (SE + BGM / フェード / ループ)
- **ファイル I/O** — VFS / 暗号化アーカイブ (AES-256 + LZ4) / 非同期ロード / ファイル監視
- **ネットワーク** — TCP / UDP / HTTP (sync + async) / WebSocket
- **動画** — Media Foundation による動画デコード / テクスチャ描画
- **数学** — Vector2/3/4 / Matrix4x4 / Quaternion / Color (DirectXMath ラッパー)
- **衝突判定** — 2D (AABB / Circle / Polygon / SAT) / 3D (Sphere / AABB / OBB / Ray / Frustum)
- **空間分割** — Quadtree / Octree / BVH (SAH)
- **物理** — 2D カスタム物理エンジン / 3D Jolt Physics ラッパー / MeshCollider (Static・Convex・Skinned)
- **マテリアル/シェーダー** — ランタイム差し替え (材質パラメータ・テクスチャ・シェーダー)
- **DXLib 互換** — GXLib.h ヘッダー 1 つで DXLib 風の簡易 API を提供
- **開発ツール** — シェーダーホットリロード / GPU タイムスタンププロファイラー / JSON 設定保存

## 必要環境

| 項目 | 要件 |
|------|------|
| OS | Windows 10/11 |
| コンパイラ | MSVC (Visual Studio 2022 以降) |
| C++ 標準 | C++20 |
| CMake | 3.24 以上 |
| SDK | Windows SDK 10.0.22000 以上 |
| GPU | DirectX 12 対応 |

## ビルド手順

```bash
# 1. CMake プロジェクト生成
cmake -B build -S .

# 2. Debug ビルド
cmake --build build --config Debug

# 3. Release ビルド
cmake --build build --config Release

# 4. テスト実行
ctest --test-dir build --build-config Debug
```

Visual Studio で開く場合は `build/GXLib.sln` を使用してください。

### FBX SDK（オプション）

FBX/OBJ ローダーは FBX SDK が検出できた場合のみ有効になります。デフォルトでは
`ThirdParty/FBXSDK/2020.3.9` を参照します。別の場所を使う場合は CMake 変数
`FBX_SDK_ROOT` を指定してください。

Sandbox と Samples はビルド後に `libfbxsdk.dll` を自動コピーします。独自アプリで使う場合は、
Debug/Release に対応する DLL を出力先に配置してください。

## プロジェクト構成

```
GXLib/
├── GXLib/                  # エンジン本体 (静的ライブラリ)
│   ├── Core/               # Application, Window, Timer, Logger
│   ├── Graphics/
│   │   ├── Device/         # GraphicsDevice, CommandQueue, SwapChain, GPUProfiler
│   │   ├── Pipeline/       # RootSignature, PipelineState, Shader, ShaderLibrary
│   │   ├── Resource/       # Texture, TextureManager, Buffer, RenderTarget, DepthBuffer
│   │   ├── Rendering/      # SpriteBatch, PrimitiveBatch, FontManager, TextRenderer
│   │   ├── 3D/             # Renderer3D, Camera3D, Model, Animator, Humanoid, Skybox, Terrain
│   │   ├── Layer/          # RenderLayer, LayerStack, LayerCompositor, MaskScreen
│   │   └── PostEffect/     # PostEffectPipeline, Bloom, SSAO, SSR, DoF, TAA, ...
│   ├── Input/              # Keyboard, Mouse, Gamepad, InputManager
│   ├── Audio/              # AudioDevice, Sound, SoundPlayer, MusicPlayer
│   ├── GUI/                # Widget, UIContext, UIRenderer, StyleSheet, GUILoader
│   │   └── Widgets/        # Panel, Button, TextInput, Slider, DropDown, ...
│   ├── IO/                 # FileSystem, Archive, AsyncLoader, FileWatcher
│   │   └── Network/        # TCPSocket, UDPSocket, HTTPClient, WebSocket
│   ├── Movie/              # MoviePlayer
│   ├── Math/               # Vector2/3/4, Matrix4x4, Quaternion, Color, Random
│   │   └── Collision/      # Collision2D/3D, Quadtree, Octree, BVH
│   ├── Physics/            # PhysicsWorld2D, PhysicsWorld3D (Jolt), RigidBody2D/3D, MeshCollider
│   ├── Compat/             # DXLib 互換レイヤー (GXLib.h)
│   └── ThirdParty/         # stb_image, cgltf, nlohmann/json, LZ4
├── Sandbox/                # テストアプリケーション
├── Samples/                # サンプルプロジェクト
│   ├── Shooting2D/         # 2D シューティングゲーム
│   ├── Platformer2D/       # 2D プラットフォーマー
│   ├── Walkthrough3D/      # 3D ウォークスルー
│   ├── GUIMenuDemo/        # GUI メニューデモ
│   └── PostEffectShowcase/ # ポストエフェクト一覧
├── Shaders/                # HLSL シェーダーファイル
├── Assets/                 # ランタイムアセット (CSS, XML, ...)
├── Tests/                  # Google Test ユニットテスト
├── docs/                   # ドキュメント
│   ├── tutorials/          # チュートリアル
│   ├── migration/          # DXLib 移行ガイド
│   └── APIReference.md     # API リファレンス
└── Doxyfile                # Doxygen 設定ファイル
```

## Samples

各サンプルの概要・操作・ビルド/実行は各 README を参照してください。

- [EasyHello](Samples/EasyHello/README.md) — 最小構成の 2D 入門
- [Shooting2D](Samples/Shooting2D/README.md) — 2D シューティング
- [Platformer2D](Samples/Platformer2D/README.md) — 2D プラットフォーマー
- [Walkthrough3D](Samples/Walkthrough3D/README.md) — 3D ウォークスルー
- [GUIMenuDemo](Samples/GUIMenuDemo/README.md) — XML/CSS GUI デモ
- [PostEffectShowcase](Samples/PostEffectShowcase/README.md) — ポストエフェクト一覧

## クイックスタート

### DXLib 互換 API (最小構成)

```cpp
#include "GXLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    while (ProcessMessage() == 0)
    {
        ClearDrawScreen();

        DrawString(100, 100, "Hello GXLib!", GetColor(255, 255, 255));
        DrawCircle(640, 360, 50, GetColor(255, 0, 0), TRUE);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
```

### ネイティブ API

```cpp
#include "Core/Application.h"
#include "Graphics/Device/GraphicsDevice.h"
#include "Graphics/Rendering/SpriteBatch.h"
#include "Graphics/Rendering/FontManager.h"
#include "Graphics/Rendering/TextRenderer.h"

// Application + GraphicsDevice + SpriteBatch を初期化後:

// フォント作成 (日本語フル対応)
int font = fontManager.CreateFont(L"Meiryo", 24);

// フレームループ内:
fontManager.FlushAtlasUpdates();  // フレーム境界でアトラス更新
spriteBatch.Begin(cmdList, frameIndex);
textRenderer.DrawString(font, 10, 10, L"こんにちは世界！", 0xFFFFFFFF);
spriteBatch.End();
```

## HDR レンダリングパイプライン

```
Scene → HDR RT → [SSAO] → [SSR] → [VolumetricLight] → [Bloom] → [DoF]
     → [MotionBlur] → [Outline] → [TAA] → [ColorGrading] → [AutoExposure]
     → [Tonemap] → [FXAA] → [Vignette] → Backbuffer
```

各エフェクトは独立して ON/OFF 可能。JSON 設定ファイルでパラメータを永続化できます。

## GUI システム

CSS + XML による宣言的 UI を提供します。

```xml
<!-- Assets/ui/menu.xml -->
<Panel id="root" class="mainMenu">
  <TextWidget font="large" text="Game Title" />
  <Button id="startBtn" class="menuBtn" onClick="onStartGame" text="Start" />
  <Slider id="volume" min="0" max="100" value="80" onValueChanged="onVolume" />
  <TextInput id="name" placeholder="Name..." onValueChanged="onNameChanged" />
</Panel>
```

```css
/* Assets/ui/menu.css */
.mainMenu {
  flex-direction: column;
  align-items: center;
  gap: 12;
  padding: 40;
  background-color: rgba(0, 0, 0, 0.8);
}
.menuBtn {
  width: 200; height: 48;
  corner-radius: 8;
  background-color: #2255AA;
}
.menuBtn:hover { background-color: #3366CC; }
```

対応ウィジェット: Panel, TextWidget, Button, Spacer, ProgressBar, Image, CheckBox, Slider, ScrollView, RadioButton, DropDown, ListView, TabView, Dialog, Canvas, TextInput

## サードパーティライブラリ

| ライブラリ | 用途 | ライセンス |
|-----------|------|-----------|
| [stb_image](https://github.com/nothings/stb) | 画像読み込み | Public Domain |
| [cgltf](https://github.com/jkuhlmann/cgltf) | glTF パーサー | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON パーサー | MIT |
| [LZ4](https://github.com/lz4/lz4) | 高速圧縮 | BSD-2 |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 3D 物理エンジン | MIT |
| [Google Test](https://github.com/google/googletest) | ユニットテスト | BSD-3 |
| [FBX SDK](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk) | FBX/OBJ 読み込み | Autodesk FBX SDK License |

## ドキュメント

- [API リファレンス](docs/APIReference.md) — 全公開 API の一覧
- [チュートリアル](docs/tutorials/) — ステップバイステップガイド
  - [01: はじめに](docs/tutorials/01_GettingStarted.md)
  - [02: 2D 描画](docs/tutorials/02_Drawing2D.md)
  - [03: 2D ゲーム](docs/tutorials/03_Game2D.md)
  - [04: 3D レンダリング](docs/tutorials/04_Rendering3D.md)
  - [05: GUI](docs/tutorials/05_GUI.md)
- [DXLib 移行ガイド](docs/migration/DxLibMigrationGuide.md)

### Doxygen による API ドキュメント生成

```bash
doxygen Doxyfile
# docs/api/html/index.html にHTMLが生成されます
```

## ライセンス

本プロジェクトのライセンスについてはリポジトリオーナーにお問い合わせください。
