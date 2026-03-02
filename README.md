# GXLib

DirectX 12 ベースの 2D/3D ゲームエンジン。DXLib 互換 API を備え、C++20 / Windows 環境で動作します。
このライブラリの作成には、AIを使用しています。

## 公開ページ (GitHub Pages)
https://mynameisgaku.github.io/GXLib/

## 特徴

### レンダリング
- **DirectX 12 ネイティブ** — D3D12 によるローレベル GPU 制御
- **2D 描画** — SpriteBatch / PrimitiveBatch / SpriteSheet / Animation2D / Camera2D / Tilemap (TMX/TMJ)
- **3D レンダリング** — PBR / UTS2 Toon / Phong / Subsurface / ClearCoat / glTF・FBX・OBJ・GXMD モデル / スケルタルアニメーション / Terrain (LOD + スプラッティング)
- **DXR レイトレーシング** — RTReflections / RTGI (グローバルイルミネーション) / RTSoftShadows
- **シェーダーモデル** — ShaderRegistry (6 ShaderModel x static/skinned = 14 PSO 自動管理) / ShaderModelConstants
- **シャドウ** — CSM (自動カスケード分割, 4分割) / Spot Shadow / Point Shadow (6面キューブ)
- **GPU-Driven Rendering** — GPU カリング + IndirectDraw / HiZ オクルージョンカリング
- **FrameGraph** — レンダーパス依存解決 + リソースライフタイム管理
- **Mesh Shaders** — MeshPipeline (メッシュシェーダー + メッシュレット)
- **Variable Rate Shading** — VRSManager (SRI 自動生成)
- **Dynamic Resolution** — DynamicResolution (GPU 負荷ベースの動的解像度)

### ライティング・環境
- **IBL** — Image-Based Lighting (拡散照射 + プリフィルタ環境マップ + BRDF LUT)
- **ClusteredLighting** — タイルベースクラスタードライティング
- **SkyAtmosphere** — 大気散乱シミュレーション
- **VolumetricFog** — ボリュメトリックフォグ
- **VolumetricClouds** — ボリュメトリッククラウド
- **WaterRenderer** — 水面レンダリング (FFT 波形 + 反射/屈折)
- **LightProbeSystem** — ライトプローブ / 間接照明キャプチャ
- **LightmapBaker** — ライトマップベイク

### ポストエフェクト
HDR / Bloom / Tonemapping (Reinhard/ACES/Uncharted2) / FXAA / Vignette / TAA / AutoExposure / SSAO / SSR / DoF / MotionBlur / Outline / VolumetricLight / ChromaticAberration / ColorGrading / FilmGrain / SSGI / ContactShadows / LensFlare / DynamicResolution

### アニメーション
- **Animator** — クロスフェード + ルートモーション + アニメーションイベント
- **BlendStack** — 8レイヤー Override/Additive
- **BlendTree** — 1D/2D ブレンド
- **AnimatorStateMachine** — トリガー + 遷移
- **AnimationLayer** — レイヤードアニメーション合成
- **Humanoid リターゲット** — ボーンマッピング
- **MotionMatching** — モーションマッチングシステム
- **ProceduralAnimation** — プロシージャルアニメーション
- **IK** — FootIK / LookAtIK / CCD IK / FABRIK IK / FullBody IK
- **SpringBone** — 揺れ骨シミュレーション
- **ClothSimulation** — 布シミュレーション

### 物理
- **2D カスタム物理エンジン** — RigidBody2D
- **3D Jolt Physics ラッパー** — RigidBody3D / MeshCollider (Static・Convex・Skinned)
- **VehiclePhysics** — 車両物理
- **BuoyancySystem** — 浮力シミュレーション
- **RagdollBuilder** — ラグドール自動構築
- **CharacterController** — キャラクターコントローラー (スロープ, ステップ, クラウチ)
- **PhysicsMaterial** — 物理マテリアル (摩擦・反発)
- **ClothSimulator** — 物理ベース布

### AI
- **NavMesh** — Grid A* + Octile 距離
- **NavMesh3D** — 3D ナビゲーションメッシュ
- **PolyNavMesh** — ポリゴンベースナビメッシュ
- **NavAgent** — 経路追従 + スムース回転
- **RVO** — Reciprocal Velocity Obstacles (群衆回避)
- **BehaviorTree** — ビヘイビアツリー
- **GOAP** — Goal-Oriented Action Planning

