# Phase 10c Summary: Sample Projects

## Overview
GXLibエンジンの各機能を実証する5つのサンプルプロジェクトを実装。
2Dシューティング、2Dプラットフォーマー、3Dウォークスルー、GUIメニュー、ポストエフェクトショーケースの
各サンプルが独立したWIN32アプリケーションとして動作し、エンジンの主要機能を網羅的にデモンストレーションする。

## Completion Condition
> 5つのサンプルプロジェクトが動作 → **達成**

---

## Sample Projects

| Sample | Description | Key Features Demonstrated |
|--------|-------------|---------------------------|
| Shooting2D | 2Dシューティングゲーム (DxLib互換API) | Compat API, スプライト描画, キーボード入力, 円vs矩形衝突判定, スコアHUD, ゲームオーバー/リスタート |
| Platformer2D | 2Dプラットフォーマー (DxLib互換API) | Compat API, タイルベースレベル, 重力物理, AABB衝突応答, カメラ追従(スムース補間), コイン収集 |
| Walkthrough3D | 3Dシーンウォークスルー (ネイティブAPI) | PBRレンダリング, Camera3D(WASD+マウス), Skybox, 多光源(Directional/Point/Spot), CSM+Spot+Point影, PostEffect(Bloom/SSAO/FXAA), RenderLayer合成, Fog |
| GUIMenuDemo | GUIメニューシステムデモ | UIContext, UIRenderer, StyleSheet(インラインCSS), Widget(Button/Slider/CheckBox/RadioButton/Dialog/Spacer), 画面遷移(Main/Settings), WM_CHARルーティング |
| PostEffectShowcase | 全ポストエフェクト展示 | 10種ポストエフェクト個別トグル(1-0キー), Tonemap切替(Reinhard/ACES/Uncharted2), 露出調整(+/-), 自動回転カメラ, 効果状態HUDパネル, PBR 3Dシーン |

## New Files

| File | Description |
|------|-------------|
| `Samples/Shooting2D/main.cpp` | 2Dシューティング — Compat API使用、敵スポーン・弾発射・衝突判定・スコア管理 |
| `Samples/Shooting2D/CMakeLists.txt` | `gxlib_add_sample(Shooting2D)` |
| `Samples/Platformer2D/main.cpp` | 2Dプラットフォーマー — Compat API使用、レベル構築・重力・プラットフォーム衝突・カメラ追従 |
| `Samples/Platformer2D/CMakeLists.txt` | `gxlib_add_sample(Platformer2D)` |
| `Samples/Walkthrough3D/main.cpp` | 3Dウォークスルー — ネイティブAPI使用、PBRシーン・フルシャドウパス・PostEffect・Layer合成 |
| `Samples/Walkthrough3D/CMakeLists.txt` | `gxlib_add_sample(Walkthrough3D)` |
| `Samples/GUIMenuDemo/main.cpp` | GUIメニュー — ネイティブAPI使用、CSS styled Widget tree・画面遷移・Dialogポップアップ |
| `Samples/GUIMenuDemo/CMakeLists.txt` | `gxlib_add_sample(GUIMenuDemo)` |
| `Samples/PostEffectShowcase/main.cpp` | ポストエフェクト展示 — ネイティブAPI使用、10種エフェクトトグル・Tonemap切替・HUDパネル |
| `Samples/PostEffectShowcase/CMakeLists.txt` | `gxlib_add_sample(PostEffectShowcase)` |

## Modified Files

| File | Changes |
|------|---------|
| `CMakeLists.txt` | `gxlib_add_sample` マクロ定義 + 5つの `add_subdirectory(Samples/*)` 追加 |

## Architecture

### gxlib_add_sample CMake Macro
```cmake
macro(gxlib_add_sample SAMPLE_NAME)
    add_executable(${SAMPLE_NAME} WIN32 main.cpp)
    target_link_libraries(${SAMPLE_NAME} PRIVATE GXLib)
    target_include_directories(${SAMPLE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/GXLib)
    # POST_BUILD: Shaders/ と Assets/ を出力ディレクトリにコピー
    # VS_DEBUGGER_WORKING_DIRECTORY: exe と同じディレクトリ
endmacro()
```

各サンプルの `CMakeLists.txt` は1行のみ:
```cmake
gxlib_add_sample(SampleName)
```

### 2つのAPI層
```
Compat API (DxLib互換)              Native API (GXLib直接使用)
┌─────────────────────┐           ┌──────────────────────────────┐
│ Shooting2D          │           │ Walkthrough3D                │
│ Platformer2D        │           │ GUIMenuDemo                  │
│                     │           │ PostEffectShowcase            │
│ #include GXLib.h    │           │ #include pch.h + 個別ヘッダ   │
│ GX_Init / GX_End    │           │ Application / GraphicsDevice  │
│ ProcessMessage      │           │ CommandQueue / CommandList     │
│ ClearDrawScreen     │           │ SwapChain / Renderer3D ...    │
│ ScreenFlip          │           │                              │
└─────────────────────┘           └──────────────────────────────┘
```

---

## Per-Sample Details