### ネットワーク
- **TCP / UDP / HTTP / WebSocket** — 基本ネットワーク
- **ReliableChannel** — 信頼性のある UDP 通信
- **NetworkReplicator / ReplicatedProperty** — 状態レプリケーション
- **RollbackNetcode** — ロールバックネットコード
- **NATTraversal** — NAT 穴あけ
- **MatchmakingLobby** — マッチメイキング + ロビー
- **InterestManagement** — 関心領域管理
- **CloudSave** — クラウドセーブ
- **NetworkPrediction** — ラグ補償 + 予測

### オーディオ
- **XAudio2** — SE + BGM / フェード / ループ
- **3D サウンド** — AudioEmitter + AudioListener
- **AudioBus** — バスチェーンミキサー
- **AudioMixer** — ミキシング
- **AudioDSP** — DSP エフェクトチェーン
- **AudioOcclusion** — 遮蔽減衰
- **ReverbZone** — リバーブゾーン
- **AudioSnapshot** — スナップショット切り替え
- **SoundBank** — キューベースサウンド管理
- **OggStream** — OGG ストリーミング再生

### GUI
- **Flexbox レイアウト** / CSS スタイルシート / XML 宣言的 UI
- **17 種ウィジェット** — Panel, TextWidget, Button, Spacer, ProgressBar, Image, CheckBox, Slider, ScrollView, RadioButton, DropDown, ListView, TabView, Dialog, Canvas, TextInput
- **UITween** — UI アニメーション
- **DataBinding** — データバインディング
- **DragDropManager** — ドラッグ&ドロップ
- **Accessibility** — アクセシビリティ対応

### ECS (Entity Component System)
- **Data-Oriented** — World / Archetype / ComponentStorage / Query / System
- **EntityBridge** — 既存 Entity/Scene との橋渡し

### Editor
- **Gizmo** — 移動/回転/スケールギズモ
- **EntityPicker** — エンティティピッキング
- **MaterialEditor** — マテリアルエディタ
- **ParticleEditor** — パーティクルエディタ
- **TimelineEditor** — タイムラインエディタ
- **ShaderGraph** — シェーダーグラフ
- **TerrainSculptor** — 地形スカルプト
- **AudioMixerUI** — オーディオミキサー UI
- **AssetBrowser** — アセットブラウザ
- **PropertyInspector** — プロパティインスペクタ
- **SceneHierarchyPanel** — シーン階層パネル
- **SceneViewport** — シーンビューポート
- **ConsoleWindow** — コンソールウィンドウ
- **PlayInEditor** — Play-in-Editor

### シーン・ゲームプレイ
- **Entity / Scene / Component** — シーングラフ
- **Prefab / PrefabVariant** — プレハブシステム
- **SceneManager** — シーン管理 + トランジション
- **ScenePersistence** — Text/Binary シリアライズ
- **CheckpointSystem** — チェックポイント
- **CutsceneSystem** — カットシーン
- **DialogueSystem / DialogueGraph** — ダイアログ
- **QuestSystem** — クエスト管理
- **InventorySystem** — インベントリ
- **GameState** — ステートマシン
- **SaveSystem** — セーブ/ロード
- **ReplaySystem** — リプレイ録画/再生

### Reflection
- **TypeInfo / TypeRegistry** — ランタイム型情報
- **ReflectMacros** — リフレクションマクロ
- **JsonSerializer** — 自動 JSON 直列化

### システム・インフラ
- **Coroutine** — コルーチン (co_yield)
- **JobSystem** — マルチスレッドジョブ
- **HotReloadManager** — ホットリロード
- **EventBus** — イベント配信
- **ObjectPool** — オブジェクトプール
- **ActionScheduler** — 遅延・繰り返しアクション
- **Profiler / ProfilerOverlay / ProfilerGUI** — CPU/GPU プロファイリング
- **MemoryProfiler** — メモリプロファイリング
- **GPUDebugLayer** — GPU デバッグレイヤー
- **GPUMemoryAllocator / TLSFAllocator / BuddyAllocator** — メモリアロケーター
- **CrashReporter** — クラッシュレポート
- **BenchmarkRunner** — ベンチマーク
- **Localization** — ローカライゼーション
- **Noise** — Perlin ノイズ (ヘッダーオンリー)
- **StringUtils** — 文字列ユーティリティ
- **UndoSystem** — Undo/Redo

### その他
- **GPU パーティクル** — Compute Shader (Init/Emit/Update) + Billboard Draw / リングバッファ
- **2D パーティクル** — ParticleSystem2D / ParticleEmitter2D
- **LOD** — LODGroup (スクリーン占有率ベース + ヒステリシスバンド)
- **デカール** — Deferred Decal (ボックス投影 + 深度再構築)
- **トレイル** — TrailRenderer (リングバッファ)
- **VFXGraph** — VFX グラフ
- **Compute Skinning** — GPU スキニング
- **TextureStreaming** — テクスチャストリーミング
- **VirtualTexture** — 仮想テクスチャ
- **SamplerFeedback** — サンプラーフィードバック
- **DirectStorage** — DirectStorage 非同期ロード
- **HDRDisplayManager** — HDR ディスプレイ出力
- **ScreenCapture / VideoRecorder** — スクリーンキャプチャ / 動画録画
- **AsyncComputeScheduler** — 非同期コンピュートスケジューリング
- **レイヤーシステム** — RenderLayer / LayerStack / LayerCompositor / MaskScreen
- **Lua スクリプティング** — Lua 5.4 + sol2 / ScriptBindings / VisualScript
- **DXLib 互換** — GXLib.h ヘッダー 1 つで DXLib 風の簡易 API を提供
- **GXModelViewer** — ImGui Docking ベース 3D モデルビューア (19パネル)
- **テキスト** — DirectWrite ラスタライズ / Unicode フルサポート / BitmapFont / SDFFont
- **動画** — Media Foundation による動画デコード
- **IME** — IME 入力対応
- **マルチスレッド描画** — ParallelCommandRecorder / IndirectCommandBuffer / AsyncComputeQueue

## サブライブラリ構成

13 サブライブラリ + umbrella の CMake モジュール構成:

| ライブラリ | 依存先 | PCH | 概要 |
|-----------|--------|-----|------|
| GXLib_Foundation | なし | pch_common.h | Math, Collision, Spatial, Transform3D |
| GXLib_Core | Foundation | pch_common.h | Application, Window, Timer, Logger, Scene, Reflect, EventBus, JobSystem 等 |
| GXLib_Input | Core | pch_common.h | Keyboard, Mouse, Gamepad, InputManager, ActionMapping, IME |
| GXLib_Audio | Core | pch_audio.h | AudioDevice, Sound, AudioManager, AudioMixer, AudioDSP, SoundBank 等 |
| GXLib_IO | Core | pch_common.h | FileSystem, Archive, Network, MoviePlayer, DirectStorage |
| GXLib_Physics | Foundation + Jolt | pch_common.h | PhysicsWorld2D/3D, Vehicle, Buoyancy, Ragdoll, CharacterController |
| GXLib_AI | Foundation | pch_common.h | NavMesh, NavMesh3D, PolyNavMesh, NavAgent, RVO, BehaviorTree, GOAP |
| GXLib_Scene | Foundation | pch_common.h | Entity, Component, Scene (データのみ) |
| GXLib_ECS | Core + Scene | pch_common.h | World, Archetype, ComponentStorage, Query, System |
| GXLib_Graphics | Core | pch_graphics.h | Device, Pipeline, Resource, Rendering, 3D, PostEffect, Layer, RayTracing, FrameGraph, Editor 等 |
| GXLib_GUI | Graphics + Input | pch_graphics.h | Widget, UIContext, UIRenderer, StyleSheet, 17 Widgets |
| GXLib_Editor | Graphics + Scene | pch_graphics.h | Gizmo, EntityPicker, MaterialEditor, ParticleEditor, TimelineEditor 等 14パネル |
| GXLib_Script | Core+多数 (optional) | pch_common.h | ScriptEngine, ScriptBindings, VisualScript (Lua 5.4 + sol2) |
| **GXLib** (umbrella) | 全サブライブラリ | pch.h | Compat/ + GX/ — DXLib 互換 API |

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

# 3. Release ビルド（最適化あり）
cmake --build build --config Release