### Shooting2D
- **API**: Compat API (`#include "Compat/GXLib.h"`)
- **ゲームループ**: `ProcessMessage()` + `ClearDrawScreen()` / `ScreenFlip()`
- **プレイヤー**: 三角形描画、左右矢印キーで移動、スペースで弾発射(クールダウン0.12秒)
- **敵**: 円形、上からランダムスポーン(間隔は時間経過で短縮)、画面下到達でゲームオーバー
- **衝突判定**: `HitCircleRect()` — 円vs矩形、弾vs敵・プレイヤーvs敵の2種
- **HUD**: スコア表示、ゲームオーバー画面(ENTER でリスタート)
- **描画**: `DrawTriangle`, `DrawBox`, `DrawCircle`, `DrawFormatStringToHandle`
- **乱数**: xorshift による自前実装

### Platformer2D
- **API**: Compat API (`#include "Compat/GXLib.h"`)
- **レベル構築**: `BuildLevel()` で地面+10個の浮遊プラットフォーム+10個のコインを配置
- **物理**: 重力800px/s^2、ジャンプ速度-450px/s、地面接地判定
- **衝突応答**: AABB overlap テスト、最小侵入軸で押し戻し(X/Y 分離)
- **カメラ**: ワールド座標にオフセット適用、スムース追従 (`lerp * 5.0 * dt`)
- **コイン**: 距離判定ピックアップ、全収集で"ALL COINS COLLECTED!"表示
- **落下リセット**: Y>800 でスタート位置に戻る、R キーでレベルリスタート

### Walkthrough3D
- **API**: ネイティブ GXLib API (全サブシステム直接使用)
- **シーン構成**: Floor(30x30 plane) + 3 Cubes(赤/緑/青) + 3 Spheres(金属/セラミック/プラスチック) + 4 Pillars(円柱)
- **PBR マテリアル**: albedo, metallic, roughness パラメータ設定
- **ライティング**: Directional + Point + Spot の3光源、Fog(Linear)
- **シャドウ**: フル影パス — 4 CSM cascades + Spot shadow + 6-face Point shadow
- **PostEffect**: Bloom + SSAO + FXAA + ACES Tonemap 有効
- **Layer**: Scene layer (PostFX) + UI layer (FPS/座標/操作説明)
- **カメラ操作**: WASD移動、QE上下、右クリックマウス回転、Shift加速
- **リサイズ対応**: `OnResize` コールバックで全サブシステム更新

### GUIMenuDemo
- **API**: ネイティブ GXLib API + GUI サブシステム
- **画面遷移**: Main → Settings → Main (ESC / Back ボタン)
- **メイン画面**: Start Game / Settings / About / Exit の4ボタン
- **設定画面**: Volume スライダー、Brightness スライダー、Fullscreen チェックボックス、VSync チェックボックス、Difficulty ラジオボタン(Easy/Normal/Hard)
- **Dialog**: About ダイアログ (タイトル + 説明 + OK ボタン)
- **スタイリング**: インライン CSS (k_StyleCSS) — `.menuButton`, `.settingsPanel`, `.backButton` 等のクラスセレクタ + `:hover` / `:pressed` 擬似クラス
- **描画**: PrimitiveBatch でグラデーション背景 + UIRenderer で GUI + SpriteBatch で FPS
- **WM_CHAR**: `AddMessageCallback` で UIContext にルーティング

### PostEffectShowcase
- **API**: ネイティブ GXLib API (Walkthrough3D ベース)
- **10種ポストエフェクト**: Bloom, SSAO, FXAA, Vignette, ColorGrading, DoF, MotionBlur, SSR, Outline, TAA
- **トグル操作**: 1-0 数字キーで各エフェクトON/OFF
- **Tonemap切替**: M キーで Reinhard → ACES → Uncharted2 サイクル
- **露出調整**: +/- キーで exposure パラメータ増減
- **自動回転**: Tab キーでカメラ自動回転トグル
- **HUD パネル**: 右上に半透明パネル、全エフェクトの ON/OFF 状態表示、選択中エフェクトをハイライト
- **アニメーション**: Cube[0] が時間経過で回転
- **EffectInfo構造体**: 関数ポインタテーブル (name, isEnabled, toggle) でエフェクト管理を抽象化

## Key Design Decisions
- **gxlib_add_sample マクロ**: WIN32 exe + GXLib リンク + Shaders/Assets コピー + VS Working Dir を1行で設定。各サンプルの CMakeLists.txt は1行のみで記述可能
- **2つのAPI層の実証**: Shooting2D / Platformer2D は DxLib互換 Compat API、Walkthrough3D / GUIMenuDemo / PostEffectShowcase はネイティブ API を使用し、両方のアプローチを実証
- **段階的な複雑さ**: 2D描画のみ → 2D+物理 → 3D+PostFX → GUI → 全PostFX展示 と難易度が段階的に上昇
- **自己完結型**: 各サンプルは main.cpp 1ファイルに収まり、外部アセット不要(プロシージャル生成メッシュ、プリミティブ描画)
- **共通シーン再利用**: Walkthrough3D と PostEffectShowcase は同一の3Dシーン構成(Floor+Cubes+Spheres+Pillars)を共有し、PostEffect の効果を比較しやすく設計

## Verification
- Build: 5つのサンプル全てビルド・リンク成功
- Shooting2D: Compat API による2D描画・入力・衝突動作確認
- Platformer2D: Compat API による重力・プラットフォーム衝突・カメラ追従動作確認
- Walkthrough3D: PBR・Shadow・PostEffect・Layer合成動作確認
- GUIMenuDemo: CSS styled Widget・画面遷移・Dialog動作確認
- PostEffectShowcase: 10種エフェクト個別トグル・HUD表示動作確認