# 4. テスト実行 (1952 テスト)
ctest --test-dir build --build-config Debug
```

Visual Studio で開く場合は `build/GXLib.sln` を使用してください。

### SDK インストール

```bash
# SDK パッケージを指定ディレクトリにインストール
cmake --install build --config Release --prefix <install-dir>
```

インストール後、利用側の CMakeLists.txt で:

```cmake
find_package(GXLib REQUIRED)
target_link_libraries(MyApp PRIVATE GXLib::GXLib)
```

### FBX SDK（オプション）

FBX/OBJ ローダーは FBX SDK が検出できた場合のみ有効になります。デフォルトでは
`ThirdParty/FBXSDK/2020.3.9` を参照します。別の場所を使う場合は CMake 変数
`FBX_SDK_ROOT` を指定してください。

## プロジェクト構成

```
GXLib/
├── GXLib/                      # エンジン本体 (静的ライブラリ)
│   ├── Core/                   # Application, Window, Timer, Logger, EventBus, JobSystem, ...
│   │   ├── Scene/             # Entity, Component, Scene, Prefab, SceneManager, ScenePersistence
│   │   └── Reflect/           # TypeInfo, TypeRegistry, ReflectMacros, JsonSerializer
│   ├── Graphics/
│   │   ├── Device/            # GraphicsDevice, CommandQueue, SwapChain, GPUProfiler, VRSManager, HDRDisplay
│   │   ├── Pipeline/          # RootSignature, PipelineState, Shader, MeshPipeline
│   │   ├── Resource/          # Texture, TextureManager, Buffer, GPUMemoryAllocator, VirtualTexture, TextureStreaming
│   │   ├── Rendering/         # SpriteBatch, PrimitiveBatch, FontManager, TextRenderer, Tilemap, Camera2D
│   │   ├── 3D/                # Renderer3D, Camera3D, Model, Animator, ShaderRegistry, Terrain, ...
│   │   │                       # SceneRenderer, GPUDrivenRenderer, GPUParticleSystem, ClothSimulation
│   │   │                       # SkyAtmosphere, WaterRenderer, VolumetricFog/Clouds, ClusteredLighting
│   │   │                       # IKSolver (CCD/FABRIK), FullBodyIK, FootIK, LookAtIK, SpringBone
│   │   │                       # MotionMatching, ProceduralAnimation, AnimationLayer, BlendStack/BlendTree
│   │   │                       # ComputeSkinning, DebugDraw3D, Decal, LODGroup, TrailRenderer, VFXGraph
│   │   ├── FrameGraph/        # FrameGraph, RenderPass, ResourceNode
│   │   ├── Layer/             # RenderLayer, LayerStack, LayerCompositor, MaskScreen
│   │   ├── PostEffect/        # PostEffectPipeline, Bloom, SSAO, SSR, DoF, TAA, SSGI, ContactShadows, ...
│   │   ├── RayTracing/        # RTAccelerationStructure, RTPipeline, RTReflections, RTGI, RTSoftShadows
│   │   └── Text/              # BitmapFont, SDFFont
│   ├── Input/                  # Keyboard, Mouse, Gamepad, InputManager, ActionMapping, IMEHandler
│   ├── Audio/                  # AudioDevice, Sound, AudioManager, AudioMixer, AudioBus, AudioDSP
│   │                           # AudioOcclusion, ReverbZone, SoundBank, AudioSnapshot, OggStream
│   ├── GUI/                    # Widget, UIContext, UIRenderer, StyleSheet, UITween, DataBinding
│   │   └── Widgets/           # Panel, Button, TextInput, Slider, DropDown, ... (17種)
│   ├── IO/                     # FileSystem, Archive, AsyncLoader, FileWatcher, DirectStorage
│   │   └── Network/           # TCP, UDP, HTTP, WebSocket, ReliableChannel, RollbackNetcode
│   │                           # NetworkReplicator, NATTraversal, MatchmakingLobby, CloudSave
│   ├── Movie/                  # MoviePlayer
│   ├── Math/                   # Vector2/3/4, Matrix4x4, Quaternion, Color, Random, Noise, Spline, Tween
│   │   └── Collision/         # Collision2D/3D, Quadtree, Octree, BVH
│   ├── Physics/                # PhysicsWorld2D/3D (Jolt), VehiclePhysics, BuoyancySystem
│   │                           # RagdollBuilder, CharacterController, ClothSimulator, PhysicsMaterial
│   ├── AI/                     # NavMesh, NavMesh3D, PolyNavMesh, NavAgent, RVO, BehaviorTree, GOAP
│   ├── ECS/                    # World, Archetype, ComponentStorage, Query, System, EntityBridge
│   ├── Editor/                 # Gizmo, EntityPicker, MaterialEditor, ParticleEditor, TimelineEditor
│   │                           # ShaderGraph, TerrainSculptor, AudioMixerUI, AssetBrowser, ...
│   ├── Script/                 # ScriptEngine, ScriptBindings, VisualScript (Lua 5.4 + sol2)
│   ├── Compat/                 # DXLib 互換レイヤー (GXLib.h)
│   ├── GX/                     # GXLib ネイティブ簡易 API (App, Draw2D, Audio, Input, ...)
│   └── ThirdParty/             # stb_image, cgltf, nlohmann/json, LZ4, ufbx
├── extern/
│   ├── gxformat/              # バイナリ形式定義 (GXMD/GXAN/GXPAK, ヘッダーオンリー)
│   ├── gxloader/              # ランタイムローダー (静的ライブラリ, ボーンマッチング)
│   └── ThirdParty/            # 外部依存 (Jolt Physics 等)
├── tools/
│   ├── gxconv/                # CLI モデルコンバーター (OBJ/FBX/glTF → .gxmd/.gxan)
│   └── gxpak/                 # CLI バンドルツール (pack/unpack/list .gxpak)
├── GXModelViewer/              # ImGui Docking ベース 3D モデルビューア
├── Sandbox/                    # テストアプリケーション
├── Game/                       # ゲームテンプレート
├── Shaders/                    # HLSL シェーダーファイル
├── Assets/                     # ランタイムアセット (CSS, XML, ...)
├── Tests/                      # Google Test ユニットテスト (1952 テスト)
├── docs/                       # API リファレンスサイト + Doxygen
├── cmake/                      # CMake モジュール (GXLibConfig.cmake)
├── template/                   # プロジェクトテンプレート
└── GXLib-SDK/                  # SDK パッケージ出力
```

## クイックスタート

### DXLib 互換 API (最小構成)

```cpp
#include "GXLib.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);                 // ウィンドウモードで起動
    SetGraphMode(1280, 720, 32);            // 画面サイズ: 1280x720, 色深度32bit
    if (GX_Init() == -1) return -1;         // エンジン初期化
    SetDrawScreen(GX_SCREEN_BACK);          // 裏画面（ダブルバッファリング）に描画

    while (ProcessMessage() == 0)           // メインループ
    {
        ClearDrawScreen();                  // 画面クリア

        DrawString(100, 100, "Hello GXLib!", GetColor(255, 255, 255));
        DrawCircle(640, 360, 50, GetColor(255, 0, 0), TRUE);

        ScreenFlip();                       // 裏画面を表画面に切り替え
    }

    GX_End();                               // 終了処理
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
fontManager.FlushAtlasUpdates();            // フレーム境界でアトラス更新
spriteBatch.Begin(cmdList, frameIndex);
textRenderer.DrawString(
    font,               // フォントハンドル
    10, 10,             // 描画位置 (x, y)
    L"こんにちは世界！",   // テキスト (Unicode 対応)
    0xFFFFFFFF          // 色 (ARGB: 白)
);
spriteBatch.End();
```

## HDR レンダリングパイプライン

3D シーンは以下の順序でポストエフェクトが適用されます。各エフェクトは独立して ON/OFF 可能です。

```
Scene → HDR RT (高ダイナミックレンジ レンダーターゲット)
     → [RTGI (DXR グローバルイルミネーション)]
     → [SSGI (スクリーン空間 GI)]
     → [SSAO (環境遮蔽)]
     → [ContactShadows (コンタクトシャドウ)]
     → [RT/SSR (DXR 反射 / スクリーン空間反射, 排他)]
     → [VolumetricLight (光の筋)]
     → [Bloom (光のにじみ)]
     → [DoF (被写界深度)]
     → [MotionBlur (動きボケ)]
     → [Outline (輪郭線)]
     → [TAA (テンポラルAA)]
     → [ColorGrading (色調調整)]
     → [AutoExposure (自動露出)]
     → [Tonemap (HDR→LDR変換)]
     → [LensFlare (レンズフレア)]
     → [ChromaticAberration (色収差)]
     → [FilmGrain (フィルムグレイン)]
     → [FXAA (高速AA)]
     → [Vignette (周辺減光)]
     → [DynamicResolution (動的解像度)]
     → Backbuffer (画面表示)
```

JSON 設定ファイル (`post_effects.json`) でパラメータを永続化できます（F12 で保存）。

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
  flex-direction: column;   /* 子要素を縦方向に並べる */
  align-items: center;      /* 横方向の中央揃え */
  gap: 12;                  /* 子要素間のスペース */
  padding: 40;              /* 内側余白 */
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
| [stb_vorbis](https://github.com/nothings/stb) | OGG デコード | Public Domain |
| [cgltf](https://github.com/jkuhlmann/cgltf) | glTF パーサー | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON パーサー | MIT |
| [LZ4](https://github.com/lz4/lz4) | 高速圧縮 | BSD-2 |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 3D 物理エンジン | MIT |
| [Lua 5.4](https://www.lua.org/) | スクリプト言語 | MIT |
| [sol2](https://github.com/ThePhD/sol2) | Lua C++ バインディング | MIT |
| [Google Test](https://github.com/google/googletest) | ユニットテスト | BSD-3 |
| [ufbx](https://github.com/bqqbarbhg/ufbx) | FBX パーサー (gxconv) | MIT |
| [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | OBJ パーサー (gxconv) | MIT |
| [Dear ImGui](https://github.com/ocornut/imgui) | GXModelViewer UI (Docking) | MIT |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | 3D ギズモ | MIT |
| [ImPlot](https://github.com/epezent/implot) | グラフ描画 | MIT |
| [ImNodes](https://github.com/Nelarius/imnodes) | ノードエディタ | MIT |
| [FBX SDK](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk) | FBX/OBJ 読み込み (オプション) | Autodesk FBX SDK License |

Lua 5.4 と sol2 は CMake FetchContent により自動ダウンロードされます。

## ドキュメント

- [API リファレンスサイト](https://mynameisgaku.github.io/GXLib/) — GitHub Pages で公開
- [用語集 (Glossary)](docs/GXLib_ProjectDoc.md) — プロジェクトドキュメント

### Doxygen による API ドキュメント生成

```bash
cmake --build build --target docs
# docs/ 配下に HTML が生成されます
```

## トラブルシューティング

### よくある問題

| 症状 | 原因 | 対処 |
|------|------|------|
| シェーダーファイルが見つからない | VS_DEBUGGER_WORKING_DIRECTORY が exe の場所と一致していない | CMakeLists.txt で `VS_DEBUGGER_WORKING_DIRECTORY` が `$<TARGET_FILE_DIR:...>` に設定されていることを確認してください |
| `cmake -B build` で Jolt Physics のダウンロードに失敗する | ネットワーク接続またはプロキシの問題 | `cmake -B build -S . --fresh` で再試行、またはプロキシ設定を確認してください |
| デバッグ実行時にクラッシュする | D3D12 デバッグレイヤーが GPU バリデーション違反を検出 | 出力ウィンドウの D3D12 エラーメッセージを確認してください。`GraphicsDevice::Initialize(false)` でデバッグレイヤーを無効化すると原因を切り分けられます |
| テクスチャが表示されない | テクスチャファイルのパスが間違っている、またはアセットがコピーされていない | `Shaders/` と `Assets/` がビルド出力先にコピーされているか確認してください |
| FBX モデルが読み込めない | FBX SDK がインストールされていない | `ThirdParty/FBXSDK/2020.3.9` に SDK を配置するか、glTF 形式を使用してください |
| フォントが表示されない | FontManager の初期化前にテキスト描画を呼んでいる | `FontManager::FlushAtlasUpdates()` をフレーム開始時に呼んでください |
| ビルドが遅い | プリコンパイルドヘッダーが無効になっている | モジュール別 PCH (pch_common.h / pch_graphics.h / pch_audio.h) が有効か確認してください |
| 新しいファイルがビルドに含まれない | GLOB_RECURSE による自動収集 | `cmake -B build -S .` で CMake を再生成してください |

### テスト実行

```bash
cmake -B build -S .
cmake --build build --config Debug
ctest --test-dir build --build-config Debug --output-on-failure
```

## ライセンス

本プロジェクトのライセンスについてはリポジトリオーナーにお問い合わせください。
