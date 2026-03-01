# GXLib Project Documentation

> GXLib — DirectX 12 2D/3D ゲームエンジン 統合ドキュメント

## 目次

- [Part I: プロジェクト理念・設計](#part-i-プロジェクト理念設計)
  - [1. GXLib Project Directive](#1-gxlib-project-directive)
  - [2. GXLib Framework Plan](#2-gxlib-framework-plan)
- [Part II: 用語集 (Glossary)](#part-ii-用語集-glossary)
- [Part III: チュートリアル](#part-iii-チュートリアル)
  - [Tutorial 00: Prerequisites](#tutorial-00-prerequisites)
  - [Tutorial 01: Getting Started](#tutorial-01-getting-started)
  - [Tutorial 02: 2D Drawing](#tutorial-02-2d-drawing)
  - [Tutorial 03: 2D Game](#tutorial-03-2d-game)
  - [Tutorial 04: 3D Rendering](#tutorial-04-3d-rendering)
  - [Tutorial 05: GUI](#tutorial-05-gui)
  - [Tutorial 06: GXEasy 2D Game](#tutorial-06-gxeasy-2d-game)
  - [Tutorial 07: 3D Scene](#tutorial-07-3d-scene)
  - [Tutorial 08: Asset Pipeline](#tutorial-08-asset-pipeline)
- [Part IV: DxLib マイグレーションガイド](#part-iv-dxlib-マイグレーションガイド)
- [Part V: API リファレンス原稿](#part-v-api-リファレンス原稿)
  - [Audio API](#audio-api)
  - [Graphics API](#graphics-api)
  - [GXEasy API](#gxeasy-api)
  - [Input API](#input-api)
  - [Math API](#math-api)
  - [Scene API](#scene-api)
- [Part VI: 開発フェーズ指令書](#part-vi-開発フェーズ指令書)
  - [Phase 31-34 Directive](#phase-31-34-directive)
  - [Phase 36-40 Directive](#phase-36-40-directive)
  - [Phase 37 Directive](#phase-37-directive)
  - [Phase 41-45 Directive](#phase-41-45-directive)
- [Part VII: フェーズ完了ログ](#part-vii-フェーズ完了ログ)
  - [Phase 0 Summary](#phase-0-summary)
  - [Phase 1 Summary](#phase-1-summary)
  - [Phase 2 Summary](#phase-2-summary)
  - [Phase 3 Summary](#phase-3-summary)
  - [Phase 4a Summary](#phase-4a-summary)
  - [Phase 4b Summary](#phase-4b-summary)
  - [Phase 4cde Summary](#phase-4cde-summary)
  - [Phase 4fg Summary](#phase-4fg-summary)
  - [Phase 4h Summary](#phase-4h-summary)
  - [Phase 4i Summary](#phase-4i-summary)
  - [Phase 4j Summary](#phase-4j-summary)
  - [Phase 4k Summary](#phase-4k-summary)
  - [Phase 4l Summary](#phase-4l-summary)
  - [Phase 4mno Summary](#phase-4mno-summary)
  - [Phase 5 Summary](#phase-5-summary)
  - [Phase 6ab Summary](#phase-6ab-summary)
  - [Phase 6c Summary](#phase-6c-summary)
  - [Phase 7 Summary](#phase-7-summary)
  - [Phase 8 Summary](#phase-8-summary)
  - [Phase 9 Summary](#phase-9-summary)
  - [Phase 10a Summary](#phase-10a-summary)
  - [Phase 10c Summary](#phase-10c-summary)
  - [Phase 10 Remaining Work](#phase-10-remaining-work)
- [Part VIII: レポート・分析](#part-viii-レポート分析)
  - [Bug Report](#bug-report)
  - [Project Analysis (2026-02-11)](#project-analysis-2026-02-11)
  - [Project Analysis (2026-02-21)](#project-analysis-2026-02-21)
  - [Documentation Audit](#documentation-audit)
  - [Phase 40 Bug Fix Session](#phase-40-bug-fix-session)


---

# Part I: プロジェクト理念・設計

## 1. GXLib Project Directive

> **この文書は、本プロジェクトに携わる AI（Claude）が作業の方向性を見失わないための最上位ドキュメントである。**
> **実装判断に迷った時、スコープが曖昧な時、優先度を決める時、必ずこの文書の「第1章 なぜ作るのか」に立ち返ること。**

---

## ■ 第1章：なぜ作るのか — 本プロジェクトの存在意義

### 1.1 解決すべき問題（これが全ての出発点）

DXライブラリは日本のゲーム開発教育・ホビーシーンで広く使われているが、**以下の根本的な欠陥**を抱えている：

1. **DirectX 11 止まり** — DirectX 12 に非対応で、モダン GPU の性能を活かせない
2. **見た目がショボい・安っぽい** — PBR もHDR も標準では使えず、ビジュアル品質に天井がある
3. **ポストエフェクトが全て自前実装** — Bloom 1つ入れるだけで大量のコードが必要で、クソめんどくさい
4. **描画レイヤー管理がない** — 描画順を制御する標準的な仕組みがなく、煩雑なコードを強いられる
5. **GUIシステムが存在しない** — メニューやHUDを作る標準手段がない

**これらの問題を「根本から」解決する。パッチではなく、ゼロから作り直す。**

### 1.2 本プロジェクトの正体

**DXライブラリの完全上位互換フレームワークを、DirectX 12 ベースでゼロから構築する。**

「上位互換」とは：
- DXライブラリにある機能は **全て** 備える（一つ残らず）
- DXライブラリに **ない** 機能を大量に追加する（後述）
- DXライブラリの使いやすさは維持しつつ、表現力を桁違いに引き上げる

### 1.3 絶対に達成すべきゴール（完成条件）

**以下の5条件を「全て」満たした時のみ、このプロジェクトは完成とする：**

| # | 完成条件 | 妥協不可の理由 |
|---|---------|--------------|
| G1 | DXライブラリの全APIカテゴリを網羅 | 「上位互換」を名乗る以上、抜けがあってはならない |
| G2 | ポストエフェクトパイプラインが標準搭載され、設定ファイルでON/OFF可能 | これがないと「ショボい見た目」問題が解決しない |
| G3 | 描画レイヤーシステムが動作し、レイヤー単位でブレンド・エフェクトが可能 | DXLibの最大の不満の一つを解消するため |
| G4 | XMLベースのGUIシステムでメニュー・HUD・エディタUIが構築可能 | GUIなしのフレームワークは片手落ち |
| G5 | サンプルプロジェクト群（2Dゲーム、3Dゲーム、GUIデモ）が動作する | 動くサンプルがなければ使い物にならない |

### 1.4 作業中に常に意識すべき設計理念

実装のあらゆる判断は、以下の理念に照らして行うこと：

1. **DXライブラリ互換** — 既存DXLibユーザーが違和感なく移行できるAPI設計
2. **モダンレンダリング** — PBR、HDR、ポストエフェクトは「標準」。オプションではなくデフォルト
3. **レイヤードアーキテクチャ** — 低レベルAPI（D3D12直接操作）と高レベルAPI（DXLib風の簡単操作）の両方を提供
4. **宣言的GUI** — XMLとCSS-likeスタイルシートでUIを記述。C++コードでレイアウトを書かせない
5. **拡張性** — プラグインやカスタムシェーダーで自由に拡張できる構造

---

## ■ 第2章：何を作るのか — 技術仕様

### 2.1 技術スタック

| 項目 | 選定 | 選定理由 |
|------|------|---------|
| グラフィックスAPI | DirectX 12 (D3D12) | DXLibがDX11止まりという問題の直接的解答 |
| シェーダー言語 | HLSL (Shader Model 6.x) | DX12ネイティブ、最新機能が使える |
| シェーダーコンパイラ | DXC (DirectX Shader Compiler) | SM6.x対応の公式コンパイラ |
| ビルドシステム | CMake + vcpkg | 業界標準、依存管理が楽 |
| 言語 | C++20 | コンセプト、ranges、コルーチン等を活用 |
| オーディオ | XAudio2 / WASAPI | Windows標準、低遅延 |
| 入力 | XInput + Raw Input + DirectInput(後方互換) | 全入力デバイスカバー |
| 物理 | 内製2D + Jolt Physics(3D、オプション) | 軽量2D物理は自前、3Dは実績あるライブラリ |
| GUI記述 | XML + CSS-like スタイルシート | Web開発者にも馴染みやすい宣言的UI |
| スクリプト | Lua（オプション） | 軽量で組み込みやすい |
| テスト | Google Test + GPUベース回帰テスト | 品質保証 |

### 2.2 命名規則（プロジェクト全体で統一）

| 対象 | 規則 | 例 |
|------|------|---|
| 名前空間 | `GX::` | `GX::Graphics`, `GX::Audio` |
| クラス | PascalCase | `SpriteBatch`, `RenderLayer` |
| メソッド | PascalCase | `DrawSprite()`, `LoadTexture()` |
| メンバ変数 | `m_camelCase` | `m_width`, `m_renderTarget` |
| 定数 | `k_PascalCase` | `k_MaxLayers`, `k_DefaultFOV` |
| ファイル名 | PascalCase.h/.cpp | `SpriteBatch.h`, `SpriteBatch.cpp` |

---

## ■ 第3章：DXライブラリ全機能マッピング — 漏れなく網羅するための一覧

> **この表がチェックリストである。全行に対応実装が完了して初めてゴールG1を達成する。**

### 3.1 システム系

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| ウィンドウ管理 | `DxLib_Init`, `SetGraphMode`, `ChangeWindowMode` | Win32ウィンドウ + DXGI SwapChain |
| メッセージ処理 | `ProcessMessage` | 内部メッセージループ + イベントコールバック |
| フルスクリーン切替 | `ChangeWindowMode` | DXGIフルスクリーン + ボーダーレス |
| 解像度変更 | `SetGraphMode` | 動的スワップチェーンリサイズ |
| DPI対応 | （なし） | **【新規】** Per-Monitor DPI V2 |
| マルチウィンドウ | （なし） | **【新規】** マルチウィンドウレンダリング |
| タイマー | `GetNowCount`, `GetNowHiPerformanceCount` | `QueryPerformanceCounter`ベース |
| FPS制御 | `SetWaitVSyncFlag` | VSync + フレームレートリミッター |
| ログ出力 | `printfDx`, `ErrorLogAdd` | 構造化ログ（レベル別、ファイル出力） |

### 3.2 描画系 — 2D

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| 画像読込・描画 | `LoadGraph`, `DrawGraph` | テクスチャ管理 + スプライトバッチ |
| 画像分割読込 | `LoadDivGraph` | スプライトシート + アトラスパッカー |
| 画像回転拡大 | `DrawRotaGraph`, `DrawExtendGraph` | Transform2D指定描画 |
| 画像ブレンド | `SetDrawBlendMode` | ブレンドステート（加算、乗算、α等） |
| 画像輝度設定 | `SetDrawBright` | シェーダーパラメータ / カラーマスク |
| 図形描画 | `DrawLine`, `DrawBox`, `DrawCircle`, `DrawTriangle` | プリミティブバッチレンダラー |
| アンチエイリアス図形 | `DrawLineAA`, `DrawCircleAA` | MSAA + シェーダーベースAA |
| ピクセル操作 | `GetPixelSoftImage`, `DrawPixelSoftImage` | CPU側ソフトイメージ + Readback |
| グラフィックフィルタ | `GraphFilter`（モノ、ガウス、明度等） | **ポストエフェクトパイプラインで代替** |
| 描画先変更 | `SetDrawScreen`, `MakeScreen` | RenderTargetシステム |
| **描画レイヤー** | （なし） | **【新規・重要】** レイヤースタック（Z-order、ブレンド、エフェクト） |
| **スプライトアニメーション** | （なし） | **【新規】** アニメーションコントローラー |

### 3.3 描画系 — 3D

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| 3Dモデル読込 | `MV1LoadModel` | glTF / FBX / OBJローダー |
| モデル描画 | `MV1DrawModel` | メッシュレンダラー + インスタンシング |
| モデルアニメーション | `MV1AttachAnim`, `MV1SetAttachAnimTime` | スケルタルアニメーション + ブレンドツリー |
| モデル衝突判定 | `MV1CollCheck_Sphere`, `MV1CollCheck_Line` | メッシュコリジョン |
| カメラ制御 | `SetCameraPositionAndTarget_UpVecY` | Cameraコンポーネント（Perspective / Ortho） |
| ライティング | `SetLightDirection`, `SetLightDifColor` | **【新規】PBRライティング**（Directional, Point, Spot, Area） |
| マテリアル | `MV1SetMaterialDifColor` | **【新規】PBRマテリアル**（Albedo, Normal, Metallic, Roughness, AO） |
| シャドウマップ | `SetShadowMapDrawArea` | **【新規】CSM** + PCF/VSM |
| フォグ | `SetFogEnable`, `SetFogColor` | ボリューメトリックフォグ / 距離フォグ |
| 3D図形 | `DrawSphere3D`, `DrawCone3D` | プリミティブメッシュ生成 |
| Zバッファ | `SetUseZBuffer3D` | 深度ステート制御 |
| **環境マップ** | （なし） | **【新規】** キューブマップ / IBL反射 |
| **PBR** | （なし） | **【新規】** Cook-Torrance BRDF |
| **スカイボックス** | （なし） | **【新規】** HDRスカイボックス / プロシージャル空 |
| **地形** | （なし） | **【新規】** ハイトマップ地形 + LOD |

### 3.4 ポストエフェクト（ゴールG2に直結 — 全て新規）

| エフェクト | 実装方式 |
|-----------|---------|
| Bloom | Dual Kawase / ガウシアン ダウンサンプリング |
| Tonemapping | ACES / Reinhard / Uncharted 2 / AgX |
| HDR | 浮動小数テクスチャ (R16G16B16A16_FLOAT) |
| SSAO | GTAO / HBAO |
| 被写界深度 (DoF) | Bokeh DoF（六角形/円形） |
| モーションブラー | Per-Object / Cameraベース |
| カラーグレーディング | 3D LUT |
| FXAA / TAA | FXAA 3.11 / TAA |
| ビネット | シェーダーベース |
| 色収差 | シェーダーベース |
| スクリーンスペースリフレクション | SSR (Hi-Z trace) |
| ボリューメトリックライト | レイマーチング |
| 輪郭線（トゥーン） | Sobel / 法線・深度エッジ検出 |

### 3.5 シェーダー

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| 頂点シェーダー | `LoadVertexShader` | HLSL SM6.xコンパイル + ホットリロード |
| ピクセルシェーダー | `LoadPixelShader` | 同上 |
| 定数バッファ | `SetShaderConstantReg` | CBV自動バインド |
| シェーダー描画 | `SetUseVertexShader` | マテリアルシステム統合 |
| **コンピュートシェーダー** | （なし） | **【新規】** CSパイプライン |
| **シェーダーホットリロード** | （なし） | **【新規】** ファイル監視による即時反映 |
| **シェーダーバリアント** | （なし） | **【新規】** プリプロセッサ定義による分岐管理 |

### 3.6 サウンド

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| サウンド読込・再生 | `LoadSoundMem`, `PlaySoundMem` | XAudio2ベース |
| BGMストリーミング | `PlayMusic` | ストリーミングデコード |
| 音量制御 | `ChangeVolumeSoundMem` | デシベルベース音量制御 |
| パン制御 | `ChangePanSoundMem` | ステレオパン |
| 再生速度 | `SetFrequencySoundMem` | ピッチシフト |
| 3Dサウンド | `Set3DPositionSoundMem` | **3D空間音響**（HRTFオプション） |
| 対応フォーマット | WAV, OGG, MP3 | WAV, OGG, MP3, FLAC, OPUS |
| **サウンドミキサー** | （なし） | **【新規】** バス・エフェクトチェーン |
| **リアルタイムエフェクト** | （なし） | **【新規】** リバーブ、EQ、コンプレッサー |

### 3.7 入力

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| キーボード | `CheckHitKey`, `GetHitKeyStateAll` | Raw Input |
| マウス | `GetMousePoint`, `GetMouseInput` | Raw Input + カーソル管理 |
| マウスホイール | `GetMouseWheelRotVol` | WM_MOUSEWHEEL |
| ジョイパッド | `GetJoypadInputState`, `GetJoypadAnalogInput` | XInput + DirectInputフォールバック |
| ジョイパッド振動 | `StartJoypadVibration` | XInputバイブレーション |
| タッチ | `GetTouchInputNum` | WM_TOUCH / WM_POINTER |
| **入力マッピング** | （なし） | **【新規】** アクションマップ（設定ファイルで再マップ可能） |
| **入力バッファリング** | （なし） | **【新規】** 格闘ゲーム向けコマンドバッファ |
| **デッドゾーン設定** | （なし） | **【新規】** アナログスティック デッドゾーン |

### 3.8 文字描画

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| 文字列描画 | `DrawString`, `DrawFormatString` | **SDFフォントレンダリング** |
| フォント作成 | `CreateFontToHandle` | DirectWrite + フォントアトラス |
| 文字列幅取得 | `GetDrawStringWidth` | テキスト計測API |
| 文字コード | `SetUseCharCodeFormat` | UTF-8 / UTF-16 |
| **リッチテキスト** | （なし） | **【新規】** カラー、サイズ混在テキスト |
| **テキストレイアウト** | （なし） | **【新規】** ワードラップ、行間、カーニング |
| **ビットマップフォント** | （なし） | **【新規】** BMFont形式対応 |

### 3.9 ネットワーク

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| TCP接続 | `ConnectNetWork`, `NetWorkSend` | Winsock2非同期TCP |
| UDP | `MakeUDPSocket`, `NetWorkSendUDP` | Winsock2 UDP |
| HTTP | `GetHTTP`, `GetHTTPRes` | WinHTTP / libcurl |
| **WebSocket** | （なし） | **【新規】** WebSocketクライアント |
| **非同期IO** | （なし） | **【新規】** IOCPベース非同期 |

### 3.10 ファイル・アーカイブ

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| ファイル読み書き | `FileRead_open`, `FileRead_gets` | std::filesystem + 非同期IO |
| DXAアーカイブ | `SetDXArchiveExtension` | **カスタムアーカイブ**（AES暗号化対応） |
| メモリ上読込 | `CreateGraphFromMem` | メモリストリーム対応 |
| **アセットホットリロード** | （なし） | **【新規】** ファイル監視による即時反映 |
| **非同期アセット読込** | （なし） | **【新規】** バックグラウンドロード + プログレス |

### 3.11 算術・衝突判定

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| ベクトル演算 | `VGet`, `VAdd`, `VCross` | SIMD最適化数学ライブラリ（DirectXMathラップ） |
| 行列演算 | `MGetIdent`, `MMult` | Matrix4x4クラス |
| 衝突判定 | `HitCheck_Sphere_Sphere`, `HitCheck_Line_Triangle` | 衝突判定ユーティリティ |
| **2D物理** | （なし） | **【新規】** 簡易2D物理エンジン |
| **空間分割** | （なし） | **【新規】** Quadtree / Octree / BVH |

### 3.12 動画・マスク

| DXLib機能 | DXLib関数例 | GXLib対応方針 |
|-----------|-----------|---------------|
| 動画再生 | `PlayMovie`, `OpenMovieToGraph` | Media Foundationデコード→テクスチャ |
| 動画フレーム取得 | `SeekMovieToGraph`, `TellMovieToGraph` | フレーム単位シーク |
| マスク描画 | `CreateMaskScreen`, `DrawMask` | ステンシルバッファ + マスクテクスチャ |
| マスク図形 | `DrawFillMask`, `DrawCircleMask` | ステンシル描画 |

---

## ■ 第4章：新規システム詳細設計

> **この章は、DXライブラリに「ない」ものを定義する。ここがこのフレームワークの差別化ポイントであり、存在意義である。「第1章の問題」を解決するのがこの章の設計。**

### 4.1 描画レイヤーシステム（ゴールG3に直結）

**なぜ必要か:** DXライブラリには描画順を管理する仕組みがなく、`DrawGraph`の呼び出し順でしか制御できない。これはゲームが複雑になると地獄になる。

```
┌─────────────────────────────────┐
│         最終合成出力              │
├─────────────────────────────────┤
│  Layer: UI (Z: 1000)            │ ← ポストエフェクト適用外
│  Layer: HUD (Z: 900)            │ ← ポストエフェクト適用外
│  Layer: PostFX                  │ ← ポストエフェクトパイプライン
│  Layer: Particles (Z: 500)      │
│  Layer: Characters (Z: 400)     │
│  Layer: World (Z: 100)          │ ← PBRレンダリング
│  Layer: Background (Z: 0)       │
│  Layer: Skybox (Z: -1000)       │
└─────────────────────────────────┘
```

**レイヤーが持つ機能:**
- 個別のRenderTargetを保持
- レイヤー単位のブレンドモード設定（通常、加算、乗算、スクリーン...）
- レイヤー単位の不透明度制御
- レイヤー単位のポストエフェクト適用可否
- レイヤーのカメラ独立設定（パララックススクロール等）
- レイヤーのソートモード設定（Z-sort, Y-sort, 挿入順）

### 4.2 ポストエフェクトパイプライン（ゴールG2に直結）

**なぜ必要か:** DXライブラリで見た目がショボくなる最大の原因。Bloomすら自前で書くのはクソめんどくさい。これを設定ファイル1つで制御できるようにする。

```
Scene → [HDR RenderTarget]
         ↓
    ┌─ SSAO ──────────┐
    ├─ SSR ───────────┤
    ├─ MotionBlur ────┤
    ├─ DoF ───────────┤   ← 設定ファイルで ON/OFF、順序変更可能
    ├─ Bloom ─────────┤
    ├─ ColorGrading ──┤
    ├─ Tonemapping ───┤
    ├─ FXAA/TAA ──────┤
    ├─ Vignette ──────┤
    └─ ChromaticAberr ┘
         ↓
    [LDR BackBuffer] → Present
```

**設定ファイル例 (post_effects.json):**
```json
{
  "postEffects": {
    "bloom": { "enabled": true, "threshold": 1.0, "intensity": 0.8, "radius": 4 },
    "tonemapping": { "enabled": true, "operator": "ACES", "exposure": 1.2 },
    "ssao": { "enabled": true, "radius": 0.5, "bias": 0.025, "samples": 32 },
    "dof": { "enabled": false },
    "fxaa": { "enabled": true }
  }
}
```

### 4.3 XML-GUIシステム（ゴールG4に直結）

**なぜ必要か:** DXライブラリにはGUIがないため、メニュー画面1つ作るにも大量のC++コードが必要。XMLで宣言的に書けるようにすることで、UIの構築速度を10倍にする。

#### XMLレイアウト定義
```xml
<!-- ui/main_menu.xml -->
<Window id="mainMenu" width="100%" height="100%">
  <Panel id="centerPanel" layout="vertical" align="center" valign="center"
         background="#00000080" padding="20" cornerRadius="8">

    <Text id="title" text="My Game" fontSize="48" fontFamily="GameFont"
          color="#FFFFFF" shadow="2,2,#000000" />

    <Spacer height="40" />

    <Button id="btnStart" width="300" height="60" text="ゲーム開始"
            class="menuButton" onClick="onStartGame" />
    <Button id="btnOption" width="300" height="60" text="オプション"
            class="menuButton" onClick="onOpenOptions" />
    <Button id="btnExit" width="300" height="60" text="終了"
            class="menuButton" onClick="onExit" />
  </Panel>
</Window>
```

#### CSS-likeスタイルシート
```css
/* ui/styles/menu.gss */
.menuButton {
    background: linear-gradient(#4A90D9, #357ABD);
    color: #FFFFFF;
    fontSize: 24;
    fontFamily: "GameFont";
    cornerRadius: 6;
    border: 2px solid #2A5A8E;
    transition: background 0.2s;
}
.menuButton:hover {
    background: linear-gradient(#5BA0E9, #4590DD);
    transform: scale(1.05);
}
.menuButton:pressed {
    background: linear-gradient(#2A5A8E, #1A4A7E);
    transform: scale(0.95);
}
```

#### C++バインド
```cpp
auto ui = GX::GUI::Load("ui/main_menu.xml", "ui/styles/menu.gss");
ui->Bind("onStartGame", [&]() { scene.TransitionTo<GameScene>(); });
ui->Bind("onOpenOptions", [&]() { ui->Show("optionsPanel"); });
ui->Bind("onExit", [&]() { GX::System::Exit(); });

// 動的操作
ui->Find<GX::GUI::Text>("title")->SetText("Updated Title");
ui->Find<GX::GUI::Button>("btnStart")->SetEnabled(false);
```

#### GUIウィジェット一覧（全て実装すること）

Window, Panel, Text, Button, Image, TextInput, Slider, CheckBox, RadioGroup/Radio, DropDown, ListView, ScrollView, ProgressBar, TabView, Dialog, Canvas, Spacer

---

## ■ 第5章：アーキテクチャ — ディレクトリ構成

> **この構成に従ってファイルを配置すること。勝手にディレクトリ構造を変えない。**

```
GXLib/
├── Core/                    # コアシステム
│   ├── Application.h/cpp    # アプリケーションライフサイクル
│   ├── Window.h/cpp         # ウィンドウ管理
│   ├── Timer.h/cpp          # 高精度タイマー
│   ├── Logger.h/cpp         # ログシステム
│   ├── Memory/              # メモリ管理（Pool, Stack, Linear）
│   ├── Event/               # イベントシステム（EventBus, Delegate）
│   └── Config/              # 設定管理（JSON/INI）
│
├── Graphics/                # 描画エンジン
│   ├── Device/              # D3D12デバイス管理
│   │   ├── GraphicsDevice.h # デバイス初期化・管理
│   │   ├── SwapChain.h      # スワップチェーン
│   │   ├── CommandQueue.h   # コマンドキュー
│   │   ├── CommandList.h    # コマンドリスト
│   │   ├── DescriptorHeap.h # デスクリプタヒープ管理
│   │   ├── Fence.h          # GPU同期
│   │   └── GPUResource.h    # リソース基底クラス
│   ├── Pipeline/            # パイプライン管理
│   │   ├── PipelineState.h  # PSO管理
│   │   ├── RootSignature.h  # ルートシグネチャ
│   │   ├── Shader.h         # シェーダーコンパイル・管理
│   │   └── ShaderLibrary.h  # シェーダーライブラリ + ホットリロード
│   ├── Resource/            # GPUリソース
│   │   ├── Texture.h        # テクスチャ
│   │   ├── Buffer.h         # 頂点/インデックス/定数バッファ
│   │   ├── RenderTarget.h   # レンダーターゲット
│   │   ├── DepthBuffer.h    # 深度バッファ
│   │   └── SamplerState.h   # サンプラー
│   ├── 2D/                  # 2D描画
│   │   ├── SpriteBatch.h    # スプライトバッチ
│   │   ├── SpriteSheet.h    # スプライトシート
│   │   ├── PrimitiveBatch.h # 図形バッチ
│   │   ├── TextRenderer.h   # テキスト描画(SDF)
│   │   ├── FontManager.h    # フォント管理
│   │   └── Animation2D.h    # スプライトアニメーション
│   ├── 3D/                  # 3D描画
│   │   ├── Mesh.h / Model.h / SkeletalAnim.h
│   │   ├── Camera.h / Light.h / Material.h
│   │   ├── ShadowMap.h / Skybox.h / Terrain.h
│   │   └── Primitive3D.h
│   ├── Layer/               # 描画レイヤー
│   │   ├── RenderLayer.h
│   │   ├── LayerStack.h
│   │   └── LayerCompositor.h
│   ├── PostFX/              # ポストエフェクト
│   │   ├── PostEffectPipeline.h
│   │   ├── PostEffect.h     # 基底クラス
│   │   ├── Bloom.h / Tonemapping.h / SSAO.h / DoF.h
│   │   ├── MotionBlur.h / ColorGrading.h / FXAA.h / TAA.h
│   │   ├── Vignette.h / ChromaticAberration.h
│   │   ├── SSR.h / VolumetricLight.h / OutlineEffect.h
│   │   └── （各.cppは対応する.hと同名）
│   └── Renderer.h           # レンダラー統合
│
├── Audio/                   # オーディオ
│   ├── AudioDevice.h / Sound.h / SoundPlayer.h
│   ├── MusicPlayer.h / AudioMixer.h
│   ├── Audio3D.h / AudioEffect.h
│   └── （各.cpp）
│
├── Input/                   # 入力
│   ├── Keyboard.h / Mouse.h / Gamepad.h / Touch.h
│   ├── InputManager.h / ActionMap.h
│   └── （各.cpp）
│
├── GUI/                     # GUIシステム
│   ├── GUIManager.h         # 統合管理
│   ├── GUIParser.h          # XMLパーサー
│   ├── GUIStyleSheet.h      # スタイルシートエンジン
│   ├── GUIRenderer.h        # GUI描画
│   ├── GUILayout.h          # レイアウトエンジン（Flexbox-like）
│   ├── GUIAnimation.h       # UIアニメーション
│   ├── GUIEvent.h           # UIイベント伝搬
│   └── Widgets/             # ウィジェット群
│       ├── Widget.h（基底） / Panel.h / Button.h / Text.h
│       ├── TextInput.h / Image.h / Slider.h / CheckBox.h
│       ├── RadioButton.h / DropDown.h / ListView.h
│       ├── ScrollView.h / ProgressBar.h / TabView.h
│       ├── Dialog.h / Canvas.h
│       └── （各.cpp）
│
├── IO/                      # ファイル・ネットワーク
│   ├── FileSystem.h / Archive.h / AsyncLoader.h
│   ├── Network/ (TCPSocket, UDPSocket, HTTPClient, WebSocket)
│   └── Serialization.h
│
├── Math/                    # 数学ライブラリ
│   ├── Vector2.h / Vector3.h / Vector4.h
│   ├── Matrix4x4.h / Quaternion.h / Color.h
│   ├── MathUtil.h / Random.h
│   └── Collision/ (AABB, Sphere, Ray, Frustum, CollisionUtil)
│
├── Physics/                 # 物理（オプション）
│   ├── Physics2D.h / PhysicsWorld.h
│   └── （各.cpp）
│
├── Movie/                   # 動画再生
│   └── MoviePlayer.h / MoviePlayer.cpp
│
├── Compat/                  # DXライブラリ互換レイヤー
│   ├── DxLibCompat.h        # DXLib関数名互換API
│   └── DxLibTypes.h         # 型変換
│
└── Utility/                 # ユーティリティ
    ├── StringUtil.h / PathUtil.h / Hash.h
    ├── ThreadPool.h / Profiler.h
    └── （各.cpp）
```

---

## ■ 第6章：実装フェーズ — 順序と各フェーズの完了条件

> **必ずフェーズ順に実装すること。前のフェーズが完了条件を満たさないまま次に進んではならない。**
> **各フェーズの「成果物」が動作確認できて初めて次に進む。**

---

### Phase 0: D3D12基盤構築

**完了条件:** 色付き三角形がウィンドウに描画される

- CMakeプロジェクト構成 + vcpkg依存管理
- Win32ウィンドウ作成・メッセージループ
- D3D12デバイス初期化
  - Factory, Device, CommandQueue作成
  - SwapChain作成（ダブルバッファリング）
  - DescriptorHeap管理クラス
  - Fence による CPU-GPU同期
  - CommandAllocator / CommandList管理
- パイプラインステート基盤
  - RootSignatureビルダー
  - PSOビルダー
  - DXCによるシェーダーコンパイル
- 三角形描画（Hello Triangle）
- フレームタイミング・FPS制御
- ログシステム

---

### Phase 1: 2D描画エンジン

**完了条件:** DXLibの2Dサンプルが全て再現可能

- テクスチャ管理（WIC/stb_image、キャッシュ、ミップマップ）
- スプライトバッチ（DrawGraph, DrawRotaGraph, DrawRectGraph, DrawExtendGraph, DrawModiGraph相当 + ブレンド + カラー）
- スプライトシート・アトラス（LoadDivGraph相当 + アニメーション再生）
- プリミティブバッチ（全図形 + AA対応）
- RenderTarget（MakeScreen / SetDrawScreen / GetDrawScreenGraph相当）
- ソフトイメージ（CPU側ピクセル操作 + GPU転送）
- カメラ2D

---

### Phase 2: テキスト・入力・サウンド

**完了条件:** 音が鳴り、操作可能な2Dゲームが作れる

- **テキスト:** DirectWrite → SDFフォントアトラス → SDFレンダリング、DrawString/DrawFormatString相当、フォントハンドル、文字幅取得、リッチテキスト、BMFont
- **入力:** キーボード(Raw Input)、マウス(Raw Input)、ゲームパッド(XInput+DInput)、アクションマッピング
- **サウンド:** XAudio2初期化、WAV/OGG/MP3デコード、再生制御、音量/パン/速度、BGMストリーミング、3Dサウンド、ミキサー、エフェクト

---

### Phase 3: 3D描画エンジン

**完了条件:** PBR litシーンが描画でき、モデルがアニメーションする

- モデルローダー（glTF 2.0 必須、FBX/OBJオプション）
- PBRレンダリング（Cook-Torrance BRDF、マテリアルシステム、ライティング、IBL）
- シャドウ（CSM + PCF、ポイントライト/スポットライト シャドウ）
- カメラ（Perspective/Ortho、FPS/TPS/Free）
- スカイボックス（キューブマップ + HDR + プロシージャル）
- フォグ、3Dプリミティブ、インスタンシング、LOD、地形

---

### Phase 4: ポストエフェクトパイプライン

**完了条件:** 全エフェクトがON/OFFでき、ビジュアルが劇的に向上する。JSON設定ファイルで制御可能。

- PostEffectPipelineフレームワーク（チェーン管理、中間RT自動管理、JSON設定）
- Bloom, Tonemapping（ACES+自動露出）, SSAO(GTAO), DoF, MotionBlur
- ColorGrading(3D LUT), FXAA, TAA
- Vignette, ChromaticAberration, SSR, VolumetricLight, OutlineEffect

---

### Phase 5: 描画レイヤーシステム

**完了条件:** 背景→ゲーム→UIが独立レイヤーで管理され合成される

- RenderLayerクラス（個別RT、Z-order、ブレンド、不透明度、カメラ）
- LayerStack管理（追加/削除/並替/グループ）
- LayerCompositor（合成シェーダー、レイヤー単位PostFX制御、マスクレイヤー）
- ステンシルマスクシステム（DXLibのマスク機能再現）

---

### Phase 6: XML-GUIシステム

**完了条件:** XMLとCSSでメニュー・HUDが構築でき、C++でイベント処理可能

- パーサー（XML + CSS-likeスタイルシート → ウィジェットツリー構築）
- レイアウトエンジン（ボックスモデル、Flexbox、Grid、Absolute、%/px/auto）
- 描画（角丸矩形、ボーダー、影、グラデーション、9-slice）
- インタラクション（ヒットテスト、イベント伝搬、フォーカス、ゲームパッドナビ、D&D）
- アニメーション（プロパティアニメ、Easing、transition、ページ遷移）
- 全ウィジェット実装（Widget一覧参照）
- C++バインディング（イベントバインド、動的操作、データバインディング）

---

### Phase 7: ファイル・ネットワーク・動画

**完了条件:** 暗号化アーカイブからのアセット読込、HTTP通信、動画再生が動作

- ファイルシステム抽象化（物理/アーカイブ透過アクセス、マウントポイント）
- カスタムアーカイブ（パッキングツール、AES-256暗号化、LZ4/zstd圧縮）
- 非同期アセットローダー + ホットリロード
- TCP/UDP/HTTP/WebSocket
- Media Foundation動画デコード → テクスチャ出力

---

### Phase 8: 数学・物理・衝突判定

**完了条件:** DXLibの全数学・衝突判定関数 + 空間分割 + 簡易物理が動作

- 数学ライブラリ（DirectXMathラッパー: Vector, Matrix, Quaternion, Color, MathUtil, Random）
- 衝突判定（2D: AABB, Circle, Polygon, Line / 3D: AABB, Sphere, Ray, Frustum, OBB + Sweep）
- 空間分割（Quadtree, Octree, BVH）
- 簡易2D物理（リジッドボディ、コリジョンレスポンス、トリガー）
- Jolt Physics統合（3D、オプション）

---

### Phase 9: DXLib互換レイヤー + シェーダーホットリロード

**完了条件:** `#include "DxLibCompat.h"` で既存コードが動作する目処が立つ

- DxLibCompat.h（全パブリック関数ラッパー、型変換、定数マッピング）
- シェーダーホットリロード（ファイル監視→DXC再コンパイル→PSO再生成+エラーオーバーレイ）
- シェーダーバリアント管理（#define分岐、キャッシュ）
- 移行ガイドドキュメント

---

### Phase 10: 最適化・品質・ドキュメント

**完了条件:** ドキュメント完備、全サンプル動作、プロファイラ動作

- メモリアロケータ、リソースバリア最適化、マルチスレッドCmdList、GPUプロファイラ
- テクスチャストリーミング、描画コールバッチング最適化
- D3D12 Debug Layer統合、GPU回帰テスト、Google Test、メモリリーク検出
- APIリファレンス（Doxygen）
- HTML+CSSで、初学者から上級者まで参照可能なAPIリファレンスドキュメント (Unity6.3LTS Reference{https://docs.unity3d.com/ScriptReference/index.html?ampDeviceId=hPIxdkbj3lm9Jo6NA8nChz&ampSessionId=1770652517877&ampTimestamp=1770738922167})
- チュートリアル（Getting Started → 2D → 3D → GUI）
- **サンプルプロジェクト（ゴールG5）:**
  - 2Dシューティング
  - 2Dプラットフォーマー
  - 3Dウォークスルー
  - GUIメニューデモ
  - ポストエフェクトショーケース
- DXLib移行ガイド

---

## ■ 第7章：技術的リスクと対策

| リスク | 対策 |
|--------|------|
| D3D12の複雑さ（バリア、ヒープ） | Phase 0で基盤を固める。D3D12MA活用 |
| シェーダーコンパイル時間 | DXC + キャッシュ + ホットリロードを早期実装 |
| glTF/FBXのエッジケース | cgltfを基本、Assimpはフォールバック |
| GUIレイアウト複雑化 | Flexboxのみ先行、Gridは後回し |
| パフォーマンスボトルネック | GPUプロファイラをPhase 0から組み込み |
| スコープクリープ | フェーズ毎に「最小限動くもの」を作る |

---

## ■ 第8章：作業上の注意事項（CLIで作業するAIへ）

### 8.1 方向性を見失った時

1. **第1章に戻れ。** 全ての判断基準はそこにある。
2. 「これはDXLibの上位互換に必要か？」と自問しろ。答えがYesなら進め、Noならスコープ外。
3. 5つのゴール（G1〜G5）のどれに貢献するか明確にしろ。どれにも貢献しないならやるな。

### 8.2 実装判断の優先順位

1. **正しく動く** > パフォーマンス > コードの美しさ
2. 各フェーズの完了条件を満たすことが最優先
3. 過度な抽象化をしない。DXLibの「使いやすさ」が設計理念の筆頭にあることを忘れるな
4. 「後でやる」を恐れるな。フェーズ分けはそのためにある

### 8.3 やってはいけないこと

- フェーズを飛ばすこと
- ディレクトリ構成を勝手に変えること
- 命名規則を逸脱すること
- 完了条件を満たさずに次のフェーズに進むこと
- DXLib互換を無視してオレオレAPIだけで進めること
- ポストエフェクト・レイヤー・GUIのいずれかを「後回しにしすぎる」こと（これらが存在意義）

### 8.4 このドキュメントの更新

フェーズ完了時に、以下をこのドキュメントに追記すること：
- 完了日
- 実際にかかった期間
- フェーズ中に発生した変更点や学び
- 次フェーズへの申し送り事項

※完了日/期間は Phase Summary の最終更新時刻を基準に記録。
　期間は、複数 Summary があるフェーズは最初〜最後の差分、
　単一 Summary は前フェーズ完了との差分として推定。

#### Phase 0: D3D12基盤構築 — 完了
- 完了日: 2026-02-08 17:32（Phase0_Summary.md 最終更新）
- 実際にかかった期間: 0分（起点）
- 実装ハイライト: Win32ウィンドウ + D3D12デバイス初期化 + ダブルバッファSwapChain + RootSignatureBuilder/PSOBuilder + DXCシェーダーコンパイル + Hello Triangle描画
- 変更点/学び: DescriptorHeapのfreelist管理パターンを確立。Fenceによる明示的CPU-GPU同期が以降の全フェーズの基盤となった
- 次フェーズへの申し送り: Phase1で2D描画エンジン（Sprite/Texture/Primitive基盤）を実装。

#### Phase 1: 2D描画エンジン — 完了
- 完了日: 2026-02-08 18:24（Phase1_Summary.md 最終更新）
- 実際にかかった期間: 52分
- 実装ハイライト: SpriteBatch (DrawGraph/DrawRotaGraph等), PrimitiveBatch (全図形+AA), TextureManager (stb_image+WIC), RenderTarget, SoftImage, Camera2D, SpriteSheet, Animation2D
- 変更点/学び: DynamicBufferのダブルバッファリングと頂点バッファオフセット管理が重要。SpriteBatch.hにはBuffer.hのincludeが必須
- 次フェーズへの申し送り: Phase2でテキスト/入力/サウンドを実装し、2D基盤へ統合。

#### Phase 2: テキスト・入力・サウンド — 完了
- 完了日: 2026-02-08 19:07（Phase2_Summary.md 最終更新）
- 実際にかかった期間: 42分
- 実装ハイライト: DirectWriteフォントアトラス (Unicode対応、オンデマンド漢字), InputManager (Keyboard/Mouse/Gamepad), AudioManager (XAudio2 SE/BGM+フェード)
- 変更点/学び: フォントアトラスの動的拡張には FlushAtlasUpdates() のフレーム境界での一括アップロードが効果的。WIC Factoryのキャッシュ化で性能改善
- 次フェーズへの申し送り: Phase3で3D PBR描画基盤（モデル/シャドウ/ライト）を構築。

#### Phase 3: 3D描画エンジン — 完了
- 完了日: 2026-02-08 21:50（Phase3_Summary.md 最終更新）
- 実際にかかった期間: 2時間43分
- 実装ハイライト: PBR (Cook-Torrance BRDF), CSM (4 cascades), Skybox, Fog, glTFローダー (cgltf), スケルタルアニメーション, Terrain, PrimitiveBatch3D
- 変更点/学び: シャドウSRVレイアウト (t8-t11 CSM, t12 Spot, t13 Point) の固定化が安定性に寄与。PointShadowMapは6-face Texture2DArrayで実装
- 次フェーズへの申し送り: Phase4でPostEffectPipelineとエフェクトON/OFF制御を実装。

#### Phase 4: ポストエフェクトパイプライン — 完了
- 完了日: 2026-02-09 15:27（Phase4a〜Phase4mno_Summary.md 最終更新）
- 実際にかかった期間: 17時間31分
- 実装ハイライト: 13種エフェクト (Bloom, ACES/Reinhard/Uncharted2 Tonemap, FXAA, Vignette, ChromaticAberration, ColorGrading, SSAO, DoF, MotionBlur, SSR, Outline, VolumetricLight, TAA, AutoExposure) + JSON設定
- 変更点/学び: PostEffect chainはping-pong RT (2 HDR + 2 LDR) で実装。D3D12では同時に1つのCBV_SRV_UAVヒープしかバインドできないため、各エフェクトに専用SRVヒープを使用
- 次フェーズへの申し送り: Phase5でRenderLayer/LayerCompositorにPostFX出力を統合。

#### Phase 5: 描画レイヤーシステム — 完了
- 完了日: 2026-02-09 17:48（Phase5_Summary.md 最終更新）
- 実際にかかった期間: 2時間20分
- 実装ハイライト: RenderLayer (個別RT), LayerStack (Z-order), LayerCompositor (6ブレンドモード), MaskScreen (R channel矩形/円マスク)
- 変更点/学び: PostEffectPipeline.Resolve()の出力先をScene layer RTにリダイレクトすることで、PostEffectPipelineの変更なしにレイヤー統合を実現
- 次フェーズへの申し送り: Phase6でXML-GUI（レイアウト/イベント/描画）を構築。

#### Phase 6: XML-GUIシステム — 完了
- 完了日: 2026-02-09 19:44（Phase6ab_Summary.md / Phase6c_Summary.md 最終更新）
- 実際にかかった期間: 1時間53分
- 実装ハイライト: Widget tree + UIContext (Flexbox layout + 3-phase event dispatch) + UIRenderer (SDF UIRect + ScissorStack), StyleSheet (.css parser + cascade), GUILoader (XML+CSS declarative UI), 14種Widget (Panel, TextWidget, Button, Spacer, ProgressBar, Image, CheckBox, Slider, ScrollView, RadioButton, DropDown, ListView, TabView, Dialog, Canvas, TextInput), GUI Scaling (design resolution)
- 変更点/学び: DynamicBufferの同一フレーム内多重サイクル問題 (SpriteBatch/UIRectBatch) はper-frame offset counterで解決。StyleSheetは毎フレームComputeLayout()で適用されるため、CSSが常にcomputedStyleを上書きする
- 次フェーズへの申し送り: Phase7でファイル/ネットワーク/動画（VFS/Archive/Async）を実装。

#### Phase 7: ファイル・ネットワーク・動画 — 完了
- 完了日: 2026-02-10 00:49（Phase7_Summary.md 最終更新）
- 実際にかかった期間: 5時間4分
- 実装ハイライト: VFS (FileSystem + PhysicalFileProvider), Archive (.gxarc AES-256+LZ4), AsyncLoader + FileWatcher, TCP/UDP/HTTP/WebSocket, MoviePlayer (Media Foundation)
- 変更点/学び: LZ4はC言語ソースのため、CMakeでLANGUAGE C + SKIP_PRECOMPILE_HEADERS ONが必須。Archive priority=100でアーカイブがphysical fileに優先
- 次フェーズへの申し送り: Phase8でMath/Collision/Physics（Jolt）を実装。

#### Phase 8: 数学・物理・衝突判定 — 完了
- 完了日: 2026-02-10 15:25（Phase8_Summary.md 最終更新）
- 実際にかかった期間: 14時間36分
- 実装ハイライト: Vector2/3/4, Matrix4x4, Quaternion (DirectXMath継承), Color, MathUtil, Random, Collision2D/3D (SAT, Moller-Trumbore), Quadtree/Octree/BVH (SAH), PhysicsWorld2D (custom impulse), PhysicsWorld3D (Jolt v5.3.0 PIMPL)
- 変更点/学び: DirectXMath XMFLOAT系の継承でゼロオーバーヘッド相互変換を実現。Jolt統合ではUSE_STATIC_MSVC_RUNTIME_LIBRARY OFF, RegisterDefaultAllocator(), RVec3型変換ヘルパーの分離が必要
- 次フェーズへの申し送り: Phase9でDXLib互換APIとShader Hot Reloadを統合。

#### Phase 9: DXLib互換レイヤー + シェーダーホットリロード — 完了
- 完了日: 2026-02-10 15:25（Phase9_Summary.md 最終更新）
- 実際にかかった期間: 1分未満（サマリ更新差分）
- 実装ハイライト: CompatContext singleton (全サブシステム保持), Compat_*.cpp (描画/入力/音声/3D/数学), DIK→VK変換テーブル, ShaderLibrary (hash cache + バリアント), ShaderHotReload (FileWatcher→debounce→GPU Flush→PSO rebuild), PSO Rebuilder (18レンダラー登録), エラーオーバーレイ
- 変更点/学び: ActiveBatch auto-switchingでSprite/Primitive切替をユーザーから隠蔽。.hlsli変更時は全キャッシュクリアが安全(include依存追跡は複雑すぎる)
- 次フェーズへの申し送り: Phase10で最適化/品質/ドキュメント/サンプルを完成。

#### Phase 10: 最適化・品質・ドキュメント — 完了
- 完了日: 2026-02-10 15:25（Phase10a_Summary.md / Phase10c_Summary.md 最終更新）
- 実際にかかった期間: 1分未満（サマリ更新差分）
- 実装ハイライト:
  - 10a: GPUProfiler (D3D12 Timestamp Query, ダブルバッファリードバック, HUDオーバーレイ), FrameAllocator, PoolAllocator, BarrierBatch
  - 10b: Google Test (151テスト), APIリファレンス (1120エントリ), Doxygen, チュートリアル5本, DXLib移行ガイド, README
  - 10c: サンプルプロジェクト5本 (Shooting2D, Platformer2D, Walkthrough3D, GUIMenuDemo, PostEffectShowcase)
- 変更点/学び: FrameAllocatorのアラインメントは実アドレスベースで計算必須(バッファ先頭アドレスが境界にない場合の問題)。gxlib_add_sampleマクロで1行サンプル追加を実現
- 次フェーズへの申し送り: Phase10+ としてマルチスレッドCmdList/ストリーミング/GPU回帰テスト等をバックログ管理。

---

> **最後に繰り返す。このプロジェクトの核心は「DXライブラリは見た目がショボいし機能が足りない。DirectX 12で完全上位互換を作る」である。この一文を常に頭に置いて作業せよ。**

## 2. GXLib Framework Plan

> **DXライブラリ完全上位互換 + モダンレンダリング + XML-GUI + ポストエフェクト標準搭載**

---

## 0. プロジェクト概要

### 0.1 動機と目的

DXライブラリは教育・ホビー向けとして広く使われているが、以下の根本的な問題を抱えている：

- DirectX 9/11 止まりで DirectX 12 に非対応
- 描画表現が貧弱でプロレベルのビジュアルに到達できない
- ポストエフェクトが標準搭載されておらず全て自前実装が必要
- 描画レイヤー管理の仕組みがなく、描画順制御が煩雑
- GUIシステムが存在しない

本プロジェクトでは、DXライブラリの全機能を網羅しつつ、上記の問題を根本解決した **DirectX 12 ベースの完全上位互換フレームワーク** を構築する。

### 0.2 設計理念

1. **DXライブラリ互換**: 既存の DXLib ユーザーが違和感なく移行できる API 設計
2. **モダンレンダリング**: PBR、HDR、ポストエフェクトを標準搭載
3. **レイヤードアーキテクチャ**: 低レベル API と高レベル API の両方を提供
4. **宣言的 GUI**: XML/スクリプトによる UI 定義
5. **拡張性**: プラグインやカスタムシェーダーで自由に拡張可能

### 0.3 技術スタック

| 項目 | 選定 |
|------|------|
| グラフィックス API | DirectX 12 (D3D12) |
| シェーダー言語 | HLSL (Shader Model 6.x) |
| シェーダーコンパイラ | DXC (DirectX Shader Compiler) |
| ビルドシステム | CMake + vcpkg |
| 言語 | C++20 (モジュール対応準備) |
| オーディオ | XAudio2 / WASAPI |
| 入力 | XInput + Raw Input + DirectInput(後方互換) |
| 物理 | 内製 2D + Jolt Physics(3D、オプション) |
| GUI記述 | XML + CSS-like スタイルシート |
| スクリプト | Lua（オプション） |
| テスト | Google Test + GPU ベースの回帰テスト |

### 0.4 ゴール定義

**完成条件 = 以下の全てを満たすこと：**

1. DXライブラリの全 API カテゴリを網羅（後述の機能マッピング表を100%カバー）
2. ポストエフェクトパイプラインが標準搭載され、設定ファイルで ON/OFF 可能
3. 描画レイヤーシステムが動作し、レイヤー単位でのブレンド・エフェクトが可能
4. XML ベースの GUI システムでメニュー・HUD・エディタ UI が構築可能
5. サンプルプロジェクト群（2Dゲーム、3Dゲーム、GUIデモ）が動作する

---

## 1. DXライブラリ機能マッピング表

DXライブラリの全機能カテゴリを洗い出し、本フレームワークでの対応方針を定義する。

### 1.1 システム系

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| ウィンドウ管理 | `DxLib_Init`, `SetGraphMode`, `ChangeWindowMode` | Win32 ウィンドウ + DXGI SwapChain |
| メッセージ処理 | `ProcessMessage` | 内部メッセージループ + イベントコールバック |
| フルスクリーン切替 | `ChangeWindowMode` | DXGI フルスクリーン + ボーダーレス |
| 解像度変更 | `SetGraphMode` | 動的スワップチェーンリサイズ |
| DPI対応 | （なし） | **新規**: Per-Monitor DPI V2 対応 |
| マルチウィンドウ | （なし） | **新規**: マルチウィンドウレンダリング |
| タイマー | `GetNowCount`, `GetNowHiPerformanceCount` | `QueryPerformanceCounter` ベース |
| FPS制御 | `SetWaitVSyncFlag` | VSync + フレームレートリミッター |
| ログ出力 | `printfDx`, `ErrorLogAdd` | 構造化ログシステム (レベル別、ファイル出力) |

### 1.2 描画系 — 2D

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| 画像読込・描画 | `LoadGraph`, `DrawGraph` | テクスチャ管理 + スプライトバッチ |
| 画像分割読込 | `LoadDivGraph` | スプライトシート + アトラスパッカー |
| 画像回転拡大 | `DrawRotaGraph`, `DrawExtendGraph` | Transform2D 指定描画 |
| 画像ブレンド | `SetDrawBlendMode` | ブレンドステート (加算、乗算、αなど) |
| 画像輝度設定 | `SetDrawBright` | シェーダーパラメータ / カラーマスク |
| 図形描画 | `DrawLine`, `DrawBox`, `DrawCircle`, `DrawTriangle` | プリミティブバッチレンダラー |
| アンチエイリアス図形 | `DrawLineAA`, `DrawCircleAA` | MSAA + シェーダーベース AA |
| ピクセル操作 | `GetPixelSoftImage`, `DrawPixelSoftImage` | CPU側ソフトイメージ + Readback |
| グラフィックフィルタ | `GraphFilter` (モノ、ガウス、明度等) | **ポストエフェクトパイプラインで代替** |
| 描画先変更 | `SetDrawScreen`, `MakeScreen` | RenderTarget システム |
| **描画レイヤー** | （なし） | **新規**: レイヤースタック (Z-order、ブレンド、エフェクト) |
| **スプライトアニメーション** | （なし） | **新規**: アニメーションコントローラー |

### 1.3 描画系 — 3D

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| 3Dモデル読込 | `MV1LoadModel` | glTF / FBX / OBJ ローダー |
| モデル描画 | `MV1DrawModel` | メッシュレンダラー + インスタンシング |
| モデルアニメーション | `MV1AttachAnim`, `MV1SetAttachAnimTime` | スケルタルアニメーション + ブレンドツリー |
| モデル衝突判定 | `MV1CollCheck_Sphere`, `MV1CollCheck_Line` | メッシュコリジョン |
| カメラ制御 | `SetCameraPositionAndTarget_UpVecY` | Camera コンポーネント (Perspective / Ortho) |
| ライティング | `SetLightDirection`, `SetLightDifColor` | **PBR ライティング** (Directional, Point, Spot, Area) |
| マテリアル | `MV1SetMaterialDifColor` | **PBR マテリアル** (Albedo, Normal, Metallic, Roughness, AO) |
| シャドウマップ | `SetShadowMapDrawArea` | **CSM (Cascaded Shadow Maps)** + PCF/VSM |
| フォグ | `SetFogEnable`, `SetFogColor` | ボリューメトリックフォグ / 距離フォグ |
| 3D図形 | `DrawSphere3D`, `DrawCone3D` | プリミティブメッシュ生成 |
| Zバッファ | `SetUseZBuffer3D` | 深度ステート制御 |
| **環境マップ** | （なし） | **新規**: キューブマップ / IBL 反射 |
| **PBR** | （なし） | **新規**: Cook-Torrance BRDF |
| **スカイボックス** | （なし） | **新規**: HDR スカイボックス / プロシージャル空 |
| **地形** | （なし） | **新規**: ハイトマップ地形 + LOD |

### 1.4 ポストエフェクト（全て新規）

| エフェクト | 実装方式 |
|-----------|---------|
| Bloom | Dual Kawase / ガウシアン ダウンサンプリング |
| Tonemapping | ACES / Reinhard / Uncharted 2 / AgX |
| HDR | 浮動小数テクスチャ (R16G16B16A16_FLOAT) |
| SSAO | GTAO / HBAO |
| 被写界深度 (DoF) | Bokeh DoF (六角形/円形) |
| モーションブラー | Per-Object / Camera ベース |
| カラーグレーディング | 3D LUT |
| FXAA / TAA | FXAA 3.11 / TAA |
| ビネット | シェーダーベース |
| 色収差 | シェーダーベース |
| スクリーンスペースリフレクション | SSR (Hi-Z trace) |
| ボリューメトリックライト | レイマーチング |
| 輪郭線 (トゥーン) | Sobel / 法線・深度エッジ検出 |

### 1.5 シェーダー

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| 頂点シェーダー | `LoadVertexShader` | HLSL SM 6.x コンパイル + ホットリロード |
| ピクセルシェーダー | `LoadPixelShader` | 同上 |
| 定数バッファ | `SetShaderConstantReg` | CBV 自動バインド |
| シェーダー描画 | `SetUseVertexShader` | マテリアルシステム統合 |
| **コンピュートシェーダー** | （なし） | **新規**: CS パイプライン |
| **シェーダーホットリロード** | （なし） | **新規**: ファイル監視による即時反映 |
| **シェーダーバリアント** | （なし） | **新規**: プリプロセッサ定義による分岐管理 |

### 1.6 サウンド

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| サウンド読込・再生 | `LoadSoundMem`, `PlaySoundMem` | XAudio2 ベース |
| BGM ストリーミング | `PlayMusic` | ストリーミングデコード |
| 音量制御 | `ChangeVolumeSoundMem` | デシベルベース音量制御 |
| パン制御 | `ChangePanSoundMem` | ステレオパン |
| 再生速度 | `SetFrequencySoundMem` | ピッチシフト |
| 3Dサウンド | `Set3DPositionSoundMem` | **3D 空間音響** (HRTF オプション) |
| 対応フォーマット | WAV, OGG, MP3 | WAV, OGG, MP3, FLAC, OPUS |
| **サウンドミキサー** | （なし） | **新規**: バス・エフェクトチェーン |
| **リアルタイムエフェクト** | （なし） | **新規**: リバーブ、EQ、コンプレッサー |

### 1.7 入力

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| キーボード | `CheckHitKey`, `GetHitKeyStateAll` | Raw Input |
| マウス | `GetMousePoint`, `GetMouseInput` | Raw Input + カーソル管理 |
| マウスホイール | `GetMouseWheelRotVol` | WM_MOUSEWHEEL |
| ジョイパッド | `GetJoypadInputState`, `GetJoypadAnalogInput` | XInput + DirectInput フォールバック |
| ジョイパッド振動 | `StartJoypadVibration` | XInput バイブレーション |
| タッチ | `GetTouchInputNum` | WM_TOUCH / WM_POINTER |
| **入力マッピング** | （なし） | **新規**: アクションマップ (設定ファイルで再マップ可能) |
| **入力バッファリング** | （なし） | **新規**: 格闘ゲーム向けコマンドバッファ |
| **デッドゾーン設定** | （なし） | **新規**: アナログスティック デッドゾーン |

### 1.8 文字描画

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| 文字列描画 | `DrawString`, `DrawFormatString` | **SDF フォントレンダリング** |
| フォント作成 | `CreateFontToHandle` | DirectWrite + フォントアトラス |
| 文字列幅取得 | `GetDrawStringWidth` | テキスト計測 API |
| 文字コード | `SetUseCharCodeFormat` | UTF-8 / UTF-16 |
| **リッチテキスト** | （なし） | **新規**: カラー、サイズ混在テキスト |
| **テキストレイアウト** | （なし） | **新規**: ワードラップ、行間、カーニング |
| **ビットマップフォント** | （なし） | **新規**: BMFont 形式対応 |

### 1.9 ネットワーク

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| TCP接続 | `ConnectNetWork`, `NetWorkSend` | Winsock2 非同期 TCP |
| UDP | `MakeUDPSocket`, `NetWorkSendUDP` | Winsock2 UDP |
| HTTP | `GetHTTP`, `GetHTTPRes` | WinHTTP / libcurl |
| **WebSocket** | （なし） | **新規**: WebSocket クライアント |
| **非同期IO** | （なし） | **新規**: IOCP ベース非同期 |

### 1.10 ファイル・アーカイブ

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| ファイル読み書き | `FileRead_open`, `FileRead_gets` | std::filesystem + 非同期IO |
| DXA アーカイブ | `SetDXArchiveExtension` | **カスタムアーカイブ** (AES暗号化対応) |
| メモリ上読込 | `CreateGraphFromMem` | メモリストリーム対応 |
| **アセットホットリロード** | （なし） | **新規**: ファイル監視による即時反映 |
| **非同期アセット読込** | （なし） | **新規**: バックグラウンドロード + プログレス |

### 1.11 算術・衝突判定

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| ベクトル演算 | `VGet`, `VAdd`, `VCross` | SIMD 最適化 数学ライブラリ (DirectXMath ラップ) |
| 行列演算 | `MGetIdent`, `MMult` | Matrix4x4 クラス |
| 衝突判定 | `HitCheck_Sphere_Sphere`, `HitCheck_Line_Triangle` | 衝突判定ユーティリティ |
| **2D物理** | （なし） | **新規**: 簡易 2D 物理エンジン (AABB, Circle, Polygon) |
| **空間分割** | （なし） | **新規**: Quadtree / Octree / BVH |

### 1.12 動画

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| 動画再生 | `PlayMovie`, `OpenMovieToGraph` | Media Foundation デコード → テクスチャ |
| 動画フレーム取得 | `SeekMovieToGraph`, `TellMovieToGraph` | フレーム単位シーク |

### 1.13 マスク

| DXLib 機能 | 対応関数例 | GXLib 対応方針 |
|-----------|-----------|---------------|
| マスク描画 | `CreateMaskScreen`, `DrawMask` | ステンシルバッファ + マスクテクスチャ |
| マスク図形 | `DrawFillMask`, `DrawCircleMask` | ステンシル描画 |

---

## 2. 新規システム詳細設計

### 2.1 描画レイヤーシステム

```
┌─────────────────────────────────┐
│         最終合成出力              │
├─────────────────────────────────┤
│  Layer: UI (Z: 1000)            │ ← ポストエフェクト適用外
│  Layer: HUD (Z: 900)            │ ← ポストエフェクト適用外
│  Layer: PostFX                  │ ← ポストエフェクトパイプライン
│  Layer: Particles (Z: 500)      │
│  Layer: Characters (Z: 400)     │
│  Layer: World (Z: 100)          │ ← PBR レンダリング
│  Layer: Background (Z: 0)       │
│  Layer: Skybox (Z: -1000)       │
└─────────────────────────────────┘
```

**レイヤーの機能:**
- 個別の RenderTarget を保持
- レイヤー単位のブレンドモード設定（通常、加算、乗算、スクリーン...）
- レイヤー単位の不透明度制御
- レイヤー単位のポストエフェクト適用可否
- レイヤーのカメラ独立設定（パララックススクロール等）
- レイヤーのソートモード設定（Z-sort, Y-sort, 挿入順）

### 2.2 ポストエフェクトパイプライン

```
Scene → [HDR RenderTarget]
         ↓
    ┌─ SSAO ──────────┐
    ├─ SSR ───────────┤
    ├─ MotionBlur ────┤
    ├─ DoF ───────────┤   ← 設定ファイルで ON/OFF、順序変更可能
    ├─ Bloom ─────────┤
    ├─ ColorGrading ──┤
    ├─ Tonemapping ───┤
    ├─ FXAA/TAA ──────┤
    ├─ Vignette ──────┤
    └─ ChromaticAberr ┘
         ↓
    [LDR BackBuffer] → Present
```

**設定例 (JSON):**
```json
{
  "postEffects": {
    "bloom": { "enabled": true, "threshold": 1.0, "intensity": 0.8, "radius": 4 },
    "tonemapping": { "enabled": true, "operator": "ACES", "exposure": 1.2 },
    "ssao": { "enabled": true, "radius": 0.5, "bias": 0.025, "samples": 32 },
    "dof": { "enabled": false },
    "fxaa": { "enabled": true }
  }
}
```

### 2.3 XML-GUI システム

**設計コンセプト: Web のような宣言的 UI を C++ ゲームで**

#### XML レイアウト定義
```xml
<!-- ui/main_menu.xml -->
<Window id="mainMenu" width="100%" height="100%">
  <Panel id="centerPanel" layout="vertical" align="center" valign="center"
         background="#00000080" padding="20" cornerRadius="8">

    <Text id="title" text="My Game" fontSize="48" fontFamily="GameFont"
          color="#FFFFFF" shadow="2,2,#000000" />

    <Spacer height="40" />

    <Button id="btnStart" width="300" height="60" text="ゲーム開始"
            class="menuButton" onClick="onStartGame" />
    <Button id="btnOption" width="300" height="60" text="オプション"
            class="menuButton" onClick="onOpenOptions" />
    <Button id="btnExit" width="300" height="60" text="終了"
            class="menuButton" onClick="onExit" />
  </Panel>
</Window>
```

#### CSS-like スタイルシート
```css
/* ui/styles/menu.gss */
.menuButton {
    background: linear-gradient(#4A90D9, #357ABD);
    color: #FFFFFF;
    fontSize: 24;
    fontFamily: "GameFont";
    cornerRadius: 6;
    border: 2px solid #2A5A8E;
    transition: background 0.2s;
}

.menuButton:hover {
    background: linear-gradient(#5BA0E9, #4590DD);
    transform: scale(1.05);
}

.menuButton:pressed {
    background: linear-gradient(#2A5A8E, #1A4A7E);
    transform: scale(0.95);
}
```

#### C++ バインド
```cpp
// GUI の読み込みと操作
auto ui = GX::GUI::Load("ui/main_menu.xml", "ui/styles/menu.gss");

// イベントバインド
ui->Bind("onStartGame", [&]() { scene.TransitionTo<GameScene>(); });
ui->Bind("onOpenOptions", [&]() { ui->Show("optionsPanel"); });
ui->Bind("onExit", [&]() { GX::System::Exit(); });

// 動的操作
ui->Find<GX::GUI::Text>("title")->SetText("Updated Title");
ui->Find<GX::GUI::Button>("btnStart")->SetEnabled(false);
```

#### GUI ウィジェット一覧

| ウィジェット | 説明 |
|------------|------|
| Window | ルートコンテナ |
| Panel | レイアウトコンテナ (horizontal/vertical/grid/absolute) |
| Text | テキスト表示 |
| Button | ボタン |
| Image | 画像表示 |
| TextInput | テキスト入力欄 |
| Slider | スライダー |
| CheckBox | チェックボックス |
| RadioGroup / Radio | ラジオボタン |
| DropDown | ドロップダウン |
| ListView | スクロール可能リスト |
| ScrollView | スクロール可能コンテナ |
| ProgressBar | プログレスバー |
| TabView | タブ切替 |
| Dialog | モーダルダイアログ |
| Canvas | カスタム描画エリア |

---

## 3. アーキテクチャ

### 3.1 モジュール構成

```
GXLib/
├── Core/                    # コアシステム
│   ├── Application.h/cpp    # アプリケーションライフサイクル
│   ├── Window.h/cpp         # ウィンドウ管理
│   ├── Timer.h/cpp          # 高精度タイマー
│   ├── Logger.h/cpp         # ログシステム
│   ├── Memory/              # メモリ管理
│   │   ├── Allocator.h      # カスタムアロケータ
│   │   ├── PoolAllocator.h  # プールアロケータ
│   │   └── StackAllocator.h # スタックアロケータ
│   ├── Event/               # イベントシステム
│   │   ├── EventBus.h       # パブリッシュ・サブスクライブ
│   │   └── Delegate.h       # 型安全コールバック
│   └── Config/              # 設定管理
│       └── ConfigManager.h  # JSON/INI 設定読み書き
│
├── Graphics/                # 描画エンジン
│   ├── Device/              # D3D12 デバイス管理
│   │   ├── GraphicsDevice.h # デバイス初期化・管理
│   │   ├── SwapChain.h      # スワップチェーン
│   │   ├── CommandQueue.h   # コマンドキュー
│   │   ├── CommandList.h    # コマンドリスト
│   │   ├── DescriptorHeap.h # デスクリプタヒープ管理
│   │   ├── Fence.h          # GPU 同期
│   │   └── GPUResource.h    # リソース基底クラス
│   │
│   ├── Pipeline/            # パイプライン管理
│   │   ├── PipelineState.h  # PSO 管理
│   │   ├── RootSignature.h  # ルートシグネチャ
│   │   ├── Shader.h         # シェーダーコンパイル・管理
│   │   └── ShaderLibrary.h  # シェーダーライブラリ + ホットリロード
│   │
│   ├── Resource/            # GPU リソース
│   │   ├── Texture.h        # テクスチャ
│   │   ├── Buffer.h         # 頂点/インデックス/定数バッファ
│   │   ├── RenderTarget.h   # レンダーターゲット
│   │   ├── DepthBuffer.h    # 深度バッファ
│   │   └── SamplerState.h   # サンプラー
│   │
│   ├── 2D/                  # 2D 描画
│   │   ├── SpriteBatch.h    # スプライトバッチ
│   │   ├── SpriteSheet.h    # スプライトシート
│   │   ├── PrimitiveBatch.h # 図形バッチ
│   │   ├── TextRenderer.h   # テキスト描画 (SDF)
│   │   ├── FontManager.h    # フォント管理
│   │   └── Animation2D.h    # スプライトアニメーション
│   │
│   ├── 3D/                  # 3D 描画
│   │   ├── Mesh.h           # メッシュ
│   │   ├── Model.h          # 3Dモデル (glTF/FBX)
│   │   ├── SkeletalAnim.h   # スケルタルアニメーション
│   │   ├── Camera.h         # カメラ
│   │   ├── Light.h          # ライト (Directional/Point/Spot)
│   │   ├── Material.h       # PBR マテリアル
│   │   ├── ShadowMap.h      # CSM シャドウ
│   │   ├── Skybox.h         # スカイボックス
│   │   ├── Terrain.h        # 地形
│   │   └── Primitive3D.h    # 3Dプリミティブ
│   │
│   ├── Layer/               # 描画レイヤー
│   │   ├── RenderLayer.h    # レイヤー定義
│   │   ├── LayerStack.h     # レイヤースタック管理
│   │   └── LayerCompositor.h # レイヤー合成
│   │
│   ├── PostFX/              # ポストエフェクト
│   │   ├── PostEffectPipeline.h  # パイプライン管理
│   │   ├── PostEffect.h         # エフェクト基底
│   │   ├── Bloom.h              # Bloom
│   │   ├── Tonemapping.h        # トーンマッピング
│   │   ├── SSAO.h               # SSAO
│   │   ├── DoF.h                # 被写界深度
│   │   ├── MotionBlur.h         # モーションブラー
│   │   ├── ColorGrading.h       # カラーグレーディング
│   │   ├── FXAA.h               # FXAA
│   │   ├── TAA.h                # TAA
│   │   ├── Vignette.h           # ビネット
│   │   ├── ChromaticAberration.h # 色収差
│   │   ├── SSR.h                # スクリーンスペースリフレクション
│   │   ├── VolumetricLight.h    # ボリューメトリックライト
│   │   └── OutlineEffect.h     # 輪郭線
│   │
│   └── Renderer.h           # レンダラー統合 (2D/3D/PostFX/Layer合成)
│
├── Audio/                   # オーディオ
│   ├── AudioDevice.h        # XAudio2 デバイス
│   ├── Sound.h              # サウンドリソース
│   ├── SoundPlayer.h        # 再生管理
│   ├── MusicPlayer.h        # BGM ストリーミング
│   ├── AudioMixer.h         # ミキサー・バス
│   ├── Audio3D.h            # 3D 空間音響
│   └── AudioEffect.h        # リバーブ等エフェクト
│
├── Input/                   # 入力
│   ├── Keyboard.h           # キーボード
│   ├── Mouse.h              # マウス
│   ├── Gamepad.h            # ゲームパッド (XInput + DInput)
│   ├── Touch.h              # タッチ入力
│   ├── InputManager.h       # 統合入力管理
│   └── ActionMap.h          # アクションマッピング
│
├── GUI/                     # GUI システム
│   ├── GUIManager.h         # GUI 統合管理
│   ├── GUIParser.h          # XML パーサー
│   ├── GUIStyleSheet.h      # スタイルシートエンジン
│   ├── GUIRenderer.h        # GUI 描画
│   ├── GUILayout.h          # レイアウトエンジン (Flexbox-like)
│   ├── GUIAnimation.h       # UI アニメーション・トランジション
│   ├── GUIEvent.h           # UI イベント伝搬
│   └── Widgets/             # ウィジェット
│       ├── Widget.h         # 基底ウィジェット
│       ├── Panel.h
│       ├── Button.h
│       ├── Text.h
│       ├── TextInput.h
│       ├── Image.h
│       ├── Slider.h
│       ├── CheckBox.h
│       ├── RadioButton.h
│       ├── DropDown.h
│       ├── ListView.h
│       ├── ScrollView.h
│       ├── ProgressBar.h
│       ├── TabView.h
│       ├── Dialog.h
│       └── Canvas.h
│
├── IO/                      # ファイル・ネットワーク
│   ├── FileSystem.h         # ファイルシステム抽象化
│   ├── Archive.h            # アーカイブ (暗号化対応)
│   ├── AsyncLoader.h        # 非同期アセットロード
│   ├── Network/             # ネットワーク
│   │   ├── TCPSocket.h
│   │   ├── UDPSocket.h
│   │   ├── HTTPClient.h
│   │   └── WebSocket.h
│   └── Serialization.h      # シリアライゼーション
│
├── Math/                    # 数学ライブラリ
│   ├── Vector2.h / Vector3.h / Vector4.h
│   ├── Matrix4x4.h
│   ├── Quaternion.h
│   ├── Color.h
│   ├── MathUtil.h           # Lerp, Clamp, SmoothStep 等
│   ├── Random.h             # 乱数生成
│   └── Collision/           # 衝突判定
│       ├── AABB.h
│       ├── Sphere.h
│       ├── Ray.h
│       ├── Frustum.h
│       └── CollisionUtil.h
│
├── Physics/                 # 物理 (オプショナル)
│   ├── Physics2D.h          # 簡易 2D 物理
│   └── PhysicsWorld.h       # Jolt 統合 (3D)
│
├── Movie/                   # 動画再生
│   └── MoviePlayer.h        # Media Foundation ベース
│
├── Compat/                  # DXライブラリ互換レイヤー
│   ├── DxLibCompat.h        # DXLib 関数名互換 API
│   └── DxLibTypes.h         # 型変換
│
└── Utility/                 # ユーティリティ
    ├── StringUtil.h         # 文字列ユーティリティ
    ├── PathUtil.h           # パス操作
    ├── Hash.h               # ハッシュ関数
    ├── ThreadPool.h         # スレッドプール
    └── Profiler.h           # パフォーマンスプロファイラ
```

### 3.2 DXライブラリ互換レイヤー

既存の DXLib ユーザーが段階的に移行できるよう、互換 API を提供する：

```cpp
// Compat/DxLibCompat.h — DXLib スタイルの関数を GXLib にマッピング
namespace DxLibCompat {
    inline int DxLib_Init()         { return GX::Application::Init(); }
    inline int DxLib_End()          { return GX::Application::Shutdown(); }
    inline int ProcessMessage()     { return GX::Application::ProcessMessages(); }
    inline int DrawGraph(int x, int y, int handle, int transFlag) {
        return GX::Graphics2D::DrawSprite(handle, {x, y}, transFlag);
    }
    // ... 全 DXLib 関数をマッピング
}
```

---

## 4. 実装フェーズ

### フェーズ 0: 基盤構築（目安: 4-6 週）

**ゴール: ウィンドウ表示 + D3D12 初期化 + 三角形描画**

- [ ] CMake プロジェクト構成 + vcpkg 依存管理
- [ ] Win32 ウィンドウ作成・メッセージループ
- [ ] D3D12 デバイス初期化
  - [ ] Factory, Device, CommandQueue 作成
  - [ ] SwapChain 作成 (ダブルバッファリング)
  - [ ] DescriptorHeap 管理クラス
  - [ ] Fence による CPU-GPU 同期
  - [ ] CommandAllocator / CommandList 管理
- [ ] パイプラインステート基盤
  - [ ] RootSignature ビルダー
  - [ ] PSO ビルダー
  - [ ] DXC によるシェーダーコンパイル
- [ ] 三角形描画 (Hello Triangle)
- [ ] フレームタイミング・FPS制御
- [ ] ログシステム

**成果物:** 色付き三角形がウィンドウに描画される

---

### フェーズ 1: 2D 描画エンジン（目安: 6-8 週）

**ゴール: DXLib の 2D 描画機能を全て再現**

- [ ] テクスチャ管理
  - [ ] WIC / stb_image による画像読込 (PNG, JPG, BMP, TGA, DDS)
  - [ ] テクスチャリソース管理 (アップロード、キャッシュ)
  - [ ] ミップマップ生成
- [ ] スプライトバッチ
  - [ ] 動的頂点バッファによるバッチ処理
  - [ ] DrawGraph 相当 (位置指定描画)
  - [ ] DrawRotaGraph 相当 (回転・拡大)
  - [ ] DrawRectGraph 相当 (矩形切り出し)
  - [ ] DrawExtendGraph 相当 (拡縮)
  - [ ] DrawModiGraph 相当 (自由変形)
  - [ ] ブレンドモード (None, Alpha, Add, Sub, Mul, Screen...)
  - [ ] カラーモジュレーション (SetDrawBright 相当)
- [ ] スプライトシート・アトラス
  - [ ] LoadDivGraph 相当
  - [ ] テクスチャアトラスパッカー
  - [ ] スプライトアニメーション再生
- [ ] プリミティブバッチ
  - [ ] DrawLine / DrawLineAA
  - [ ] DrawBox / DrawFillBox
  - [ ] DrawCircle / DrawCircleAA / DrawOval
  - [ ] DrawTriangle / DrawFillTriangle
  - [ ] DrawPixel
  - [ ] アンチエイリアス対応 (距離ベース AA シェーダー)
- [ ] RenderTarget
  - [ ] MakeScreen 相当 (任意サイズ RT 作成)
  - [ ] SetDrawScreen 相当 (描画先切替)
  - [ ] GetDrawScreenGraph 相当 (スクリーンキャプチャ)
- [ ] ソフトイメージ
  - [ ] CPU 側ピクセル操作
  - [ ] GPU ← → CPU 転送
- [ ] カメラ2D (ビュー行列変換)

**成果物:** DXLib の 2D サンプルが全て再現可能

---

### フェーズ 2: テキスト・入力・サウンド（目安: 4-6 週）

**ゴール: ゲームとして最低限遊べるインフラ**

#### テキスト描画
- [ ] DirectWrite によるフォントラスタライズ
- [ ] SDF フォントアトラス生成
- [ ] SDF テキストレンダリングシェーダー
- [ ] DrawString / DrawFormatString 相当
- [ ] フォントハンドル管理 (CreateFontToHandle 相当)
- [ ] 文字列幅・高さ取得
- [ ] リッチテキスト (色・サイズ混在)
- [ ] ビットマップフォント (BMFont 対応)

#### 入力
- [ ] キーボード (Raw Input)
  - [ ] CheckHitKey / GetHitKeyStateAll 相当
  - [ ] キーリピート制御
- [ ] マウス (Raw Input)
  - [ ] 位置取得、ボタン状態、ホイール
  - [ ] カーソル表示制御、クリッピング
- [ ] ゲームパッド
  - [ ] XInput 対応 (Xbox コントローラー)
  - [ ] DirectInput フォールバック (汎用パッド)
  - [ ] アナログスティック デッドゾーン
  - [ ] バイブレーション
- [ ] アクションマッピングシステム
  - [ ] 設定ファイルによるキーバインド定義
  - [ ] ランタイム再マップ

#### サウンド
- [ ] XAudio2 初期化
- [ ] WAV / OGG / MP3 デコード
- [ ] サウンドリソース管理 (LoadSoundMem 相当)
- [ ] 再生制御 (Play, Stop, Pause, Resume)
- [ ] 音量・パン・再生速度
- [ ] BGM ストリーミング再生
- [ ] 3D サウンド (X3DAudio)
- [ ] オーディオミキサー (マスター / BGM / SE / Voice バス)
- [ ] エフェクトチェーン (リバーブ等)

**成果物:** 音が鳴り、操作可能な 2D ゲームが作れる

---

### フェーズ 3: 3D 描画エンジン（目安: 8-12 週）

**ゴール: PBR ベースの 3D レンダリング**

#### モデルローダー
- [ ] glTF 2.0 ローダー (cgltf / tinygltf)
  - [ ] メッシュ、マテリアル、テクスチャ
  - [ ] スケルタルアニメーション
  - [ ] モーフターゲット
- [ ] FBX ローダー (Assimp オプション)
- [ ] OBJ ローダー
- [ ] MV1 ローダー (DXLib 互換、オプション)

#### PBR レンダリング
- [ ] Cook-Torrance BRDF 実装
  - [ ] GGX 法線分布
  - [ ] Smith-GGX ジオメトリ関数
  - [ ] Fresnel (Schlick 近似)
- [ ] PBR マテリアルシステム
  - [ ] Albedo, Normal, Metallic, Roughness, AO マップ
  - [ ] Emissive マップ
- [ ] ライティング
  - [ ] Directional Light
  - [ ] Point Light (減衰付き)
  - [ ] Spot Light
  - [ ] 複数ライト対応 (Forward+ or Deferred)
- [ ] IBL (Image Based Lighting)
  - [ ] 環境キューブマップ
  - [ ] Irradiance Map
  - [ ] Pre-filtered Specular Map
  - [ ] BRDF LUT

#### シャドウ
- [ ] CSM (Cascaded Shadow Maps) 実装
  - [ ] 分割数設定
  - [ ] PCF フィルタリング
  - [ ] VSM オプション
- [ ] ポイントライトシャドウ (キューブマップ)
- [ ] スポットライトシャドウ

#### カメラ
- [ ] Perspective / Orthographic
- [ ] FPS / TPS / フリーカメラ
- [ ] カメラ振動 (スクリーンシェイク)

#### スカイボックス
- [ ] キューブマップスカイボックス
- [ ] HDR 環境マップ
- [ ] プロシージャル空 (Preetham / Hosek-Wilkie)

#### その他 3D
- [ ] フォグ (線形、指数、ボリューメトリック)
- [ ] 3D プリミティブ描画 (球、箱、円柱等)
- [ ] インスタンシング描画
- [ ] LOD システム
- [ ] 地形 (ハイトマップ + テッセレーション)

**成果物:** PBR lit シーンが描画でき、モデルがアニメーションする

---

### フェーズ 4: ポストエフェクトパイプライン（目安: 6-8 週）

**ゴール: 全ポストエフェクトが動作し、設定ファイルで制御可能**

- [ ] PostEffectPipeline フレームワーク
  - [ ] エフェクトチェーン管理
  - [ ] 中間 RT の自動管理
  - [ ] 設定ファイル (JSON) による有効/無効切替
  - [ ] エフェクトパラメータのランタイム変更
- [ ] Bloom
  - [ ] 輝度抽出
  - [ ] Dual Kawase ダウンサンプリング
  - [ ] アップサンプリング + 合成
- [ ] Tonemapping
  - [ ] ACES Filmic
  - [ ] Reinhard
  - [ ] AgX
  - [ ] 自動露出 (Eye Adaptation)
- [ ] SSAO (GTAO)
  - [ ] 法線 + 深度からの AO 計算
  - [ ] ブラー + 適用
- [ ] 被写界深度 (DoF)
  - [ ] CoC (Circle of Confusion) 計算
  - [ ] Bokeh シミュレーション
- [ ] モーションブラー
  - [ ] Velocity Buffer 生成
  - [ ] ブラー適用
- [ ] カラーグレーディング
  - [ ] 3D LUT 適用
  - [ ] LUT ベイク・ブレンド
- [ ] アンチエイリアス
  - [ ] FXAA 3.11
  - [ ] TAA (Temporal Anti-Aliasing)
- [ ] その他
  - [ ] ビネット
  - [ ] 色収差 (Chromatic Aberration)
  - [ ] SSR (Screen Space Reflections)
  - [ ] ボリューメトリックライト
  - [ ] 輪郭線 (トゥーン向け)

**成果物:** 全エフェクトが ON/OFF でき、ビジュアルが劇的に向上

---

### フェーズ 5: 描画レイヤーシステム（目安: 3-4 週）

**ゴール: レイヤー単位での描画制御が完全動作**

- [ ] RenderLayer クラス
  - [ ] 個別 RenderTarget
  - [ ] Z-order ソート
  - [ ] ブレンドモード設定
  - [ ] 不透明度
  - [ ] カメラ参照 (パララックス)
- [ ] LayerStack 管理
  - [ ] レイヤー追加・削除・並び替え
  - [ ] レイヤーグループ
- [ ] LayerCompositor
  - [ ] レイヤー合成シェーダー
  - [ ] レイヤー単位 PostFX 適用制御
  - [ ] マスクレイヤー
- [ ] ステンシルマスクシステム
  - [ ] DXLib のマスク機能再現
  - [ ] 図形マスク、テクスチャマスク

**成果物:** 背景 → ゲーム → UI が独立レイヤーで管理され合成される

---

### フェーズ 6: XML-GUI システム（目安: 8-10 週）

**ゴール: XML でゲーム UI が構築・操作できる**

#### パーサー・データモデル
- [ ] XML パーサー (pugixml or RapidXML)
- [ ] スタイルシートパーサー (CSS-like)
- [ ] ウィジェットツリー構築
- [ ] プロパティ解決 (インライン > class > デフォルト)

#### レイアウトエンジン
- [ ] ボックスモデル (margin, border, padding)
- [ ] Flexbox ライクレイアウト (horizontal, vertical)
- [ ] Grid レイアウト
- [ ] Absolute positioning
- [ ] パーセント / ピクセル / auto サイジング
- [ ] アンカー / 整列

#### 描画
- [ ] ウィジェット描画 (角丸矩形、ボーダー、影)
- [ ] テキスト描画統合
- [ ] 画像表示
- [ ] グラデーション背景
- [ ] 9-slice / 9-patch

#### インタラクション
- [ ] ヒットテスト
- [ ] イベント伝搬 (バブリング / キャプチャ)
- [ ] フォーカス管理 (Tab ナビゲーション)
- [ ] ゲームパッド UI ナビゲーション
- [ ] ドラッグ & ドロップ

#### アニメーション
- [ ] プロパティアニメーション (position, color, opacity, scale)
- [ ] Easing 関数
- [ ] CSS transition 相当
- [ ] ページ遷移トランジション

#### ウィジェット実装
- [ ] 基底 Widget クラス
- [ ] Panel, Button, Text, Image
- [ ] TextInput (IME 対応)
- [ ] Slider, CheckBox, RadioButton
- [ ] DropDown, ListView, ScrollView
- [ ] ProgressBar, TabView, Dialog
- [ ] Canvas (カスタム描画)

#### C++ バインディング
- [ ] イベントバインド API
- [ ] 動的ウィジェット操作 (Find, Show, Hide, SetProperty)
- [ ] データバインディング (値の双方向同期)

**成果物:** XML + CSS でメニュー・HUD が構築でき、C++ でイベント処理可能

---

### フェーズ 7: ファイル・ネットワーク・動画（目安: 4-6 週）

**ゴール: 残りの DXLib 機能を網羅**

#### ファイル・アーカイブ
- [ ] ファイルシステム抽象化
  - [ ] 物理ファイル / アーカイブ の透過アクセス
  - [ ] マウントポイント
- [ ] カスタムアーカイブ
  - [ ] パッキングツール
  - [ ] AES-256 暗号化
  - [ ] 圧縮 (LZ4 / zstd)
- [ ] 非同期アセットローダー
  - [ ] バックグラウンドスレッドでのロード
  - [ ] プログレスコールバック
  - [ ] ロード完了イベント
- [ ] アセットホットリロード
  - [ ] ファイル監視 (ReadDirectoryChangesW)
  - [ ] テクスチャ / シェーダー / GUI のリロード

#### ネットワーク
- [ ] TCP ソケット (非同期 IOCP)
- [ ] UDP ソケット
- [ ] HTTP クライアント (WinHTTP)
- [ ] WebSocket クライアント

#### 動画
- [ ] Media Foundation ベース動画デコード
- [ ] テクスチャへのフレーム出力
- [ ] 再生制御 (Play, Stop, Seek, Pause)

**成果物:** 暗号化アーカイブからのアセット読込、HTTP通信、動画再生が動作

---

### フェーズ 8: 数学・物理・衝突判定（目安: 3-4 週）

**ゴール: ゲームロジックに必要な計算基盤**

- [ ] 数学ライブラリ (DirectXMath ラッパー)
  - [ ] Vector2, Vector3, Vector4
  - [ ] Matrix4x4
  - [ ] Quaternion
  - [ ] Color (RGBA, HSV 変換)
  - [ ] MathUtil (Lerp, SmoothStep, Remap, Clamp)
  - [ ] 乱数 (メルセンヌ・ツイスタ / PCG)
- [ ] 衝突判定
  - [ ] 2D: AABB, Circle, Polygon, Line
  - [ ] 3D: AABB, Sphere, Ray, Frustum, OBB
  - [ ] Sweep / Continuous Detection
- [ ] 空間分割
  - [ ] Quadtree (2D)
  - [ ] Octree (3D)
  - [ ] BVH
- [ ] 簡易 2D 物理
  - [ ] リジッドボディ
  - [ ] コリジョンレスポンス
  - [ ] トリガー判定
- [ ] Jolt Physics 統合 (3D、オプション)

**成果物:** DXLib の全数学・衝突判定関数 + 空間分割 + 簡易物理

---

### フェーズ 9: DXLib 互換レイヤー + シェーダーホットリロード（目安: 3-4 週）

**ゴール: 既存 DXLib コードの移植を容易にする**

- [ ] DxLibCompat.h
  - [ ] DXLib の全パブリック関数に対応するラッパー
  - [ ] 型変換 (COLOR_U8 → GX::Color 等)
  - [ ] 定数マッピング (DX_BLENDMODE_ALPHA → GX::BlendMode::Alpha 等)
- [ ] 移行ガイドドキュメント
- [ ] シェーダーホットリロード
  - [ ] ファイル監視 → DXC 再コンパイル → PSO 再生成
  - [ ] エラー表示 (コンパイルエラーをオーバーレイ表示)
- [ ] シェーダーバリアント管理
  - [ ] #define による条件分岐
  - [ ] バリアントキャッシュ

**成果物:** `#include "DxLibCompat.h"` で既存コードが動作する目処が立つ

---

### フェーズ 10: 最適化・品質・ドキュメント（目安: 4-6 週）

**ゴール: プロダクション品質**

#### パフォーマンス最適化
- [ ] メモリアロケータ (プール、スタック、リニア)
- [ ] リソースバリア最適化
- [ ] マルチスレッドコマンドリスト記録
- [ ] GPU タイムスタンプ プロファイラ
- [ ] 描画コールバッチング最適化
- [ ] テクスチャストリーミング

#### 品質
- [ ] バリデーションレイヤー (D3D12 Debug Layer 統合)
- [ ] GPU ベース回帰テスト (スクリーンショット比較)
- [ ] ユニットテスト (Google Test)
- [ ] メモリリーク検出

#### ドキュメント
- [ ] API リファレンス (Doxygen)
- [ ] チュートリアル (Getting Started → 2Dゲーム → 3Dゲーム → GUI)
- [ ] サンプルプロジェクト群
  - [ ] 2D シューティング
  - [ ] 2D プラットフォーマー
  - [ ] 3D ウォークスルー
  - [ ] GUI メニューデモ
  - [ ] ポストエフェクトショーケース
- [ ] DXLib 移行ガイド

**成果物:** ドキュメント完備、サンプル動作、プロファイラ動作

---

## 5. 全体スケジュール概算

| フェーズ | 内容 | 目安期間 | 累計 |
|---------|------|---------|------|
| Phase 0 | D3D12 基盤 + Hello Triangle | 4-6 週 | ~6 週 |
| Phase 1 | 2D 描画エンジン | 6-8 週 | ~14 週 |
| Phase 2 | テキスト・入力・サウンド | 4-6 週 | ~20 週 |
| Phase 3 | 3D 描画 (PBR) | 8-12 週 | ~30 週 |
| Phase 4 | ポストエフェクト | 6-8 週 | ~38 週 |
| Phase 5 | 描画レイヤー | 3-4 週 | ~42 週 |
| Phase 6 | XML-GUI | 8-10 週 | ~50 週 |
| Phase 7 | ファイル・ネット・動画 | 4-6 週 | ~56 週 |
| Phase 8 | 数学・物理 | 3-4 週 | ~60 週 |
| Phase 9 | 互換レイヤー + ホットリロード | 3-4 週 | ~64 週 |
| Phase 10 | 最適化・品質・ドキュメント | 4-6 週 | ~68 週 |

**総見積: 約 60-70 週（1年〜1年3ヶ月）**

※ 個人開発・フルタイム相当の場合。パートタイムなら 1.5〜2 倍。

---

## 6. 技術的リスクと対策

| リスク | 影響 | 対策 |
|--------|------|------|
| D3D12 の複雑さ (リソースバリア、ヒープ管理) | 開発遅延、バグ多発 | Phase 0 で徹底的に基盤を固める。D3D12MA (メモリアロケータ) 活用 |
| シェーダーコンパイル時間 | 開発効率低下 | DXC + キャッシュ + ホットリロードを早期実装 |
| glTF/FBX のエッジケース | モデル読込バグ | cgltf (軽量) を基本にし、Assimp はフォールバック |
| GUI のレイアウト複雑化 | 実装量膨大 | Flexbox のみ先行実装。Grid は後回し |
| パフォーマンスボトルネック | フレームレート低下 | GPU プロファイラを Phase 0 から組み込み |
| スコープクリープ | 完成しない | フェーズ毎に「最小限動くもの」を作る。機能追加は後から |

---

## 7. 開発ルール

1. **フェーズ毎にビルド可能**: 各フェーズ完了時にコンパイル・動作するサンプルがある
2. **テスト駆動**: 数学・衝突判定は必ずユニットテスト
3. **ドキュメント先行**: 各クラスの public API は実装前にヘッダーで設計
4. **Git ブランチ戦略**: `main` (安定) / `develop` (統合) / `feature/*` (機能別)
5. **命名規則**:
   - 名前空間: `GX::` (Graphics eXtended)
   - クラス: PascalCase (`SpriteBatch`)
   - メソッド: PascalCase (`DrawSprite`)
   - メンバ変数: `m_camelCase`
   - 定数: `k_PascalCase`
6. **エラーハンドリング**: HRESULT チェック + 構造化ログ、致命的エラーはアサート

---

## 8. 次のアクション

**Phase 0 から着手する。具体的な最初のステップ:**

1. CMake プロジェクトを作成 (`GXLib` ライブラリ + `Sandbox` テストアプリ)
2. Win32 ウィンドウを表示
3. D3D12 デバイスを初期化し、画面をクリアカラーで塗りつぶす
4. 三角形を描画

> 準備ができたら Phase 0 の実装に入ろう。


---

# Part II: 用語集 (Glossary)

GXLib のドキュメントやAPIリファレンスに登場する専門用語をまとめています。
初めて見る用語があれば、このページで意味を確認してください。

---

## DirectX 12 / GPU 基礎

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| **D3D12** | Direct3D 12 | Microsoft の 3D グラフィックス API。GPU を直接制御して高速な描画を実現します。GXLib の描画基盤です。 |
| **GPU** | Graphics Processing Unit | グラフィックス処理専用のプロセッサ。描画計算を CPU の代わりに高速実行します。 |
| **DXGI** | DirectX Graphics Infrastructure | GPU やモニターなどのグラフィックスハードウェアを管理する低レベル API です。 |
| **SwapChain** | スワップチェーン | 画面に表示する画像（バッファ）を2枚以上用意し、表示用と描画用を切り替える仕組みです。ちらつき防止（ダブルバッファリング）に使います。 |
| **CommandList** | コマンドリスト | GPU に実行させたい描画命令をまとめて記録するリストです。「何を描画するか」を書き込み、まとめて GPU に送信します。 |
| **CommandQueue** | コマンドキュー | CommandList を GPU に送信するための窓口です。記録した描画命令を実際に実行させます。 |
| **Fence** | フェンス | CPU と GPU の同期に使う仕組みです。GPU の処理完了を CPU 側で待つために使います。 |
| **DescriptorHeap** | デスクリプタヒープ | GPU がテクスチャやバッファにアクセスするための「目次」のようなものです。リソースの場所を GPU に教えます。 |
| **RTV** | Render Target View | 描画先（画面やテクスチャ）を GPU に示すためのビュー（見方）です。「ここに描画してね」という指定です。 |
| **DSV** | Depth Stencil View | 奥行き情報（深度バッファ）への参照です。3D で物体の前後関係を判定するために使います。 |
| **SRV** | Shader Resource View | シェーダーがテクスチャやバッファを「読み取る」ための参照です。 |
| **UAV** | Unordered Access View | シェーダーがバッファを「読み書き」するための参照です。SRV と違い、書き込みもできます。 |
| **CBV** | Constant Buffer View | 定数バッファ（毎フレーム変わるカメラ位置や行列などのデータ）への参照です。 |
| **PSO** | Pipeline State Object | 描画に必要な設定（シェーダー、ブレンドモード、深度テスト等）をひとまとめにしたオブジェクトです。描画の「レシピ」のようなものです。 |
| **Root Signature** | ルートシグネチャ | シェーダーが使うリソース（テクスチャ、定数バッファ等）の配置を定義する設計図です。 |
| **HLSL** | High-Level Shading Language | GPU 上で動くプログラム（シェーダー）を書くための言語です。C/C++ に似た文法を持ちます。 |
| **DXR** | DirectX Raytracing | DirectX 12 のレイトレーシング拡張。光線を飛ばしてリアルな反射や影を計算します。 |
| **BLAS** | Bottom-Level Acceleration Structure | レイトレーシングで使う加速構造の下位レベル。個々のメッシュの形状データを格納します。 |
| **TLAS** | Top-Level Acceleration Structure | レイトレーシングで使う加速構造の上位レベル。シーン全体のオブジェクト配置を管理します。 |
| **State Object** | ステートオブジェクト | DXR のレイトレーシングパイプラインを定義するオブジェクト。RayGen / Miss / ClosestHit シェーダーを含みます。 |
| **Shader Table** | シェーダーテーブル | レイトレーシングで各シェーダーのエントリポイントとパラメータをまとめたテーブルです。 |

## グラフィックス技術

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| **PBR** | Physically Based Rendering (物理ベースレンダリング) | 現実の光の振る舞いを物理法則に基づいてシミュレートする描画手法です。リアルな質感を表現できます。 |
| **BRDF** | Bidirectional Reflectance Distribution Function | 光が物体表面でどのように反射するかを定義する数学関数です。PBR の核となる概念です。 |
| **Albedo** | アルベド（基本色） | 物体の素の色です。光の影響を除いた純粋な表面の色を指します。 |
| **Metallic** | メタリック（金属度） | 物体が金属か非金属かを 0〜1 で指定します。0 = プラスチック等、1 = 金属です。 |
| **Roughness** | ラフネス（粗さ） | 表面のざらつき度合いを 0〜1 で指定します。0 = 鏡のようにツルツル、1 = ざらざらです。 |
| **Emissive** | エミッシブ（発光色） | 物体自身が光る色です。ネオンサインや溶岩のような自己発光を表現します。 |
| **HDR** | High Dynamic Range (高ダイナミックレンジ) | 通常の 0〜1 の色範囲を超えた明るさを扱う技術。太陽光のような非常に明るい光を正しく表現できます。 |
| **LDR** | Low Dynamic Range (低ダイナミックレンジ) | 通常の 0〜255 (0〜1) の色範囲。最終的にモニターに表示される色範囲です。 |
| **Tonemap** | トーンマッピング | HDR の広い明るさ範囲を、モニターで表示可能な LDR に変換する処理です。写真の露出補正に似ています。 |
| **Bloom** | ブルーム | 明るい部分から光がにじみ出す効果。太陽や電球の周りに光の輪が見える現象を再現します。 |
| **SSAO** | Screen Space Ambient Occlusion (環境遮蔽) | 物体の隅や隙間に自然な影を追加する技術。角や溝が暗くなり、立体感が増します。 |
| **SSR** | Screen Space Reflections (スクリーン空間反射) | 画面に映っている情報を使って反射を計算する技術。水面や光沢のある床の映り込みを表現します。 |
| **TAA** | Temporal Anti-Aliasing (テンポラルアンチエイリアシング) | 複数フレームの情報を組み合わせてジャギー（ギザギザ）を滑らかにする技術です。 |
| **FXAA** | Fast Approximate Anti-Aliasing (高速近似アンチエイリアシング) | 画像処理で素早くジャギーを軽減するポストエフェクトです。TAA より軽量ですが精度は低めです。 |
| **DoF** | Depth of Field (被写界深度) | カメラのピントが合っている範囲以外をぼかす効果。写真のボケ味を再現します。 |
| **CSM** | Cascaded Shadow Maps (カスケードシャドウマップ) | 広い範囲の影を効率的に描画する技術。カメラからの距離に応じて影の解像度を変えます。 |
| **PCF** | Percentage Closer Filtering | 影の境界を滑らかにするフィルタリング技術。ジャギーのないソフトな影を作ります。 |
| **FOV** | Field of View (視野角) | カメラが見える範囲の角度。人間の目は約 120°ですが、ゲームでは 60°〜90° が一般的です。 |
| **MotionBlur** | モーションブラー（動きボケ） | カメラや物体が速く動いた時に残像が出る効果。スピード感を演出します。 |
| **VolumetricLight** | ボリュームライト（ゴッドレイ） | 光の筋が空間中に見える効果。森の木漏れ日や窓から差し込む光線を表現します。 |
| **ColorGrading** | カラーグレーディング | 映像全体の色調を調整する処理。映画のような雰囲気を演出できます。 |
| **AutoExposure** | 自動露出調整 | 暗いシーンでは明るく、明るいシーンでは暗く自動調整する機能。人間の目の順応を再現します。 |
| **Render Target** | レンダーターゲット (RT) | 描画結果を書き込むテクスチャです。画面に直接描画する代わりに、このテクスチャに描画してから加工できます。 |
| **RTGI** | Ray Traced Global Illumination | DXR を使ったグローバルイルミネーション。間接光を計算してシーンの明るさをリアルにします。GXLib では半解像度 + テンポラル蓄積 + A-Trous フィルタで実装しています。 |
| **Toon Shading** | トゥーンシェーディング（セルシェーディング） | アニメや漫画のような見た目を再現する描画手法。明暗をくっきり分けて平面的に見せます。 |
| **UTS2** | Unity Toon Shader 2.0 | Unity が公開したトゥーンシェーダーの仕様。GXLib の Toon シェーダーはこの仕様をベースに実装しています（ダブルシェード3ゾーン、リムライト、ハイカラー等）。 |
| **ShaderModel** | シェーダーモデル | GXLib で定義されたマテリアルの描画方式の種類（PBR, Toon, Phong, Unlit, Subsurface, ClearCoat）。ShaderRegistry が自動的に対応する PSO を選択します。 |
| **BlendTree** | ブレンドツリー | 複数のアニメーションをパラメータ（速度や方向など）に基づいて自動合成する仕組み。1D（1パラメータ）と 2D（2パラメータ）をサポートしています。 |
| **A-Trous Filter** | ア・トゥルーフィルタ | ウェーブレットベースの空間フィルタ。RTGI のノイズ除去に使用し、少ない反復で広範囲をぼかせます。 |
| **GI** | Global Illumination (グローバルイルミネーション) | 間接光を含むシーン全体の照明を計算する技術。直接光だけでなく、壁や床からの反射光も考慮してリアルな明るさを再現します。 |
| **IBL** | Image-Based Lighting (画像ベース照明) | 環境マップ（HDR 画像）を光源として使う照明手法。周囲の景色が物体に映り込み、リアルな環境反射を実現します。GXLib では拡散/鏡面の 2 パスで実装しています。 |
| **LOD** | Level of Detail (詳細度レベル) | カメラからの距離に応じてモデルの詳細度を切り替える最適化技術。遠くの物体は粗いモデルで描画し、GPU 負荷を下げます。 |
| **ExecuteIndirect** | 間接描画 | GPU 駆動の描画呼び出し。CPU がコマンドバッファに描画引数を書き込み、GPU が一括実行する。DrawCall オーバーヘッドを大幅に削減できます。 |
| **Async Compute** | 非同期コンピュート | Graphics キューと並行して Compute Shader を実行する技術。ポストエフェクトやパーティクル更新を Graphics と同時に処理し、GPU 稼働率を向上させます。 |
| **Root Motion** | ルートモーション | アニメーションのルートボーン（腰や重心）の移動をキャラクターの実際の移動に反映する技術。アニメーションが移動距離を直接決定するため、足が滑らない自然な動きになります。 |
| **Animation Event** | アニメーションイベント | アニメーションの特定フレームに埋め込まれたコールバック情報。足音エフェクトや攻撃判定の発生タイミングをアニメーションに同期できます。 |
| **Tilemap** | タイルマップ | 小さなタイル画像をグリッド状に並べて広い 2D マップを構成する手法。RPG やプラットフォーマーの背景に広く使われます。 |
| **TMX** | Tiled Map XML | [Tiled Map Editor](https://www.mapeditor.org/) が出力する XML 形式のタイルマップデータ。GXLib の Tilemap クラスがパース対応しています。 |

## アニメーション

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| **IK** | Inverse Kinematics (逆運動学) | 目標位置から関節角度を逆算する技術。足を地面に接地させたり、頭を目標に向けたりする用途で使います。 |
| **FK** | Forward Kinematics (順運動学) | 関節角度から先端位置を順方向に計算する通常のアニメーション方式。IK の逆の操作です。 |
| **Skinning** | スキニング | スケルトン（骨格）の動きに応じてメッシュの頂点を変形させる処理。キャラクターアニメーションの基本技術です。 |
| **Cross-Fade** | クロスフェード | あるアニメーションから別のアニメーションへ滑らかに切り替える補間処理。ブレンドレートを時間で変化させます。 |
| **Blend Stack** | ブレンドスタック | 複数のアニメーションレイヤーを重ね合わせる仕組み。Override（上書き）と Additive（加算）の 2 モードで最大 8 レイヤーをサポートします。 |

## ゲームプレイ・AI

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| **ECS** | Entity-Component-System | ゲームオブジェクトを「エンティティ（ID）」「コンポーネント（データ）」「システム（ロジック）」に分離する設計パターン。GXLib では Unity 風の Entity+Component 構成を採用しています。 |
| **A*** | A-star (エースター) | 最短経路を効率的に探索するアルゴリズム。NavMesh 上でのキャラクター移動経路を計算します。ヒューリスティック関数で探索範囲を限定して高速化します。 |
| **NavMesh** | Navigation Mesh (ナビゲーションメッシュ) | AI キャラクターが歩行可能な領域をメッシュで表現したもの。GXLib ではグリッドベースの A* 実装を採用しています。 |
| **Lua** | — | 軽量なスクリプト言語。ゲームロジックを C++ から分離してホットリロードやモディング対応を実現します。GXLib では Lua 5.4 + sol2 で統合しています。 |

## 空間・物理・数学

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| **AABB** | Axis-Aligned Bounding Box (軸平行境界ボックス) | 座標軸に沿った長方形（3D では直方体）。物体を囲む最も単純な当たり判定領域です。回転しません。 |
| **OBB** | Oriented Bounding Box (有向境界ボックス) | 任意の角度で回転できる境界ボックス。AABB より正確に物体を囲めますが、計算が重くなります。 |
| **BVH** | Bounding Volume Hierarchy (境界ボリューム階層) | 当たり判定を高速化するツリー構造。物体をグループ分けして、不要な判定をスキップします。 |
| **SAH** | Surface Area Heuristic | BVH のツリーを効率的に構築するためのアルゴリズム。表面積に基づいて分割方法を決めます。 |
| **SAT** | Separating Axis Theorem (分離軸定理) | 2つの凸多角形が重なっているかを判定する数学的手法。すべての軸で分離できなければ衝突です。 |
| **CoC** | Circle of Confusion (錯乱円) | DoF（被写界深度）の計算で使う値。ピントがずれた点がどれだけぼけるかを表す円の大きさです。 |
| **Quadtree** | 四分木 | 2D 空間を4分割して管理するツリー構造。広い空間での当たり判定を高速化します。 |
| **Octree** | 八分木 | 3D 空間を8分割して管理するツリー構造。Quadtree の 3D 版です。 |
| **Quaternion** | クォータニオン（四元数） | 3D 回転を表現する数学的な方法。ジンバルロック（回転の不具合）が起きないため、3D ゲームで広く使われます。 |
| **deltaTime** | デルタタイム（フレーム間隔時間） | 前フレームから現在フレームまでの経過時間（秒）。これを掛けることで、FPS に依存しない一定速度の動きを実現します。 |

## その他

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| **DIK** | DirectInput Key codes | DirectInput で定義されたキーコード。`KEY_INPUT_SPACE` のような定数で、DXLib 互換 API で使います。 |
| **Flexbox** | Flexible Box Layout | Web の CSS で使われるレイアウト方式。GXLib の GUI でも採用しており、要素の並び方向・配置・間隔を柔軟に制御できます。 |
| **glTF** | GL Transmission Format | 3D モデルの標準フォーマット。メッシュ、マテリアル、アニメーションなどを1つのファイルにまとめられます。GXLib の 3D モデル形式です。 |
| **GXMD** | GXLib Model Data | GXLib 独自のバイナリモデル形式。glTF/FBX/OBJ から gxconv で変換し、gxloader で高速に読み込みます。 |
| **GXAN** | GXLib Animation Data | GXLib 独自のバイナリアニメーション形式。スケルタルアニメーションデータを格納します。 |
| **GXPAK** | GXLib Package | 複数のアセットファイルを LZ4 圧縮してまとめたバンドル形式。PakFileProvider で VFS に統合できます。 |
| **VFS** | Virtual File System (仮想ファイルシステム) | 実ファイルとアーカイブを統一的に扱えるファイルアクセス層。ゲーム配布時にアセットをアーカイブ化しても、コードを変更せずに読み込めます。 |
| **ImGui** | Dear ImGui | 即時モード GUI ライブラリ。GXModelViewer の UI フレームワークとして使用しています。Docking ブランチ対応。 |
| **ufbx** | — | 軽量な FBX パーサーライブラリ。gxconv の FBX インポーターで使用しています。 |
| **XAudio2** | — | Microsoft のオーディオ API。GXLib のサウンド再生基盤です。SE やBGM の再生に使います。 |
| **DirectWrite** | — | Microsoft のテキストレンダリング API。高品質な文字描画に使います。GXLib のフォントシステムの基盤です。 |
| **Compat** | Compatibility (互換性) | GXLib の DXLib 互換レイヤー。DXLib と同じ関数名・引数で使えるラッパーAPIです。 |
| **PCH** | Precompiled Header (プリコンパイル済みヘッダー) | よく使うヘッダーを事前にコンパイルしておき、ビルド時間を短縮する仕組みです。GXLib では `pch.h` がこれにあたります。 |
| **CMake** | — | C++ プロジェクトのビルド設定を管理するツール。`CMakeLists.txt` にビルドルールを書き、Visual Studio のプロジェクトファイルを自動生成できます。 |
| **WAV** | Waveform Audio File Format | 非圧縮の音声ファイル形式。GXLib のサウンドシステムで対応しています。 |
| **Jolt Physics** | — | オープンソースの 3D 物理エンジン。GXLib の 3D 物理シミュレーション（衝突応答、剛体運動）に使用しています。 |

---

> **用語が見つからない場合**: [API リファレンス](index.html) の各クラス説明にも用語の補足があります。
> それでも不明な場合は、GitHub Issues でお気軽にご質問ください。


---

# Part III: チュートリアル

## Tutorial 00: Prerequisites

このガイドでは、GXLib を使い始めるにあたって必要な知識と、あると便利な知識を整理しています。
「自分に使えるか不安」という方は、まずここを確認してください。

## 必須知識

以下の知識がないと、チュートリアルの内容を理解するのが難しくなります。

### C++ 基礎

GXLib は C++ で書かれたライブラリです。以下の知識が必要です:

- **変数、関数、if/for/while** — 基本的なプログラミング構文
- **構造体 (`struct`) とクラス (`class`)** — `object.Method()` の形で使います
- **ポインタと参照** — `*` や `&` の意味がわかること
- **ヘッダーファイル (`#include`)** — ファイルの分割と読み込み

> **どこで学べる？**
> - [C++ 入門 (cpprefjp)](https://cpprefjp.github.io/) — 日本語の C++ リファレンス
> - 書籍「ロベールのC++入門講座」「新・明解C++入門」なども定番です

### Windows 基本操作

- **コマンドプロンプト** (または PowerShell, Terminal) の起動方法
- コマンドの実行方法（`cd`, `dir` 程度の基本操作）
- **Visual Studio 2022** のインストールと、ソリューション (.sln) の開き方

### CMake 基本操作

GXLib のビルドに CMake を使います。必要なのは以下の2つのコマンドだけです:

```bash
cmake -B build -S .           # ビルドファイルの生成
cmake --build build --config Debug  # ビルドの実行
```

> **CMake とは？**
> C++ プロジェクトのビルド設定を管理するツールです。
> `CMakeLists.txt` というファイルにビルドルールを書くと、
> Visual Studio のプロジェクトファイル (.sln) を自動で生成してくれます。
>
> 詳しくは [CMake 公式チュートリアル](https://cmake.org/cmake/help/latest/guide/tutorial/) を参照してください。

## あると望ましい知識

以下は知っていると理解が深まりますが、なくてもチュートリアルは進められます。

### Win32 API の基礎

GXLib のプログラムは `WinMain` から始まります。

```cpp
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
```

> **WinMain とは？**
> Windows のデスクトップアプリケーション（コンソールでない）のエントリーポイント（開始点）です。
> 通常の C++ プログラムは `main()` から始まりますが、
> ウィンドウを持つアプリケーションは `WinMain()` から始まります。
>
> 引数の意味:
> - `HINSTANCE hInstance` — アプリケーションのインスタンスハンドル（識別子）
> - 2つ目の `HINSTANCE` — 昔の Windows で使われていた引数（常に NULL、無視して OK）
> - `LPSTR` — コマンドライン引数（文字列）
> - `int` — ウィンドウの表示方法（通常は無視して OK）

### 2D ゲームの座標系

GXLib の 2D 座標系は以下のようになっています:

```
(0, 0) ──────→ X 軸（右が正）
│
│
↓
Y 軸（下が正）
```

- 画面の左上が原点 `(0, 0)`
- 右に行くほど X が増える
- **下に行くほど Y が増える**（数学の座標系とは逆）

これは Windows、DirectX、ほとんどの 2D ゲームエンジンで共通のルールです。

### CSS の基礎（GUI チュートリアル向け）

[05_GUI.md](05_GUI.md) では CSS ライクなスタイル指定を使います。
Web 開発で CSS を触ったことがあれば、すぐに馴染めます。

知っていると役立つ概念:
- **セレクタ** — `#id` や `.class` でスタイル適用先を指定
- **Flexbox** — 要素の並び方向（row / column）や配置を制御するレイアウト方式
- **プロパティ** — `width`, `height`, `padding`, `margin`, `color` 等

> CSS を知らなくても、チュートリアルのサンプルをコピー＆修正して進められます。

## 知らなくて大丈夫なこと

以下の知識は **GXLib が内部で処理するため、ユーザーが直接扱う必要はありません**。
特に DXLib 互換 API (Compat) を使う場合は、意識する必要はほぼありません。

| 知識 | なぜ不要か |
|------|-----------|
| Direct3D 12 の詳細 | Compat API が内部で管理。ネイティブ API を使う場合も GXLib が大部分をラップしています |
| GPU プログラミング (HLSL) | シェーダーは GXLib に組み込み済み。カスタムシェーダーを書く場合のみ必要です |
| Win32 ウィンドウメッセージループ | `Application` クラスが自動管理します |
| COM (Component Object Model) | DirectX の内部で使われますが、GXLib が隠蔽しています |
| スレッド / 非同期処理 | AsyncLoader などが内部で管理。基本的な使い方では不要です |

## 2つの API レベル

GXLib には2種類の API があります。自分のレベルに合った方を選んでください:

### Compat API（初心者向け）

DXLib 互換の簡易 API です。`#include "Compat/GXLib.h"` で使えます。

```cpp
// 画像を読み込んで描画 — これだけで動きます
int tex = LoadGraph("player.png");
DrawGraph(100, 200, tex, TRUE);
```

**向いている人**: C++ の基本は知っているが、ゲームエンジンやグラフィックスAPIは初めての方

### ネイティブ API（中〜上級者向け）

GXLib の全機能にアクセスできる API です。より細かい制御が可能です。

```cpp
// SpriteBatch による描画 — 自由度が高い
spriteBatch.Begin(cmdList, frameIndex);
spriteBatch.Draw(texHandle, 100.0f, 200.0f);
spriteBatch.End();
```

**向いている人**: DirectX や他のゲームエンジンの経験がある方、描画パイプラインを制御したい方

## 次のステップ

環境が整ったら、チュートリアルを始めましょう:

1. [01_GettingStarted.md](01_GettingStarted.md) — プロジェクトのビルドとウィンドウ表示
2. [02_Drawing2D.md](02_Drawing2D.md) — スプライト・図形・テキストの描画
3. [03_Game2D.md](03_Game2D.md) — 入力・サウンド・衝突判定
4. [04_Rendering3D.md](04_Rendering3D.md) — 3D 描画・PBR・ライティング
5. [05_GUI.md](05_GUI.md) — XML + CSS による GUI 構築

用語がわからなくなったら → [用語集 (Glossary)](../Glossary.md)

---

## よくある問題（環境構築トラブルシューティング）

### Visual Studio のインストール関連

**症状**: CMake で `No CMAKE_CXX_COMPILER could be found` と出る

**対処法**:
1. Visual Studio Installer を開き、「C++ によるデスクトップ開発」ワークロードがチェックされているか確認
2. 個別のコンポーネントで「MSVC v143 - VS 2022 C++ x64/x86 ビルドツール」が入っているか確認
3. インストール後、コマンドプロンプトを再起動してから `cmake` を実行

**症状**: `Windows SDK not found` 系のエラー

**対処法**:
- Visual Studio Installer → 個別のコンポーネント → 「Windows 10 SDK (10.0.19041.0)」以上をインストール
- 複数バージョンの SDK がある場合、CMake が古いものを選ぶことがあります。`cmake -B build -S . -DCMAKE_SYSTEM_VERSION=10.0` で明示指定も可能です

### CMake 関連

**症状**: `cmake` コマンドが見つからない

**対処法**:
- CMake をインストールする際に「PATH に追加」オプションを有効にしたか確認
- PowerShell で `cmake --version` を実行して確認。表示されない場合は PATH 環境変数に `C:\Program Files\CMake\bin` を追加
- Visual Studio 同梱の CMake を使う場合は「Developer Command Prompt for VS 2022」から実行

**症状**: `cmake -B build -S .` で `CMakeLists.txt not found`

**対処法**:
- コマンドを実行しているディレクトリが GXLib のルートフォルダ（`CMakeLists.txt` がある場所）か確認
- `dir CMakeLists.txt` (コマンドプロンプト) または `ls CMakeLists.txt` (PowerShell) で存在を確認

### Git 関連

**症状**: `git clone` でエラーが出る

**対処法**:
- Git がインストールされているか確認: `git --version`
- インストールされていない場合は [git-scm.com](https://git-scm.com/) からダウンロード
- プロキシ環境の場合は `git config --global http.proxy http://proxy:port` でプロキシ設定

### GPU ドライバー関連

**症状**: 実行時に `D3D12 device creation failed` や `DXGI_ERROR_UNSUPPORTED` が出る

**対処法**:
- GPU ドライバーを最新版にアップデート（[NVIDIA](https://www.nvidia.com/drivers), [AMD](https://www.amd.com/en/support), [Intel](https://www.intel.com/content/www/us/en/download-center/home.html)）
- DirectX 12 対応 GPU が搭載されているか確認（Windows 10 以降の大半の GPU は対応）
- `dxdiag` コマンドで DirectX のバージョンと GPU 情報を確認

## Tutorial 01: Getting Started

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

---

## 補足: WinMain とダブルバッファリング

### WinMain の各引数について

```cpp
int WINAPI WinMain(
    HINSTANCE hInstance,    // 現在のアプリケーションのインスタンスハンドル
    HINSTANCE hPrevInstance,// 常に NULL（Win16 時代の名残、使わない）
    LPSTR     lpCmdLine,   // コマンドライン引数（文字列として渡される）
    int       nCmdShow     // ウィンドウの初期表示方法（通常は無視して OK）
)
```

通常の C++ プログラムの `int main(int argc, char* argv[])` とは異なり、Windows GUI アプリケーションは `WinMain` から開始します。GXLib の Compat API (`GX_Init()`) や Application クラスが内部でウィンドウの作成やメッセージループを処理するため、ユーザーが Win32 API を直接扱う必要はありません。

### ダブルバッファリングの仕組み

```
フレーム N:
  [裏画面(Back)]  ← 描画中（ClearDrawScreen → 描画 → ...）
  [表画面(Front)] ← モニターに表示中（前フレームの結果）

ScreenFlip() 実行:
  [裏画面(Back)]  ← 表画面だったものが次の描画先に
  [表画面(Front)] ← 裏画面だったものがモニターに表示される
```

GXLib 内部では DirectX 12 の SwapChain が 2 枚のバッファ (`k_BufferCount = 2`) を管理し、`Present()` で表裏を切り替えています。Compat API の `ScreenFlip()` はこの `Present()` のラッパーです。

### GXEasy.h の推奨

サンプルプロジェクトを書く際は `GXEasy.h` のインクルードを推奨します。

```cpp
#include "GXEasy.h"  // FormatT, TChar, TString, <format> を一括提供
```

`GXEasy.h` には `std::format` 関連のユーティリティ（`FormatT`、`TChar`、`TString`）が集約されており、各サンプルで個別にインクルードする手間を省けます。全サンプルプロジェクトはこのヘッダーだけで基本的な文字列フォーマットが利用可能です。

## Tutorial 02: 2D Drawing

スプライト、図形、テキストの描画方法を解説します。

## このチュートリアルで学ぶこと

- 画像（スプライト）の読み込みと描画
- スプライトシートによるアニメーション素材の管理
- 基本図形（線、四角形、円など）の描画
- ブレンドモード（半透明、加算合成）
- テキストの描画とフォント管理
- ネイティブ API による Camera2D / Animation2D

## 前提知識

- [01_GettingStarted.md](01_GettingStarted.md) の内容（ビルドと Hello Window）
- 2D 座標系の理解（左上が原点、右が X+、下が Y+）

→ 座標系について詳しくは [00_Prerequisites.md](00_Prerequisites.md) を参照

## スプライト描画

### 画像の読み込みと描画

```cpp
// 画像ファイルをGPUメモリに読み込む
// 戻り値の texHandle は画像を識別する番号（ハンドル）
int texHandle = LoadGraph("Assets/player.png");

// 画像をそのまま描画
DrawGraph(
    100, 200,       // 描画位置の左上座標 (x=100, y=200)
    texHandle,      // 描画する画像のハンドル
    TRUE            // TRUE: 透過処理あり（PNG の透明部分を透過する）
);

// 拡大・回転して描画
DrawRotaGraph(
    640, 480,       // 描画中心座標 (x=640, y=480)（※左上ではなく中心）
    2.0,            // 拡大率 (1.0 = 等倍, 2.0 = 2倍拡大, 0.5 = 半分)
    0.5,            // 回転角度（ラジアン単位, 約28.6度。π ≒ 3.14 で180度）
    texHandle,      // 画像ハンドル
    TRUE            // 透過処理あり
);

// 指定範囲に伸縮して描画（引き伸ばし）
DrawExtendGraph(
    0, 0,           // 描画先の左上座標
    320, 240,       // 描画先の右下座標 → 320x240 の範囲に伸縮
    texHandle,      // 画像ハンドル
    TRUE            // 透過処理あり
);

// 画像の一部だけ切り出して描画（スプライトシート用）
DrawRectGraph(
    100, 100,       // 描画位置の左上座標
    0, 0,           // 画像内の切り出し開始位置 (左上からのオフセット)
    32, 32,         // 切り出すサイズ (幅32px × 高さ32px)
    texHandle,      // 画像ハンドル
    TRUE,           // 透過処理あり
    FALSE           // FALSE: 左右反転なし（TRUE にすると鏡像になる）
);

// 使い終わったら解放（GPUメモリを返却）
DeleteGraph(texHandle);
```

> **DrawGraph と DrawRotaGraph の座標の違いに注意**
>
> `DrawGraph` は **左上座標** を指定しますが、
> `DrawRotaGraph` は **中心座標** を指定します。
> 回転の中心を画像の中央にするためこのような設計になっています。

### スプライトシート（分割読み込み）

1枚の大きな画像に複数のキャラクターポーズが並んだ「スプライトシート」を
一度に読み込み、フレーム番号で簡単に描画できます。

```cpp
int sprites[16];  // 16コマ分のハンドルを格納する配列

LoadDivGraph(
    "Assets/character.png",  // スプライトシート画像ファイル
    16,     // 総コマ数 (= 横分割数 × 縦分割数)
    4, 4,   // 横に4コマ, 縦に4コマの格子状に分割
    32, 32, // 1コマのサイズ (幅32px × 高さ32px)
    sprites // ハンドルの格納先配列
);

// フレーム番号を指定して描画（0〜15）
// アニメーションは frameIndex を時間経過で変えるだけ
DrawGraph(x, y, sprites[frameIndex], TRUE);
```

### ネイティブ API（SpriteBatch）

ネイティブ API では `SpriteBatch` クラスを使います。
より細かい制御（ソース矩形、スケール、回転、色の個別指定）が可能です。

```cpp
GX::SpriteBatch spriteBatch;
spriteBatch.Initialize(device, cmdList);

// --- フレームループ内 ---
spriteBatch.Begin(cmdList, frameIndex);
// Begin() と End() の間に描画命令を記録する

// シンプルな描画
spriteBatch.Draw(
    texHandle,          // テクスチャハンドル
    100.0f, 200.0f      // 描画位置 (x, y)
);

// フル指定の描画
spriteBatch.Draw(
    texHandle,                          // テクスチャハンドル
    100.0f, 200.0f,                     // 描画位置 (x, y)
    0.0f, 0.0f, 64.0f, 64.0f,          // ソース矩形: 画像内の切り出し範囲 (x, y, 幅, 高さ)
    2.0f, 2.0f,                         // スケール (横2倍, 縦2倍)
    0.5f,                               // 回転角度（ラジアン）
    1.0f, 1.0f, 1.0f, 1.0f             // カラー乗算 (R, G, B, A) 1.0=変化なし
);

spriteBatch.End();
// End() で実際に GPU 描画命令が発行される
```

## プリミティブ描画

### 基本図形

```cpp
// 直線
DrawLine(
    0, 0,                   // 始点 (x1, y1)
    100, 100,               // 終点 (x2, y2)
    GetColor(255, 0, 0),    // 色 (R=255, G=0, B=0 = 赤)
    2                       // 線の太さ（ピクセル）
);

// 矩形（四角形）
DrawBox(
    50, 50,                 // 左上 (x1, y1)
    200, 150,               // 右下 (x2, y2)
    GetColor(0, 255, 0),    // 色 (緑)
    TRUE                    // TRUE: 塗りつぶし, FALSE: 枠線のみ
);

// 円
DrawCircle(
    320, 240,               // 中心座標 (x, y)
    50,                     // 半径（ピクセル）
    GetColor(0, 0, 255),    // 色 (青)
    TRUE                    // TRUE: 塗りつぶし
);

// 三角形
DrawTriangle(
    100, 300,               // 頂点1 (x1, y1)
    200, 200,               // 頂点2 (x2, y2)
    300, 300,               // 頂点3 (x3, y3)
    GetColor(255, 255, 0),  // 色 (黄)
    TRUE                    // TRUE: 塗りつぶし
);

// 楕円
DrawOval(
    400, 300,               // 中心座標 (x, y)
    80, 40,                 // 横半径, 縦半径（横長の楕円）
    GetColor(255, 128, 0),  // 色 (オレンジ)
    TRUE                    // TRUE: 塗りつぶし
);
```

### ブレンドモード

ブレンドモードを使うと、半透明描画や光のエフェクトが実現できます。

```cpp
// --- アルファブレンド（半透明）---
SetDrawBlendMode(
    GX_BLENDMODE_ALPHA,     // アルファブレンド: 描画色と背景色を透明度で混合
    128                     // 不透明度 (0=完全に透明, 255=完全に不透明, 128=半透明)
);
DrawBox(100, 100, 300, 200, GetColor(255, 0, 0), TRUE);
SetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);  // ブレンドを元に戻す（忘れると後続の描画も半透明に！）

// --- 加算合成（光のエフェクト）---
// 色を「足し算」するので、重ねるほど明るくなります
SetDrawBlendMode(
    GX_BLENDMODE_ADD,       // 加算合成: 光源、炎、パーティクルに最適
    200                     // 強度 (0-255)
);
DrawCircle(320, 240, 100, GetColor(255, 255, 200), TRUE);
SetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);  // 必ず戻す
```

> **なぜブレンドモードを「戻す」必要があるのか？**
>
> `SetDrawBlendMode` はグローバルな状態を変更します。
> 戻さないと、以降の描画すべてにブレンドが適用されてしまいます。
> 半透明描画の直後に `GX_BLENDMODE_NOBLEND` を呼ぶのを習慣にしてください。

## テキスト描画

### デフォルトフォント

```cpp
// 基本テキスト描画
DrawString(
    10, 10,                         // 描画位置の左上 (x, y)
    "Score: 12345",                 // 表示テキスト
    GetColor(255, 255, 255)         // 文字色 (白)
);

// 書式付きテキスト（printf 形式で変数を埋め込める）
DrawFormatString(
    10, 40,                         // 描画位置
    GetColor(255, 255, 0),          // 文字色 (黄)
    "HP: %d / %d", currentHP, maxHP // %d に整数値が入る
);

// 文字列の描画幅を取得（テキスト中央揃えなどに使用）
int width = GetDrawStringWidth(
    "Hello",    // 測定するテキスト
    5           // 文字数
);
```

### カスタムフォント

```cpp
// フォントを作成
int font = CreateFontToHandle(
    "MS Gothic",    // フォント名（OS にインストールされているフォント）
    24,             // フォントサイズ（ピクセル）
    3               // 太さ (0=通常, 大きいほど太い)
);

// 作成したフォントを指定して描画
DrawStringToHandle(
    100, 100,                       // 描画位置 (x, y)
    "Title",                        // 表示テキスト
    GetColor(255, 255, 255),        // 文字色
    font                            // フォントハンドル
);

// 使い終わったら解放
DeleteFontToHandle(font);
```

> **フォントの日本語対応について**
>
> **Compat API** (簡易API) のテキスト関数は ASCII 文字 (英数字・記号) のみ対応しています。
> **ネイティブ API** の `TextRenderer` は Unicode をフルサポートしており、
> 日本語テキストも描画可能です。日本語を使いたい場合はネイティブ API を利用してください。
>
> ```cpp
> // ネイティブ API で日本語テキスト描画
> textRenderer.DrawString(fontHandle, 10, 10, L"こんにちは世界！", 0xFFFFFFFF);
> ```

## ネイティブ API の高度な機能

### Camera2D

2D カメラを使うと、「カメラ」の位置・ズーム・回転を変えることで
ゲームワールド全体を動かすことができます。プレイヤーの追従や画面のシェイクに使います。

```cpp
GX::Camera2D camera;
camera.SetPosition(100.0f, 200.0f);    // カメラの注視位置（ワールド座標）
camera.SetZoom(1.5f);                  // ズーム倍率 (1.0=等倍, 1.5=1.5倍拡大)
camera.SetRotation(0.1f);              // 回転角度 (ラジアン)

// カメラのビュー行列を SpriteBatch に適用
// これにより、全ての描画がカメラ基準に変換されます
spriteBatch.SetViewMatrix(camera.GetViewMatrix());
```

### SpriteSheet / Animation2D

スプライトシートとアニメーションを組み合わせると、
パラパラ漫画のようなキャラクターアニメーションが簡単に実装できます。

```cpp
GX::SpriteSheet sheet;
sheet.Initialize(
    texHandle,      // スプライトシートの画像ハンドル
    32, 32,         // 1コマのサイズ (幅32px, 高さ32px)
    4, 4            // グリッド (横4コマ × 縦4コマ = 計16コマ)
);

GX::Animation2D anim;
anim.AddFrames(
    sheet,          // スプライトシート
    0, 3,           // 使用するフレーム範囲 (0番〜3番の4コマ)
    0.1f            // 1コマの表示時間 (0.1秒 = 秒間10コマ)
);

// フレームループ内:
anim.Update(deltaTime);                     // 経過時間でアニメーション進行
anim.Draw(spriteBatch, 100.0f, 200.0f);     // 現在のフレームを描画
```

> **deltaTime（デルタタイム）とは？**
>
> 前フレームから現在フレームまでの経過時間（秒）です。
> これをアニメーション速度や移動速度に掛けることで、
> フレームレート (FPS) に依存しない一定速度の動作を実現できます。
> 詳しくは [03_Game2D.md](03_Game2D.md) の「フレームレート制御」を参照してください。

## よくある問題

### 画像が表示されない

- ファイルパスが正しいか確認（`Assets/` からの相対パス）
- `LoadGraph` の戻り値が -1 でないか確認（-1 は読み込み失敗）
- 対応画像形式: PNG, JPG, BMP, TGA

### 描画が全体的に半透明 / 暗い

- `SetDrawBlendMode` を呼んだ後、`GX_BLENDMODE_NOBLEND` に戻し忘れていないか確認
- `SetDrawBright` で明るさを変更した後、元に戻し忘れていないか確認

### DrawRotaGraph で位置がずれる

- `DrawRotaGraph` は **中心座標** を指定します（DrawGraph は左上座標）
- 画像の中心を (x, y) に合わせたい場合はそのまま使えます
- 左上を合わせたい場合は `(x + 幅/2, y + 高さ/2)` を指定してください

---

## 関数パラメータ一覧

### スプライト描画関数

#### DrawGraph

| パラメータ | 型 | 説明 |
|---|---|---|
| x | int | 描画位置の左上 X 座標 |
| y | int | 描画位置の左上 Y 座標 |
| grHandle | int | `LoadGraph` で取得した画像ハンドル |
| transFlag | int | `TRUE`: 透過処理あり（PNG の透明部分を透過）、`FALSE`: 不透明描画 |

#### DrawRotaGraph

| パラメータ | 型 | 説明 |
|---|---|---|
| x | int | 描画中心の X 座標（左上ではなく中心） |
| y | int | 描画中心の Y 座標 |
| extRate | double | 拡大率（1.0 = 等倍、2.0 = 2倍、0.5 = 半分） |
| angle | double | 回転角度（ラジアン、時計回り）。度数法との変換: ラジアン = 度 * PI / 180 |
| grHandle | int | 画像ハンドル |
| transFlag | int | 透過処理フラグ |

#### DrawExtendGraph

| パラメータ | 型 | 説明 |
|---|---|---|
| x1 | int | 描画先の左上 X 座標 |
| y1 | int | 描画先の左上 Y 座標 |
| x2 | int | 描画先の右下 X 座標 |
| y2 | int | 描画先の右下 Y 座標 |
| grHandle | int | 画像ハンドル |
| transFlag | int | 透過処理フラグ |

#### DrawRectGraph

| パラメータ | 型 | 説明 |
|---|---|---|
| destX | int | 描画位置の左上 X 座標 |
| destY | int | 描画位置の左上 Y 座標 |
| srcX | int | 画像内の切り出し開始 X 座標 |
| srcY | int | 画像内の切り出し開始 Y 座標 |
| width | int | 切り出す幅（ピクセル） |
| height | int | 切り出す高さ（ピクセル） |
| grHandle | int | 画像ハンドル |
| transFlag | int | 透過処理フラグ |
| turnFlag | int | `TRUE`: 左右反転描画、`FALSE`: 通常描画 |

#### LoadDivGraph

| パラメータ | 型 | 説明 |
|---|---|---|
| fileName | const char* | スプライトシート画像ファイルパス |
| allNum | int | 総コマ数（= xNum * yNum） |
| xNum | int | 横方向の分割数 |
| yNum | int | 縦方向の分割数 |
| xSize | int | 1コマの幅（ピクセル） |
| ySize | int | 1コマの高さ（ピクセル） |
| handleArray | int* | ハンドルを格納する配列（要素数 >= allNum） |

### プリミティブ描画関数

#### DrawLine

| パラメータ | 型 | 説明 |
|---|---|---|
| x1 | int | 始点 X |
| y1 | int | 始点 Y |
| x2 | int | 終点 X |
| y2 | int | 終点 Y |
| color | unsigned int | `GetColor(R, G, B)` で取得した色 |
| thickness | int | 線の太さ（ピクセル） |

#### DrawBox

| パラメータ | 型 | 説明 |
|---|---|---|
| x1 | int | 左上 X |
| y1 | int | 左上 Y |
| x2 | int | 右下 X |
| y2 | int | 右下 Y |
| color | unsigned int | 色 |
| fillFlag | int | `TRUE`: 塗りつぶし、`FALSE`: 枠線のみ |

#### DrawCircle

| パラメータ | 型 | 説明 |
|---|---|---|
| x | int | 中心 X |
| y | int | 中心 Y |
| r | int | 半径（ピクセル） |
| color | unsigned int | 色 |
| fillFlag | int | `TRUE`: 塗りつぶし、`FALSE`: 枠線のみ |

### ブレンドモード

#### SetDrawBlendMode

| パラメータ | 型 | 説明 |
|---|---|---|
| blendMode | int | `GX_BLENDMODE_NOBLEND` (ブレンドなし)、`GX_BLENDMODE_ALPHA` (半透明)、`GX_BLENDMODE_ADD` (加算合成) |
| blendParam | int | ブレンド強度 (0-255)。ALPHA: 不透明度、ADD: 加算強度 |

## 次のステップ

- [03_Game2D.md](03_Game2D.md) — 入力処理とサウンドを追加してゲームを作る
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認

## Tutorial 03: 2D Game

入力処理、サウンド、衝突判定を組み合わせて 2D ゲームを作る方法を解説します。

## このチュートリアルで学ぶこと

- キーボード・マウス・ゲームパッドによる入力処理
- 「押した瞬間」を検出するトリガー入力
- 効果音 (SE) と BGM の再生
- 2D 衝突判定（矩形、円）
- フレームレートに依存しない移動（deltaTime）

## 前提知識

- [02_Drawing2D.md](02_Drawing2D.md) の内容（描画の基本）
- スプライトの読み込みと描画ができること

## 入力処理

### キーボード

```cpp
// 特定キーが「押されている」かを判定（押し続けている間ずっと TRUE）
if (CheckHitKey(KEY_INPUT_SPACE))
{
    // スペースキー押下中（移動や連射に使う）
}

// 全キーの状態を一括取得（複数キーの同時判定に便利）
char keys[256];             // 256個のキーの状態を格納する配列
GetHitKeyStateAll(keys);    // keys[i] != 0 なら i番目のキーが押されている

if (keys[KEY_INPUT_LEFT])  playerX -= speed;   // 左キー → 左に移動
if (keys[KEY_INPUT_RIGHT]) playerX += speed;   // 右キー → 右に移動
if (keys[KEY_INPUT_UP])    playerY -= speed;   // 上キー → 上に移動 (Y軸は下が正なので減算)
if (keys[KEY_INPUT_DOWN])  playerY += speed;   // 下キー → 下に移動
```

> **CheckHitKey と GetHitKeyStateAll の使い分け**
>
> | 関数 | 用途 | 特徴 |
> |------|------|------|
> | `CheckHitKey(キーコード)` | 1つのキーを確認 | シンプルだが複数キーで何度も呼ぶとやや非効率 |
> | `GetHitKeyStateAll(配列)` | 全キーを一括取得 | 1回の呼び出しで256キー分の状態が取れる |
>
> どちらも「押されている間ずっと TRUE」です。
> 「押した瞬間だけ」検出したい場合はネイティブ API の `IsKeyTriggered` を使います（下記参照）。

### マウス

```cpp
// マウスカーソルの座標を取得（ウィンドウ内の座標）
int mx, my;
GetMousePoint(&mx, &my);    // &mx, &my: ポインタ経由で値を受け取る

// マウスボタンの判定（ビットフラグ: 複数ボタン同時判定可能）
int mouse = GetMouseInput();
if (mouse & MOUSE_INPUT_LEFT)  { /* 左ボタンが押されている */ }
if (mouse & MOUSE_INPUT_RIGHT) { /* 右ボタンが押されている */ }

// マウスホイールの回転量（正=上回転, 負=下回転, 0=動いていない）
int wheel = GetMouseWheelRotVol();
```

### ゲームパッド

```cpp
int pad = GetJoypadInputState(GX_INPUT_PAD1);  // パッド1の状態取得（ビットフラグ）

if (pad & PAD_INPUT_UP)    playerY -= speed;    // 方向パッド上
if (pad & PAD_INPUT_DOWN)  playerY += speed;    // 方向パッド下
if (pad & PAD_INPUT_LEFT)  playerX -= speed;    // 方向パッド左
if (pad & PAD_INPUT_RIGHT) playerX += speed;    // 方向パッド右
if (pad & PAD_INPUT_A)     Fire();              // Aボタン（Xboxコントローラーの場合）
```

### ネイティブ API（トリガー検出）

Compat API の `CheckHitKey` は「押されている間ずっと TRUE」を返します。
「押した瞬間だけ」を検出したい場合（ジャンプ、メニュー選択など）は、
ネイティブ API の `IsKeyTriggered` を使います。

```cpp
GX::InputManager input;
input.Initialize(window);

// --- フレームループ内 ---
input.Update();  // 毎フレーム呼ぶ（前フレームとの差分を計算）

// トリガー: 押された「瞬間」のみ TRUE（押し続けても繰り返さない）
if (input.GetKeyboard().IsKeyTriggered(VK_SPACE))
{
    // ジャンプ開始（1回だけ実行される）
}

// 押されている間ずっと TRUE（移動などに使う）
if (input.GetKeyboard().IsKeyDown(VK_LEFT))
{
    playerX -= speed * deltaTime;
}
```

> **CheckHitKey vs IsKeyTriggered の比較**
>
> | 状況 | CheckHitKey (Compat) | IsKeyTriggered (Native) |
> |------|---------------------|------------------------|
> | キーを押した瞬間 | TRUE | TRUE |
> | キーを押し続けている | TRUE | FALSE |
> | キーを離した | FALSE | FALSE |
>
> ジャンプやショットの発射は `IsKeyTriggered` が適切です。
> 移動のように押し続けたい場合は `CheckHitKey` や `IsKeyDown` を使います。

## サウンド

### 効果音（SE）

```cpp
// 音声ファイルをメモリに読み込む
int seShotHandle = LoadSoundMem("Assets/shot.wav");

// 再生
PlaySoundMem(
    seShotHandle,       // サウンドハンドル
    GX_PLAYTYPE_BACK    // 再生方式:
                        //   GX_PLAYTYPE_BACK   = バックグラウンド再生（処理を止めずに再生）
                        //   GX_PLAYTYPE_NORMAL  = 再生完了まで処理を停止（通常使わない）
);

// 音量変更 (0=無音, 255=最大)
ChangeVolumeSoundMem(200, seShotHandle);

// 使い終わったら解放
DeleteSoundMem(seShotHandle);
```

> **なぜ `GX_PLAYTYPE_BACK` を使うのか？**
>
> `GX_PLAYTYPE_NORMAL` は音の再生が完了するまでプログラムが止まります。
> ゲーム中に使うと画面がフリーズしてしまうため、
> ほぼ全ての場合で `GX_PLAYTYPE_BACK`（バックグラウンド再生）を使います。

### BGM

```cpp
// ファイルパスを指定してストリーミング再生
PlayMusic(
    "Assets/bgm.wav",      // 音声ファイルパス
    GX_PLAYTYPE_LOOP        // GX_PLAYTYPE_LOOP: ループ再生（曲が終わると最初から繰り返す）
);

// 再生中かチェック (1=再生中, 0=停止中)
if (CheckMusic()) { /* BGM 再生中 */ }

// 停止
StopMusic();
```

> **対応音声フォーマット**: 現在は **WAV 形式のみ** 対応しています。
> MP3 や OGG を使いたい場合は、事前に WAV に変換してください。

## 衝突判定

### AABB 衝突判定（矩形 vs 矩形）

AABB (Axis-Aligned Bounding Box) は、座標軸に平行な長方形による当たり判定です。
計算が軽量なため、多くのゲームで基本的な当たり判定に使われます。

```cpp
#include "Math/Collision/Collision2D.h"
using namespace GX;

// 矩形の定義: AABB2D(左上座標, 右下座標)
AABB2D playerRect(
    {playerX, playerY},                     // 左上 (min)
    {playerX + 32, playerY + 32}            // 右下 (max) — 32x32 ピクセルの矩形
);
AABB2D enemyRect(
    {enemyX, enemyY},                       // 左上
    {enemyX + 32, enemyY + 32}              // 右下
);

// 2つの矩形が重なっているか判定
if (Collision2D::TestAABBvsAABB(playerRect, enemyRect))
{
    // 衝突している！ — ダメージ処理など
}
```

### 円 vs 円

円の当たり判定は、キャラクターや弾のように「丸い」ものに適しています。

```cpp
// 円の定義: Circle(中心座標, 半径)
Circle bullet(
    {bulletX, bulletY},     // 弾の中心座標
    4.0f                    // 弾の半径 (4ピクセル)
);
Circle enemy(
    {enemyX + 16, enemyY + 16},    // 敵の中心（32x32画像の中心 = +16, +16）
    16.0f                           // 敵の当たり判定半径 (16ピクセル)
);

if (Collision2D::TestCirclevsCircle(bullet, enemy))
{
    // ヒット！ — 敵を倒す処理
}

// 衝突の詳細情報が必要な場合（跳ね返りなど）
auto hit = Collision2D::IntersectCirclevsCircle(bullet, enemy);
if (hit)
{
    // hit.point  — 衝突した位置 (Vector2)
    // hit.normal — 衝突面の法線（跳ね返り方向の計算に使う）
    // hit.depth  — 重なり深さ（めり込み補正に使う）
}
```

## サンプル：シューティングゲーム

`Samples/Shooting2D/` に完全なシューティングゲームのサンプルがあります。

主な構成要素:
- プレイヤー移動（キーボード入力）
- 弾の発射と移動
- 敵の出現と衝突判定
- スコア表示
- 効果音

```bash
# ビルドと実行
cmake --build build --config Debug --target Shooting2D
```

## フレームレート制御

フレームレートに依存しない移動を実現するため、**deltaTime**（フレーム間の経過時間）を使います。

```cpp
// 前フレームの時刻を記録
int prevTime = GetNowCount();   // 現在時刻をミリ秒で取得

while (ProcessMessage() == 0)
{
    int nowTime = GetNowCount();
    float deltaTime = (nowTime - prevTime) / 1000.0f;
    // ミリ秒を秒に変換 (例: 16ms → 0.016秒)
    prevTime = nowTime;

    // deltaTime を速度に掛けて移動量を計算
    // speed=300 なら「1秒間に300ピクセル移動」という意味になる
    playerX += speed * deltaTime;
    // 60FPS: 300 × 0.016 = 4.8px/フレーム
    // 30FPS: 300 × 0.033 = 9.9px/フレーム → どちらも1秒で300px移動
}
```

> **なぜ deltaTime を使うのか？**
>
> deltaTime を使わないと、移動速度が FPS（フレームレート）に依存します:
> - 60FPS のPCでは `speed × 60回/秒` で高速移動
> - 30FPS のPCでは `speed × 30回/秒` で低速移動
>
> deltaTime を掛けることで「1秒あたりの移動量」を指定でき、
> どのPCでも同じ速度で動作します。

## よくある問題

### キー入力が効かない

- `ProcessMessage()` を毎フレーム呼んでいるか確認（ウィンドウメッセージ処理に必要）
- ネイティブ API の場合、`input.Update()` を毎フレーム呼んでいるか確認
- ウィンドウにフォーカスが当たっているか確認（別ウィンドウがアクティブだと入力を受け取りません）

### 音が鳴らない

- ファイルパスが正しいか確認
- WAV 形式か確認（MP3, OGG は未対応）
- `LoadSoundMem` の戻り値が -1 でないか確認（-1 = 読み込み失敗）

### 衝突判定がうまくいかない

- AABB の座標が正しいか確認（左上 < 右下 になっているか）
- 円の中心座標がスプライトの中心と合っているか確認
- 画像サイズと当たり判定サイズが一致しているか確認

### 動きがカクカクする / PC ごとに速度が違う

- deltaTime を使っているか確認（上記「フレームレート制御」参照）
- `deltaTime` に異常に大きな値が入っていないか確認（初回フレームは 0 に clamp するとよい）

---

## なぜ衝突判定が必要か？ -- ゲームにおける当たり判定の考え方

### 衝突判定の目的

ゲームの世界ではオブジェクト（プレイヤー、敵、弾、壁）は単なる画像データであり、物理的な「実体」を持ちません。画像が画面上で重なっていても、プログラムが明示的に「重なっている」と判定しない限り、何も起きません。衝突判定はゲームに「インタラクション（相互作用）」を与える基盤です。

| シーン | 衝突判定の用途 |
|--------|--------------|
| 弾が敵に当たる | 弾と敵の当たり判定 → ダメージ処理 |
| プレイヤーが壁を通り抜けない | プレイヤーと壁の当たり判定 → 移動制限 |
| アイテムを拾う | プレイヤーとアイテムの当たり判定 → アイテム取得 |
| 地面に着地する | プレイヤーと地面の当たり判定 → 落下停止 |

### AABB vs 円 -- 使い分けの指針

| 判定方式 | 長所 | 短所 | 適するオブジェクト |
|---------|------|------|------------------|
| **AABB (矩形)** | 計算が非常に高速、実装が簡単 | 斜めや丸い形状に精度が低い | ブロック、壁、UI要素、タイル |
| **円 (Circle)** | 回転に強い（円は回しても形が変わらない）、自然な判定 | 四角い物体には隙間ができる | キャラクター、弾、爆発範囲 |

### なぜ完璧なピクセル判定を使わないのか？

画像のピクセル単位で当たり判定（ピクセルパーフェクト判定）を行うと精度は最高ですが、計算コストが極めて高くなります。多くの弾や敵が同時に存在するゲームでは、毎フレーム数百〜数千回の判定が走るため、AABB や円のような「近似形状」でまず高速に判定し、必要な場合のみ精密判定を行うのが一般的な手法です。

```
近似判定（AABB/円）で「衝突の可能性あり」を高速に絞り込む
  ↓ 衝突候補のみ
精密判定（必要であれば）で正確な判定を行う
```

この2段階アプローチは「ブロードフェーズ / ナローフェーズ」と呼ばれ、GXLib の空間分割構造（Quadtree、BVH）もこの考え方に基づいています。

## 次のステップ

- [04_Rendering3D.md](04_Rendering3D.md) — 3D描画とPBRレンダリング
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認

## Tutorial 04: 3D Rendering

PBR (Physically Based Rendering, 物理ベースレンダリング)、カメラ、ライティング、3D モデルの使い方を解説します。

## このチュートリアルで学ぶこと

- 簡易 API で 3D モデルを表示する方法
- カメラの設定（位置、注視点、視野角）
- ライティング（太陽光、点光源、スポットライト）
- PBR マテリアル（金属度・粗さによるリアルな質感表現）
- プリミティブメッシュの生成と描画
- glTF / GXMD モデルの読み込みとスケルタルアニメーション
- シェーダーモデル（PBR, Toon, Phong 等）の切り替え
- ポストエフェクト（画面全体の映像加工）
- DXR レイトレーシング反射 / RTGI

## 前提知識

- [02_Drawing2D.md](02_Drawing2D.md) の内容（描画の基本）
- 3D 空間の座標系の概念（X=右、Y=上、Z=奥）

## 簡易 API での 3D 描画

```cpp
// --- カメラ設定 ---
SetCameraPositionAndTarget(
    VGet(0.0f, 5.0f, -10.0f),   // カメラの位置 (X=0, Y=5(上), Z=-10(手前))
    VGet(0.0f, 0.0f, 0.0f)      // 注視点: カメラが向く先 (原点)
);
SetCameraNearFar(
    0.1f,       // ニアクリップ: これより近い物体は描画しない（0 にしないこと）
    1000.0f     // ファークリップ: これより遠い物体は描画しない
);

// --- モデル読み込みと設定 ---
int model = LoadModel("Assets/models/character.gltf");  // glTF形式のモデルを読み込み
SetModelPosition(model, VGet(0.0f, 0.0f, 0.0f));       // モデルの位置 (原点に配置)
SetModelScale(model, VGet(1.0f, 1.0f, 1.0f));          // スケール (等倍)
SetModelRotation(model, VGet(0.0f, 0.5f, 0.0f));       // 回転 (Y軸に0.5ラジアン≒28.6度)

// --- フレームループ内 ---
ClearDrawScreen();
DrawModel(model);   // モデル描画
ScreenFlip();

// --- 終了時 ---
DeleteModel(model);
```

> **glTF とは？**
>
> glTF (GL Transmission Format) は 3D モデルの標準フォーマットです。
> メッシュ、マテリアル、テクスチャ、アニメーションをひとつのファイルにまとめられます。
> Blender 等の 3D ソフトから直接エクスポートでき、GXLib はこの形式に対応しています。
> (DXLib で使われていた .x, .mv1 形式には対応していません)

## ネイティブ API

### カメラ

```cpp
GX::Camera3D camera;
camera.SetPosition(0.0f, 5.0f, -10.0f);    // カメラ位置 (ワールド座標)
camera.SetTarget(0.0f, 0.0f, 0.0f);        // 注視点（カメラが向く先）
camera.SetFov(GX::MathUtil::PI / 4.0f);    // FOV (Field of View, 視野角): 45度
                                            // 人間の視野は~120度だが、ゲームでは45~90度が一般的
                                            // 小さい値=望遠（ズームイン）, 大きい値=広角
camera.SetNearFar(0.1f, 1000.0f);          // 描画範囲: 0.1m〜1000m
camera.SetAspectRatio(1280.0f / 960.0f);   // アスペクト比: 画面の横幅÷高さ
camera.Update();                            // 行列を再計算（設定変更後に必ず呼ぶ）
```

> **なぜニアクリップを 0 にしてはいけないのか？**
>
> ニアクリップを 0 にすると、深度バッファ（奥行き判定）の精度が極端に低下し、
> 遠くの物体が前後関係を正しく表示できなくなります（Z-fighting という現象）。
> 通常は 0.1〜1.0 に設定してください。

### ライティング

GXLib は3種類のライトをサポートしています。

```cpp
// --- ディレクショナルライト（太陽光のような平行光源）---
GX::Light light;
light.type = GX::LightType::Directional;
light.direction = GX::Vector3(0.3f, -1.0f, 0.5f);     // 光の向き (斜め上から照射)
light.color = GX::Vector3(1.0f, 0.95f, 0.9f);          // 光の色 (やや暖色の白)
light.intensity = 3.0f;                                 // 強度 (HDR なので 1.0 以上も OK)

// --- ポイントライト（電球のような全方向光源）---
GX::Light pointLight;
pointLight.type = GX::LightType::Point;
pointLight.position = GX::Vector3(5.0f, 3.0f, 0.0f);   // ライトの位置
pointLight.color = GX::Vector3(1.0f, 0.5f, 0.2f);      // 暖色のオレンジ
pointLight.intensity = 10.0f;                            // 強度
pointLight.radius = 20.0f;                               // 光が届く範囲 (20m)

// --- スポットライト（懐中電灯のような方向付き光源）---
GX::Light spotLight;
spotLight.type = GX::LightType::Spot;
spotLight.position = GX::Vector3(0.0f, 10.0f, 0.0f);   // 位置 (上方)
spotLight.direction = GX::Vector3(0.0f, -1.0f, 0.0f);  // 向き (真下)
spotLight.intensity = 15.0f;                             // 強度
spotLight.innerAngle = 0.3f;    // 内側の角度 (ラジアン): この範囲は最大の明るさ
spotLight.outerAngle = 0.5f;    // 外側の角度: ここから外は徐々に暗くなり、外側で完全に暗くなる
```

### マテリアル（PBR）

PBR マテリアルは 4 つのパラメータで物体の質感を表現します。

```cpp
GX::Material material;
material.albedo = GX::Vector3(0.8f, 0.2f, 0.1f);  // ベース色 (赤っぽい色)
                                                     // 光の影響を除いた「素の色」
material.metallic = 0.0f;      // 金属度: 0.0=非金属(プラスチック,木,石), 1.0=金属(鉄,金,銀)
                                // 中間値は通常使わない（現実の素材は0か1のどちらか）
material.roughness = 0.5f;     // 粗さ: 0.0=完全な鏡面（映り込みがくっきり）
                                //        1.0=ざらざら（反射がぼやける）
material.emissive = GX::Vector3(0.0f, 0.0f, 0.0f); // 発光色 (0,0,0=発光なし)
                                                     // ネオンや溶岩の自己発光に使う
```

> **PBR の考え方**
>
> 従来の描画方式では「反射の強さ」「光沢度」などのパラメータを手動調整していましたが、
> PBR では物理法則に基づいて「金属度」と「粗さ」だけで自然な質感を表現できます。
> Albedo（基本色）を決めて、Metallic と Roughness を調整するだけでリアルな見た目になります。

### メッシュ生成

プログラムから基本的な3Dメッシュ（形状データ）を生成できます。

```cpp
GX::MeshData cube = GX::MeshData::CreateCube(1.0f);
// 立方体: 一辺 1.0m

GX::MeshData sphere = GX::MeshData::CreateSphere(0.5f, 32, 16);
// 球: 半径 0.5m, 横方向32分割, 縦方向16分割
// 分割数が多いほど滑らかだが頂点数が増える

GX::MeshData cylinder = GX::MeshData::CreateCylinder(0.5f, 2.0f, 16);
// 円柱: 半径 0.5m, 高さ 2.0m, 周方向16分割

GX::MeshData plane = GX::MeshData::CreatePlane(10.0f, 10.0f);
// 平面: 幅 10.0m × 奥行き 10.0m（地面や壁に使える）
```

### 3D 描画ループ

```cpp
GX::Renderer3D renderer;
renderer.Initialize(device, cmdList);

// --- フレーム描画 ---
renderer.BeginScene(camera);        // シーン描画の開始（カメラ設定を適用）

renderer.SetDirectionalLight(light);     // 太陽光を設定
renderer.AddPointLight(pointLight);      // 点光源を追加（複数追加可能）

// メッシュの位置・回転・スケールを設定
GX::Transform3D transform;
transform.SetPosition(0.0f, 0.0f, 0.0f);   // 位置: 原点
transform.SetRotation(0.0f, angle, 0.0f);   // Y軸回転 (angle は毎フレーム増加させる等)
transform.SetScale(1.0f, 1.0f, 1.0f);      // スケール: 等倍

renderer.DrawMesh(
    cube,                           // 描画するメッシュデータ
    material,                       // 適用するマテリアル
    transform.GetWorldMatrix()      // ワールド変換行列（位置・回転・スケールをまとめた行列）
);

renderer.EndScene();                // シーン描画の終了
```

### glTF / GXMD モデル

```cpp
#include "Graphics/3D/Model.h"

// glTF モデルの読み込み
auto model = GX::ModelLoader::LoadFromFile(
    "Assets/models/character.gltf", // モデルファイルパス (glTF または GXMD)
    device,                         // GraphicsDevice (GPU デバイス)
    texManager                      // TextureManager (テクスチャ管理)
);
// 戻り値は std::unique_ptr<Model>（自動でメモリ解放される）

// 描画
renderer.DrawModel(*model, worldMatrix);

// スケルタルアニメーション（骨格によるアニメーション）
if (model->HasAnimations())
{
    model->UpdateAnimation(deltaTime);  // deltaTime でアニメーションを進行
}
```

## ポストエフェクト

ポストエフェクトは、3D シーンの描画が完了した後に画面全体に適用する映像加工です。
映画のような映像表現を実現します。

```cpp
GX::PostEffectPipeline postFX;
postFX.Initialize(
    device, cmdList,
    width, height           // 画面解像度 (例: 1280, 960)
);

// 個別エフェクトの設定
postFX.SetBloomEnabled(true);           // Bloom (光のにじみ) を有効化
postFX.SetBloomThreshold(1.0f);         // 輝度がこの値を超えた部分がにじむ
postFX.SetBloomIntensity(0.3f);         // にじみの強さ (0.0=なし, 1.0=強い)

postFX.SetFXAAEnabled(true);            // FXAA (高速近似アンチエイリアシング) を有効化
                                        // → ジャギー（斜め線のギザギザ）を滑らかにする

postFX.SetTonemapMode(GX::TonemapMode::ACES);
// トーンマッピング: HDR → LDR 変換方式
// ACES = 映画業界標準の色変換（自然で鮮やかな仕上がり）

postFX.SetVignetteEnabled(true);        // ビネット: 画面の四隅を暗くする効果

// --- フレーム描画 ---
postFX.Begin(cmdList);              // HDR レンダーターゲットへの描画を開始
// ... ここに3D描画処理 ...
postFX.Resolve(
    cmdList,                        // コマンドリスト
    deltaTime,                      // フレーム間隔 (TAA, AutoExposure 等が使用)
    depthBuffer,                    // 深度バッファ (SSAO, DoF が奥行き情報を必要とする)
    camera                          // カメラ (SSAO, SSR がカメラ行列を必要とする)
);
// Resolve() で全ポストエフェクトが順番に適用され、最終画像がバックバッファに出力される
```

### 利用可能なポストエフェクト一覧

| エフェクト | 正式名称 | 効果 |
|-----------|---------|------|
| **Bloom** | ブルーム | 明るい部分から光がにじみ出す（太陽や電球の光輪） |
| **SSAO** | Screen Space Ambient Occlusion (環境遮蔽) | 隅や溝に自然な影を追加して立体感を出す |
| **SSR** | Screen Space Reflections (スクリーン空間反射) | 水面や光沢のある床に映り込みを表示 |
| **DoF** | Depth of Field (被写界深度) | ピント範囲外をぼかして一眼カメラ風に |
| **MotionBlur** | モーションブラー | カメラや物体の動きに残像を付ける |
| **TAA** | Temporal Anti-Aliasing (テンポラルAA) | 複数フレームでジャギーを滑らかに（FXAA より高品質） |
| **VolumetricLight** | ボリュームライト (ゴッドレイ) | 光の筋を表示（森の木漏れ日風） |
| **OutlineEffect** | アウトライン | 物体の輪郭線を描画（トゥーンシェーディング風） |
| **AutoExposure** | 自動露出調整 | 暗い/明るいシーンで自動的に明るさ調整 |
| **ColorGrading** | カラーグレーディング | 映像全体の色調を調整（映画風の雰囲気に） |

## シャドウ

CSM (Cascaded Shadow Maps, カスケードシャドウマップ) が自動的に有効です。
カメラから近い範囲は高解像度、遠い範囲は低解像度で影を描画し、
広い範囲を効率的にカバーします。

スポットライトとポイントライトの影も自動で処理されます。

## Skybox

```cpp
GX::Skybox skybox;
skybox.Initialize(
    device, cmdList,
    "Assets/skybox.hdr"            // HDR 環境マップ画像 (空の画像)
);

// 描画ループ内（3D シーンの最初に描画）
skybox.Draw(cmdList, camera);
```

## よくある問題

### モデルが表示されない

- glTF 形式 (.gltf / .glb) または GXMD 形式 (.gxmd) か確認（.fbx, .obj, .x, .mv1 は LoadModel では非対応。gxconv で変換可能）
- ファイルパスが正しいか確認
- カメラの位置がモデルと同じ場所になっていないか確認（離れた位置から見る必要がある）
- ニアクリップ / ファークリップの範囲にモデルが入っているか確認

### 画面が真っ暗（ライトが効かない）

- ディレクショナルライトを設定しているか確認
- ライトの intensity が 0 になっていないか確認
- ポストエフェクトのトーンマッピングが有効か確認（HDR 値がそのまま表示されると白飛びする場合がある）

### ポストエフェクトが効かない

- `postFX.Begin()` と `postFX.Resolve()` で 3D 描画を挟んでいるか確認
- `Resolve()` に `depthBuffer` と `camera` を渡しているか確認（SSAO, DoF に必要）
- HDR レンダリングが有効か確認

## シェーダーモデル

GXLib は PBR 以外にも複数のシェーダーモデル（描画方式）をサポートしています。
Material の `shaderModel` を変更するだけで、PSO（描画設定）が自動的に切り替わります。

```cpp
// マテリアルのシェーダーモデルを指定
material.shaderModel = GX::ShaderModel::PBR;         // 物理ベースレンダリング（デフォルト）
material.shaderModel = GX::ShaderModel::Toon;        // トゥーンシェーディング（UTS2 ベース）
material.shaderModel = GX::ShaderModel::Phong;       // Phong シェーディング
material.shaderModel = GX::ShaderModel::Unlit;       // ライティングなし（UI 用等）
material.shaderModel = GX::ShaderModel::Subsurface;  // サブサーフェス散乱（肌、蝋等）
material.shaderModel = GX::ShaderModel::ClearCoat;   // クリアコート（車の塗装等）
```

> **ShaderRegistry について**
>
> ShaderRegistry は 6 種類のシェーダーモデル x static/skinned = 14 PSO を一括管理します。
> Material の shaderModel フィールドに応じて適切な PSO が自動選択されるため、
> ユーザーが PSO を手動で切り替える必要はありません。
> Toon シェーダーはアウトライン描画用の追加パスを自動で実行します。

### Toon シェーダー（UTS2 ベース）

Toon シェーダーは UTS2 (Unity Toon Shader 2.0) をベースにした、アニメ調の描画方式です。
ダブルシェード（3ゾーン遷移: ベース色 → 1st シェード → 2nd シェード）、リムライト、
ハイカラー、スムース法線ベースのアウトラインを備えています。

```cpp
material.shaderModel = GX::ShaderModel::Toon;

// Toon 固有パラメータは ShaderModelParams 経由で設定
auto& params = material.shaderParams;
params.shadeColor()     = {0.6f, 0.3f, 0.3f};   // 1st シェード色
params.shade2ndColor()  = {0.3f, 0.15f, 0.15f};  // 2nd シェード色
params.outlineWidth()   = 0.002f;                 // アウトライン幅
params.outlineColor()   = {0.1f, 0.05f, 0.05f};  // アウトライン色
```

## アセットパイプライン

GXLib には独自のバイナリモデル形式 (.gxmd / .gxan) と変換ツールがあります。

```bash
# FBX → GXMD/GXAN 変換
gxconv input.fbx -o output.gxmd

# 複数アセットを GXPAK にバンドル
gxpak pack bundle.gxpak Assets/

# GXPAK の内容一覧
gxpak list bundle.gxpak
```

ランタイムでは gxloader を使って高速に読み込み、PakFileProvider で VFS に統合できます。

## DXR レイトレーシング

DXR 対応 GPU では、ハードウェアレイトレーシングによる高品質な反射と
グローバルイルミネーションを利用できます。

- **RTReflections** — DXR レイトレーシング反射（Y キーでトグル、SSR と排他）
- **RTGI** — グローバルイルミネーション（G キーでトグル、半解像度 + テンポラル蓄積 + A-Trous フィルタ）

> **注意**: DXR は ID3D12Device5 対応 GPU が必要です。非対応 GPU では SSR にフォールバックしてください。

## アニメーションブレンド

Animator に加え、高度なアニメーション制御が利用できます。

- **BlendStack** — 最大 8 レイヤーの Override/Additive ブレンド
- **BlendTree** — 1D/2D パラメータによるアニメーション自動合成
- **AnimatorStateMachine** — トリガーと遷移条件によるステートマシン

---

## 補足: PBR（物理ベースレンダリング）の仕組み

### 従来の描画方式との違い

従来のシェーディング（Phong 等）では「拡散反射の強さ」「鏡面反射の強さ」「光沢度」といった見た目ベースのパラメータを手動調整していました。これはアーティストの勘に頼る部分が大きく、ライティング環境が変わると見た目が崩れることがありました。

PBR は「光がどのように物体と相互作用するか」を物理法則に基づいてシミュレートします。

| 概念 | 説明 |
|------|------|
| **エネルギー保存** | 物体に入射した光エネルギーは、反射と吸収の合計が入射量を超えない |
| **フレネル効果** | 浅い角度で見るほど反射が強くなる（水面を斜めから見ると鏡のように反射する現象） |
| **マイクロファセット理論** | 表面の微細な凹凸が反射の広がり（粗さ）を決める |

### パラメータの直感的な理解

```
Metallic (金属度):
  0.0 ─── 非金属 ─── 1.0 金属
  木、石、プラスチック、肌     鉄、金、銀、銅

Roughness (粗さ):
  0.0 ─── 鏡面 ─── 1.0 ざらざら
  鏡、磨いた金属、水面       コンクリート、ゴム、布
```

- **非金属 (Metallic=0)**: 拡散反射が主体。Albedo がそのまま色として見える
- **金属 (Metallic=1)**: 鏡面反射が主体。Albedo が反射色を決める（金は黄色く反射する）
- **中間値 (0.1〜0.9)**: 現実の素材ではほぼ使われない（汚れや錆の遷移にのみ使用）

### ポストエフェクト略語一覧

3D レンダリングのポストエフェクトには略語が多く使われます。以下に正式名称と意味をまとめます。

| 略語 | 正式名称 | 日本語訳 | 一言説明 |
|------|---------|---------|---------|
| **PBR** | Physically Based Rendering | 物理ベースレンダリング | 物理法則に基づくリアルな質感描画 |
| **HDR** | High Dynamic Range | 高ダイナミックレンジ | 0.0〜1.0 を超える輝度値を扱う（太陽の明るさ等を表現） |
| **LDR** | Low Dynamic Range | 低ダイナミックレンジ | モニター表示用の 0.0〜1.0 の範囲 |
| **SSAO** | Screen Space Ambient Occlusion | スクリーン空間環境遮蔽 | 隅や溝の自然な影を後処理で追加 |
| **SSR** | Screen Space Reflections | スクリーン空間反射 | 画面内の情報から反射を近似計算 |
| **FXAA** | Fast Approximate Anti-Aliasing | 高速近似アンチエイリアシング | 画像処理でジャギーを軽減（軽量） |
| **TAA** | Temporal Anti-Aliasing | テンポラルアンチエイリアシング | 複数フレームの蓄積でジャギーを除去（高品質） |
| **DoF** | Depth of Field | 被写界深度 | ピント範囲外のぼけ表現 |
| **CSM** | Cascaded Shadow Maps | カスケードシャドウマップ | 距離に応じて解像度を変える影描画 |
| **ACES** | Academy Color Encoding System | アカデミー色符号化システム | 映画業界標準のトーンマッピング曲線 |
| **DXR** | DirectX Raytracing | DirectXレイトレーシング | GPU ハードウェアレイトレーシング API |
| **RTGI** | Ray Traced Global Illumination | レイトレースグローバルイルミネーション | DXR による間接光の計算 |
| **IBL** | Image Based Lighting | 画像ベースライティング | 環境マップから照明情報を取得する技法 |
| **BLAS** | Bottom Level Acceleration Structure | ボトムレベル加速構造体 | DXR でメッシュ単位のレイトレース用構造体 |
| **TLAS** | Top Level Acceleration Structure | トップレベル加速構造体 | DXR でシーン全体のレイトレース用構造体 |
| **PSO** | Pipeline State Object | パイプラインステートオブジェクト | GPU の描画設定をまとめたオブジェクト |
| **RTV** | Render Target View | レンダーターゲットビュー | GPU の描画先を指すビュー |
| **FOV** | Field of View | 視野角 | カメラの見える範囲の角度 |

## 次のステップ

- [05_GUI.md](05_GUI.md) — XML+CSS による GUI 構築
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認

## Tutorial 05: GUI

XML + CSS による宣言的 GUI の構築方法を解説します。

## このチュートリアルで学ぶこと

- XML で UI の構造（ボタン、テキスト等）を定義する方法
- CSS でスタイル（色、サイズ、レイアウト）を指定する方法
- C++ でイベント（ボタンクリック等）を処理する方法
- Flexbox レイアウトによる要素の配置
- GUI スケーリング（異なる解像度への対応）

## 前提知識

- [01_GettingStarted.md](01_GettingStarted.md) の内容（ビルドとウィンドウ表示）
- CSS の基礎知識があると理解が早まりますが、なくてもサンプルから学べます

→ CSS に不慣れな方は [00_Prerequisites.md](00_Prerequisites.md) の「CSS の基礎」も参照

## 概要

GXLib の GUI システムは Web 技術にインスパイアされた構成です:

| 技術 | 役割 | Web 技術での対応 |
|------|------|-----------------|
| **XML** | ウィジェット（UI部品）の構造を定義 | HTML |
| **CSS** | スタイル（色、サイズ、レイアウト）を定義 | CSS |
| **C++** | イベントハンドラ（クリック時の処理等）を登録 | JavaScript |

> **なぜ XML + CSS なのか？**
>
> UIの見た目（CSS）と構造（XML）と動作（C++）を分離することで、
> プログラムを変更せずにデザインを調整できます。
> CSS ファイルを書き換えるだけでボタンの色やレイアウトを変更でき、
> コードの再コンパイルが不要です。

## 基本構成

### XML ファイル (menu.xml)

XML で「どのウィジェットをどの順番で配置するか」を定義します。

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!-- Panel: 他のウィジェットを入れる「箱」(コンテナ) -->
<Panel id="root" class="main-panel">
    <!-- id: C++からウィジェットを特定するための一意の識別名 -->
    <!-- class: CSSでスタイルを適用するためのクラス名 -->

    <TextWidget id="title" class="title-text">Game Menu</TextWidget>
    <!-- TextWidget: テキストを表示するウィジェット -->

    <Button id="btnStart" class="menu-button">Start Game</Button>
    <!-- Button: クリック可能なボタン -->

    <Button id="btnOptions" class="menu-button">Options</Button>
    <Button id="btnQuit" class="menu-button">Quit</Button>
</Panel>
```

### CSS ファイル (menu.css)

CSS で「ウィジェットをどう見せるか」を定義します。

```css
/* #root: id="root" の要素を指す（# = IDセレクタ） */
#root {
    width: 400;                             /* 幅 400px */
    height: 500;                            /* 高さ 500px */
    padding: 20;                            /* 内側余白 20px（コンテンツと境界の間のスペース） */
    flex-direction: column;                 /* 子要素を縦方向に並べる（column=縦, row=横） */
    align-items: center;                    /* 子要素を横方向の中央に配置 */
    gap: 15;                                /* 子要素間のスペース 15px */
    background-color: rgba(20, 20, 40, 0.9); /* 背景色（R, G, B, 透明度）— 暗い紺色で少し透明 */
    corner-radius: 10;                      /* 角の丸み 10px */
}

/* .title-text: class="title-text" の要素を指す（. = クラスセレクタ） */
.title-text {
    font-size: 32;                          /* フォントサイズ 32px */
    color: rgba(255, 255, 200, 1.0);        /* テキスト色（やや黄色みの白） */
    margin-bottom: 30;                      /* 下の外側余白 30px（次の要素との間隔） */
}

/* .menu-button: ボタンのスタイル */
.menu-button {
    width: 300;                             /* 幅 300px */
    height: 50;                             /* 高さ 50px */
    background-color: rgba(60, 80, 120, 1.0);  /* 背景色（暗い青） */
    color: rgba(255, 255, 255, 1.0);        /* テキスト色（白） */
    font-size: 20;                          /* フォントサイズ 20px */
    corner-radius: 8;                       /* 角丸 8px */
    justify-content: center;                /* テキストを横方向中央 */
    align-items: center;                    /* テキストを縦方向中央 */
}

/* 擬似クラス: マウスカーソルが上に乗った時 */
.menu-button:hover {
    background-color: rgba(80, 110, 160, 1.0);  /* 少し明るい青に変化 */
}

/* 擬似クラス: ボタンが押されている時 */
.menu-button:pressed {
    background-color: rgba(40, 60, 100, 1.0);   /* 暗い青に変化（押し込み感） */
}
```

### C++ コード

```cpp
#include "GUI/UIContext.h"      // UIの状態管理（入力処理、レイアウト計算）
#include "GUI/UIRenderer.h"     // UIの描画
#include "GUI/StyleSheet.h"     // CSSの読み込みと適用
#include "GUI/GUILoader.h"      // XMLの読み込みとイベント登録

GX::UIContext uiContext;
GX::UIRenderer uiRenderer;
GX::StyleSheet styleSheet;
GX::GUILoader loader;

// --- 初期化 ---

uiRenderer.Initialize(
    device, cmdList,        // 描画用のデバイスとコマンドリスト
    spriteBatch,            // 2D描画用（ウィジェット内部で使用）
    textRenderer,           // テキスト描画用
    fontManager             // フォント管理用
);

uiContext.SetDesignResolution(1280, 960);
// デザイン解像度: UIは1280x960基準で設計し、実際のウィンドウサイズに
// 自動スケーリングされます（後述「GUIスケーリング」参照）

// フォント登録（UIで使うフォントを事前に読み込む）
int fontHandle = fontManager.LoadFont("Arial", 20);
loader.RegisterFont("default", fontHandle);
// "default" = XML/CSSから参照するフォント名

// --- イベント登録 ---
// XMLの id で指定したウィジェットにクリック時の処理を登録
loader.RegisterEvent(
    "btnStart",                     // XML の id="btnStart" に対応
    [](GX::Widget&) {              // ラムダ式（無名関数）: クリック時に実行される処理
        StartGame();                // ゲーム開始処理
    }
);
loader.RegisterEvent("btnQuit", [](GX::Widget&) {
    PostQuitMessage(0);             // ウィンドウを閉じる (Win32 API)
});

// --- XML + CSS 読み込み ---
auto root = loader.LoadFromFile("Assets/ui/menu.xml");
// XMLを解析してウィジェットツリーを構築

styleSheet.LoadFromFile("Assets/ui/menu.css");
// CSSファイルを読み込み

// --- レイアウト初期化 ---
uiContext.SetRoot(root);                    // ルートウィジェットを設定
styleSheet.ApplyToTree(*root);              // CSSスタイルをツリー全体に適用
uiContext.ComputeLayout(1280, 960);         // レイアウトを計算（幅, 高さ）

// --- フレームループ内 ---
uiContext.ProcessInputEvents(inputManager);  // マウスクリックやホバーを検出
uiContext.ComputeLayout(1280, 960);          // レイアウトを毎フレーム再計算
// ※毎フレーム呼ぶ理由: アニメーションやウィンドウリサイズに対応するため

uiRenderer.Begin();
uiRenderer.DrawWidgetTree(*root);           // ウィジェットツリーを描画
uiRenderer.End();
```

> **なぜ ComputeLayout を毎フレーム呼ぶのか？**
>
> ウィンドウサイズの変更、ウィジェットの表示/非表示切り替え、
> テキスト内容の変更、アニメーションなどでレイアウトが変わる可能性があるためです。
> Flexbox レイアウトの再計算は軽量なので、毎フレーム呼んでも問題ありません。

## 利用可能なウィジェット

| ウィジェット | XML タグ | 説明 | 用途例 |
|---|---|---|---|
| Panel | `<Panel>` | 他のウィジェットを入れるコンテナ | メニュー画面の枠、グループ化 |
| TextWidget | `<TextWidget>` | テキスト表示 | タイトル、説明文 |
| Button | `<Button>` | クリック可能なボタン | スタートボタン、設定項目 |
| CheckBox | `<CheckBox>` | ON/OFF 切替 | 設定のON/OFF |
| Slider | `<Slider>` | 数値スライダー | 音量調整、明るさ調整 |
| ProgressBar | `<ProgressBar>` | 進行状況バー | ロード画面、HP ゲージ |
| Image | `<Image>` | 画像表示 | アイコン、背景画像 |
| TextInput | `<TextInput>` | テキスト入力欄 | プレイヤー名入力 |
| DropDown | `<DropDown>` | ドロップダウン選択 | 解像度選択、難易度選択 |
| RadioButton | `<RadioButton>` | 排他的選択（1つだけ選べる） | 難易度選択 |
| ListView | `<ListView>` | スクロール可能なリスト | ランキング表示 |
| ScrollView | `<ScrollView>` | スクロール可能な領域 | 長い説明文、設定画面 |
| TabView | `<TabView>` | タブ切替 | 設定画面のカテゴリ分け |
| Dialog | `<Dialog>` | モーダルダイアログ | 確認ダイアログ、警告表示 |
| Canvas | `<Canvas>` | カスタム描画領域 | ミニマップ、グラフ |
| Spacer | `<Spacer>` | 空白スペース | 余白の確保、レイアウト調整 |

## レイアウト（Flexbox）

GXLib の GUI は CSS Flexbox ライクなレイアウトをサポートします。
Flexbox は「要素を1列に並べて、柔軟に配置する」レイアウト方式です。

```css
#container {
    flex-direction: row;
    /* 子要素の並び方向:
       row    = 横方向（左→右）
       column = 縦方向（上→下）*/

    justify-content: center;
    /* 並び方向（主軸）の配置:
       flex-start    = 先頭寄せ
       center        = 中央
       flex-end       = 末尾寄せ
       space-between = 等間隔（両端は詰める）*/

    align-items: center;
    /* 並び方向と直交する方向（交差軸）の配置:
       flex-start = 上寄せ(row時) / 左寄せ(column時)
       center     = 中央
       flex-end   = 下寄せ(row時) / 右寄せ(column時)
       stretch    = 引き伸ばし */

    gap: 10;
    /* 子要素間のスペース（ピクセル）*/

    flex-grow: 1;
    /* 余剰スペースの配分比率:
       0 = 余白を取らない（デフォルト）
       1 = 余白を均等配分
       2 = 他の要素の2倍の余白を受け取る */
}
```

> **Flexbox の考え方**
>
> Flexbox は「箱の中に子要素を並べる」イメージです:
>
> **flex-direction: row（横並び）**
> ```
> [A] [B] [C]  ← 左から右へ
> ```
>
> **flex-direction: column（縦並び）**
> ```
> [A]
> [B]  ← 上から下へ
> [C]
> ```
>
> justify-content は並び方向、align-items は直交方向を制御します。

## スタイルプロパティ

| プロパティ | 型 | 説明 | 例 |
|---|---|---|---|
| width / height | float | サイズ (px) | `width: 200;` |
| min-width / max-width | float | 最小/最大サイズ制約 | `max-width: 500;` |
| padding | float | 内側余白（コンテンツと境界の間） | `padding: 10;` |
| padding-left/right/top/bottom | float | 内側余白 (個別指定) | `padding-top: 20;` |
| margin | float | 外側余白（他の要素との間隔） | `margin: 5;` |
| margin-left/right/top/bottom | float | 外側余白 (個別指定) | `margin-bottom: 15;` |
| background-color | rgba() | 背景色 | `rgba(0,0,0,0.8)` |
| color | rgba() | テキスト色 | `rgba(255,255,255,1.0)` |
| border-color | rgba() | ボーダー（枠線）色 | `rgba(100,100,100,1.0)` |
| border-width | float | ボーダー幅 (px) | `border-width: 2;` |
| corner-radius | float | 角丸半径 (px, 0=直角) | `corner-radius: 8;` |
| font-size | float | フォントサイズ (px) | `font-size: 20;` |
| opacity | float | 不透明度 (0=透明, 1=不透明) | `opacity: 0.5;` |
| overflow | hidden/visible/scroll | はみ出し制御 | `overflow: scroll;` |
| position | relative/absolute | 配置方法 | `position: absolute;` |

> **padding と margin の違い**
>
> ```
> ┌──── margin（外側余白）─────────────┐
> │  ┌─── border（枠線）───────────┐  │
> │  │  ┌── padding（内側余白）──┐  │  │
> │  │  │                        │  │  │
> │  │  │    コンテンツ            │  │  │
> │  │  │                        │  │  │
> │  │  └────────────────────────┘  │  │
> │  └──────────────────────────────┘  │
> └────────────────────────────────────┘
> ```

## 擬似クラス

マウス操作に応じてスタイルを変更できます。

```css
Button:hover   { background-color: rgba(100, 100, 200, 1.0); }
/* マウスカーソルがボタン上にある時 */

Button:pressed { background-color: rgba(50, 50, 150, 1.0); }
/* ボタンが押されている時 */

Button:disabled { opacity: 0.5; }
/* ボタンが無効状態の時（半透明になる） */

TextInput:focused { border-color: rgba(100, 150, 255, 1.0); }
/* テキスト入力欄にフォーカスがある時（青い枠線） */
```

## GUI スケーリング

デザイン解像度を設定すると、実際のウィンドウサイズに自動スケーリングされます。

```cpp
uiContext.SetDesignResolution(1280, 960);
```

> **なぜスケーリングが必要か？**
>
> 異なるモニター解像度（1920x1080, 2560x1440 等）で同じ UI を表示するため、
> 1つの基準解像度で UI をデザインし、実際の画面サイズに合わせて拡大/縮小します。
> これにより、どの解像度でも同じ見た目の UI が表示されます。
>
> 例: 1280x960 でデザインした UI を 1920x1080 の画面で表示すると、
> 自動的に 1.5 倍にスケーリングされます。

## サンプル

`Samples/GUIMenuDemo/` に完全な GUI メニューのサンプルがあります。

```bash
cmake --build build --config Debug --target GUIMenuDemo
```

## よくある問題

### ウィジェットが表示されない

- `uiContext.SetRoot(root)` を呼んでいるか確認
- `styleSheet.ApplyToTree(*root)` を呼んでいるか確認
- `uiContext.ComputeLayout()` を呼んでいるか確認
- CSSで `width` / `height` が 0 になっていないか確認

### ボタンのクリックが反応しない

- `uiContext.ProcessInputEvents(inputManager)` を毎フレーム呼んでいるか確認
- `loader.RegisterEvent("id", callback)` の id が XML の id と一致しているか確認
- ボタンの上に別のウィジェットが重なっていないか確認

### レイアウトが崩れる

- `flex-direction` が正しいか確認（`row` = 横並び、`column` = 縦並び）
- 親の `width` / `height` が子より小さくなっていないか確認
- `padding` と `margin` を混同していないか確認（上記の図を参照）
- F2 キーでレイアウトデバッグ表示（GUIMenuDemo サンプルで利用可能）

### フォントが表示されない

- `loader.RegisterFont("default", fontHandle)` でフォントを登録しているか確認
- `fontManager.LoadFont()` の戻り値が有効なハンドルか確認

---

## 補足: CSS と Flexbox の基本概念

### CSS とは何か

CSS (Cascading Style Sheets) は、もともと Web ページの見た目を定義するために生まれた技術です。GXLib の GUI システムはこの CSS の仕組みを借用しており、Web 開発の経験があればほぼ同じ感覚で使えます。

**「Cascading（カスケーディング）」の意味**: スタイルが「滝のように上から流れ落ちる」仕組みです。親要素に設定したスタイルが子要素にも継承され、より具体的なセレクタが優先されます。

```css
/* 全ての Button に適用（一般的） */
Button {
    font-size: 16;
    color: rgba(200, 200, 200, 1.0);
}

/* class="menu-button" の Button に適用（より具体的 → こちらが優先） */
.menu-button {
    font-size: 20;
}

/* id="btnStart" の Button に適用（最も具体的 → 最優先） */
#btnStart {
    font-size: 24;
}
```

優先順位: `#id` > `.class` > `タグ名`

### セレクタの種類

| セレクタ | 書き方 | 意味 | 例 |
|---------|--------|------|-----|
| ID セレクタ | `#名前` | 特定の1要素にだけ適用 | `#btnStart { ... }` |
| クラスセレクタ | `.名前` | 同じクラスの全要素に適用 | `.menu-button { ... }` |
| タグセレクタ | `タグ名` | 同じ種類の全要素に適用 | `Button { ... }` |
| 擬似クラス | `:状態` | 特定の状態の時だけ適用 | `.menu-button:hover { ... }` |

### Flexbox の詳細解説

Flexbox は「1次元レイアウト」のモデルです。要素を横一列（row）か縦一列（column）に並べます。

#### 主軸と交差軸

Flexbox を理解する鍵は「主軸（Main Axis）」と「交差軸（Cross Axis）」です。

```
flex-direction: row の場合:

  主軸 →→→→→→→→→→→→→→→→
  ┌──────────────────────────┐  ↑
  │ [A]    [B]    [C]       │  │ 交差軸
  └──────────────────────────┘  ↓

  justify-content → 主軸方向の配置（左右）
  align-items     → 交差軸方向の配置（上下）


flex-direction: column の場合:

  交差軸 →→→→→
  ┌────────────┐  ↑
  │    [A]     │  │
  │    [B]     │  │ 主軸
  │    [C]     │  │
  └────────────┘  ↓

  justify-content → 主軸方向の配置（上下）
  align-items     → 交差軸方向の配置（左右）
```

#### よく使うレイアウトパターン

**中央揃え（縦横とも中央）**:
```css
#container {
    flex-direction: column;
    justify-content: center;
    align-items: center;
}
```

**等間隔配置（ナビゲーションバー風）**:
```css
#navbar {
    flex-direction: row;
    justify-content: space-between;
    align-items: center;
}
```

**余白を埋めるフレキシブルレイアウト**:
```css
/* 3つの子要素のうち、中央だけが余白を全て吸収 */
.sidebar { width: 200; }
.content { flex-grow: 1; }  /* 残り全ての幅を取る */
.sidebar-right { width: 200; }
```

### Web CSS との違い

GXLib の CSS は Web の CSS をベースにしていますが、いくつかの違いがあります。

| 項目 | Web CSS | GXLib CSS |
|------|---------|-----------|
| 単位 | `px`, `em`, `%`, `vh` 等 | 数値のみ（px 相当、単位記号なし） |
| 色指定 | `#FF0000`, `rgb()`, `rgba()`, 色名 | `rgba()` のみ |
| フォント | `font-family: "Arial", sans-serif` | `RegisterFont()` で事前登録 |
| 角丸 | `border-radius` | `corner-radius` |
| レイアウト | Flexbox, Grid, Block, Inline 等 | Flexbox のみ |
| メディアクエリ | `@media (max-width: 600px)` | なし（`SetDesignResolution` で一括スケーリング） |

## 次のステップ

- [DXLib 移行ガイド](../migration/DxLibMigrationGuide.md) — DXLib からの移行方法
- [API リファレンス](../index.html) — 全 GUI ウィジェットの API 詳細
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認

## Tutorial 06: GXEasy 2D Game

GXEasy::App を使って、最小限のコードで 2D ゲームを作る方法を解説します。

## このチュートリアルで学ぶこと

- GXEasy::App の概要と利点
- AppConfig によるウィンドウ設定
- Start / Update / Draw ライフサイクル
- CompatContext を使った 3D シーンへのアクセス
- deltaTime を使ったフレームレート非依存の移動
- FormatT によるテキスト表示

## 前提知識

- [01_GettingStarted.md](01_GettingStarted.md) の内容（ビルドとウィンドウ表示）
- [03_Game2D.md](03_Game2D.md) の内容（入力処理の基本）

## GXEasy::App とは

従来の Compat API (`GX_Init` / `ProcessMessage` / `ScreenFlip`) を使う場合、
初期化やメインループの雛形コードを毎回書く必要がありました。
GXEasy::App はその定型処理をラッパークラスに隠蔽し、
ゲームロジックだけに集中できるようにした仕組みです。

| 方式 | 特徴 |
|------|------|
| Compat API (従来) | `WinMain` にループを自分で書く。細かい制御が可能 |
| **GXEasy::App** | クラスを継承するだけ。初期化・ループ・終了を自動管理 |

> **GXEasy.h をインクルードするだけで使えます。**
> Compat API (`DrawString`, `CheckHitKey` 等) も同時に利用可能です。

## 最小サンプル

```cpp
#include "GXEasy.h"

class MyApp : public GXEasy::App
{
public:
    void Start() override
    {
        // 初期化処理（1回だけ呼ばれる）
    }

    void Update(float dt) override
    {
        // 更新処理（毎フレーム呼ばれる）
        // dt = 前フレームからの経過時間（秒）
    }

    void Draw() override
    {
        // 描画処理（毎フレーム、Update の後に呼ばれる）
    }
};

// エントリーポイント: WinMain を自動生成するマクロ
GX_EASY_APP(MyApp)
```

`GX_EASY_APP(MyApp)` マクロが `WinMain` を生成し、
`MyApp` のインスタンスを作成してエンジンを起動します。
`GX_Init()` / `GX_End()` / メインループは全て内部で処理されるため、
ユーザーが書く必要はありません。

## AppConfig でウィンドウを設定する

`GetConfig()` をオーバーライドして、ウィンドウの設定を変更できます。

```cpp
GXEasy::AppConfig GetConfig() const override
{
    GXEasy::AppConfig config;
    config.title    = L"My 2D Game";   // ウィンドウタイトル
    config.width    = 1280;            // 画面幅 (px)
    config.height   = 720;             // 画面高さ (px)
    config.windowed = true;            // true=ウィンドウモード, false=フルスクリーン
    config.bgR = 10;                   // 背景色 R (0-255)
    config.bgG = 10;                   // 背景色 G
    config.bgB = 30;                   // 背景色 B
    config.allowEscapeExit = true;     // true=ESCキーで終了
    config.maxDeltaTime    = 0.1f;     // dt の上限 (秒)。処理落ち時の暴走防止
    config.targetFps       = 60;       // FPS 上限 (0=無制限)
    return config;
}
```

> **maxDeltaTime はなぜ必要か？**
>
> ウィンドウのドラッグ中やブレークポイント停止後に dt が極端に大きくなり、
> オブジェクトが画面外に吹き飛ぶことがあります。
> `maxDeltaTime = 0.1f` に設定すると、dt は最大でも 0.1 秒に制限されます。

## ライフサイクル

GXEasy::App の各メソッドは以下の順序で呼ばれます。

```
GetConfig()     ← ウィンドウ設定を取得
    |
OnBoot()        ← GX_Init() の前（特殊な初期設定用）
    |
GX_Init()       ← エンジン初期化（自動）
    |
Start()         ← 初期化処理（1回だけ）
    |
+-- Update(dt)  ← 毎フレーム更新
|   Draw()      ← 毎フレーム描画
|   ScreenFlip  ← 画面更新（自動）
+-- ループ
    |
Release()       ← 終了処理（1回だけ）
    |
GX_End()        ← エンジン終了（自動）
```

`autoClear` と `autoPresent` がデフォルトで `true` なので、
`ClearDrawScreen()` と `ScreenFlip()` を自分で呼ぶ必要はありません。

## 移動するオブジェクトの例

矢印キーで円を動かし、画面上に情報を表示するサンプルです。

```cpp
#include "GXEasy.h"

class MovingCircleApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title  = L"Moving Circle";
        config.width  = 1280;
        config.height = 720;
        config.bgR = 12; config.bgG = 12; config.bgB = 28;
        return config;
    }

    void Start() override
    {
        m_x = 640.0f;  // 画面中央付近
        m_y = 360.0f;
        m_score = 0;
    }

    void Update(float dt) override
    {
        // deltaTime を掛けてフレームレート非依存にする
        const float speed = 300.0f;  // 1秒あたり 300 ピクセル移動
        if (CheckHitKey(KEY_INPUT_LEFT))  m_x -= speed * dt;
        if (CheckHitKey(KEY_INPUT_RIGHT)) m_x += speed * dt;
        if (CheckHitKey(KEY_INPUT_UP))    m_y -= speed * dt;
        if (CheckHitKey(KEY_INPUT_DOWN))  m_y += speed * dt;

        // 画面外に出ないよう制限
        m_x = (std::max)(30.0f, (std::min)(1250.0f, m_x));
        m_y = (std::max)(30.0f, (std::min)(690.0f, m_y));

        m_totalTime += dt;
    }

    void Draw() override
    {
        // 円を描画 (x, y, 半径, 色, 塗りつぶし)
        DrawCircle(
            static_cast<int>(m_x), static_cast<int>(m_y),
            30, GetColor(255, 200, 80), TRUE
        );

        // FormatT でテキストを整形して表示
        DrawString(10, 10,
            FormatT(TEXT("Pos: ({:.0f}, {:.0f})  Time: {:.1f}s"),
                    m_x, m_y, m_totalTime).c_str(),
            GetColor(255, 255, 255));

        DrawString(10, 35,
            TEXT("Arrow keys: Move  ESC: Quit"),
            GetColor(140, 180, 220));
    }

private:
    float m_x = 0, m_y = 0;
    float m_totalTime = 0;
    int   m_score = 0;
};

GX_EASY_APP(MovingCircleApp)
```

## FormatT について

`FormatT` は `std::format` をラップした関数で、
UNICODE / ANSI ビルドの両方に対応しています。

```cpp
// 数値の整形表示
TString text = FormatT(TEXT("Score: {}  HP: {}/{}"), score, hp, maxHp);

// 浮動小数点の桁数指定
TString fps = FormatT(TEXT("FPS: {:.1f}"), 1.0f / dt);

// 複数の値を組み合わせ
TString info = FormatT(TEXT("Player ({:.0f}, {:.0f}) - Level {}"), x, y, level);

// DrawString で画面に表示
DrawString(10, 10, text.c_str(), GetColor(255, 255, 255));
```

> **注意:** `FormatT` の引数は値渡し（by-value）です。
> これは MSVC の `std::make_format_args` が lvalue を要求するための制約です。

## CompatContext で 3D 機能にアクセスする

GXEasy::App の内部には `CompatContext` というシングルトンがあり、
Renderer3D / Camera3D / PostEffectPipeline 等の 3D オブジェクトを保持しています。
3D 描画が必要な場合は、このコンテキストを経由してアクセスします。

```cpp
#include "Compat/CompatContext.h"
#include "Graphics/3D/MeshData.h"
#include "Graphics/3D/Light.h"
#include "Graphics/3D/Material.h"

void Start() override
{
    auto& ctx      = GX_Internal::CompatContext::Instance();
    auto& renderer = ctx.renderer3D;
    auto& camera   = ctx.camera;
    auto& postFX   = ctx.postEffect;

    // ポストエフェクト設定
    postFX.SetTonemapMode(GX::TonemapMode::ACES);
    postFX.GetBloom().SetEnabled(true);
    postFX.SetFXAAEnabled(true);

    // カメラ設定
    float aspect = (float)ctx.swapChain.GetWidth() / ctx.swapChain.GetHeight();
    camera.SetPerspective(XM_PIDIV4, aspect, 0.1f, 500.0f);
    camera.SetPosition(0.0f, 5.0f, -10.0f);
    camera.LookAt({ 0.0f, 0.0f, 0.0f });

    // メッシュとマテリアルの作成
    m_mesh = renderer.CreateGPUMesh(GX::MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f));
    m_material.constants.albedoFactor = { 0.8f, 0.2f, 0.1f, 1.0f };
}
```

> 3D 描画の完全な手順は [04_Rendering3D.md](04_Rendering3D.md) を参照してください。

## ビルドと実行

GXEasy::App を使ったサンプルは `Samples/EasyHello/` にあります。

```bash
cmake --build build --config Debug --target EasyHello
```

## よくある問題

### dt が大きすぎてオブジェクトが飛ぶ

- `AppConfig::maxDeltaTime` を `0.1f` に設定してください（デフォルトで設定済み）

### ESC で終了しない

- `AppConfig::allowEscapeExit` が `true` になっているか確認してください

### 3D 描画が表示されない

- `ctx.FlushAll()` を 3D 描画前に呼んでいるか確認してください（2D バッチとの競合を防ぐ）
- `postEffect.BeginScene()` / `EndScene()` / `Resolve()` の呼び出し順を確認してください

## 次のステップ

- [07_3DScene.md](07_3DScene.md) -- Scene/Entity システムで 3D シーンを構築する
- [08_AssetPipeline.md](08_AssetPipeline.md) -- gxconv/gxpak でアセットを管理する
- [04_Rendering3D.md](04_Rendering3D.md) -- PBR/ライティング/ポストエフェクトの詳細

## Tutorial 07: 3D Scene

Entity と Scene を使った 3D シーングラフの構築方法を解説します。

## このチュートリアルで学ぶこと

- Entity（エンティティ）と Scene（シーン）の概念
- Component（コンポーネント）の追加と取得
- Transform3D による位置・回転・スケールの操作
- 親子階層によるワールド行列の合成
- ScriptComponent によるカスタムロジック
- SceneSerializer での JSON 保存・読み込み

## 前提知識

- [04_Rendering3D.md](04_Rendering3D.md) の内容（3D 描画の基本）
- [06_GXEasy2DGame.md](06_GXEasy2DGame.md) の内容（GXEasy::App の使い方）

## 概要

GXLib の Scene/Entity システムは Unity のゲームオブジェクトに近い設計です。

| 概念 | 説明 | Unity での対応 |
|------|------|---------------|
| **Scene** | エンティティのコンテナ。更新と描画を管理する | Scene |
| **Entity** | ゲーム内のオブジェクト。Transform3D を内蔵する | GameObject |
| **Component** | エンティティに機能を追加する部品 | MonoBehaviour 等 |

## Scene とエンティティの作成

```cpp
#include "Core/Scene/Scene.h"

// シーンを作成
auto scene = std::make_unique<GX::Scene>("MyScene");

// エンティティを作成（Scene が所有権を持つ）
GX::Entity* player = scene->CreateEntity("Player");
GX::Entity* enemy  = scene->CreateEntity("Enemy");

// 名前で検索
GX::Entity* found = scene->FindEntity("Player");

// エンティティ数を取得
uint32_t count = scene->GetEntityCount();  // 2

// エンティティを削除
scene->DestroyEntity(enemy);
```

> **Entity のライフサイクル**
>
> `CreateEntity()` で生成されたエンティティは Scene が所有します。
> `DestroyEntity()` を呼ぶとフレーム末に削除されます。
> Scene の破棄時に全エンティティが自動的に解放されるため、
> 手動で `delete` する必要はありません。

## Transform3D の操作

すべてのエンティティは Transform3D を内蔵しています。
位置・回転・スケールを設定して、3D 空間での配置を制御します。

```cpp
GX::Entity* box = scene->CreateEntity("Box");

// 位置の設定
box->GetTransform().SetPosition(3.0f, 1.0f, 0.0f);

// 回転の設定（ラジアン: X, Y, Z）
box->GetTransform().SetRotation(0.0f, 0.5f, 0.0f);

// スケールの設定
box->GetTransform().SetScale(2.0f, 1.0f, 1.0f);  // X方向に2倍

// 均等スケール
box->GetTransform().SetScale(1.5f);  // 全方向 1.5 倍

// 現在の値を取得
auto pos = box->GetTransform().GetPosition();  // XMFLOAT3
auto rot = box->GetTransform().GetRotation();  // XMFLOAT3 (Euler)
auto scl = box->GetTransform().GetScale();     // XMFLOAT3
```

## Component の追加

コンポーネントはエンティティに機能を追加する部品です。
テンプレートメソッドで追加・取得・削除を行います。

```cpp
#include "Core/Scene/Components.h"

GX::Entity* obj = scene->CreateEntity("MyObject");

// MeshRenderer コンポーネントを追加
auto* meshComp = obj->AddComponent<GX::MeshRendererComponent>();
meshComp->model = myModel;      // Model* を設定
meshComp->castShadow = true;    // 影を落とす

// コンポーネントの取得（存在しない場合は nullptr）
auto* mesh = obj->GetComponent<GX::MeshRendererComponent>();
if (mesh)
{
    mesh->castShadow = false;
}

// コンポーネントの存在確認
if (obj->HasComponent<GX::MeshRendererComponent>())
{
    // MeshRenderer が付いている
}

// コンポーネントの削除
obj->RemoveComponent<GX::MeshRendererComponent>();
```

### ビルトインコンポーネント一覧

| コンポーネント | 説明 | 用途 |
|---------------|------|------|
| MeshRendererComponent | 静的メッシュの描画 | 建物、小物、地形 |
| SkinnedMeshRendererComponent | スケルタルアニメーション付きメッシュ | キャラクター |
| CameraComponent | カメラ | プレイヤー視点、監視カメラ |
| LightComponent | ライト | 照明、松明 |
| ScriptComponent | カスタムロジック | 移動、回転、ゲームルール |
| AudioSourceComponent | 音源 | 効果音、環境音 |
| TerrainComponent | 地形 | フィールド |
| LODComponent | LOD グループ | 遠距離の描画最適化 |
| ParticleSystemComponent | パーティクル | エフェクト |

## ScriptComponent でカスタムロジック

ScriptComponent はラムダ式でカスタムロジックを記述できるコンポーネントです。
`onUpdate` が毎フレーム呼ばれ、`onStart` は初回の `Scene::Update()` で 1 回だけ呼ばれます。

```cpp
auto* entity = scene->CreateEntity("RotatingCube");
entity->GetTransform().SetPosition(0, 1.0f, 0);

auto* script = entity->AddComponent<GX::ScriptComponent>();

// 初回に1回だけ実行
script->onStart = []()
{
    // 初期化処理
};

// 毎フレーム実行（dt = 経過時間）
script->onUpdate = [entity](float dt)
{
    auto rot = entity->GetTransform().GetRotation();
    entity->GetTransform().SetRotation(
        rot.x,
        rot.y + dt * 1.0f,  // Y軸に毎秒1ラジアン回転
        rot.z
    );
};
```

## 親子階層

エンティティは親子関係を持つことができます。
子エンティティの Transform は親の Transform に追従します。

```cpp
// ロボットアームの例
auto* base = scene->CreateEntity("Base");
base->GetTransform().SetPosition(0, 0.5f, 0);

auto* arm = scene->CreateEntity("Arm");
arm->SetParent(base);                          // base の子にする
arm->GetTransform().SetPosition(0, 1.5f, 0);  // 親からの相対位置

auto* hand = scene->CreateEntity("Hand");
hand->SetParent(arm);                          // arm の子にする
hand->GetTransform().SetPosition(0, 1.0f, 0); // arm からの相対位置
```

親を回転させると、子も一緒に回転します。

```cpp
// ベースを回転させると、arm と hand も追従する
auto* baseScript = base->AddComponent<GX::ScriptComponent>();
baseScript->onUpdate = [base](float dt)
{
    auto rot = base->GetTransform().GetRotation();
    base->GetTransform().SetRotation(rot.x, rot.y + dt * 0.5f, rot.z);
};
```

### ワールド行列の取得

親子関係を考慮したワールド行列（最終的な変換行列）を取得するには、
`GetWorldMatrix()` を使います。

```cpp
// ワールド行列: 親の変換を再帰的に合成した最終行列
XMMATRIX worldMat = hand->GetWorldMatrix();

// 描画時に使用
renderer.DrawMesh(mesh, worldMat);
```

> `GetTransform()` はローカル変換（親からの相対）を返します。
> `GetWorldMatrix()` はワールド変換（全ての親を合成した絶対）を返します。

## Scene の更新と描画

```cpp
// 毎フレームの更新（ScriptComponent の onUpdate が呼ばれる）
scene->Update(deltaTime);

// 描画方法1: カリングなし（全エンティティ描画）
scene->Render(renderer);

// 描画方法2: フラスタムカリングあり（カメラの視界外を除外）
scene->Render(renderer, camera);

// 描画統計を確認
auto stats = scene->GetLastRenderStats();
// stats.totalEntities   — 全エンティティ数
// stats.visibleEntities — 描画されたエンティティ数
// stats.culledEntities  — カリングで除外されたエンティティ数
// stats.drawCalls       — 描画コール数
```

## SceneSerializer で保存・読み込み

シーンの状態を JSON ファイルに保存し、後から復元できます。

```cpp
#include "Core/Scene/SceneSerializer.h"

// --- 保存 ---
GX::SceneSerializer::SaveToJson(*scene, "Saves/level01.json");

// --- 読み込み ---
auto loadedScene = std::make_unique<GX::Scene>();

// ModelLoadCallback: JSONに記録されたモデルパスからModel*を解決する関数
GX::SceneSerializer::LoadFromJson(*loadedScene, "Saves/level01.json",
    [&](const std::string& path) -> GX::Model*
    {
        // パスに応じてモデルをロードして返す
        return modelMap[path];
    }
);
```

### JSON の構造例

```json
{
    "name": "MyScene",
    "entities": [
        {
            "id": 1,
            "name": "Player",
            "active": true,
            "transform": {
                "position": [0.0, 1.0, 0.0],
                "rotation": [0.0, 0.0, 0.0],
                "scale": [1.0, 1.0, 1.0]
            },
            "components": [
                {
                    "type": "MeshRenderer",
                    "sourcePath": "Assets/models/player.gxmd",
                    "castShadow": true
                }
            ],
            "children": [2, 3]
        }
    ]
}
```

> **ModelLoadCallback が必要な理由**
>
> Model は GPU リソースを含むため、JSON にそのまま保存できません。
> 代わりにファイルパスだけを保存し、読み込み時にコールバックで
> 実際の Model* を解決します。

## 完全なサンプル

`Samples/SceneShowcase/` にシーングラフの完全なサンプルがあります。
ロボットアームの親子階層と、独立して回転するキューブ群を表示します。

```bash
cmake --build build --config Debug --target SceneShowcase
```

## よくある問題

### エンティティが描画されない

- `MeshRendererComponent` を追加して `model` を設定しているか確認
- `scene->Render(renderer)` を呼んでいるか確認
- エンティティの `IsActive()` が `true` か確認

### 親子関係が反映されない

- `SetParent()` を呼んだ後に `scene->Update()` を実行しているか確認
- ローカル座標が意図通りか確認（`SetPosition` は親からの相対位置）

### ScriptComponent の onUpdate が呼ばれない

- `scene->Update(dt)` を毎フレーム呼んでいるか確認
- `ScriptComponent` の `SetEnabled(false)` になっていないか確認

### JSON の読み込みでモデルが表示されない

- `ModelLoadCallback` を正しく実装しているか確認
- コールバック内でモデルを読み込んで返しているか確認

## 次のステップ

- [08_AssetPipeline.md](08_AssetPipeline.md) -- gxconv/gxpak でアセットを管理する
- [04_Rendering3D.md](04_Rendering3D.md) -- PBR マテリアルとポストエフェクトの詳細
- [05_GUI.md](05_GUI.md) -- XML+CSS による GUI 構築

## Tutorial 08: Asset Pipeline

gxconv でモデルを変換し、gxpak でバンドル化し、VFS でランタイム読み込みする
一連のワークフローを解説します。

## このチュートリアルで学ぶこと

- GXMD / GXAN バイナリ形式の概要
- gxconv によるモデル変換手順
- gxpak によるアセットバンドル作成
- VFS (仮想ファイルシステム) と PakFileProvider
- bone_matcher によるボーンマッチング（アニメーションリターゲット）

## 前提知識

- [04_Rendering3D.md](04_Rendering3D.md) の内容（3D モデルの読み込みと描画）
- コマンドプロンプトの基本操作

## なぜ独自フォーマットが必要か

glTF や FBX は汎用的で便利ですが、ゲームのランタイムには無駄が多い形式です。

| 項目 | glTF / FBX | GXMD / GXAN |
|------|-----------|-------------|
| 解析速度 | JSON/XML パース + バイナリ変換が必要 | ヘッダ + 固定オフセット読み込みで即座にロード |
| 頂点形式 | 各ツールで異なる | GXLib の頂点構造体とバイナリ互換 |
| マテリアル | PBR パラメータの変換が必要 | ShaderModelParams (256B) をそのままGPUに送れる |
| ファイルサイズ | テキスト JSON を含む | バイナリのみ。GXPAK で LZ4 圧縮も可能 |

## GXMD 形式 (.gxmd)

GXMD はメッシュ・マテリアル・スケルトン・アニメーションを単一バイナリにまとめた
3D モデル形式です。

### ファイル構造

```
[FileHeader 128B]           ← マジック、バージョン、各チャンクへのオフセット
[StringTable]               ← メッシュ名、マテリアル名、ボーン名等の UTF-8 文字列
[MeshChunk x meshCount]     ← メッシュごとの頂点数・インデックス数・AABB
[MaterialChunk x matCount]  ← マテリアルごとの ShaderModel + パラメータ 256B
[VertexData]                ← 頂点データ (Standard 48B or Skinned 80B)
[IndexData]                 ← インデックスデータ (16bit or 32bit)
[BoneData]                  ← ボーン階層 + 逆バインド行列
[AnimationData]             ← キーフレームアニメーション
```

### 頂点形式

| 型 | サイズ | 内容 | GXLib 対応 |
|----|--------|------|-----------|
| VertexStandard | 48B | position + normal + uv + tangent | Vertex3D_PBR |
| VertexSkinned | 80B | Standard + joints(4) + weights(4) | Vertex3D_Skinned |

> 頂点構造体は GXLib のランタイム頂点型とバイナリ互換です。
> memcpy でそのまま GPU バッファにコピーできます。

## GXAN 形式 (.gxan)

GXAN はスタンドアロンのアニメーション形式です。
スケルトンに依存せず、**ボーン名ベース**でチャネルを定義するため、
異なるモデル間でアニメーションを共有できます。

```
[GxanHeader 64B]                ← マジック、チャネル数、再生時間
[StringTable]                   ← ボーン名の文字列テーブル
[GxanChannelDesc x channelCount] ← 各チャネルのボーン名・ターゲット・キー数
[KeyData]                       ← VectorKey (T/S用) / QuatKey (R用)
```

## gxconv: モデル変換ツール

gxconv は OBJ / FBX / glTF を GXMD / GXAN 形式に変換する CLI ツールです。

### 基本的な使い方

```bash
# glTF → GXMD 変換（出力ファイル名は自動で .gxmd になる）
gxconv character.gltf character.gxmd

# FBX → GXMD 変換
gxconv character.fbx character.gxmd

# OBJ → GXMD 変換
gxconv scene.obj scene.gxmd
```

### オプション

```bash
# ファイル情報の表示（変換せずに内容を確認）
gxconv character.gltf --info

# シェーダーモデルを指定（デフォルトは standard = PBR）
gxconv character.gltf character.gxmd --shader-model toon

# Toon アウトラインの幅を指定
gxconv character.gltf character.gxmd --shader-model toon --toon-outline 0.002

# アニメーションを除外（メッシュのみ）
gxconv character.fbx character.gxmd --no-anim

# アニメーションのみを GXAN として出力
gxconv character.fbx walk.gxan --anim-only

# 16bit インデックスを強制（頂点数 65535 以下の場合にファイルサイズ削減）
gxconv scene.obj scene.gxmd --index16
```

### 対応シェーダーモデル

| 名前 | 説明 |
|------|------|
| standard | PBR（デフォルト） |
| unlit | ライティングなし |
| toon | トゥーンシェーディング (UTS2) |
| phong | Phong シェーディング |
| subsurface | サブサーフェス散乱 |
| clearcoat | クリアコート |

### 変換例: Blender からの完全な手順

```bash
# 1. Blender から glTF でエクスポート
#    Blender > File > Export > glTF 2.0 (.glb/.gltf)
#    設定: Format=glTF Binary (.glb), Mesh + Armature + Animation にチェック

# 2. GXMD に変換（メッシュ + スケルトン + アニメーション）
gxconv character.glb character.gxmd

# 3. 追加アニメーションを GXAN として別途出力
gxconv run_anim.glb run.gxan --anim-only

# 4. 変換結果を確認
gxconv character.gxmd --info
```

## gxpak: アセットバンドルツール

gxpak は複数のアセットを単一の .gxpak ファイルにまとめるツールです。
LZ4 圧縮に対応しており、配布サイズの削減と読み込み高速化を実現します。

### パック（バンドル作成）

```bash
# ディレクトリ内の全ファイルをバンドル化
gxpak pack -o game_assets.gxpak -d Assets/

# LZ4 圧縮を有効にしてバンドル
gxpak pack -o game_assets.gxpak -d Assets/ --compress
```

ディレクトリ構造の例:

```
Assets/
  models/
    character.gxmd
    enemy.gxmd
  animations/
    walk.gxan
    run.gxan
  textures/
    character_albedo.png
    character_normal.png
```

### 一覧表示

```bash
gxpak list -i game_assets.gxpak
```

出力例:

```
GXPAK: game_assets.gxpak
  Version: 1, Entries: 6

  [0] Model    models/character.gxmd  (124800 -> 89600 bytes, 71.8%)
  [1] Model    models/enemy.gxmd  (98304 bytes)
  [2] Anim     animations/walk.gxan  (8192 -> 5120 bytes, 62.5%)
  [3] Anim     animations/run.gxan  (6144 -> 4096 bytes, 66.7%)
  [4] Tex      textures/character_albedo.png  (262144 bytes)
  [5] Tex      textures/character_normal.png  (131072 bytes)
```

### 展開（アンパック）

```bash
gxpak unpack -i game_assets.gxpak -d output/
```

## VFS と PakFileProvider

GXLib の VFS（仮想ファイルシステム）は、ファイルの読み込み元を抽象化する仕組みです。
PakFileProvider を登録すると、GXPAK バンドル内のファイルを通常のファイルパスで
読み込めるようになります。

### VFS の仕組み

```
アプリケーション
    |
    v
FileSystem (VFS)
    |
    +-- PhysicalFileProvider (優先度: 0)   ← ディスク上のファイル
    +-- PakFileProvider      (優先度: 100) ← .gxpak 内のファイル
```

PakFileProvider は優先度 100 で登録されるため、
同じパスのファイルがディスクとバンドルの両方にある場合、
バンドル側が優先されます。

### 使い方

```cpp
#include "IO/FileSystem.h"
#include "IO/PakFileProvider.h"

// FileSystem を初期化
GX::FileSystem fs;

// GXPAK をマウント
auto pakProvider = std::make_shared<GX::PakFileProvider>();
if (pakProvider->Open("game_assets.gxpak"))
{
    fs.Mount(pakProvider);  // VFS に登録
}

// 通常のファイルパスで読み込み（VFS が自動的にバンドル内を検索）
auto data = fs.ReadFile("models/character.gxmd");
if (data.IsValid())
{
    // data.Data() — バイトデータへのポインタ
    // data.Size() — バイト数
    // LZ4 圧縮エントリは自動的に伸長される
}

// ファイルの存在確認
if (fs.Exists("textures/character_albedo.png"))
{
    // バンドル内にファイルが存在する
}
```

### gxloader でのメモリ読み込み

gxloader にはメモリバッファから直接ロードする関数があり、
VFS と組み合わせて使えます。

```cpp
#include <model_loader.h>
#include <anim_loader.h>

// VFS からバイトデータを取得
auto modelData = fs.ReadFile("models/character.gxmd");

// メモリから直接 GXMD をロード（ファイル I/O なし）
auto loadedModel = gxloader::LoadGxmdFromMemory(
    modelData.Data(), modelData.Size()
);

// 同様にアニメーションもメモリからロード
auto animData = fs.ReadFile("animations/walk.gxan");
auto loadedAnim = gxloader::LoadGxanFromMemory(
    animData.Data(), animData.Size()
);
```

## ボーンマッチング（リターゲット）

異なるツールで作られたモデルとアニメーションでは、
ボーン名の命名規則が異なることがあります。
bone_matcher はこの問題を 4 段階のフォールバック戦略で解決します。

### フォールバック戦略

| ステップ | 方法 | 例 |
|---------|------|-----|
| 1 | 完全一致 | `Hips` == `Hips` |
| 2 | 大文字小文字無視 | `hips` == `Hips` |
| 3 | プレフィックス除去 + 大文字小文字無視 | `mixamorig:Hips` -> `hips` == `Hips` |
| 4 | 数値サフィックス除去 + ステップ3 | `mixamorig:Hips.001` -> `hips` == `Hips` |

### 使い方

```cpp
#include <bone_matcher.h>
#include <anim_loader.h>
#include <model_loader.h>

// モデルのスケルトンからボーン名一覧を取得
std::vector<std::string> skeletonNames;
for (const auto& joint : loadedModel->joints)
{
    skeletonNames.push_back(joint.name);
}

// GXAN のチャネルをモデルのボーンに対応付ける
for (const auto& channel : loadedAnim->channels)
{
    int boneIndex = gxloader::MatchBoneName(
        channel.boneName,   // アニメーション側のボーン名
        skeletonNames       // モデル側のボーン名一覧
    );

    if (boneIndex >= 0)
    {
        // マッチ成功: channel のキーフレームを boneIndex に適用
    }
    else
    {
        // マッチ失敗: このボーンのアニメーションはスキップ
    }
}
```

> **NormalizeBoneName** を使うと、ボーン名を正規化して比較に使えます。
>
> ```cpp
> std::string normalized = gxloader::NormalizeBoneName("mixamorig:LeftUpperArm.001");
> // → "leftupperarm"
> ```

### よくあるプレフィックスパターン

| ツール | プレフィックス例 |
|--------|-----------------|
| Mixamo | `mixamorig:` |
| Blender | `Armature\|` |
| 数値サフィックス | `.001`, `.002` |

bone_matcher はこれらを自動的に除去してマッチングを試みます。

## 実践ワークフロー

ゲーム開発における典型的なアセットパイプラインの流れです。

```
[Blender/Maya]
    |  glTF / FBX エクスポート
    v
[gxconv]
    |  .gxmd / .gxan に変換
    v
[Assets/ フォルダ]
    |  開発中はファイルを直接読み込み
    v
[gxpak pack]
    |  リリース時に .gxpak にバンドル
    v
[配布パッケージ]
    game.exe + game_assets.gxpak
```

開発中は Assets フォルダから直接読み込み、
リリースビルドでは GXPAK をマウントして配布します。

## よくある問題

### gxconv で「Unknown format」エラー

- 入力ファイルの拡張子が `.obj`, `.fbx`, `.gltf`, `.glb` のいずれかであることを確認
- ファイルが壊れていないことを確認（Blender で開き直してみる）

### GXPAK 内のテクスチャが読み込めない

- バンドル内のパスが正しいか `gxpak list` で確認
- VFS に PakFileProvider をマウントしているか確認
- テクスチャの読み込みが VFS 経由になっているか確認

### ボーンマッチングが失敗する

- `gxconv --info` でモデルのボーン名一覧を確認
- アニメーション側のボーン名を確認（Mixamo はプレフィックスが付く）
- 4 段階全てで不一致の場合は、ボーン名を手動でリネームする必要があります

### GXMD のファイルサイズが大きい

- `--index16` オプションで 16bit インデックスを使用（頂点数 65535 以下の場合）
- GXPAK の `--compress` で LZ4 圧縮を適用

## 次のステップ

- [07_3DScene.md](07_3DScene.md) -- Scene/Entity でシーンを構築する
- [04_Rendering3D.md](04_Rendering3D.md) -- PBR/シェーダーモデル/ポストエフェクト
- [06_GXEasy2DGame.md](06_GXEasy2DGame.md) -- GXEasy::App の使い方


---

# Part IV: DxLib マイグレーションガイド

## 概要

GXLib は DXLib 互換の簡易 API（Compat API）を提供しています。
`Compat/GXLib.h` をインクルードすることで、DXLib に似た関数群を使用できます。

> **Compat API とは？**
>
> DXLib で書かれたコードを最小限の変更で GXLib に移行できるようにする互換レイヤーです。
> 関数名と引数がほぼ同じなので、DXLib の経験がそのまま活かせます。
> ただし一部の制限があります（後述の「非互換項目」参照）。

## 移行手順

### 1. ヘッダーの変更

```cpp
// 変更前 (DXLib)
#include <DxLib.h>

// 変更後 (GXLib)
#include "Compat/GXLib.h"
```

### 2. 初期化・終了の変更

```cpp
// 変更前 (DXLib)
DxLib_Init();   // DXLib の初期化
DxLib_End();    // DXLib の終了

// 変更後 (GXLib)
GX_Init();      // GXLib の初期化（内部で DirectX 12 を初期化）
GX_End();       // GXLib の終了（GPU リソースを解放）
```

### 3. 定数名の変更

プレフィックスが `DX_` から `GX_` に変わります。

| DXLib | GXLib | 説明 |
|---|---|---|
| `DX_SCREEN_BACK` | `GX_SCREEN_BACK` | 裏画面（ダブルバッファリング用） |
| `DX_SCREEN_FRONT` | `GX_SCREEN_FRONT` | 表画面 |
| `DX_BLENDMODE_NOBLEND` | `GX_BLENDMODE_NOBLEND` | ブレンドなし（通常描画） |
| `DX_BLENDMODE_ALPHA` | `GX_BLENDMODE_ALPHA` | アルファブレンド（半透明） |
| `DX_BLENDMODE_ADD` | `GX_BLENDMODE_ADD` | 加算合成（光のエフェクト向き） |
| `DX_BLENDMODE_SUB` | `GX_BLENDMODE_SUB` | 減算合成 |
| `DX_BLENDMODE_MUL` | `GX_BLENDMODE_MUL` | 乗算合成 |
| `DX_PLAYTYPE_NORMAL` | `GX_PLAYTYPE_NORMAL` | 通常再生（完了まで停止） |
| `DX_PLAYTYPE_BACK` | `GX_PLAYTYPE_BACK` | バックグラウンド再生（推奨） |
| `DX_PLAYTYPE_LOOP` | `GX_PLAYTYPE_LOOP` | ループ再生 |
| `DX_FONTTYPE_NORMAL` | `GX_FONTTYPE_NORMAL` | 通常フォント |
| `DX_INPUT_PAD1` | `GX_INPUT_PAD1` | ゲームパッド1 |

### 4. 完全な移行コード例

以下は DXLib 版の簡単なゲームを GXLib に移行する例です。

**DXLib 版（移行前）:**
```cpp
#include <DxLib.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(640, 480, 32);
    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    int playerX = 320, playerY = 400;
    int tex = LoadGraph("player.png");

    while (ProcessMessage() == 0)
    {
        ClearDrawScreen();

        char keys[256];
        GetHitKeyStateAll(keys);
        if (keys[KEY_INPUT_LEFT])  playerX -= 5;
        if (keys[KEY_INPUT_RIGHT]) playerX += 5;

        DrawGraph(playerX, playerY, tex, TRUE);
        DrawFormatString(10, 10, GetColor(255, 255, 255), "X: %d", playerX);

        ScreenFlip();
    }

    DeleteGraph(tex);
    DxLib_End();
    return 0;
}
```

**GXLib 版（移行後）:**
```cpp
#include "Compat/GXLib.h"  // ← ヘッダー変更

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    ChangeWindowMode(TRUE);
    SetGraphMode(640, 480, 32);
    if (GX_Init() == -1) return -1;      // ← DxLib_Init → GX_Init
    SetDrawScreen(GX_SCREEN_BACK);       // ← DX_ → GX_

    int playerX = 320, playerY = 400;
    int tex = LoadGraph("player.png");   // 同名関数 — 変更不要

    while (ProcessMessage() == 0)        // 同名関数 — 変更不要
    {
        ClearDrawScreen();               // 同名関数 — 変更不要

        char keys[256];
        GetHitKeyStateAll(keys);         // 同名関数 — 変更不要
        if (keys[KEY_INPUT_LEFT])  playerX -= 5;
        if (keys[KEY_INPUT_RIGHT]) playerX += 5;

        DrawGraph(playerX, playerY, tex, TRUE);  // 同名関数 — 変更不要
        DrawFormatString(10, 10, GetColor(255, 255, 255), "X: %d", playerX);

        ScreenFlip();                    // 同名関数 — 変更不要
    }

    DeleteGraph(tex);                    // 同名関数 — 変更不要
    GX_End();                            // ← DxLib_End → GX_End
    return 0;
}
```

> **まとめ: 変更が必要なのは3箇所だけ**
> 1. `#include` ヘッダー
> 2. `DxLib_Init()` → `GX_Init()` / `DxLib_End()` → `GX_End()`
> 3. `DX_` プレフィックス → `GX_` プレフィックス

### 5. 関数対応表

#### システム

| DXLib | GXLib | 備考 |
|---|---|---|
| `DxLib_Init()` | `GX_Init()` | |
| `DxLib_End()` | `GX_End()` | |
| `ProcessMessage()` | `ProcessMessage()` | 同名 |
| `SetMainWindowText()` | `SetMainWindowText()` | 同名 |
| `ChangeWindowMode()` | `ChangeWindowMode()` | 同名 |
| `SetGraphMode()` | `SetGraphMode()` | 同名 |
| `GetColor()` | `GetColor()` | 戻り値: 0xFFRRGGBB |
| `GetNowCount()` | `GetNowCount()` | 同名 |
| `SetDrawScreen()` | `SetDrawScreen()` | 同名 |
| `ClearDrawScreen()` | `ClearDrawScreen()` | 同名 |
| `ScreenFlip()` | `ScreenFlip()` | 同名 |
| `SetBackgroundColor()` | `SetBackgroundColor()` | 同名 |

#### 2D描画

| DXLib | GXLib | 備考 |
|---|---|---|
| `LoadGraph()` | `LoadGraph()` | 同名 |
| `DeleteGraph()` | `DeleteGraph()` | 同名 |
| `LoadDivGraph()` | `LoadDivGraph()` | 同名 |
| `GetGraphSize()` | `GetGraphSize()` | 同名 |
| `DrawGraph()` | `DrawGraph()` | 同名 |
| `DrawRotaGraph()` | `DrawRotaGraph()` | 同名 |
| `DrawExtendGraph()` | `DrawExtendGraph()` | 同名 |
| `DrawRectGraph()` | `DrawRectGraph()` | 同名 |
| `DrawModiGraph()` | `DrawModiGraph()` | 同名 |

#### プリミティブ

| DXLib | GXLib | 備考 |
|---|---|---|
| `DrawLine()` | `DrawLine()` | 同名 |
| `DrawBox()` | `DrawBox()` | 同名 |
| `DrawCircle()` | `DrawCircle()` | 同名 |
| `DrawTriangle()` | `DrawTriangle()` | 同名 |
| `DrawOval()` | `DrawOval()` | 同名 |
| `DrawPixel()` | `DrawPixel()` | 同名 |

#### ブレンドモード

| DXLib | GXLib | 備考 |
|---|---|---|
| `SetDrawBlendMode()` | `SetDrawBlendMode()` | 定数名が DX_ → GX_ に変更 |
| `SetDrawBright()` | `SetDrawBright()` | 同名 |

#### テキスト

| DXLib | GXLib | 備考 |
|---|---|---|
| `DrawString()` | `DrawString()` | Compat API は ASCII のみ |
| `DrawFormatString()` | `DrawFormatString()` | Compat API は ASCII のみ |
| `GetDrawStringWidth()` | `GetDrawStringWidth()` | 同名 |
| `CreateFontToHandle()` | `CreateFontToHandle()` | 同名 |
| `DeleteFontToHandle()` | `DeleteFontToHandle()` | 同名 |
| `DrawStringToHandle()` | `DrawStringToHandle()` | Compat API は ASCII のみ |
| `DrawFormatStringToHandle()` | `DrawFormatStringToHandle()` | Compat API は ASCII のみ |
| `GetDrawStringWidthToHandle()` | `GetDrawStringWidthToHandle()` | 同名 |

#### 入力

| DXLib | GXLib | 備考 |
|---|---|---|
| `CheckHitKey()` | `CheckHitKey()` | DIK (DirectInput Key) コード互換 |
| `GetHitKeyStateAll()` | `GetHitKeyStateAll()` | 同名 |
| `GetMouseInput()` | `GetMouseInput()` | 同名 |
| `GetMousePoint()` | `GetMousePoint()` | 同名 |
| `GetMouseWheelRotVol()` | `GetMouseWheelRotVol()` | 同名 |
| `GetJoypadInputState()` | `GetJoypadInputState()` | 同名 |

> **DIK コードとは？**
>
> DirectInput で定義されたキーボードのキーコードです。
> `KEY_INPUT_SPACE`, `KEY_INPUT_LEFT` などの定数名で使用します。
> GXLib では DXLib と同じキーコード定数をサポートしており、そのまま移行できます。

#### サウンド

| DXLib | GXLib | 備考 |
|---|---|---|
| `LoadSoundMem()` | `LoadSoundMem()` | 同名 |
| `PlaySoundMem()` | `PlaySoundMem()` | 定数名が DX_ → GX_ |
| `StopSoundMem()` | `StopSoundMem()` | 同名 |
| `DeleteSoundMem()` | `DeleteSoundMem()` | 同名 |
| `ChangeVolumeSoundMem()` | `ChangeVolumeSoundMem()` | 0-255 |
| `CheckSoundMem()` | `CheckSoundMem()` | 同名 |
| `PlayMusic()` | `PlayMusic()` | 同名 |
| `StopMusic()` | `StopMusic()` | 同名 |
| `CheckMusic()` | `CheckMusic()` | 同名 |

#### 3D

| DXLib | GXLib | 備考 |
|---|---|---|
| `MV1LoadModel()` | `LoadModel()` | 関数名変更、glTF / GXMD 形式対応 |
| `MV1DeleteModel()` | `DeleteModel()` | 関数名変更 |
| `MV1DrawModel()` | `DrawModel()` | 関数名変更 |
| `MV1SetPosition()` | `SetModelPosition()` | 関数名変更 |
| `MV1SetScale()` | `SetModelScale()` | 関数名変更 |
| `MV1SetRotationXYZ()` | `SetModelRotation()` | 関数名変更 |
| `SetCameraPositionAndTarget_UpVecY()` | `SetCameraPositionAndTarget()` | Up固定(Y軸) |
| `SetCameraNearFar()` | `SetCameraNearFar()` | 同名 |

#### 数学

| DXLib | GXLib | 備考 |
|---|---|---|
| `VGet()` | `VGet()` | 同名 |
| `VAdd()` | `VAdd()` | 同名 |
| `VSub()` | `VSub()` | 同名 |
| `VScale()` | `VScale()` | 同名 |
| `VDot()` | `VDot()` | 同名 |
| `VCross()` | `VCross()` | 同名 |
| `VNorm()` | `VNorm()` | 同名 |
| `VSize()` | `VSize()` | 同名 |
| `MGetIdent()` | `MGetIdent()` | 同名 |
| `MMult()` | `MMult()` | 同名 |
| `MGetRotX/Y/Z()` | `MGetRotX/Y/Z()` | 同名 |
| `MGetTranslate()` | `MGetTranslate()` | 同名 |

## 非互換項目

### Compat API の制限 vs ネイティブ API

| 制限事項 | Compat API (互換レイヤー) | ネイティブ API |
|----------|--------------------------|---------------|
| **テキスト** | ASCII のみ (英数字・記号) | Unicode フルサポート (日本語OK) |
| **フォント** | CreateFontToHandle (基本機能) | FontManager (アトラス管理、動的拡張) |
| **トリガー入力** | 非対応 (押下中のみ) | IsKeyTriggered で押した瞬間を検出 |
| **ポストエフェクト** | 非対応 | PostEffectPipeline で HDR + 全エフェクト |
| **GUI** | 非対応 | XML + CSS 宣言的 UI |
| **物理演算** | 非対応 | PhysicsWorld2D / PhysicsWorld3D (Jolt) |
| **ネットワーク** | 非対応 | TCP / UDP / HTTP / WebSocket |

### カラー値
- DXLib: `GetColor()` は内部形式のカラー値を返す
- GXLib: `GetColor()` は `0xFFRRGGBB` 形式を返す

### 3D モデル形式
- DXLib: `.x`, `.mv1` 形式に対応
- GXLib: **glTF (.gltf/.glb)** および **GXMD (.gxmd)** に対応（Blender 等からエクスポート、gxconv で変換可能）
- gxconv ツールで FBX/OBJ/glTF → .gxmd/.gxan への一括変換が可能

### サウンド形式
- DXLib: `.mp3`, `.ogg`, `.wav` 等に広く対応
- GXLib: `.wav` のみ対応（XAudio2 ベース）

### 描画方式の違い
- DXLib: Direct3D 9/11 ベース（即時モード描画）
- GXLib: **Direct3D 12 ベース**（コマンドリスト、バッチ描画）
- 描画順序やタイミングが異なる場合がある

> **即時モードとコマンドリストの違い**
>
> DXLib (D3D9/11) は `DrawGraph()` を呼んだ瞬間に描画されます（即時モード）。
> GXLib (D3D12) は描画命令をコマンドリストに記録し、後でまとめて実行します。
> Compat API ではこの違いを内部で吸収していますが、
> 描画順序に依存するコードでは挙動が異なる場合があります。

### 未実装の DXLib 関数

以下の DXLib 関数は GXLib に互換実装がありません:

| 関数 | 機能 |
|------|------|
| `SetTransColor()` | 透過色指定 |
| `SetDrawArea()` | 描画領域制限 |
| `SaveDrawScreen()` | スクリーンショット保存 |
| `MakeScreen()` | オフスクリーン描画 |
| `GetDrawScreenGraph()` | 画面キャプチャ |
| `SetFontSize()` | デフォルトフォントサイズ変更 |
| ネットワーク関連 | DXLib の HTTP 関数は未対応（GXLib はネイティブ API のみ） |

## GXLib 固有の機能

DXLib にない GXLib 独自機能です。移行後にステップアップとして活用できます。

| 機能 | 説明 | 関連チュートリアル |
|------|------|-------------------|
| **HDR レンダリング + ポストエフェクト** | Bloom, SSAO, SSR, DoF, TAA 等 | [04_Rendering3D](../../docs/tutorials/04_Rendering3D.md) |
| **DXR レイトレーシング反射** | ハードウェアレイトレーシングによるリアルな反射 (SSR と排他) | [API リファレンス](../index.html) |
| **DXR RTGI** | グローバルイルミネーション (間接光の自動計算) | [API リファレンス](../index.html) |
| **PBR マテリアル** | メタリック/ラフネスによるリアルな質感 | [04_Rendering3D](../../docs/tutorials/04_Rendering3D.md) |
| **Toon シェーダー (UTS2)** | セルシェーディング、3ゾーン、スムース法線アウトライン | [04_Rendering3D](../../docs/tutorials/04_Rendering3D.md) |
| **ShaderRegistry** | 6 種シェーダーモデル × static/skinned = 14 PSO 自動管理 | [API リファレンス](../index.html) |
| **アニメーションブレンド** | BlendStack / BlendTree / AnimatorStateMachine | [API リファレンス](../index.html) |
| **アセットパイプライン** | gxconv (モデル変換) / gxloader (ランタイムローダー) / gxpak (バンドル) | [API リファレンス](../index.html) |
| **GXModelViewer** | ImGui Docking ベースの 3D モデルビューア / エディタ | — |
| **レンダーレイヤーシステム** | Scene + UI の分離合成 | [API リファレンス](../index.html) |
| **XML + CSS GUI** | 宣言的UIシステム | [05_GUI](../../docs/tutorials/05_GUI.md) |
| **VFS + アーカイブ** | 暗号化アーカイブ (AES-256 + LZ4) | [API リファレンス](../index.html) |
| **シェーダーホットリロード** | F9 キーで HLSL を再コンパイル | — |
| **GPU プロファイラ** | P キーで GPU 処理時間を表示 | — |
| **Jolt Physics 3D** | 物理シミュレーション | [API リファレンス](../index.html) |
| **WebSocket / HTTP クライアント** | ネットワーク通信 | [API リファレンス](../index.html) |

## 移行後のステップアップ

Compat API でゲームが動くようになったら、段階的にネイティブ API に移行すると
GXLib の全機能を活用できます。

### ステップ 1: テキストを日本語対応にする

```cpp
// Compat API (ASCII のみ)
DrawString(10, 10, "Score: 100", GetColor(255, 255, 255));

// ネイティブ API (日本語対応)
textRenderer.DrawString(fontHandle, 10, 10, L"スコア: 100", 0xFFFFFFFF);
```

### ステップ 2: トリガー入力を追加する

```cpp
// Compat API (押下中ずっと TRUE)
if (CheckHitKey(KEY_INPUT_SPACE)) { /* 押している間ずっと実行 */ }

// ネイティブ API (押した瞬間だけ TRUE)
if (input.GetKeyboard().IsKeyTriggered(VK_SPACE)) { /* 1回だけ実行 */ }
```

### ステップ 3: ポストエフェクトを追加する

Compat API の描画をそのまま使いつつ、PostEffectPipeline を追加して
Bloom や FXAA を有効にすることもできます。詳しくは [04_Rendering3D](../../docs/tutorials/04_Rendering3D.md) を参照。

## 用語集

専門用語がわからない場合は [用語集 (Glossary)](../Glossary.md) を参照してください。


---

# Part V: API リファレンス原稿

## Audio API

名前空間: `GX`。XAudio2 ベースのオーディオシステム。ハンドル管理、3D 空間音響対応。

## AudioManager

ハンドルベースのサウンド管理クラス。同一パスの二重読み込みはキャッシュで防止される。

### 初期化・終了

| メソッド | 説明 |
|---|---|
| `bool Initialize()` | オーディオシステム全体を初期化する |
| `void Shutdown()` | 全サウンドを解放してデバイスを破棄する |
| `void Update(float deltaTime)` | BGM フェード、3D 計算、SE クリーンアップを行う |

### サウンド読み込み・解放

| メソッド | 説明 |
|---|---|
| `int LoadSound(const std::wstring& filePath)` | WAV を読み込みハンドルを返す。失敗時 -1 |
| `void ReleaseSound(int handle)` | サウンドハンドルを解放する |

### SE 再生

| メソッド | 説明 |
|---|---|
| `void PlaySound(int handle, float volume=1.0f, float pan=0.0f)` | 効果音を再生する。同じ音を複数同時に鳴らせる |
| `void PlaySoundOnBus(int handle, AudioBus& bus, float volume=1.0f)` | 指定バスに出力して再生する |
| `void SetSoundVolume(int handle, float volume)` | 音量を設定する (0.0-1.0) |

### BGM 再生

| メソッド | 説明 |
|---|---|
| `void PlayMusic(int handle, bool loop=true, float volume=1.0f)` | BGM を再生する |
| `void StopMusic()` | BGM を停止する |
| `void PauseMusic()` / `void ResumeMusic()` | BGM の一時停止・再開 |
| `void FadeInMusic(float seconds)` | BGM フェードイン |
| `void FadeOutMusic(float seconds)` | BGM フェードアウト (完了後に自動停止) |
| `bool IsMusicPlaying()` | BGM が再生中か |

### 3D サウンド

| メソッド | 説明 |
|---|---|
| `int PlaySound3D(int handle, AudioEmitter& emitter, float volume=1.0f)` | 3D 空間内で再生する。ボイス ID を返す |
| `void StopSound3D(int voiceId)` | 3D サウンドを停止する |
| `void SetListener(const AudioListener& listener)` | リスナー位置を設定する (毎フレーム更新推奨) |
| `void SetMasterVolume(float volume)` | マスターボリュームを設定する (0.0-1.0) |

### サブシステムアクセス

| メソッド | 説明 |
|---|---|
| `AudioDevice& GetDevice()` | XAudio2 デバイスを取得する |
| `SoundPlayer& GetSoundPlayer()` | SE プレイヤーを取得する |
| `MusicPlayer& GetMusicPlayer()` | BGM プレイヤーを取得する |
| `AudioMixer& GetMixer()` | ミキサーを取得する (BGM/SE/Voice バス) |

## AudioEmitter

3D 空間内の音源を定義する。

| メソッド | 説明 |
|---|---|
| `void SetPosition(const XMFLOAT3& pos)` | 音源位置を設定する |
| `void SetVelocity(const XMFLOAT3& vel)` | 速度を設定する (ドップラー効果用) |
| `void SetDirection(const XMFLOAT3& front)` | 向きを設定する (指向性コーン用) |
| `void SetInnerRadius(float radius)` | 内側半径を設定する (この範囲内はフル音量) |
| `void SetMaxDistance(float distance)` | 最大距離を設定する (この距離以遠は無音) |
| `void SetCone(innerAngle, outerAngle, outerVolume)` | 指向性コーンを設定する (ラジアン) |
| `void DisableCone()` | コーンを無効化する (全方向均等) |

## AudioListener

3D 空間内の聴取者を定義する。通常は Camera3D と連動させる。

| メソッド | 説明 |
|---|---|
| `void SetPosition(const XMFLOAT3& pos)` | リスナー位置を設定する |
| `void SetOrientation(const XMFLOAT3& front, const XMFLOAT3& up)` | 向きを設定する |
| `void SetVelocity(const XMFLOAT3& vel)` | 速度を設定する (ドップラー効果用) |
| `void UpdateFromCamera(const Camera3D& camera, float deltaTime)` | カメラから位置・方向を自動更新する |

### 使用例

```cpp
// 初期化
AudioManager audio;
audio.Initialize();
int seFire = audio.LoadSound(L"Assets/fire.wav");
int bgm    = audio.LoadSound(L"Assets/bgm.wav");

// BGM 再生
audio.PlayMusic(bgm, true, 0.7f);

// SE 再生
audio.PlaySound(seFire);

// 3D サウンド
AudioEmitter emitter;
emitter.SetPosition({10.0f, 0.0f, 5.0f});
emitter.SetMaxDistance(50.0f);
int voiceId = audio.PlaySound3D(seFire, emitter);

AudioListener listener;
listener.UpdateFromCamera(camera, deltaTime);
audio.SetListener(listener);

// 毎フレーム
audio.Update(deltaTime);

// 終了
audio.Shutdown();
```

## Graphics API

名前空間: `GX`。DirectX 12 ベースの 3D レンダリングシステム。

## Renderer3D

PBR / Toon / Phong 等のシェーダーモデル、CSM シャドウ、フォグ、スカイボックスに対応する 3D レンダラー。

### 初期化

| メソッド | 説明 |
|---|---|
| `bool Initialize(device, cmdQueue, width, height)` | 3D レンダラーを初期化する |
| `void OnResize(uint32_t width, uint32_t height)` | 画面サイズ変更を処理する |

### フレーム描画

| メソッド | 説明 |
|---|---|
| `void Begin(cmdList, frameIndex, camera, time)` | メインパスのフレームを開始する |
| `void SetLights(lights, count, ambient)` | ライト配列を設定する (最大16灯) |
| `void SetMaterial(const Material& mat)` | マテリアルを設定する |
| `void DrawMesh(mesh, transform)` | GPUMesh を描画する |
| `void DrawModel(model, transform)` | Model を描画する (マテリアル自動バインド) |
| `void DrawSkinnedModel(model, transform, animator)` | スキニングモデルを描画する |
| `void DrawModelInstanced(model, transforms, count)` | GPU インスタンシング描画 |
| `void DrawTerrain(terrain, transform)` | 地形を描画する |
| `void End()` | フレーム描画を終了する |

### マテリアルオーバーライド

| メソッド | 説明 |
|---|---|
| `void SetMaterialOverride(const Material* mat)` | 全サブメッシュにマテリアルを強制適用する |
| `void ClearMaterialOverride()` | マテリアルオーバーライドを解除する |
| `void SetWireframeMode(bool enabled)` | ワイヤフレーム表示モード |

### シャドウ

| メソッド | 説明 |
|---|---|
| `void UpdateShadow(const Camera3D& camera)` | シャドウマップを更新する |
| `void SetShadowEnabled(bool enabled)` | シャドウの有効/無効 |
| `void BeginShadowPass(cmdList, frameIndex, cascadeIndex)` | CSM シャドウパス開始 |
| `void EndShadowPass(cascadeIndex)` | CSM シャドウパス終了 |

### サブシステム

| メソッド | 説明 |
|---|---|
| `Skybox& GetSkybox()` | スカイボックス |
| `PrimitiveBatch3D& GetPrimitiveBatch3D()` | 3D プリミティブ描画 |
| `DepthBuffer& GetDepthBuffer()` | 深度バッファ |
| `TextureManager& GetTextureManager()` | テクスチャ管理 |
| `MaterialManager& GetMaterialManager()` | マテリアル管理 |
| `IBL& GetIBL()` | イメージベースドライティング |

## Camera3D

3D カメラ。Free / FPS / TPS モード対応。

### 射影設定

| メソッド | 説明 |
|---|---|
| `void SetPerspective(fovY, aspect, nearZ, farZ)` | 透視投影を設定する |
| `void SetOrthographic(width, height, nearZ, farZ)` | 正射影を設定する |

### 位置・方向

| メソッド | 説明 |
|---|---|
| `void SetPosition(x, y, z)` / `SetPosition(pos)` | カメラ位置を設定する |
| `void LookAt(const XMFLOAT3& target)` | ターゲットを注視する (pitch/yaw 自動設定) |
| `void Rotate(deltaPitch, deltaYaw)` | カメラを回転する |
| `void MoveForward(distance)` / `MoveRight(d)` / `MoveUp(d)` | カメラを移動する |
| `void SetMode(CameraMode mode)` | Free / FPS / TPS モード切替 |
| `void SetTPSDistance(float d)` / `SetTPSOffset(offset)` | TPS パラメータ |

### 行列取得

| メソッド | 説明 |
|---|---|
| `XMMATRIX GetViewMatrix()` | ビュー行列 |
| `XMMATRIX GetProjectionMatrix()` | 射影行列 |
| `XMMATRIX GetViewProjectionMatrix()` | ビュー射影行列 |
| `const XMFLOAT3& GetPosition()` | カメラ位置 |
| `XMFLOAT3 GetForward()` / `GetRight()` / `GetUp()` | 方向ベクトル |

## Material

PBR マテリアルデータ。シェーダーモデル: `Standard` / `Unlit` / `Toon` / `Phong` / `Subsurface` / `ClearCoat`。

### 主要フィールド

| フィールド | 型 | 説明 |
|---|---|---|
| `constants.albedoFactor` | `XMFLOAT4` | アルベド色 (RGBA) |
| `constants.metallicFactor` | `float` | 金属度 (0.0 - 1.0) |
| `constants.roughnessFactor` | `float` | 粗さ (0.0 - 1.0) |
| `constants.emissiveStrength` | `float` | 自発光の強度 |
| `albedoMapHandle` | `int` | アルベドテクスチャハンドル (-1=なし) |
| `normalMapHandle` | `int` | ノーマルマップハンドル (-1=なし) |
| `shaderModel` | `ShaderModel` | シェーダーモデル種別 |
| `shaderParams` | `ShaderModelParams` | シェーダーモデル固有パラメータ (256B) |

## Light / LightData

ライトの生成ファクトリとデータ構造。

| 静的メソッド | 説明 |
|---|---|
| `Light::CreateDirectional(direction, color, intensity)` | 平行光源を生成する |
| `Light::CreatePoint(position, range, color, intensity)` | 点光源を生成する |
| `Light::CreateSpot(position, direction, range, spotAngleDeg, color, intensity)` | スポットライトを生成する |

### LightData フィールド

| フィールド | 説明 |
|---|---|
| `position` | 位置 (Point/Spot) |
| `direction` | 方向 (Directional/Spot) |
| `color` / `intensity` | 色と強度 |
| `range` | 到達距離 (Point/Spot) |
| `spotAngle` | スポット角度 (cos 値) |
| `type` | `LightType` (Directional/Point/Spot) |

## PostEffectPipeline

HDR ポストエフェクトパイプライン。エフェクトチェーン:
`HDR -> [RTGI] -> [SSAO] -> [RT/SSR] -> [VolumetricLight] -> [Bloom] -> [DoF] -> [MotionBlur] -> [Outline] -> [TAA] -> [ColorGrading] -> [AutoExposure] -> [Tonemap] -> [FXAA] -> [Vignette] -> LDR`

### 基本操作

| メソッド | 説明 |
|---|---|
| `bool Initialize(device, width, height)` | パイプラインを初期化する |
| `void BeginScene(cmdList, frameIndex, dsvHandle, camera)` | HDR シーン描画を開始する |
| `void EndScene()` | シーン描画を終了する |
| `void Resolve(backBufferRTV, depthBuffer, camera, deltaTime)` | 全エフェクトを実行して出力する |

### エフェクト設定

| メソッド | 説明 |
|---|---|
| `void SetTonemapMode(TonemapMode mode)` | Reinhard / ACES / Uncharted2 |
| `void SetExposure(float v)` | 露出値 |
| `void SetFXAAEnabled(bool)` | FXAA の有効/無効 |
| `void SetVignetteEnabled(bool)` | ビネットの有効/無効 |
| `void SetColorGradingEnabled(bool)` | カラーグレーディングの有効/無効 |
| `bool LoadSettings(path)` / `bool SaveSettings(path)` | JSON 設定の保存/読み込み (F12) |

### サブエフェクトアクセス

| メソッド | 説明 |
|---|---|
| `SSAO& GetSSAO()` | SSAO パラメータ |
| `Bloom& GetBloom()` | ブルーム |
| `DepthOfField& GetDoF()` | 被写界深度 |
| `MotionBlur& GetMotionBlur()` | モーションブラー |
| `SSR& GetSSR()` | スクリーン空間反射 |
| `TAA& GetTAA()` | 時間的アンチエイリアシング |
| `AutoExposure& GetAutoExposure()` | 自動露出 |

### 使用例

```cpp
PostEffectPipeline postFX;
postFX.Initialize(device, 1920, 1080);
postFX.GetBloom().SetEnabled(true);
postFX.GetSSAO().SetEnabled(true);

// レンダリングループ
postFX.BeginScene(cmdList, frameIndex, dsvHandle, camera);
renderer.Begin(cmdList, frameIndex, camera, time);
renderer.SetLights(lights, lightCount, ambient);
renderer.DrawModel(model, transform);
renderer.End();
postFX.EndScene();
depthBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
postFX.Resolve(backBufferRTV, depthBuffer, camera, deltaTime);
depthBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
```

## GXEasy API

GXEasy は GXLib の簡易アプリケーションラッパーと DXLib 互換関数を提供する。

## GXEasy::App クラス

アプリケーションの基底クラス。継承してライフサイクルメソッドをオーバーライドする。

| メソッド | 説明 |
|---|---|
| `virtual AppConfig GetConfig() const` | ウィンドウ設定を返す。既定値を変えたい場合にオーバーライドする |
| `virtual void OnBoot()` | `GX_Init()` の前に呼ばれる |
| `virtual void Start()` | `GX_Init()` の直後に1回だけ呼ばれる（初期化処理） |
| `virtual void Update(float dt)` | 毎フレーム呼ばれる。`dt` は前フレームからの経過秒数 |
| `virtual void Draw()` | `Update()` の後に毎フレーム呼ばれる（描画処理） |
| `virtual void Release()` | `GX_End()` の前に1回だけ呼ばれる（後片付け） |

## AppConfig 構造体

| フィールド | 型 | 既定値 | 説明 |
|---|---|---|---|
| `title` | `std::wstring` | `L"GXLib Easy App"` | ウィンドウタイトル |
| `width` | `int` | `1280` | 画面幅 |
| `height` | `int` | `720` | 画面高さ |
| `windowed` | `bool` | `true` | ウィンドウモード |
| `autoClear` | `bool` | `true` | 自動画面クリア |
| `autoPresent` | `bool` | `true` | 自動ScreenFlip |
| `allowEscapeExit` | `bool` | `true` | ESCキーで終了 |
| `targetFps` | `int` | `0` | FPS上限（0=無制限） |
| `bgR/bgG/bgB` | `int` | `0` | 背景色 RGB (0-255) |

### 使用例

```cpp
#include "GXEasy.h"

class MyApp : public GXEasy::App {
    AppConfig GetConfig() const override {
        AppConfig c;
        c.title = L"My Game";
        c.width = 1920;
        c.height = 1080;
        return c;
    }
    void Start() override { /* 初期化 */ }
    void Update(float dt) override { /* 更新 */ }
    void Draw() override { /* 描画 */ }
};

GX_EASY_APP(MyApp)
```

## DXLib 互換関数

### システム

| 関数 | 説明 |
|---|---|
| `int GX_Init()` | GXLib を初期化する。成功時 0、失敗時 -1 |
| `int GX_End()` | 全リソースを解放して終了する |
| `int ProcessMessage()` | ウィンドウメッセージ処理。ウィンドウが閉じられた場合 -1 |
| `int SetMainWindowText(const TCHAR* title)` | ウィンドウタイトルを設定する |
| `int ChangeWindowMode(int flag)` | TRUE でウィンドウモード |
| `int SetGraphMode(int w, int h, int bits)` | 画面解像度と色深度を設定する（GX_Init 前に呼ぶ） |
| `unsigned int GetColor(int r, int g, int b)` | RGB 値から 0xFFRRGGBB 形式のカラー値を生成する |
| `int SetDrawScreen(int screen)` | 描画先スクリーン設定 (`GX_SCREEN_BACK`) |
| `int ClearDrawScreen()` | 画面クリアしてフレーム開始 |
| `int ScreenFlip()` | バックバッファを表示してフレーム終了 |
| `int SetBackgroundColor(int r, int g, int b)` | 背景色を設定する |

### 2D 描画

| 関数 | 説明 |
|---|---|
| `int LoadGraph(const TCHAR* path)` | 画像を読み込みハンドルを返す |
| `int DeleteGraph(int handle)` | グラフィックハンドルを解放する |
| `int LoadDivGraph(path, allNum, xNum, yNum, xSize, ySize, handleBuf)` | 画像を分割読み込みする |
| `int DrawGraph(int x, int y, int handle, int trans)` | 画像を描画する |
| `int DrawRotaGraph(cx, cy, ext, angle, handle, trans)` | 回転拡縮描画する |
| `int DrawExtendGraph(x1, y1, x2, y2, handle, trans)` | 拡大縮小描画する |
| `int DrawLine(x1, y1, x2, y2, color, thickness)` | 直線を描画する |
| `int DrawBox(x1, y1, x2, y2, color, fill)` | 矩形を描画する |
| `int DrawCircle(cx, cy, r, color, fill)` | 円を描画する |
| `int SetDrawBlendMode(int mode, int param)` | ブレンドモードを設定する |

### テキスト

| 関数 | 説明 |
|---|---|
| `int DrawString(int x, int y, const TCHAR* str, unsigned int color)` | 文字列を描画する |
| `int DrawFormatString(x, y, color, format, ...)` | 書式付き文字列を描画する |
| `int CreateFontToHandle(name, size, thick, type)` | フォントハンドルを作成する |
| `int DrawStringToHandle(x, y, str, color, fontHandle)` | 指定フォントで描画する |

### 入力

| 関数 | 説明 |
|---|---|
| `int CheckHitKey(int keyCode)` | キーが押されているか (1=押下、0=非押下) |
| `int GetHitKeyStateAll(char* buf)` | 全256キーの押下状態を配列に取得する |
| `int GetMouseInput()` | マウスボタン状態をビットフラグで取得する |
| `int GetMousePoint(int* x, int* y)` | マウス座標を取得する |
| `int GetJoypadInputState(int inputType)` | ゲームパッド入力をビットフラグで取得する |

### サウンド

| 関数 | 説明 |
|---|---|
| `int LoadSoundMem(const TCHAR* path)` | サウンドを読み込みハンドルを返す |
| `int PlaySoundMem(int handle, int playType, int resume)` | サウンドを再生する |
| `int StopSoundMem(int handle)` | サウンドを停止する |
| `int ChangeVolumeSoundMem(int volume, int handle)` | 音量を変更する (0-255) |
| `int PlayMusic(const TCHAR* path, int playType)` | BGM を再生する |

### FormatT ヘルパー

```cpp
// UNICODE/ANSI 両対応の std::format ラッパー
TString text = FormatT(TEXT("Score: %d"), score);
```

## Input API

名前空間: `GX`。キーボード、マウス、ゲームパッド、アクションマッピングの入力管理。

## Keyboard

Win32 仮想キーコード (VK_*) ベースのキーボード入力管理。

| メソッド | 説明 |
|---|---|
| `void Initialize()` | 全キー状態を初期化する |
| `void Update()` | フレーム更新。前フレーム状態を保存し、現在状態を反映する |
| `bool ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)` | Win32 メッセージを処理する |
| `bool IsKeyDown(int key)` | キーが押されているか (押下中 = true) |
| `bool IsKeyTriggered(int key)` | このフレームで押されたか (トリガー判定) |
| `bool IsKeyReleased(int key)` | このフレームで離されたか |

### 使用例

```cpp
auto& kb = inputManager.GetKeyboard();
if (kb.IsKeyTriggered(VK_SPACE)) {
    // ジャンプ処理
}
if (kb.IsKeyDown(VK_LEFT)) {
    // 左移動
}
```

**注意**: DXLib 互換の `CheckHitKey()` は押下判定のみ。トリガー判定には `IsKeyTriggered()` を使う。

## Mouse

Win32 メッセージベースのマウス入力管理。

### ボタン定数 (`MouseButton` 名前空間)

| 定数 | 値 | 説明 |
|---|---|---|
| `MouseButton::Left` | 0 | 左ボタン |
| `MouseButton::Right` | 1 | 右ボタン |
| `MouseButton::Middle` | 2 | 中ボタン |

### メソッド

| メソッド | 戻り値 | 説明 |
|---|---|---|
| `GetX()` / `GetY()` | `int` | マウスの X/Y 座標 (クライアント領域基準) |
| `GetDeltaX()` / `GetDeltaY()` | `int` | 前フレームからの移動量 |
| `GetWheel()` | `int` | ホイール回転量 (正=上方向) |
| `IsButtonDown(int button)` | `bool` | ボタンが押されているか |
| `IsButtonTriggered(int button)` | `bool` | このフレームで押されたか |
| `IsButtonReleased(int button)` | `bool` | このフレームで離されたか |

### 使用例

```cpp
auto& mouse = inputManager.GetMouse();
if (mouse.IsButtonTriggered(MouseButton::Left)) {
    int x = mouse.GetX();
    int y = mouse.GetY();
    // クリック処理
}
```

## Gamepad

XInput 対応ゲームパッド管理。最大4台同時対応。デッドゾーン処理済み。

### ボタン定数 (`PadButton` 名前空間)

| 定数 | 説明 |
|---|---|
| `PadButton::A / B / X / Y` | フェイスボタン |
| `PadButton::DPadUp / Down / Left / Right` | 十字キー |
| `PadButton::LeftShoulder / RightShoulder` | LB / RB |
| `PadButton::Start / Back` | Start / Back |
| `PadButton::LeftThumb / RightThumb` | スティック押し込み |

### メソッド

| メソッド | 説明 |
|---|---|
| `bool IsConnected(int pad)` | パッドが接続されているか (pad: 0-3) |
| `bool IsButtonDown(int pad, int button)` | ボタンが押されているか |
| `bool IsButtonTriggered(int pad, int button)` | このフレームで押されたか |
| `bool IsButtonReleased(int pad, int button)` | このフレームで離されたか |
| `float GetLeftStickX/Y(int pad)` | 左スティック (-1.0 - 1.0) |
| `float GetRightStickX/Y(int pad)` | 右スティック (-1.0 - 1.0) |
| `float GetLeftTrigger(int pad)` | 左トリガー (0.0 - 1.0) |
| `float GetRightTrigger(int pad)` | 右トリガー (0.0 - 1.0) |

### 使用例

```cpp
auto& pad = inputManager.GetGamepad();
if (pad.IsConnected(0)) {
    float moveX = pad.GetLeftStickX(0);
    if (pad.IsButtonTriggered(0, PadButton::A)) { /* ジャンプ */ }
}
```

## ActionMapping

論理アクション名と物理入力の対応を管理する入力抽象化レイヤー。

### InputBinding ファクトリ

| 静的メソッド | 説明 |
|---|---|
| `InputBinding::Key(int vk)` | キーボードキーバインド |
| `InputBinding::KeyAxis(int vk, float scale)` | キーを軸として使用 (scale で方向指定) |
| `InputBinding::MouseBtn(int btn)` | マウスボタンバインド |
| `InputBinding::PadBtn(int btn, int pad=0)` | ゲームパッドボタンバインド |
| `InputBinding::PadAxis(GamepadAxisId axis, float s, float dz, int pad)` | ゲームパッド軸バインド |

### ActionMapping メソッド

| メソッド | 説明 |
|---|---|
| `void DefineAction(name, bindings)` | アクションを定義する。複数バインディング可 |
| `void RemoveAction(name)` | アクションを削除する |
| `void Update(keyboard, mouse, gamepad)` | 全アクション状態を更新する (毎フレーム呼ぶ) |
| `bool IsActionPressed(name)` | アクションが押されているか |
| `bool IsActionTriggered(name)` | このフレームで押されたか |
| `bool IsActionReleased(name)` | このフレームで離されたか |
| `float GetActionValue(name)` | アナログ値 (-1.0 - 1.0) |
| `bool LoadFromFile(path)` | JSON からバインド設定を読み込む |
| `bool SaveToFile(path)` | JSON にバインド設定を保存する |

### 使用例

```cpp
ActionMapping actions;
actions.DefineAction("Jump", {
    InputBinding::Key(VK_SPACE),
    InputBinding::PadBtn(PadButton::A)
});
actions.DefineAction("MoveX", {
    InputBinding::KeyAxis(VK_RIGHT, 1.0f),
    InputBinding::KeyAxis(VK_LEFT, -1.0f),
    InputBinding::PadAxis(GamepadAxisId::LeftStickX)
});

// 毎フレーム
actions.Update(keyboard, mouse, gamepad);
if (actions.IsActionTriggered("Jump")) { /* ジャンプ */ }
float moveX = actions.GetActionValue("MoveX");
```

## Math API

名前空間: `GX`。全型は DirectXMath の XMFLOAT 系を継承し、ゼロオーバーヘッドで相互変換できる。

## Vector2

2D 浮動小数点ベクトル (`XMFLOAT2` 継承)。

| コンストラクタ | 説明 |
|---|---|
| `Vector2()` | (0, 0) で初期化 |
| `Vector2(float x, float y)` | 指定成分で初期化 |
| `Vector2(const XMFLOAT2& v)` | XMFLOAT2 から変換 |

| メソッド | 戻り値 | 説明 |
|---|---|---|
| `Length()` | `float` | ベクトルの長さ |
| `LengthSquared()` | `float` | 長さの2乗 (sqrt 不要で高速) |
| `Normalized()` | `Vector2` | 正規化したコピーを返す |
| `Normalize()` | `void` | 自身を正規化する |
| `Dot(v)` | `float` | 内積 |
| `Cross(v)` | `float` | 2D 外積 (スカラー) |
| `Distance(v)` | `float` | 2点間の距離 |

| 静的メソッド | 説明 |
|---|---|
| `Vector2::Zero()` | (0, 0) |
| `Vector2::One()` | (1, 1) |
| `Vector2::Lerp(a, b, t)` | 線形補間 |
| `Vector2::Min(a, b)` / `Max(a, b)` | 各成分ごとの最小/最大 |

演算子: `+ - * / += -= *= == != -` (単項)

## Vector3

3D 浮動小数点ベクトル (`XMFLOAT3` 継承)。

| コンストラクタ | 説明 |
|---|---|
| `Vector3()` | (0, 0, 0) で初期化 |
| `Vector3(float x, float y, float z)` | 指定成分で初期化 |

| メソッド | 戻り値 | 説明 |
|---|---|---|
| `Length()` / `LengthSquared()` | `float` | 長さ / 長さの2乗 |
| `Normalized()` / `Normalize()` | `Vector3`/`void` | 正規化 |
| `Dot(v)` | `float` | 内積 |
| `Cross(v)` | `Vector3` | 外積 |
| `Distance(v)` / `DistanceSquared(v)` | `float` | 2点間距離 |

| 静的メソッド | 説明 |
|---|---|
| `Zero()` / `One()` / `Up()` / `Down()` / `Forward()` / `Right()` | 定数ベクトル |
| `Lerp(a, b, t)` | 線形補間 |
| `Reflect(direction, normal)` | 反射ベクトル |
| `Transform(v, matrix)` | 行列で座標変換 (w=1) |
| `TransformNormal(v, matrix)` | 行列で法線変換 (w=0) |

## Vector4

4D 浮動小数点ベクトル (`XMFLOAT4` 継承)。Vector3 + float w から構築可能。

## Matrix4x4

4x4 行列 (`XMFLOAT4X4` 継承)。既定値は単位行列。

| メソッド | 説明 |
|---|---|
| `ToXMMATRIX()` | `XMMATRIX` に変換する |
| `operator*(m)` | 行列乗算 |
| `Inverse()` | 逆行列 |
| `Transpose()` | 転置行列 |
| `Determinant()` | 行列式 |
| `TransformPoint(v)` | 点を変換 (w=1) |
| `TransformVector(v)` | 方向を変換 (w=0) |

| 静的ファクトリ | 説明 |
|---|---|
| `Identity()` | 単位行列 |
| `Translation(x, y, z)` | 平行移動行列 |
| `Scaling(x, y, z)` / `Scaling(uniform)` | 拡大縮小行列 |
| `RotationX(rad)` / `RotationY(rad)` / `RotationZ(rad)` | 軸回転行列 |
| `RotationRollPitchYaw(pitch, yaw, roll)` | オイラー角回転行列 |
| `LookAtLH(eye, target, up)` | ビュー行列 |
| `PerspectiveFovLH(fovY, aspect, near, far)` | 透視投影行列 |
| `OrthographicLH(w, h, near, far)` | 正射影行列 |

## Quaternion

回転クォータニオン (`XMFLOAT4` 継承)。既定値は単位クォータニオン (0,0,0,1)。

| メソッド | 説明 |
|---|---|
| `operator*(q)` | 回転の合成 |
| `Normalized()` / `Normalize()` | 正規化 |
| `Conjugate()` | 共役 |
| `Inverse()` | 逆クォータニオン |
| `ToEuler()` | オイラー角 (pitch, yaw, roll) に変換 |
| `RotateVector(v)` | ベクトルを回転する |

| 静的メソッド | 説明 |
|---|---|
| `Identity()` | 単位クォータニオン |
| `FromAxisAngle(axis, radians)` | 任意軸回転 |
| `FromEuler(pitch, yaw, roll)` | オイラー角から生成 |
| `FromRotationMatrix(m)` | 回転行列から生成 |
| `Slerp(a, b, t)` | 球面線形補間 |
| `Lerp(a, b, t)` | 正規化線形補間 (NLerp) |

## Color

RGBA 色 (float4、0.0-1.0)。

| コンストラクタ | 説明 |
|---|---|
| `Color()` | 白 (1,1,1,1) |
| `Color(float r, g, b, a=1.0f)` | float 成分指定 |
| `Color(uint32_t rgba)` | 0xRRGGBBAA から生成 |
| `Color(uint8_t r, g, b, a=255)` | 整数成分指定 |

| メソッド/静的メソッド | 説明 |
|---|---|
| `ToRGBA()` / `ToABGR()` | 32bit 整数に変換する |
| `ToXMFLOAT4()` | XMFLOAT4 に変換する |
| `FromHSV(h, s, v, a)` | HSV 色空間から生成する |
| `Lerp(a, b, t)` | 線形補間 |
| `White()` / `Black()` / `Red()` / `Green()` / `Blue()` | プリセット色 |

## Spline

スプライン曲線。`SplineType`: `Linear`, `CatmullRom`, `CubicBezier`。

| メソッド | 説明 |
|---|---|
| `AddPoint(point)` | 制御点を追加する |
| `SetPoint(index, point)` | 制御点を設定する |
| `SetType(type)` | 補間タイプを設定する |
| `SetClosed(bool)` | 閉曲線にするかを設定する |
| `Evaluate(t)` | パラメータ t (0-1) で位置を評価する |
| `EvaluateTangent(t)` | 接線方向を評価する |
| `GetTotalLength(subdivisions)` | 近似全長を取得する |
| `EvaluateByDistance(distance)` | 弧長パラメータで位置を評価する |
| `FindClosestParameter(point)` | 最近接点のパラメータを求める |

## MathUtil

`GX::MathUtil` 名前空間のユーティリティ関数。

| 関数 | 説明 |
|---|---|
| `Clamp(value, min, max)` | 値をクランプする |
| `Lerp(a, b, t)` | 線形補間 |
| `InverseLerp(a, b, value)` | 逆線形補間 |
| `Remap(value, fromMin, fromMax, toMin, toMax)` | 範囲リマップ |
| `SmoothStep(edge0, edge1, x)` | 3次スムーズステップ |
| `DegreesToRadians(deg)` / `RadiansToDegrees(rad)` | 角度変換 |
| `NormalizeAngle(rad)` | 角度を [-PI, PI] に正規化する |
| `ApproximatelyEqual(a, b, eps)` | 浮動小数点近似比較 |

定数: `PI`, `TAU` (2*PI), `EPSILON` (1e-6f)

## Scene API

名前空間: `GX`。エンティティ・コンポーネントベースのシーン管理システム。

## Scene

エンティティのコンテナ。更新・描画・フラスタムカリングを管理する。

### エンティティ管理

| メソッド | 説明 |
|---|---|
| `Entity* CreateEntity(const std::string& name = "Entity")` | エンティティを作成して返す |
| `void DestroyEntity(Entity* entity)` | エンティティを破棄する (次フレームで実削除) |
| `Entity* FindEntity(const std::string& name)` | 名前でエンティティを検索する |
| `Entity* FindEntityByID(uint32_t id)` | ID でエンティティを検索する |
| `const auto& GetEntities()` | 全エンティティ一覧を取得する |
| `const auto& GetRootEntities()` | ルートエンティティ一覧を取得する |
| `uint32_t GetEntityCount()` | エンティティ数を取得する |

### 更新・描画

| メソッド | 説明 |
|---|---|
| `void Update(float deltaTime)` | 全エンティティを更新する (ScriptComponent の onUpdate を呼ぶ) |
| `void Render(Renderer3D& renderer)` | 全エンティティを描画する (カリングなし) |
| `void Render(Renderer3D& renderer, const Camera3D& camera)` | フラスタムカリング付き描画 |
| `RenderStats GetLastRenderStats()` | 描画統計情報を取得する |

### RenderStats

| フィールド | 説明 |
|---|---|
| `totalEntities` | 全エンティティ数 |
| `visibleEntities` | 可視エンティティ数 |
| `culledEntities` | カリングされた数 |
| `drawCalls` | ドローコール数 |
| `instancedBatches` | インスタンシングバッチ数 |
| `instancedEntities` | インスタンシング対象エンティティ数 |

### デバッグ描画

| フラグ | 説明 |
|---|---|
| `SceneDebug_BoundingSpheres` | バウンディング球を表示する |
| `SceneDebug_AABBs` | AABB を表示する |
| `SceneDebug_Frustum` | フラスタムを表示する |
| `SceneDebug_LODLevels` | LOD レベルを表示する |

## Entity

Transform3D を内蔵するゲームオブジェクト。親子階層とコンポーネントシステムをサポートする。

### 基本操作

| メソッド | 説明 |
|---|---|
| `const std::string& GetName()` / `SetName(name)` | 名前の取得/設定 |
| `uint32_t GetID()` | 一意の ID を取得する |
| `bool IsActive()` / `SetActive(bool)` | アクティブ状態 |
| `Transform3D& GetTransform()` | Transform3D を取得する (常に存在) |
| `XMMATRIX GetWorldMatrix()` | 親の変換を考慮したワールド行列 |

### 階層

| メソッド | 説明 |
|---|---|
| `void SetParent(Entity* parent)` | 親エンティティを設定する (nullptr でルートに戻る) |
| `Entity* GetParent()` | 親を取得する |
| `const auto& GetChildren()` | 子エンティティ一覧を取得する |

### コンポーネント

| メソッド | 説明 |
|---|---|
| `T* AddComponent<T>()` | コンポーネントを追加して返す |
| `T* GetComponent<T>()` | コンポーネントを取得する (なければ nullptr) |
| `bool HasComponent<T>()` | コンポーネントの有無を確認する |
| `void RemoveComponent<T>()` | コンポーネントを削除する |

### バウンディング

| メソッド | 説明 |
|---|---|
| `void SetBounds(const AABB3D& aabb)` | ローカル AABB を設定する |
| `Sphere GetWorldBoundingSphere()` | ワールド空間のバウンディング球を取得する |

## ビルトインコンポーネント

### MeshRendererComponent

静的メッシュの描画を担当するコンポーネント。

| フィールド | 型 | 説明 |
|---|---|---|
| `model` | `Model*` | 描画対象のモデル |
| `ownedModel` | `unique_ptr<Model>` | インポートしたモデルの所有権 |
| `materials` | `vector<Material>` | マテリアル配列 |
| `castShadow` | `bool` | 影を落とすか |
| `submeshVisibility` | `vector<bool>` | サブメッシュごとの表示 ON/OFF |
| `sourcePath` | `string` | インポート元ファイルパス |
| `useMaterialOverride` | `bool` | マテリアルオーバーライド有効化 |

### SkinnedMeshRendererComponent

スキニングアニメーション付きメッシュ。

| フィールド | 型 | 説明 |
|---|---|---|
| `model` | `Model*` | モデル |
| `animator` | `unique_ptr<Animator>` | アニメーター |
| `sourcePath` | `string` | インポート元ファイルパス |

### その他のコンポーネント

| コンポーネント | 説明 |
|---|---|
| `CameraComponent` | カメラ。`camera` (Camera3D), `isMain` (bool) |
| `LightComponent` | ライト。`lightData` (LightData) |
| `TerrainComponent` | 地形。`terrain` (Terrain*) |
| `LODComponent` | LOD グループ。`lodGroup` (LODGroup) |
| `AudioSourceComponent` | オーディオ。`soundHandle`, `playOnStart`, `loop` |
| `ParticleSystemComponent` | パーティクルシステム |
| `ScriptComponent` | ユーザーロジック。`onUpdate`, `onStart`, `onDestroy` コールバック |

## SceneSerializer

シーンの JSON 直列化。nlohmann/json 使用。

| 静的メソッド | 説明 |
|---|---|
| `bool SaveToJson(scene, filePath)` | シーンを JSON ファイルに保存する |
| `bool LoadFromJson(scene, filePath, modelLoader)` | JSON からシーンを読み込む |
| `std::string ToJsonString(scene)` | シーンを JSON 文字列に変換する |
| `bool FromJsonString(scene, json, modelLoader)` | JSON 文字列からシーンを復元する |

`ModelLoadCallback = std::function<Model*(const std::string& path)>` -- モデルパスから `Model*` を解決するコールバック。

### 使用例

```cpp
// シーン構築
Scene scene("MyScene");
Entity* player = scene.CreateEntity("Player");
auto* mesh = player->AddComponent<MeshRendererComponent>();
mesh->model = playerModel;

Entity* child = scene.CreateEntity("Weapon");
child->SetParent(player);
child->GetTransform().SetPosition(1.0f, 0.5f, 0.0f);

auto* light = scene.CreateEntity("Sun");
auto* lc = light->AddComponent<LightComponent>();
lc->lightData = Light::CreateDirectional({-0.5f, -1.0f, 0.5f}, {1,1,1}, 2.0f);

auto* npc = scene.CreateEntity("NPC");
auto* script = npc->AddComponent<ScriptComponent>();
script->onUpdate = [](float dt) { /* ロジック */ };

// 更新・描画
scene.Update(deltaTime);
scene.Render(renderer, camera);

// 保存・読み込み
SceneSerializer::SaveToJson(scene, "scene.json");

Scene loaded("Loaded");
SceneSerializer::LoadFromJson(loaded, "scene.json", [&](const std::string& path) {
    return modelCache[path];
});
```


---

# Part VI: 開発フェーズ指令書

## Phase 31-34 Directive

## Context

Phase 0-30 完了済み。エンジンは完全な 2D/3D 描画パイプライン、DXR レイトレーシング、
ポストエフェクト、GUI、物理、オーディオ、シーングラフ、ECS、パーティクル（CPU/GPU）、
ナビメッシュ、LOD、デカール、IBL、IK、GPU インスタンシング、アクションマッピング、
GXModelViewer（ImGui Docking + 19 パネル）、アセットパイプライン（gxformat/gxconv/gxloader/gxpak）
を持つ成熟状態。

本指令書は **安定化・統合・最適化・新機能** の 4 フェーズを網羅し、
任意の Claude インスタンスが独立して各 Phase を実装できることを目的とする。

---

## 全体ロードマップ

| Phase | 名称 | 概要 | 依存 |
|-------|------|------|------|
| **31** | 安定化 & 品質パス | 全サンプルのバグ修正・パターン統一・エッジケース対応 | なし |
| **32** | GXModelViewer ↔ Scene統合 | Viewer の SceneGraph を GX::Scene ベースに移行 | Phase 31 |
| **33** | レンダリング最適化 | フラスタムカリング・自動バッチング・LOD統合 | Phase 31 |
| **34** | 新エンジン機能 | トレイルレンダラー・スプライン・テキストレイアウト・プロファイラ階層化等 | Phase 31 |

Phase 31 は全ての前提。Phase 32/33/34 は Phase 31 完了後に並列着手可能。

---

## Phase 31: 安定化 & 品質パス

### 目的
全 17+ サンプルの実行確認、既知バグ修正、描画パターン統一。
エンジン基盤の信頼性を確保してから機能追加に進む。

### 31a: 既知バグ修正

#### Bug 1: Audio3DShowcase — WAV ファイルリーク
- **ファイル**: `Samples/Audio3DShowcase/main.cpp`
- **問題**: `Start()` で `_tone_a4.wav`, `_tone_cs5.wav`, `_tone_e5.wav` を作業ディレクトリに生成するが、
  終了時に削除しない
- **修正方針**: GXEasy::App に `Release()` オーバーライドがあれば追加。なければ `~App()` デストラクタか
  atexit で `std::filesystem::remove()` を呼ぶ。
  ```cpp
  void Release() override
  {
      // Release はないかもしれない → デストラクタか WM_DESTROY で
      std::filesystem::remove("_tone_a4.wav");
      std::filesystem::remove("_tone_cs5.wav");
      std::filesystem::remove("_tone_e5.wav");
  }
  ```
- **確認事項**: GXEasy::App に Release/Shutdown 仮想関数があるか確認。なければ追加。
  `#include <filesystem>` は pch.h にないので `<cstdio>` の `std::remove()` を使うか、
  `<filesystem>` をサンプル内でインクルード。

#### Bug 2: PostEffectShowcase — DepthBuffer 遷移パターン不統一
- **ファイル**: `Samples/PostEffectShowcase/main.cpp`
- **問題**: Walkthrough3D, SceneShowcase 等は `depthBuffer.TransitionTo(SRV)` → `Resolve()` →
  `depthBuffer.TransitionTo(DEPTH_WRITE)` のパターンだが、PostEffectShowcase は遷移なしで Resolve() を呼ぶ。
- **修正方針**: 全 3D サンプルで以下のパターンに統一:
  ```cpp
  // Renderer3D::End() の後
  ctx.postEffect.EndScene();

  auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
  depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                         depthBuffer, ctx.camera, m_lastDt);
  depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);
  ```
- **対象サンプル**: PostEffectShowcase, IBLShowcase, InstanceShowcase
  （これらが DepthBuffer 遷移を省略していないか全て確認）

#### Bug 3: CSM バリア遷移ガード（修正済み — 確認のみ）
- **ファイル**: `GXLib/Graphics/3D/Renderer3D.cpp` (lines 906-950)
- **状態**: 前セッションで修正済み。CSM 4 カスケード + Spot + Point の全シャドウマップに対して
  SRV バリア遷移ガードを追加済み。
- **確認事項**: `BeginShadowPass`/`EndShadowPass` を呼ばないサンプル（SceneShowcase 等）で
  D3D12 検証レイヤーエラーが出ないことを確認。

#### Bug 4: SSAO 深度バッファ復元削除（修正済み — 確認のみ）
- **ファイル**: `GXLib/Graphics/PostEffect/SSAO.cpp`
- **状態**: Execute() 末尾の `depthBuffer.TransitionTo(DEPTH_WRITE)` を削除済み。
  深度は SRV のまま後続エフェクトに渡す。
- **確認事項**: SSAO 有効時に DoF, MotionBlur, TAA が正常動作すること。

### 31b: パターン統一

#### 統一 1: `(std::max)()` / `(std::min)()` パターン
- **問題**: Windows.h の min/max マクロと STL の std::min/std::max が衝突する
- **対象**: 全サンプル `.cpp` ファイル
- **確認コマンド**:
  ```bash
  grep -rn "std::min\|std::max" Samples/ --include="*.cpp" | grep -v "(std::max)\|(std::min)"
  ```
- **修正**: `std::max(...)` → `(std::max)(...)` に統一
- **注意**: GXLib 本体は pch.h に `NOMINMAX` があるため問題ないが、サンプルは
  `<Windows.h>` を直接インクルードする場合があり衝突する

#### 統一 2: FormatT テンプレート共通化
- **問題**: SceneShowcase, NavmeshShowcase, GPUParticleShowcase 等が同一の `FormatT` テンプレートを
  各ファイルに重複定義している
- **修正方針**: `GXEasy.h` に `FormatT` を定義して全サンプルから重複を削除
  ```cpp
  // GXEasy.h に追加
  template <class... Args>
  TString FormatT(const TChar* fmt, Args... args)
  {
  #ifdef UNICODE
      return std::vformat(fmt, std::make_wformat_args(args...));
  #else
      return std::vformat(fmt, std::make_format_args(args...));
  #endif
  }
  ```
- **注意**: `Args... args` は **by-value** 必須（`Args&&` + `std::forward` は P2905R2/MSVC14.44 で
  `make_format_args` が lvalue 要求するため不可）。MEMORY.md の FormatT 項を参照。

#### 統一 3: LookAtCamera ヘルパー共通化
- **問題**: SceneShowcase, NavmeshShowcase 等で同一の `LookAtCamera` 関数が重複
- **修正方針**: `GXEasy.h` に移動、または `Camera3D` に `LookAt(target)` メソッドを追加

#### 統一 4: WASD カメラ操作の共通化
- **問題**: 複数の 3D サンプルが WASD+QE+マウスルックの同一コードを持つ
- **修正方針**: `GXEasy.h` にヘルパークラス `FPSCameraController` を追加
  ```cpp
  struct FPSCameraController
  {
      float moveSpeed = 5.0f;
      float sprintMultiplier = 3.0f;
      float mouseSensitivity = 0.003f;

      void Update(GX::Camera3D& camera, const GX::Keyboard& kb,
                  const GX::Mouse& mouse, float dt);
  };
  ```

### 31c: エッジケース対応

#### Edge 1: ParticleSystem2D コメント修正
- **ファイル**: `GXLib/Graphics/Rendering/ParticleSystem2D.h` line 63
- **問題**: コメントが `1x1白テクスチャ` のままだが実装は `16x16`
- **修正**: コメントを `16x16白テクスチャ` に修正

#### Edge 2: Entity::GetComponent の一時インスタンス生成
- **ファイル**: `GXLib/Core/Scene/Entity.h` lines 55-67
- **問題**: `GetComponent<T>()` が呼ばれるたびに `T temp;` で一時インスタンスを生成して
  `GetType()` を呼ぶ。パフォーマンス上の問題（特にホットパスで呼ぶ場合）
- **修正方針**: `constexpr` または `static` で ComponentType を取得する方式に変更
  ```cpp
  template<typename T>
  T* GetComponent() const
  {
      constexpr ComponentType type = T::k_Type; // T に static constexpr k_Type を追加
      int idx = static_cast<int>(type);
      if (idx < static_cast<int>(ComponentType::_Count) && m_componentLookup[idx] >= 0)
          return static_cast<T*>(m_components[m_componentLookup[idx]].get());
      return nullptr;
  }
  ```
  各コンポーネントに `static constexpr ComponentType k_Type = ComponentType::XXX;` を追加。
  仮想関数 `GetType()` は残す（ランタイム用途のため）。

#### Edge 3: Scene::DestroyEntity の遅延削除安全性
- **ファイル**: `GXLib/Core/Scene/Scene.cpp`
- **確認事項**: `DestroyEntity()` で `m_pendingDestroy` に追加した後、同フレーム内の
  `Render()` でそのエンティティが描画される可能性。`IsActive()` が false に設定されるタイミングを確認。
  必要なら `DestroyEntity()` 内で即座に `entity->SetActive(false)` を呼ぶ。

#### Edge 4: GXEasy::App ライフサイクル確認
- **ファイル**: `GXLib/Compat/GXEasy.h` (または相当)
- **確認事項**: `Release()` 仮想関数の存在確認。なければ追加:
  ```cpp
  virtual void Release() {} // リソース解放用、メインループ終了後に呼ばれる
  ```

### 31d: ビルド検証

全修正後に以下を実行:
```bash
cmake -B build -S .
cmake --build build --config Debug 2>&1
```

エラーゼロを確認。可能であれば各サンプル `.exe` を起動して D3D12 検証レイヤーエラーがないことを確認。

---

## Phase 32: GXModelViewer ↔ Engine Scene 統合

### 目的
GXModelViewer の独自 `SceneGraph`/`SceneEntity` を GX::Scene/GX::Entity ベースに移行し、
エンジン側のシーン管理と統一する。これにより：
- Viewer で作成したシーンをエンジンのシリアライザで保存/読み込み可能
- エンジン側のコンポーネントシステムを Viewer から直接操作可能
- コードの重複を排除

### 32a: 現状分析

#### GXModelViewer SceneGraph（現在の実装）
```
場所: GXModelViewer/Scene/SceneGraph.h
構造: SceneEntity（直接フィールド方式）
  - name, transform, model*, ownedModel, materialOverride
  - animator, selectedClipIndex
  - submeshVisibility, showBones, showWireframe
  - parentIndex (int), visible, _pendingRemoval
管理: SceneGraph クラス
  - vector<SceneEntity> + freeIndices + pendingRemovals
  - selectedEntity, selectedBone
```

#### GX::Scene（エンジン側）
```
場所: GXLib/Core/Scene/Scene.h, Entity.h, Components.h
構造: Entity（コンポーネントベース）
  - m_id, m_name, m_active, m_transform
  - m_parent (Entity*), m_children (vector<Entity*>)
  - m_components (vector<unique_ptr<Component>>)
  - m_componentLookup[] (O(1) 検索)
管理: Scene クラス
  - vector<unique_ptr<Entity>> + rootEntities + pendingDestroy
  - FindEntity, FindEntityByID, FindComponentsOfType<T>
```

#### 差分マッピング

| SceneEntity フィールド | GX::Entity 対応 |
|----------------------|----------------|
| name | Entity::GetName() |
| transform | Entity::GetTransform() |
| model* | MeshRendererComponent::model |
| ownedModel | MeshRendererComponent に所有権移動 |
| materialOverride | MeshRendererComponent::materials[0] |
| useMaterialOverride | （MeshRendererComponent に bool 追加） |
| animator | SkinnedMeshRendererComponent::animator |
| selectedClipIndex | エディタ固有状態 → EditorMetadata |
| submeshVisibility | MeshRendererComponent に追加 |
| showBones | エディタ固有状態 → EditorMetadata |
| showWireframe | エディタ固有状態 → EditorMetadata |
| parentIndex | Entity::SetParent() (ポインタベース) |
| visible | Entity::SetActive() |
| sourcePath | MeshRendererComponent に追加 |
| _pendingRemoval | Scene::DestroyEntity() 内部 |

### 32b: 移行アーキテクチャ

#### Step 1: MeshRendererComponent 拡張
```cpp
// GXLib/Core/Scene/Components.h — 既存を拡張
struct MeshRendererComponent : Component
{
    ComponentType GetType() const override { return ComponentType::MeshRenderer; }
    Model* model = nullptr;
    std::unique_ptr<Model> ownedModel;       // 追加: モデル所有権
    std::vector<Material> materials;
    bool castShadow = true;
    bool receiveShadow = true;
    std::vector<bool> submeshVisibility;     // 追加: サブメッシュ可視性
    std::string sourcePath;                  // 追加: インポート元パス
    bool useMaterialOverride = false;        // 追加: マテリアルオーバーライド有効化
    Material materialOverride;               // 追加: オーバーライドマテリアル
};
```

#### Step 2: エディタ固有メタデータコンポーネント
```cpp
// GXModelViewer/Scene/EditorMetadata.h — 新規
#pragma once
#include "Core/Scene/Component.h"

/// @brief エディタ専用メタデータ（エンジン側には存在しない）
struct EditorMetadata : GX::Component
{
    GX::ComponentType GetType() const override { return GX::ComponentType::Custom; }

    int  selectedClipIndex = -1;    // タイムラインで選択中のクリップ
    bool showBones = false;         // ボーン可視化
    bool showWireframe = false;     // ワイヤフレーム表示
};
```

#### Step 3: SceneGraph → Scene アダプタ

SceneGraph を一気に置換するのではなく、段階的に移行:

1. **Phase 32b-1**: `SceneGraph` クラス内部で `GX::Scene` を保持するアダプタ版を作成
   ```cpp
   class SceneGraph
   {
   public:
       // 既存 API は維持（パネルへの影響最小化）
       int AddEntity(const std::string& name);
       void RemoveEntity(int index);
       SceneEntity* GetEntity(int index);
       // ...

       // 新規: GX::Scene への直接アクセス
       GX::Scene& GetScene() { return m_scene; }

   private:
       GX::Scene m_scene;
       // SceneEntity ラッパー（一時的、段階的に消す）
       std::vector<SceneEntityProxy> m_proxies;
   };
   ```

2. **Phase 32b-2**: 各パネルを GX::Entity ベースに移行
   - SceneHierarchyPanel: `SceneEntity*` → `GX::Entity*`
   - PropertyPanel: `SceneEntity.transform` → `entity->GetTransform()`
   - TimelinePanel: `SceneEntity.animator` → `entity->GetComponent<SkinnedMeshRendererComponent>()->animator`
   - 等

3. **Phase 32b-3**: SceneEntityProxy を廃止、全面的に GX::Entity 使用

### 32c: パネル移行チェックリスト

| パネル | 依存する SceneEntity フィールド | 移行方法 |
|--------|-------------------------------|---------|
| SceneHierarchyPanel | name, parentIndex, visible | Entity::GetName, GetParent, IsActive |
| PropertyPanel | transform, materialOverride, model, showWireframe, showBones | Transform3D, MeshRendererComponent, EditorMetadata |
| TimelinePanel | animator, selectedClipIndex | SkinnedMeshRendererComponent, EditorMetadata |
| AnimatorPanel | animator | SkinnedMeshRendererComponent |
| BlendTreeEditor | animator | SkinnedMeshRendererComponent |
| ModelInfoPanel | model | MeshRendererComponent |
| SkeletonPanel | model, showBones | MeshRendererComponent, EditorMetadata |
| LightingPanel | （SceneEntityに依存しない） | 変更不要 |
| PostEffectPanel | （SceneEntityに依存しない） | 変更不要 |
| SkyboxPanel | （SceneEntityに依存しない） | 変更不要 |
| TerrainPanel | （SceneEntityに依存しない） | 変更不要 |
| PerformancePanel | （SceneEntityに依存しない） | 変更不要 |
| LogPanel | （SceneEntityに依存しない） | 変更不要 |
| TextureBrowser | （SceneEntityに依存しない） | 変更不要 |
| AssetBrowserPanel | sourcePath | MeshRendererComponent::sourcePath |
| IBLPanel | （SceneEntityに依存しない） | 変更不要 |
| ParticlePanel | （SceneEntityに依存しない） | 変更不要 |
| IKPanel | （SceneEntityに依存しない） | 変更不要 |
| AudioPanel | （SceneEntityに依存しない） | 変更不要 |

**移行が必要なパネル**: SceneHierarchy, Property, Timeline, Animator, BlendTreeEditor,
ModelInfo, Skeleton, AssetBrowser（8 パネル）。残り 11 パネルは変更不要。

### 32d: シリアライゼーション統合

現在 2 つの独立したシリアライザが存在:
- `GXModelViewer/Scene/SceneSerializer.h` — Viewer 独自形式
- `GXLib/Core/Scene/SceneSerializer.h` — エンジン形式

#### 統合方針
- **エンジン側のシリアライザを権威的バージョンとする**
- Viewer 側シリアライザを薄いラッパーに変更:
  ```cpp
  // GXModelViewer/Scene/SceneSerializer.h
  class ViewerSceneSerializer
  {
  public:
      // エンジン SceneSerializer を呼び出し + エディタ固有データを追加
      static bool Save(const GX::Scene& scene, const std::string& path);
      static bool Load(GX::Scene& scene, const std::string& path,
                       GX::SceneSerializer::ModelLoadCallback modelLoader);
  };
  ```
- エディタ固有データ（EditorMetadata）は JSON の "editor" セクションに別途保存

### 32e: 移行手順まとめ

1. MeshRendererComponent に `ownedModel`, `submeshVisibility`, `sourcePath`,
   `materialOverride`, `useMaterialOverride` フィールドを追加
2. EditorMetadata コンポーネントを作成
3. SceneGraph クラスを GX::Scene ラッパーに変更
4. 8 パネルを GX::Entity ベースに移行（1パネルずつ）
5. GXModelViewerApp の描画ループを GX::Entity ベースに変更
6. ViewerSceneSerializer を GX::SceneSerializer ベースに変更
7. 旧 SceneEntity/SceneGraph を完全削除
8. ビルド検証 + 動作確認

### 注意点
- **ImGui Image**: `ImTextureRef(static_cast<ImTextureID>(gpuHandle.ptr))` パターン継続
- **std::vector<bool>**: ImGui::Checkbox に直接渡せない。ローカル bool にコピー
- **Entity ID**: Viewer の int index → Entity::GetID() (uint32_t) への移行
- **selectedEntity**: SceneGraph の int メンバー → 別管理（GXModelViewerApp に移動）

---

## Phase 33: レンダリング最適化

### 目的
Scene::Render() に自動フラスタムカリング・LOD 選択・インスタンシングバッチングを統合し、
CPU 駆動描画のパフォーマンスを最大化する。

### 33a: フラスタムカリング統合

#### 現状
- `Collision3D` に `Frustum`, `TestFrustumVsSphere`, `TestFrustumVsAABB` が実装済み
- `Frustum::FromViewProjection(viewProj)` で VP 行列からフラスタム抽出可能
- しかし **Scene::Render() も Renderer3D もカリングを呼んでいない**

#### 実装方針

##### Step 1: Entity にバウンディング情報を追加
```cpp
// Entity.h に追加
struct BoundsInfo
{
    AABB3D localAABB;      // モデルのローカルAABB
    float  boundingSphereRadius = 0.0f; // バウンディング球半径
    bool   hasBounds = false;
};

class Entity
{
    // ...
    BoundsInfo m_bounds;
public:
    void SetBounds(const AABB3D& aabb);
    const BoundsInfo& GetBounds() const { return m_bounds; }

    /// ワールド空間のバウンディング球を取得（フラスタムテスト用）
    Sphere GetWorldBoundingSphere() const;
};
```

##### Step 2: Model からバウンディング情報を自動計算
```cpp
// MeshData.h に追加（または Model に）
AABB3D ComputeAABB(const Vertex3D_PBR* vertices, uint32_t count);
```

MeshRendererComponent 追加時に自動計算:
```cpp
auto* meshComp = entity->AddComponent<MeshRendererComponent>();
meshComp->model = model;
entity->SetBounds(model->ComputeAABB()); // 自動設定
```

##### Step 3: Scene::Render にカリングを統合
```cpp
void Scene::Render(Renderer3D& renderer, const Camera3D& camera)
{
    // VP行列からフラスタムを抽出
    XMMATRIX vp = camera.GetViewMatrix() * camera.GetProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    for (const auto& entity : m_entities)
    {
        if (!entity->IsActive()) continue;

        // フラスタムカリング
        if (entity->GetBounds().hasBounds)
        {
            Sphere worldSphere = entity->GetWorldBoundingSphere();
            if (!Collision3D::TestFrustumVsSphere(frustum, worldSphere))
                continue; // 画面外 → スキップ
        }

        // MeshRendererComponent の描画
        auto* mesh = entity->GetComponent<MeshRendererComponent>();
        if (mesh && mesh->model)
        {
            // マテリアル設定
            if (mesh->useMaterialOverride)
                renderer.SetMaterialOverride(&mesh->materialOverride);

            renderer.DrawModel(*mesh->model, entity->GetTransform());

            if (mesh->useMaterialOverride)
                renderer.ClearMaterialOverride();
        }

        // SkinnedMeshRendererComponent の描画
        auto* skinned = entity->GetComponent<SkinnedMeshRendererComponent>();
        if (skinned && skinned->model && skinned->animator)
        {
            renderer.DrawSkinnedModel(*skinned->model, entity->GetTransform(),
                                      *skinned->animator);
        }
    }
}
```

##### Step 4: Scene::Render の API 拡張
```cpp
// Scene.h
class Scene
{
public:
    // 既存（カリングなし — 後方互換）
    void Render(Renderer3D& renderer);

    // 新規（カメラ付き — フラスタムカリング有効）
    void Render(Renderer3D& renderer, const Camera3D& camera);

    // 新規（カリング統計取得）
    struct RenderStats
    {
        uint32_t totalEntities;
        uint32_t visibleEntities;
        uint32_t culledEntities;
        uint32_t drawCalls;
    };
    RenderStats GetLastRenderStats() const;
};
```

### 33b: LOD 統合

#### 現状
- `LODGroup` クラスが存在（`Graphics/3D/LODGroup.h`）
- `SelectLOD(camera, transform)` でカメラ距離ベースのモデル選択が可能
- ヒステリシスバンド 5%
- しかし **Scene::Render に統合されていない**

#### 実装方針

##### Step 1: LODComponent を追加
```cpp
// Components.h に追加
struct LODComponent : Component
{
    ComponentType GetType() const override { return ComponentType::Custom; }
    // ComponentType::Custom を使うか、LOD 専用の列挙値を追加
    LODGroup lodGroup;
};
```

##### Step 2: Scene::Render で LOD を使用
```cpp
// Scene::Render 内
auto* mesh = entity->GetComponent<MeshRendererComponent>();
auto* lod = entity->GetComponent<LODComponent>();

const Model* drawModel = nullptr;
if (lod)
{
    drawModel = lod->lodGroup.SelectLOD(camera, entity->GetTransform());
    if (!drawModel) continue; // カリング距離を超えた
}
else if (mesh && mesh->model)
{
    drawModel = mesh->model;
}

if (drawModel)
{
    renderer.DrawModel(*drawModel, entity->GetTransform());
}
```

### 33c: 自動インスタンシングバッチング

#### 現状
- `InstanceBuffer` クラスが存在
- `Renderer3D::DrawModelInstanced(model, transforms[], count)` が使用可能
- しかし **自動バッチングは一切ない**（ユーザーが手動で同じモデルの Transform を集めて呼ぶ必要）

#### 実装方針

Scene::Render 内で同一モデルのエンティティをグループ化し、自動的にインスタンシングを使用する。

```cpp
void Scene::Render(Renderer3D& renderer, const Camera3D& camera)
{
    // --- Phase 1: カリング + 描画リスト構築 ---
    struct DrawEntry
    {
        const Model* model;
        Transform3D transform;
        const Material* materialOverride; // nullptr = デフォルト
        bool isSkinned;
        Animator* animator;
    };

    std::vector<DrawEntry> drawList;
    // ... カリング後の drawList 構築 ...

    // --- Phase 2: モデル別グループ化 ---
    // Model* をキーにしてグループ化
    std::unordered_map<const Model*, std::vector<size_t>> modelGroups;
    for (size_t i = 0; i < drawList.size(); ++i)
    {
        if (!drawList[i].isSkinned) // スキンドモデルはインスタンシング非対応
            modelGroups[drawList[i].model].push_back(i);
    }

    // --- Phase 3: 描画発行 ---
    for (auto& [model, indices] : modelGroups)
    {
        if (indices.size() >= k_InstancingThreshold) // 例: 4以上でインスタンシング
        {
            // Transform 配列を構築
            std::vector<Transform3D> transforms;
            transforms.reserve(indices.size());
            for (size_t idx : indices)
                transforms.push_back(drawList[idx].transform);

            renderer.DrawModelInstanced(*model, transforms.data(),
                                        static_cast<uint32_t>(transforms.size()));
        }
        else
        {
            // 個別描画
            for (size_t idx : indices)
            {
                auto& entry = drawList[idx];
                renderer.SetMaterial(/* ... */);
                renderer.DrawModel(*entry.model, entry.transform);
            }
        }
    }

    // スキンドモデルは個別描画
    for (auto& entry : drawList)
    {
        if (entry.isSkinned)
        {
            renderer.DrawSkinnedModel(*entry.model, entry.transform, *entry.animator);
        }
    }
}
```

**閾値**: `k_InstancingThreshold = 4`（4 以上の同一モデルでインスタンシング発動）

### 33d: パフォーマンス計測

#### ProfilerOverlay との連携
Phase 30 で実装済みの `ProfilerOverlay` に以下の情報を追加:
- カリング統計（total/visible/culled）
- インスタンシングバッチ数
- 個別描画コール数

```cpp
// ProfilerOverlay に追加
void SetSceneRenderStats(const Scene::RenderStats& stats);
```

### 33e: 注意事項

- **pch.h に `<unordered_map>` は未確認** — `std::unordered_map` を使う前に確認。
  なければ `<map>` を使うか、pch.h にないヘッダはサンプル内でインクルード
- **マテリアルオーバーライドとインスタンシングの併用**: 現在の `DrawModelInstanced` は
  マテリアルオーバーライドを考慮しない。同一マテリアルのインスタンスのみグループ化すること
- **スキンドモデル**: Animator のポーズが異なるためインスタンシング不可（ボーン行列が異なる）。
  ただし同一ポーズの場合（パレードアニメ等）は `DrawSkinnedModelInstanced` が使用可能
- **ソート**: 不透明メッシュは front-to-back、半透明は back-to-front でソートすると
  オーバードローを削減できるが、Phase 33 では未実装（将来的な拡張として記録）

---

## Phase 34: 新エンジン機能

### 目的
エンジンの機能的ギャップを埋め、ゲーム開発に必要な汎用機能を追加する。

### 34a: トレイル/リボンレンダラー

#### 概要
剣の軌跡、弾丸の尾、キャラクターの残像等に使う帯状メッシュの動的生成。

#### 新規ファイル
- `GXLib/Graphics/3D/TrailRenderer.h`
- `GXLib/Graphics/3D/TrailRenderer.cpp`
- `Shaders/Trail.hlsl`

#### クラス設計
```cpp
namespace GX
{

struct TrailPoint
{
    XMFLOAT3 position;
    XMFLOAT3 up;           // トレイルの「上」方向（幅の方向）
    float    width;
    Color    color;
    float    time;          // 追加時刻
};

class TrailRenderer
{
public:
    bool Initialize(ID3D12Device* device, uint32_t maxPoints = 256);

    /// @brief 新しいポイントを追加（毎フレーム呼ぶ）
    void AddPoint(const XMFLOAT3& position, const XMFLOAT3& up,
                  float width = 1.0f, const Color& color = {1,1,1,1});

    /// @brief 古いポイントを寿命で削除
    void Update(float deltaTime);

    /// @brief トレイルを描画
    void Draw(ID3D12GraphicsCommandList* cmdList,
              const Camera3D& camera, uint32_t frameIndex);

    /// @brief トレイルをクリア
    void Clear();

    // 設定
    float lifetime = 1.0f;     // ポイントの寿命（秒）
    int   textureHandle = -1;  // テクスチャ（-1=白）
    bool  fadeWithAge = true;   // 古いほど透明に
    BlendMode blendMode = BlendMode::Alpha;

private:
    std::deque<TrailPoint> m_points;
    Buffer m_vertexBuffer;     // 動的 VB (2 vertices per point)
    DynamicBuffer m_cb;
    ComPtr<ID3D12PipelineState> m_pso;
    ComPtr<ID3D12RootSignature> m_rs;
    uint32_t m_maxPoints = 256;
};

} // namespace GX
```

#### シェーダー: Trail.hlsl
```hlsl
// VS: position + width → ビルボード展開（カメラ右方向に展開）
// PS: UV.x = 0..1（幅方向）, UV.y = 0..1（先頭→末尾）→ テクスチャサンプル
// b0: ViewProjection
// t0: テクスチャ
// s0: リニアクランプ
```

#### 実装の注意点
- **std::deque**: pch.h に `<deque>` がない可能性 → `<vector>` + リングバッファで代替
- **HDR 対応**: PSO の RT フォーマットは `R16G16B16A16_FLOAT`
- **深度**: 深度テスト ON、深度書き込み OFF（半透明のため）
- **VB 更新**: DynamicBuffer パターンで毎フレーム頂点データをアップロード

### 34b: スプライン/パスシステム

#### 概要
カメラパス、NPC パトロールルート、ベジェ曲線等の汎用スプラインシステム。

#### 新規ファイル
- `GXLib/Math/Spline.h`
- `GXLib/Math/Spline.cpp`

#### クラス設計
```cpp
namespace GX
{

enum class SplineType { CatmullRom, CubicBezier, Linear };

class Spline
{
public:
    Spline(SplineType type = SplineType::CatmullRom);

    /// @brief 制御点を追加
    void AddPoint(const Vector3& point);
    void SetPoint(int index, const Vector3& point);
    void RemovePoint(int index);
    void Clear();

    /// @brief t (0..1) での位置を取得
    Vector3 Evaluate(float t) const;

    /// @brief t (0..1) での接線を取得
    Vector3 EvaluateTangent(float t) const;

    /// @brief スプラインの総距離を取得（離散近似）
    float GetTotalLength() const;

    /// @brief 距離ベースの位置取得（等速移動用）
    Vector3 EvaluateByDistance(float distance) const;

    /// @brief 制御点数
    int GetPointCount() const;
    const Vector3& GetPoint(int index) const;

    /// @brief デバッグ描画
    void DebugDraw(PrimitiveBatch3D& batch, int segments = 64,
                   uint32_t color = 0xFFFFFF00) const;

private:
    SplineType m_type;
    std::vector<Vector3> m_points;
    mutable float m_cachedLength = -1.0f;
};

/// @brief スプラインに沿ってカメラを自動移動するコントローラー
class SplineCameraController
{
public:
    void SetPath(const Spline& path);
    void SetLookAtPath(const Spline& lookAtPath); // 視点用の別スプライン

    void Play(float duration, bool loop = false);
    void Pause();
    void Stop();

    /// @brief 更新（Camera3D のポジション・ルックアットを自動設定）
    void Update(Camera3D& camera, float deltaTime);

    bool IsPlaying() const;
    float GetProgress() const; // 0..1

private:
    Spline m_path;
    Spline m_lookAtPath;
    float m_duration = 5.0f;
    float m_time = 0.0f;
    bool m_playing = false;
    bool m_loop = false;
};

} // namespace GX
```

#### 実装の注意点
- **Catmull-Rom**: `0.5 * ((2*P1) + (-P0 + P2) * t + (2*P0 - 5*P1 + 4*P2 - P3) * t^2 + (-P0 + 3*P1 - 3*P2 + P3) * t^3)`
- **距離ベース評価**: 事前に等間隔サンプリングして距離テーブルを構築、バイナリサーチで t を逆算
- **PrimitiveBatch3D::DrawLine** でデバッグ描画

### 34c: PrimitiveBatch3D ソリッドシェイプ追加

#### 概要
デバッグ可視化のためのソリッドプリミティブ（球、箱、コーン、カプセル）。

#### 修正ファイル
- `GXLib/Graphics/3D/PrimitiveBatch3D.h`
- `GXLib/Graphics/3D/PrimitiveBatch3D.cpp`

#### 追加 API
```cpp
class PrimitiveBatch3D
{
public:
    // 既存
    void DrawLine(const XMFLOAT3& from, const XMFLOAT3& to, uint32_t color);
    void DrawWireBox(const XMFLOAT3& center, const XMFLOAT3& halfExtents, uint32_t color);
    void DrawWireSphere(const XMFLOAT3& center, float radius, uint32_t color, int segments = 16);
    void DrawGrid(float size, int divisions, uint32_t color);

    // 新規（ソリッド）
    void DrawSolidBox(const XMFLOAT3& center, const XMFLOAT3& halfExtents,
                      uint32_t color, float alpha = 0.5f);
    void DrawSolidSphere(const XMFLOAT3& center, float radius,
                         uint32_t color, float alpha = 0.5f, int segments = 8);
    void DrawCone(const XMFLOAT3& tip, const XMFLOAT3& base, float radius,
                  uint32_t color, float alpha = 0.5f, int segments = 8);
    void DrawCapsule(const XMFLOAT3& p0, const XMFLOAT3& p1, float radius,
                     uint32_t color, float alpha = 0.5f, int segments = 8);
    void DrawFrustum(const XMMATRIX& invViewProj, uint32_t color);

    // ワイヤー追加
    void DrawWireCone(const XMFLOAT3& tip, const XMFLOAT3& base, float radius,
                      uint32_t color, int segments = 8);
    void DrawWireCapsule(const XMFLOAT3& p0, const XMFLOAT3& p1, float radius,
                         uint32_t color, int segments = 8);
};
```

#### 実装の注意点
- **ソリッド描画**: 三角形バッチを使用（ライン専用の既存バッチとは別のフラッシュが必要）
- **PSO**: 深度テスト ON、深度書き込み OFF、アルファブレンド ON
- **バッチサイズ**: ソリッド球 (8 segments) = 128 三角形 = 384 頂点。
  MAX 65536 頂点制限に注意

### 34d: TextRenderer テキストレイアウト強化

#### 概要
テキストの折り返し・アライメント・複数行サポートを追加。

#### 修正ファイル
- `GXLib/Graphics/Rendering/TextRenderer.h`
- `GXLib/Graphics/Rendering/TextRenderer.cpp`

#### 追加 API
```cpp
enum class TextAlign { Left, Center, Right };
enum class TextVAlign { Top, Middle, Bottom };

struct TextLayoutOptions
{
    float maxWidth = 0.0f;          // 0 = 折り返しなし
    float lineSpacing = 1.2f;       // 行間（lineHeight の倍率）
    TextAlign align = TextAlign::Left;
    TextVAlign valign = TextVAlign::Top;
    bool wordWrap = true;           // true=単語単位、false=文字単位
};

class TextRenderer
{
public:
    // 既存
    void DrawString(float x, float y, const wchar_t* text, uint32_t color, int fontHandle);
    int  GetStringWidth(const wchar_t* text, int fontHandle);

    // 新規
    void DrawStringLayout(float x, float y, const wchar_t* text,
                          uint32_t color, int fontHandle,
                          const TextLayoutOptions& options);

    int GetStringHeight(const wchar_t* text, int fontHandle,
                        const TextLayoutOptions& options);

    /// @brief テキストをバウンディングボックス内に描画
    void DrawStringInRect(float x, float y, float w, float h,
                          const wchar_t* text, uint32_t color, int fontHandle,
                          TextAlign align = TextAlign::Left,
                          TextVAlign valign = TextVAlign::Top);
};
```

#### 実装の注意点
- **折り返しアルゴリズム**: 単語区切り（スペース、句読点）でグリーディに行を構築
- **日本語対応**: 日本語には明確な単語区切りがないため、文字単位折り返しも必要
- **行間**: `FontManager::GetLineHeight(fontHandle) * lineSpacing`
- **右揃え/中央揃え**: 各行の描画開始 X を `maxWidth - lineWidth` / 2 でオフセット

### 34e: GPUProfiler 階層スコープ

#### 概要
フラットなスコープリストを階層ツリーに変更し、プロファイリング UI を改善。

#### 修正ファイル
- `GXLib/Graphics/Device/GPUProfiler.h`
- `GXLib/Graphics/Device/GPUProfiler.cpp`

#### 追加 API
```cpp
struct ScopeResult
{
    const char* name;
    float timeMs;
    int depth;              // 追加: ネスト深度 (0 = トップレベル)
    int parentIndex;        // 追加: 親スコープのインデックス (-1 = ルート)
    std::vector<int> children; // 追加: 子スコープのインデックス
};
```

#### 実装の注意点
- **スタックベースのネスト追跡**: `BeginScope()` でスタックに push、`EndScope()` で pop
- **depth** と **parentIndex** を `ScopeResult` に付与
- **ProfilerOverlay** のツリー表示: インデントで階層を表現
  ```
  Frame: 16.67ms
  ├─ ShadowPass: 2.1ms
  │  ├─ Cascade0: 0.5ms
  │  ├─ Cascade1: 0.6ms
  │  ├─ Cascade2: 0.5ms
  │  └─ Cascade3: 0.5ms
  ├─ ScenePass: 8.2ms
  │  ├─ OpaqueModels: 6.1ms
  │  └─ Skybox: 2.1ms
  └─ PostEffects: 5.3ms
     ├─ SSAO: 1.2ms
     ├─ Bloom: 0.8ms
     └─ Tonemapping: 0.3ms
  ```

### 34f: ShaderLibrary インクルード依存追跡

#### 概要
`.hlsli` ファイルの変更で依存する `.hlsl` を自動リコンパイル。

#### 修正ファイル
- `GXLib/Graphics/Pipeline/ShaderLibrary.h`
- `GXLib/Graphics/Pipeline/ShaderLibrary.cpp`

#### 実装方針
```cpp
class ShaderLibrary
{
    // 既存
    std::unordered_map<ShaderKey, ComPtr<IDxcBlob>, ShaderKeyHasher> m_cache;

    // 追加: include 依存グラフ
    std::unordered_map<std::string, std::vector<std::string>> m_includeDependencies;
    // key = normalized path of .hlsli
    // value = list of .hlsl files that include it

    void TrackDependency(const std::string& hlslFile, const std::string& includeFile);

    // InvalidateFile を拡張
    void InvalidateFile(const std::string& filePath)
    {
        // 直接のシェーダーを無効化
        InvalidateDirect(filePath);

        // include 依存の逆引き: この .hlsli を含む全 .hlsl も無効化
        auto it = m_includeDependencies.find(Normalize(filePath));
        if (it != m_includeDependencies.end())
        {
            for (const auto& dependent : it->second)
            {
                InvalidateDirect(dependent);
            }
        }
    }
};
```

#### 依存情報の収集方法
- **方法 A**: DXC コンパイル時に `-Fd` フラグでデバッグ情報を出力し、`#include` を解析
- **方法 B**: シンプルにシェーダーソースを正規表現で `#include "..."` をスキャンし、
  コンパイル成功時に依存マップに登録
  ```cpp
  // 方法 B の実装（推奨 — DXC 依存なし）
  void ShaderLibrary::ScanIncludes(const std::string& hlslPath)
  {
      std::ifstream file(hlslPath);
      std::string line;
      while (std::getline(file, line))
      {
          // #include "Foo.hlsli" パターンをマッチ
          auto pos = line.find("#include");
          if (pos != std::string::npos)
          {
              auto q1 = line.find('"', pos);
              auto q2 = line.find('"', q1 + 1);
              if (q1 != std::string::npos && q2 != std::string::npos)
              {
                  std::string includeName = line.substr(q1 + 1, q2 - q1 - 1);
                  TrackDependency(hlslPath, includeName);
              }
          }
      }
  }
  ```

### 34g: Camera3D LookAt メソッド追加

#### 概要
`Camera3D::LookAt(target)` を追加して、複数サンプルの `LookAtCamera` ヘルパーを不要にする。

#### 修正ファイル
- `GXLib/Graphics/3D/Camera3D.h`
- `GXLib/Graphics/3D/Camera3D.cpp`

```cpp
void Camera3D::LookAt(const XMFLOAT3& target)
{
    auto pos = GetPosition();
    float dx = target.x - pos.x;
    float dy = target.y - pos.y;
    float dz = target.z - pos.z;
    float dist = std::sqrt(dx * dx + dz * dz);
    SetPitch(-std::atan2(dy, dist));
    SetYaw(std::atan2(dx, dz));
}
```

### 34h: Scene::Render 統計 & デバッグオーバーレイ

#### 概要
Phase 33c のカリング統計を ProfilerOverlay に表示し、デバッグビジュアルを追加。

#### ProfilerOverlay に追加
```cpp
// Detailed モードに追加:
// "Entities: 150/200 (50 culled, 12 instanced batches)"
// "DrawCalls: 38 (instanced: 12, individual: 26)"
```

#### デバッグ描画オプション
```cpp
// Scene に追加
void Scene::SetDebugDrawFlags(uint32_t flags);

enum SceneDebugFlags : uint32_t
{
    SceneDebug_None             = 0,
    SceneDebug_BoundingSpheres  = 1 << 0,
    SceneDebug_AABBs            = 1 << 1,
    SceneDebug_Frustum          = 1 << 2,
    SceneDebug_LODLevels        = 1 << 3,  // LODレベルを色で表示
};
```

---

## 共通ルール（全 Phase 共通）

### コーディング規約
- namespace `GX` 内に配置
- `#pragma once` + Doxygen `/// @file` / `/// @brief`
- `#include "pch.h"` を .cpp の先頭に
- DirectXMath は `using namespace DirectX;` を .cpp 内のみ
- XMFLOAT3/4/4X4 はメンバー格納用、XMVECTOR/XMMATRIX は演算用
- ComPtr で COM オブジェクト管理
- `std::unique_ptr` で所有権管理
- `constexpr` で定数定義
- エラーログは `GX::Logger::Error()`、情報ログは `GX::Logger::Info()`
- No CD3DX12 helpers — raw D3D12 structs を使用
- C++ に saturate() はない — `std::max(0.0f, std::min(1.0f, x))` を使用
- pch.h に `<sstream>`, `<unordered_set>` は **ない** — 代替を使用

### ビルド手順（各 Phase 完了時）
```bash
cmake -B build -S .
cmake --build build --config Debug 2>&1
# エラーゼロを確認
```

### 新規ディレクトリの CMake 対応
- `GXLib/Core/*.cpp`, `GXLib/Graphics/*.cpp` 等の既存 GLOB_RECURSE パターンでサブディレクトリは自動収集
- `GXLib/AI/*.cpp` は Phase 29 で追加済み
- 新しいサンプルは `gxlib_add_sample(NAME SampleName)` で追加

### テスト方法
- ビルド通過確認
- 新規サンプル .exe の起動確認（クラッシュしないこと）
- 既存サンプル（17 個）が壊れていないこと:
  - EasyHello, Shooting2D, Platformer2D, Walkthrough3D, GUIMenuDemo
  - PostEffectShowcase, DXRShowcase, Particle2DShowcase, ParticleShowcase
  - IBLShowcase, InstanceShowcase, IKShowcase, Audio3DShowcase
  - ActionMappingShowcase, GPUParticleShowcase, NavmeshShowcase, SceneShowcase
- GXModelViewer が起動すること

### MEMORY.md 更新
各 Phase 完了後に MEMORY.md の Completed Phases セクションと Common Issues を更新すること。

---

## 実装優先順序

```
Phase 31 (安定化) ← 全ての前提
    ↓
┌──────────────────┐
│ 並列着手可能:      │
│ Phase 32 (Viewer統合) │
│ Phase 33 (最適化)     │
│ Phase 34 (新機能)     │
└──────────────────┘
```

Phase 34 内部の推奨順序:
1. 34g (Camera3D::LookAt) — 最も軽量、すぐに効果
2. 34c (PrimitiveBatch3D ソリッド) — デバッグに即使用可能
3. 34d (TextRenderer レイアウト) — GUI 改善に直結
4. 34e (GPUProfiler 階層) — デバッグ品質向上
5. 34f (ShaderLibrary include) — 開発ワークフロー改善
6. 34b (Spline) — ゲームプレイ向け
7. 34a (TrailRenderer) — VFX 向け
8. 34h (Scene デバッグオーバーレイ) — Phase 33 完了後

---

## 検証チェックリスト

各 Phase 完了時に以下を確認:

- [ ] `cmake -B build -S . && cmake --build build --config Debug` エラーゼロ
- [ ] 新規サンプルが起動してクラッシュしない
- [ ] 既存 17 サンプルが壊れていない
- [ ] GXModelViewer が起動する
- [ ] gxconv / gxpak がビルドできる
- [ ] D3D12 デバッグレイヤーでエラーなし
- [ ] MEMORY.md が更新されている

---

## 既知の技術的制約

| 制約 | 影響 | 回避策 |
|------|------|--------|
| pch.h に `<sstream>` なし | std::stringstream 使用不可 | std::format/std::to_string 使用 |
| pch.h に `<unordered_set>` なし | unordered_set 使用不可 | vector + sort + unique 等 |
| pch.h に `<deque>` なし | std::deque 使用不可 | vector + リングバッファ |
| pch.h に `<filesystem>` なし | std::filesystem 使用不可 | Win32 API or `<cstdio>` std::remove |
| Windows.h min/max マクロ | std::min/max と衝突 | `(std::max)(...)` パターン |
| FormatT by-value 引数 | Args&& 使用不可 | P2905R2/MSVC14.44 制約 |
| ImTextureID = ImU64 | ポインタ直接渡し不可 | `static_cast<ImTextureID>(handle.ptr)` |
| std::vector<bool> proxy | ImGui::Checkbox 互換なし | ローカル bool にコピー |
| D3D12 Root SRV | Texture2D.Sample() 不可 | shader-visible ヒープ使用 |
| ID3D12GraphicsCommandList4 | DXR 関数に必要 | pch.h で ID3D12Device5 と共にキャスト |

---

## ファイル一覧（全 Phase）

### Phase 31: 修正対象
```
Samples/Audio3DShowcase/main.cpp          — WAV リーク修正
Samples/PostEffectShowcase/main.cpp       — DepthBuffer 遷移追加
Samples/**/main.cpp                       — std::min/max 統一
GXLib/Compat/GXEasy.h                     — FormatT, FPSCameraController
GXLib/Graphics/Rendering/ParticleSystem2D.h — コメント修正
GXLib/Core/Scene/Entity.h                 — GetComponent constexpr 最適化
```

### Phase 32: 修正/新規
```
GXLib/Core/Scene/Components.h             — MeshRendererComponent 拡張
GXModelViewer/Scene/EditorMetadata.h      — 新規
GXModelViewer/Scene/SceneGraph.h          — GX::Scene ラッパー化
GXModelViewer/Scene/SceneGraph.cpp        — 〃
GXModelViewer/Panels/SceneHierarchyPanel.*
GXModelViewer/Panels/PropertyPanel.*
GXModelViewer/Panels/TimelinePanel.*
GXModelViewer/Panels/AnimatorPanel.*
GXModelViewer/Panels/BlendTreeEditor.*
GXModelViewer/Panels/ModelInfoPanel.*
GXModelViewer/Panels/SkeletonPanel.*
GXModelViewer/Panels/AssetBrowserPanel.*
GXModelViewer/Scene/SceneSerializer.*     — GX::SceneSerializer ベースに
```

### Phase 33: 修正/新規
```
GXLib/Core/Scene/Entity.h                 — BoundsInfo 追加
GXLib/Core/Scene/Entity.cpp               — GetWorldBoundingSphere 実装
GXLib/Core/Scene/Scene.h                  — Render(renderer, camera) オーバーロード
GXLib/Core/Scene/Scene.cpp                — カリング+バッチング実装
GXLib/Core/ProfilerOverlay.*              — RenderStats 表示
```

### Phase 34: 新規/修正
```
GXLib/Graphics/3D/TrailRenderer.h         — 新規
GXLib/Graphics/3D/TrailRenderer.cpp       — 新規
Shaders/Trail.hlsl                        — 新規
GXLib/Math/Spline.h                       — 新規
GXLib/Math/Spline.cpp                     — 新規
GXLib/Graphics/3D/PrimitiveBatch3D.h      — ソリッドシェイプ追加
GXLib/Graphics/3D/PrimitiveBatch3D.cpp    — 〃
GXLib/Graphics/Rendering/TextRenderer.h   — レイアウト機能追加
GXLib/Graphics/Rendering/TextRenderer.cpp — 〃
GXLib/Graphics/Device/GPUProfiler.h       — 階層スコープ
GXLib/Graphics/Device/GPUProfiler.cpp     — 〃
GXLib/Graphics/Pipeline/ShaderLibrary.h   — include 依存追跡
GXLib/Graphics/Pipeline/ShaderLibrary.cpp — 〃
GXLib/Graphics/3D/Camera3D.h              — LookAt 追加
GXLib/Graphics/3D/Camera3D.cpp            — 〃
```

## Phase 36-40 Directive

## Context

Phase 0-35 完了済み。エンジンは完全な 2D/3D 描画パイプライン、DXR レイトレーシング（反射＋GI）、
15+ ポストエフェクト、GUI、物理(2D + Jolt 3D)、オーディオ(XAudio2 + 3D)、シーングラフ/ECS、
パーティクル(CPU 2D/3D + GPU)、ナビメッシュ、LOD、デカール、IBL、IK、GPU インスタンシング、
アクションマッピング、GXModelViewer（ImGui Docking + 19 パネル）、アセットパイプライン
（gxformat/gxconv/gxloader/gxpak）、22 サンプルプロジェクトを持つ成熟状態。

本指令書は **バグ修正・テスト強化・ドキュメント改善・新機能追加** の 5 フェーズを網羅し、
任意の Claude インスタンスが独立して各 Phase を実装できることを目的とする。

**重要**: GXModelViewer はあくまでも 3D モデルを読み込み・編集・GXLib 独自形式に変換するツールであり、
シーンエディタ化は本指令書のスコープ外。

---

## 全体ロードマップ

| Phase | 名称 | 概要 | 依存 |
|-------|------|------|------|
| **36** | バグ修正 & コード品質 | TODO/FIXME 解消、エッジケース修正、コード衛生 | なし |
| **37** | テスト強化 | 既存テスト拡充 + 未カバー領域の新規テスト追加 | なし |
| **38** | ドキュメント改善 | チュートリアル刷新、API リファレンス、用語集、サンプル解説 | なし |
| **39** | 新エンジン機能（前半） | マルチスレッドレンダリング、Async Compute、間接描画 | Phase 36 |
| **40** | 新エンジン機能（後半） | Lua スクリプティング、2D タイルマップ、ルートモーション | Phase 36 |

Phase 36/37/38 は並列着手可能。Phase 39/40 は Phase 36 完了後に着手。

---

## Phase 36: バグ修正 & コード品質

### 目的
`BugReport.md`（プロジェクトルート）に記載された 76 件のバグ + TODO/FIXME/HACK コメントを
体系的に修正し、エンジンの信頼性を底上げする。

> **入力ドキュメント**: `C:\Users\g0190\Desktop\GXLib\BugReport.md`
> 全体スキャン(53件) + RT詳細調査(18件) + 計算式・手法検証(10件) = 重複除外 **76件**

### 36a: TODO/FIXME 精査手順

1. 以下のコマンドで全 TODO/FIXME/HACK を抽出:
   ```bash
   grep -rn "TODO\|FIXME\|HACK\|XXX\|WORKAROUND" GXLib/ --include="*.cpp" --include="*.h"
   grep -rn "TODO\|FIXME\|HACK" Shaders/ --include="*.hlsl" --include="*.hlsli"
   ```

2. 各項目を以下のカテゴリに分類:
   - **Critical**: クラッシュ、データ破壊、セキュリティ問題
   - **High**: 機能不全、パフォーマンス深刻劣化
   - **Medium**: 機能制限、非最適なコード
   - **Low**: コスメティック、コメント修正
   - **Won't Fix**: 意図的な制限、将来対応

3. Critical → High → Medium の順に修正

### 36b: BugReport.md 全バグ一覧と修正方針

以下は `BugReport.md` から抽出した全バグの修正指示。
**修正順序は Priority Order に従うこと。**

---

#### 36b-1: Critical (6件) — 最優先

##### C-01: DropDown — 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:178`
- **問題:** `RenderSelf()` 内のラムダで `wideItems[i]` にアクセスするが、`m_items` と `m_wideItems` のサイズ不一致時に範囲外アクセス
- **修正方針:** ラムダ内で `i < wideItems.size()` のガードを追加。`SetItems()` でも `m_wideItems` を `m_items` と同期的に更新することを保証
  ```cpp
  if (i < wideItems.size()) { /* 既存の描画処理 */ }
  ```

##### C-02: ListView — 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/ListView.cpp:134`
- **問題:** `m_wideItems[i]` にアクセスする際、`m_wideItems` のサイズが `m_items` と一致することを検証していない
- **修正方針:** C-01 と同様、アクセス前にサイズチェック。`SetItems()` で両配列を同時にリサイズ

##### C-03: TextureManager::CreateRegionHandles — 負インデックスで配列アクセス
- **File:** `GXLib/Graphics/Resource/TextureManager.cpp:184-187`
- **問題:** `AllocateHandle()` 失敗時の返り値を検証せず `m_entries[handle]` にアクセス
- **修正方針:**
  ```cpp
  int handle = AllocateHandle();
  if (handle < 0)
  {
      GX::Logger::Error("TextureManager: Failed to allocate region handle");
      return -1;  // or continue to skip this region
  }
  m_entries[handle] = /* ... */;
  ```

##### RT-C01: Sandbox で CreateGeometrySRVs() が呼ばれていない
- **File:** `Sandbox/main.cpp`
- **問題:** BLAS 構築後に `CreateGeometrySRVs()` が呼ばれていない → ジオメトリ VB/IB SRV とアルベドテクスチャ SRV が未初期化のまま DispatchRays 実行 → GPU ハング
- **修正方針:** BLAS 構築 + GPU フラッシュ後に `g_rtReflections->CreateGeometrySRVs();` を追加
- **参考:** `Samples/DXRShowcase/main.cpp:459` が正しい呼び出し例

##### RT-C02: リソースポインタの寿命管理不備（Use-After-Free リスク）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h:184-196`
- **問題:** `m_textureResources` と `m_blasGeometry` が `std::vector<ID3D12Resource*>`（生ポインタ）で保持。テクスチャやメッシュが外部で解放されるとダングリングポインタ
- **修正方針:** `std::vector<ComPtr<ID3D12Resource>>` に変更
  ```cpp
  // Before:
  std::vector<ID3D12Resource*> m_textureResources;
  std::vector<BLASGeometryInfo> m_blasGeometry;  // 内部に ID3D12Resource* あり

  // After:
  std::vector<ComPtr<ID3D12Resource>> m_textureResources;
  // BLASGeometryInfo の ID3D12Resource* も ComPtr に変更
  ```

##### RT-C03: ディスクリプタヒープ スロット衝突（frameIndex >= 2 の場合）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:341`
- **問題:** `heapBase = frameIndex * 4` でフレーム毎のスロット計算。ジオメトリ SRV は固定スロット 8 から開始。frameIndex=2 で衝突。現在 `k_BufferCount=2` なので発動しないが脆弱
- **修正方針:** フレーム毎スロットをジオメトリ/テクスチャスロットの後に配置
  ```cpp
  // ジオメトリ SRV: [0..31], テクスチャ SRV: [32..63], フレーム固有: [64 + frameIndex * 4]
  constexpr uint32_t k_FrameSlotBase = 64;
  uint32_t heapBase = k_FrameSlotBase + frameIndex * 4;
  ```

---

#### 36b-2: High (24件) — 第2優先

##### H-01: TextRenderer — vswprintf_s のバッファサイズ引数欠落
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:157`
- **修正:** `vswprintf_s(buffer, 1024, format, args)` に第2引数を追加

##### H-02: SpriteBatch::Begin — Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:199`
- **修正:**
  ```cpp
  m_mappedVertices = static_cast<SpriteVertex*>(m_vertexBuffer.Map(frameIndex));
  if (!m_mappedVertices) { GX::Logger::Error("SpriteBatch: VB Map failed"); return; }
  ```

##### H-03: PrimitiveBatch::Begin — Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/PrimitiveBatch.cpp:133-134`
- **修正:** `m_mappedTriVertices` と `m_mappedLineVertices` 両方に null チェック追加

##### H-05: AutoExposure — マップドポインタの未検証デリファレンス
- **File:** `GXLib/Graphics/PostEffect/AutoExposure.cpp:231`
- **修正:** `if (!mapped) return;` ガード追加

##### H-06: RTReflections::OnResize — HRESULT 未チェック
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:574-577`
- **修正:** `CreateCommittedResource` の戻り値を `FAILED(hr)` でチェックし、失敗時に LOG_ERROR + return

##### H-07: PostEffectPipeline — null リソースへの UAV バリア
- **File:** `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp:440-446`
- **修正:** `if (m_halfResUAV.Get())` で null チェック後にバリア発行

##### H-08: RTReflections — m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **修正:** 全コードパスで `if (!m_normalRT)` チェックを統一

##### H-09: SSR — normalRT の SRV バインド未検証
- **File:** `GXLib/Graphics/PostEffect/SSR.cpp:138`
- **修正:** `UpdateSRVHeap` で normalRT のリソース有効性を事前チェック

##### H-10: HTTPClient — 非同期操作のリソースリーク
- **File:** `GXLib/IO/Network/HTTPClient.cpp:195-210`
- **修正:** `.detach()` → `std::jthread` に変更するか、デストラクタで `m_running=false` + join。`this` キャプチャの代わりに `shared_from_this()` を使用
  ```cpp
  // デストラクタに追加
  ~HTTPClient()
  {
      m_running = false;
      // 全非同期スレッドの完了を待つ
  }
  ```

##### H-11: WebSocket — ReceiveLoop の Use-After-Free
- **File:** `GXLib/IO/Network/WebSocket.cpp:115-250`
- **修正:** `Close()` で `m_running=false` 設定後、`m_hWebSocket` 操作を mutex で保護
  ```cpp
  std::mutex m_socketMutex;
  // ReceiveLoop 内: std::lock_guard lock(m_socketMutex); でハンドルアクセスを保護
  // Close() 内: lock 取得後にハンドル無効化
  ```

##### H-12: AsyncLoader — 破棄時のレースコンディション
- **File:** `GXLib/IO/AsyncLoader.cpp:12-21`
- **修正:** デストラクタで `m_running.store(false)` 後に `m_cv.notify_all()` + `m_thread.join()`。完了キュー/ステータスマップの操作を mutex で保護

##### H-13: MoviePlayer — null ポインタデリファレンス
- **File:** `GXLib/Movie/MoviePlayer.cpp:104, 288-299`
- **修正:** `if (pOutputType) pOutputType->Release();` で null チェック。`Close()` 後の `m_texManager` 使用箇所に null ガード追加

##### H-14: Compat_2D LoadDivGraph — null ポインタ・バッファオーバーフロー
- **File:** `GXLib/Compat/Compat_2D.cpp:61-81`
- **修正:**
  ```cpp
  if (!handleBuf || allNum <= 0) return -1;
  // allNum の上限チェック（例: 1024）
  if (allNum > 1024) { GX::Logger::Error("LoadDivGraph: allNum too large"); return -1; }
  ```

##### H-15: RTReflections.hlsl — cbuffer コメントとアクセスの不一致
- **File:** `Shaders/RTReflections.hlsl:40-44, 238-239`
- **修正:** コメントを更新して `.z=texIdx, .w=hasTexture` を文書化。C++ 側 (`RTReflections.cpp:201`) で全4成分が設定されていることを確認（既に設定済み → コメント修正のみ）
  ```hlsl
  // g_InstanceRoughnessGeom[i]: .x=roughness, .y=geometryIndex, .z=texIdx, .w=hasTexture
  ```

##### H-16: Texture — 大サイズテクスチャでの整数オーバーフロー
- **File:** `GXLib/Graphics/Resource/Texture.cpp:104-106, 246-248`
- **修正:** `rowPitch * height` 計算を `uint64_t` で行う
  ```cpp
  uint64_t totalSize = static_cast<uint64_t>(rowPitch) * height;
  ```

##### H-17a: BarrierBatch — m_barriers 配列の未初期化
- **File:** `GXLib/Graphics/Device/BarrierBatch.h:22, 62-64`
- **修正:** `m_barriers` をゼロ初期化: `D3D12_RESOURCE_BARRIER m_barriers[16] = {};`

##### H-17b: DropDown::OnEvent — 空アイテム時の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:61`
- **修正:** `if (m_items.empty()) return;` ガードを `onValueChanged` 呼び出し前に追加

##### H-18a: FontManager — pixelData の null チェック欠如
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:313`
- **修正:** `if (!pixelData) { GX::Logger::Error("FontManager: pixelData is null"); return; }`

##### H-18b: TextInput::DeleteSelection — 選択範囲の境界値不正
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:93`
- **修正:** `s` と `e` をクランプ: `s = (std::max)(0, (std::min)(s, (int)m_text.size())); e = (std::max)(s, (std::min)(e, (int)m_text.size()));`

##### H-19: ScrollView — ゼロ除算リスク
- **File:** `GXLib/GUI/Widgets/ScrollView.cpp:25`
- **修正:** `if (viewH <= 0) return;` ガード追加

##### H-20: 複数ウィジェット — m_renderer の null チェック不統一
- **Files:** `Button.h, TextWidget.h, CheckBox.h, DropDown.h, ListView.h, RadioButton.h, TabView.h, TextInput.h`
- **修正:** 全 `RenderSelf()` メソッド冒頭に `if (!m_renderer) return;` を追加（未対応分のみ）

##### RT-H01: R16_UINT インデックスバッファ非対応（シェーダー側ハードコード）
- **File:** `Shaders/RTReflections.hlsl:193-196`
- **問題:** ClosestHit で `ib.Load(primIdx * 12 + N)` とストライド12バイト固定 → R16_UINT メッシュで壊れる
- **修正方針（選択肢 a — 推奨）:** `BuildBLAS` で R16_UINT を検出し R32_UINT に変換するか、R16_UINT を受け付けない
  ```cpp
  // BuildBLAS 内
  if (indexFormat == DXGI_FORMAT_R16_UINT)
  {
      GX::Logger::Warn("RTReflections: R16_UINT index buffer not supported, skipping");
      return -1;
  }
  ```
- **修正方針（選択肢 b）:** インデックスフォーマットを per-geometry cbuffer/StructuredBuffer でシェーダーに渡す

##### RT-H02: AddInstance() にインスタンス数上限チェックなし
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:165-203`
- **修正:**
  ```cpp
  void RTReflections::AddInstance(/* ... */)
  {
      if (m_instanceData.size() >= k_MaxInstances)
      {
          GX::Logger::Warn("RTReflections: Max instances ({}) reached", k_MaxInstances);
          return;
      }
      // 既存の push_back 処理
  }
  ```

##### RT-H03: BLAS ジオメトリ配列のギャップ（不連続インデックス）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:138-140`
- **修正:** `BuildBLAS` の戻り値を連番にする。内部で `m_blasGeometry.push_back()` を使い、戻り値は `m_blasGeometry.size() - 1` とする（外部インデックスとの不一致に注意）

---

#### 36b-3: Medium (32件) — 第3優先

##### M-01: TextRenderer — 改行文字の比較誤り
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:101`
- **修正:** `L'\\n'` → `L'\n'` に修正（エスケープの二重化を解消）

##### M-02: FontManager — 未初期化エントリへのアクセス
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:410`
- **修正:** フリーリストから再利用する際に FontEntry をデフォルト初期化

##### M-03: TextRenderer — テクスチャ座標のオーバーフロー
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:65`
- **修正:** `(std::min)(static_cast<int>(glyph->u0 * k_AtlasSize), k_AtlasSize - 1)` でクランプ

##### M-04: RTReflections — 冗長なリソース状態遷移
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:306-309, 466-469`
- **修正:** `RenderTarget::TransitionTo()` の `m_currentState` 追跡を活用し、同一状態への遷移をスキップ

##### M-05: RTReflections::BuildBLAS — エラーハンドリング不足
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:131-142`
- **修正:** RT-H03 の修正と合わせて、連番インデックスに変更しギャップを排除

##### M-06: RTAccelerationStructure — ストライド検証不足
- **File:** `GXLib/Graphics/RayTracing/RTAccelerationStructure.cpp:42`
- **修正:** `if (stride == 0) { LOG_ERROR(...); return -1; }` ガード追加

##### M-07: RTReflections — テクスチャスロットオーバーフロー
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:180-185`
- **修正:** `if (m_textureResources.size() >= 32) { LOG_WARN(...); return; }` ガード追加

##### M-08: Bloom::OnResize — エラー伝播なし
- **File:** `GXLib/Graphics/PostEffect/Bloom.cpp:296-299`
- **修正:** `CreateMipRenderTargets` を `bool` 返り値に変更し、失敗時に `SetEnabled(false)` 呼び出し

##### M-09: Texture — CreateEvent の戻り値未チェック
- **File:** `GXLib/Graphics/Resource/Texture.cpp:213, 356`
- **修正:** `if (!hEvent) { LOG_ERROR(...); return; }` ガード追加

##### M-10: FileWatcher — イベントハンドルリーク
- **File:** `GXLib/IO/FileWatcher.cpp:42, 57`
- **修正:** `CreateEventA()` 戻り値を検証。デストラクタで `CloseHandle()` を呼ぶ

##### M-11: Crypto — 不統一なエラーハンドリング
- **File:** `GXLib/IO/Crypto.cpp:17-30, 83-93`
- **修正:** `Encrypt/Decrypt` 両方でエラーパスに `hAlg` クリーンアップを追加。エラーログの一貫性を確保

##### M-12: Archive — 整数オーバーフロー
- **File:** `GXLib/IO/Archive.cpp:62`
- **修正:** `if (tocSize > 100 * 1024 * 1024) { LOG_ERROR("Archive: TOC too large"); return false; }` 上限チェック

##### M-13: Sound — ファイル読み込みエラーハンドリング不足
- **File:** `GXLib/Audio/Sound.cpp:44, 60-61, 68-72`
- **修正:** 各 `file.read()` 後に `if (file.fail()) return false;` チェック追加

##### M-14: SoundPlayer — コールバックの寿命管理
- **File:** `GXLib/Audio/SoundPlayer.cpp:38-47`
- **修正:** `VoiceCallback` を `std::shared_ptr` で管理。`m_activeVoices` のエントリ削除時にコールバックが生存していることを保証

##### M-15: PhysicsWorld3D — シェイプ作成の null チェック不足
- **File:** `GXLib/Physics/PhysicsWorld3D.cpp:286, 295, 304`
- **修正:** Jolt Shape 作成結果の有効性チェック追加

##### M-16: TextInput — ループ条件の off-by-one
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:203`
- **修正:** `for (int i = 1; i <= static_cast<int>(display.size()); ++i)` で `display.empty()` 時のガード追加

##### M-17: TabView — activeTab の範囲検証なし
- **File:** `GXLib/GUI/Widgets/TabView.cpp:40`
- **修正:** `m_activeTab = (std::max)(0, (std::min)(m_activeTab, (int)children.size() - 1));` でクランプ

##### M-18: DropDown::SetItems — selectedIndex の検証不正
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:26-27`
- **修正:** `if (items.empty()) { m_selectedIndex = -1; } else if (m_selectedIndex >= (int)items.size()) { m_selectedIndex = 0; }`

##### M-19: PostEffectShowcase — VSync と TargetFps の矛盾
- **File:** `Samples/PostEffectShowcase/main.cpp:44-45`
- **修正:** `config.targetFps = 240` を削除（VSync 有効時は不要）

##### M-20: TextWidget — GetIntrinsicHeight のフォールバック不整合
- **File:** `GXLib/GUI/Widgets/TextWidget.cpp:15`
- **修正:** `m_fontHandle < 0` の場合、`GetIntrinsicWidth()` と `GetIntrinsicHeight()` の両方で `0.0f` を返すように統一

##### M-21: SpriteBatch — AddQuad の境界チェック不正
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:246`
- **修正:** `m_vertexWriteOffset + (m_spriteCount + 1) * 4 > k_MaxSprites * 4` に修正（+1 で次の追加分を考慮）

##### RT-M01: 深度バッファ状態遷移の非対称性
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:307, 467, 547`
- **修正:** Execute 完了時に深度バッファの状態を呼び出し元が期待する状態に戻す。`TransitionTo()` の `m_currentState` 追跡を活用

##### RT-M02: OnResize() でディスクリプタヒープ未更新
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:550-580`
- **修正:** `OnResize()` 内でフラグを立て、次回 `Execute()` でディスクリプタを再構築

##### RT-M03: 深度 SRV フォーマットのハードコード
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:354`
- **修正:** `depth.GetResource()->GetDesc().Format` からフォーマットを取得し、適切な SRV フォーマットに変換
  ```cpp
  // D24_UNORM_S8_UINT → R24_UNORM_X8_TYPELESS
  // D32_FLOAT → R32_FLOAT
  ```

##### RT-M04: テクスチャスロットの SRV がフレーム跨ぎで残留
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:156-163`
- **修正:** `BeginFrame()` でテクスチャスロット範囲に null SRV を書き込んでクリア

##### RT-M05: m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **修正:** H-08 と統合。全パスで null チェックを一貫させる

##### RT-M06: ClosestHit の法線方向保証なし
- **File:** `Shaders/RTReflections.hlsl:221-228`
- **修正:** シェーディング法線 `N` にも表裏チェックを追加:
  ```hlsl
  if (dot(N, WorldRayDirection()) > 0)
      N = -N;
  ```

##### RT-M07: Composite パスの alpha コメント誤記
- **File:** `Shaders/RTReflectionComposite.hlsl:89`
- **修正:** コメントを正確に: `// alpha = hit type weight (Miss=0.5, ClosestHit=1.0)`

##### MATH-01: Quaternion::ToEuler() の符号誤り
- **File:** `GXLib/Math/Quaternion.h:88-101`
- **修正:** DirectXMath の `XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)` 規約（Z×Y×X 外的 XYZ）に対応する正しい抽出式:
  ```cpp
  float sinP = 2.0f * (w * x + y * z);   // 修正: - → +
  // yaw: atan2(2(wy - zx), 1 - 2(xx + yy))  // 修正: + → -
  // roll: atan2(2(wz - xy), 1 - 2(xx + zz))  // 修正: + → -
  ```
  **注意:** `FromEuler() → ToEuler()` のラウンドトリップテスト（Phase 37 test_Quaternion.cpp）で検証すること

##### MATH-02: PhysicsWorld2D — 角トルクに質量の逆数を使用
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:65`
- **修正:** 慣性モーメント `I` を RigidBody2D に追加し、角加速度 = torque / I で計算
  ```cpp
  // RigidBody2D に追加:
  float m_inertia = 1.0f;  // 慣性モーメント
  float InverseInertia() const { return (m_inertia > 0) ? 1.0f / m_inertia : 0.0f; }

  // Shape に応じた慣性モーメント計算:
  // 円形: I = 0.5 * m * r^2
  // 矩形: I = (1/12) * m * (w^2 + h^2)

  // PhysicsWorld2D.cpp:65 修正:
  body->angularVelocity += body->m_torqueAccum * (body->InverseInertia() * dt);
  ```

##### MATH-03: PhysicsWorld2D — AABB ブロードフェーズが回転を無視
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:91-96`
- **修正:** 矩形ボディの AABB 計算で4隅を回転してから min/max を取る:
  ```cpp
  AABB2D ComputeAABB(const RigidBody2D& body)
  {
      if (body.shape.type == ShapeType::Circle)
          return { body.position - Vector2(body.shape.radius), body.position + Vector2(body.shape.radius) };

      // 矩形: 4隅を回転
      float c = std::cos(body.rotation), s = std::sin(body.rotation);
      Vector2 h = body.shape.halfExtents;
      Vector2 corners[4] = { {-h.x, -h.y}, {h.x, -h.y}, {-h.x, h.y}, {h.x, h.y} };
      Vector2 mn = body.position, mx = body.position;
      for (auto& corner : corners)
      {
          Vector2 rotated = { c * corner.x - s * corner.y, s * corner.x + c * corner.y };
          rotated = rotated + body.position;
          mn = { (std::min)(mn.x, rotated.x), (std::min)(mn.y, rotated.y) };
          mx = { (std::max)(mx.x, rotated.x), (std::max)(mx.y, rotated.y) };
      }
      return { mn, mx };
  }
  ```

##### MATH-04: RTReflections ClosestHit — 法線変換に ObjectToWorld3x4 を直接使用
- **File:** `Shaders/RTReflections.hlsl:222`
- **修正:** 非一様スケーリング対応:
  ```hlsl
  // Before:
  float3 N = normalize(mul((float3x3)ObjectToWorld3x4(), normalObj));
  // After:
  float3 N = normalize(mul(normalObj, (float3x3)WorldToObject3x4()));
  ```

---

#### 36b-4: Low (14件) — 余裕があれば修正

##### L-01: SSAO — カーネルサイズ0で除算ゼロ
- **File:** `GXLib/Graphics/PostEffect/SSAO.cpp`
- **修正:** `static_assert(k_KernelSize > 0)` を追加

##### L-02: VolumetricLight — 未初期化 XMFLOAT3
- **File:** `GXLib/Graphics/PostEffect/VolumetricLight.cpp:126-127`
- **修正:** `XMFLOAT3 sunNDC = {0, 0, 0};` で初期化

##### L-03: MeshCollider — 除算ゼロ
- **File:** `GXLib/Physics/MeshCollider.cpp:50, 79`
- **修正:** `weld` の負値チェック追加。`step` が 0 の場合 `step = 1` にフォールバック

##### L-04: PhysicsWorld2D — Raycast 出力ポインタ未検証
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:260-262`
- **修正:** `if (outBody) *outBody = ...; if (outPoint) *outPoint = ...; if (outNormal) *outNormal = ...;`

##### L-05: Random — 無限ループリスク
- **File:** `GXLib/Math/Random.cpp:54-98`
- **修正:** rejection sampling ループに `constexpr int k_MaxAttempts = 1000;` の上限追加

##### L-06: Collision3D::ClosestPointOnLine — 除算ゼロ
- **File:** `GXLib/Math/Collision/Collision3D.cpp:426`
- **修正:** `float denom = ab.Dot(ab); if (denom < 1e-12f) return start;` ガード追加

##### L-07: Image Widget — UV オフセットの浮動小数点精度
- **File:** `GXLib/GUI/Widgets/Image.cpp:16-19`
- **修正:** 周期的にリセット: `if (offset > 1000.0f) offset -= 1000.0f;` （実質的に不可視なため低優先）

##### L-08: StyleSheet::ParseLength — 例外ハンドリング欠如
- **File:** `GXLib/GUI/StyleSheet.cpp:557`
- **修正:** `try { return std::stof(str); } catch (...) { LOG_WARN(...); return 0.0f; }`

##### MATH-05: DepthOfField — ガウスカーネル重みの正規化不正
- **File:** `Shaders/DepthOfField.hlsl:74-82`
- **修正:** 重み合計を 1.0 に正規化（0.41% の輝度増加 — ほぼ不可視だが正しくするのが好ましい）

##### TECH-01: RTReflections — ポイントライトに太陽シャドウを再利用
- **File:** `Shaders/RTReflections.hlsl:336`
- **修正:** ポイントライト行の `shadow` 変数を `1.0` に置換（追加シャドウレイは性能コストが高いため省略）:
  ```hlsl
  color += brdfP * pointLightColor * pointLightIntensity * NdotLp * atten;  // shadow 削除
  ```

##### TECH-02: PhysicsWorld2D — 衝突解決に角インパルスなし
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:161-217`
- **修正:** MATH-02 の慣性モーメント追加後に角インパルスを実装:
  ```cpp
  // 接触点と重心の差分ベクトル
  Vector2 rA = contactPoint - bodyA->position;
  Vector2 rB = contactPoint - bodyB->position;
  // 角インパルス: ω += (r × J_normal) / I
  bodyA->angularVelocity -= Cross2D(rA, impulse * normal) * bodyA->InverseInertia();
  bodyB->angularVelocity += Cross2D(rB, impulse * normal) * bodyB->InverseInertia();
  ```

##### TECH-03: PhysicsWorld2D — 摩擦計算にインパルス適用前の相対速度を使用
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:206`
- **修正:** 法線インパルス適用後に `relVel` を再計算してから摩擦インパルスを算出

##### RT-L02: 半解像度変数名が実態と不一致
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h/cpp`
- **修正:** `m_halfResUAV` → `m_reflectionUAV`、`m_halfWidth` → `m_rtWidth`、`m_halfHeight` → `m_rtHeight` にリネーム

---

#### 36b-5: 品質改善 (Won't Fix / Future — 参考情報)

以下は BugReport.md に記載されているが、Phase 36 では **修正不要**（将来の品質改善として記録）:

- **QUAL-01:** TAA の HDR ブレンド前トーンマップ未実装 → Phase 39 以降で検討
- **QUAL-02:** CSM の対数分割未使用 → 品質改善として Phase 39 以降で検討
- **RT-L01:** cbuffer コメントの不完全さ → H-15 で対応済み
- **RT-L03:** SetCommandList4() の毎フレーム呼び出し → 微小な冗長、修正不要
- **RT-L04:** HDR クランプ値の固定 → トーンマッパー変更時に合わせて調整
- **RT-L05:** ポイントライトのインスタンスマスク未対応 → プロダクション機能として Phase 39+

---

### 36b-6: 追加の既知問題（BugReport 外）

以下は MEMORY.md の Common Issues と開発過程で判明している追加問題：

#### Extra-1: Entity::GetComponent 一時インスタンス問題
- **ファイル**: `GXLib/Core/Scene/Entity.h`
- **問題**: `GetComponent<T>()` が毎回 `T temp;` を生成して `GetType()` を呼ぶ
- **修正方針**: 各コンポーネントに `static constexpr ComponentType k_Type` を追加し、テンプレート内で `T::k_Type` を使用
  ```cpp
  template<typename T>
  T* GetComponent() const
  {
      constexpr ComponentType type = T::k_Type;
      int idx = static_cast<int>(type);
      if (idx >= 0 && idx < static_cast<int>(ComponentType::_Count) && m_componentLookup[idx] >= 0)
          return static_cast<T*>(m_components[m_componentLookup[idx]].get());
      return nullptr;
  }
  ```

#### Extra-2: Scene::DestroyEntity 遅延削除の安全性
- **ファイル**: `GXLib/Core/Scene/Scene.cpp`
- **問題**: `DestroyEntity()` 後、同フレーム内の `Render()` で描画される可能性
- **修正方針**: `DestroyEntity()` 内で即座に `entity->SetActive(false)` を呼ぶ

#### Extra-3: DynamicBuffer フレーム境界
- **ファイル**: `GXLib/Graphics/Resource/DynamicBuffer.h`
- **問題**: `Map()` したまま `Unmap()` を呼ばずにフレームを跨ぐと未定義動作
- **修正方針**: デストラクタで Map 状態をチェックし、未 Unmap なら警告ログ出力

#### Extra-4: PostEffectPipeline Null チェック
- **ファイル**: `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp`
- **問題**: `SetRTReflections(nullptr)` / `SetRTGI(nullptr)` 呼び出し後に各エフェクトの Apply 内で nullptr デリファレンスの可能性
- **修正方針**: Apply 関数の冒頭で null チェック（既に対応済みの箇所もあるが全数確認）

---

### 36c: 修正優先順位サマリー

```
1. Critical (6件)   — C-01, C-02, C-03, RT-C01, RT-C02, RT-C03
2. High (24件)      — H-01〜H-20, RT-H01〜RT-H03
3. Medium (32件)    — M-01〜M-21, RT-M01〜RT-M07, MATH-01〜MATH-04
4. Low (14件)       — L-01〜L-08, MATH-05, TECH-01〜TECH-03, RT-L02
5. Extra (4件)      — Extra-1〜Extra-4
```

**注意:**
- **H-04（Skeleton 行列乗算順序）は誤検出として除外済み** — 計算式検証で正しいことを確認
- 同一ファイルに複数のバグがある場合はまとめて修正すること
- 修正ごとにビルド検証（36e）を実施

### 36d: コード衛生

#### 衛生 1: 未使用 include の削除
- 各 .cpp ファイルで未使用の `#include` を確認し削除
- ただし pch.h 経由のインクルードは触らない

#### 衛生 2: const 正確性
- getter メソッドが `const` 修飾されていない箇所を修正
- 特に `GetXxx() const` パターンの統一

#### 衛生 3: 範囲ベース for の活用
- `for (int i = 0; i < vec.size(); ++i)` パターンを
  `for (const auto& item : vec)` に置換（インデックスが不要な場合）

### 36e: ビルド検証
```bash
cmake -B build -S .
cmake --build build --config Debug
ctest --test-dir build --build-config Debug
```

---

## Phase 37: テスト強化

### 目的
既存テストのカバレッジを拡大し、未テストのシステムに新規テストを追加する。
GPU を必要としない純粋なロジックテストに集中する。

### 37a: 現状分析

#### テストフレームワーク
- **Google Test** (FetchContent gtest)
- CMake: `Tests/CMakeLists.txt`
- 実行: `ctest --test-dir build --build-config Debug`

#### 既存テストファイル（11 ファイル + test_main.cpp）

| ファイル | テスト対象 | テスト数 |
|---------|----------|---------|
| `test_main.cpp` | エントリーポイント（gtest_main 使用） | 0 |
| `test_Vector.cpp` | Vector2, Vector3, Vector4 | ~25 |
| `test_Matrix.cpp` | Matrix4x4 | ~10 |
| `test_Quaternion.cpp` | Quaternion | ~7 |
| `test_Color.cpp` | Color (RGBA, HSV, Lerp, Named) | ~8 |
| `test_MathUtil.cpp` | MathUtil (Lerp, Clamp, SmoothStep, Remap, InverseLerp, Degrees/Radians, NormalizeAngle, Sign, IsPowerOfTwo, NextPowerOfTwo, ApproximatelyEqual) + Random (IntRange, FloatRange, Seed, PointInCircle/Sphere, Direction2D/3D) | ~17 |
| `test_Collision2D.cpp` | AABB2D, Circle, Capsule2D, Polygon2D, Line, Raycast2D | ~15 |
| `test_Collision3D.cpp` | AABB3D, Sphere, OBB SAT, Frustum, Raycast, ClosestPoint | ~15 |
| `test_Spatial.cpp` | Quadtree, Octree, BVH (Build, Query, Raycast) | ~15 |
| `test_Crypto.cpp` | AES-256-CBC (encrypt/decrypt round-trip, wrong key), SHA-256 (known hash, deterministic), GenerateRandomBytes | ~6 |
| `test_Allocator.cpp` | PoolAllocator (alloc/free, new/delete, block growth, reuse), FrameAllocator (basic, typed, reset, alignment, capacity, sequential) | ~10 |

**合計: 約 128 テスト**

> **Google Test v1.15.2** — FetchContent 自動取得、gtest_discover_tests() で CTest 連携

#### 未テスト領域（テスト候補）

**GPU 不要（純粋ロジック）:**
- `Math/Transform2D.h` — 位置・回転・スケール変換
- `Math/Spline.h` — Evaluate, EvaluateByDistance, GetTotalLength, FindClosestParameter
- `Core/Scene/Entity.h` — コンポーネント追加/取得/削除、親子階層
- `Core/Scene/Scene.h` — エンティティ作成/破棄/検索
- `Core/Scene/SceneSerializer.h` — JSON 保存/読み込みラウンドトリップ
- `Input/ActionMapping.h` — アクション定義、バインディング評価
- `IO/FileSystem.h` — VFS マウント、ファイル解決
- `IO/Archive.h` — アーカイブ作成/読み込み
- `GUI/StyleSheet.h` — CSS パース、スタイル適用
- `AI/NavMesh.h` — パス検索（A* アルゴリズム）
- `AI/NavAgent.h` — 経路追従ロジック
- `Physics/PhysicsShape.h` — 形状作成
- `Graphics/3D/LODGroup.h` — LOD 選択ロジック（GPU 不要部分）
- `Graphics/3D/BlendStack.h` — アニメーションブレンド計算
- `Graphics/3D/BlendTree.h` — ブレンドツリー評価
- `Graphics/3D/AnimatorStateMachine.h` — ステートマシン遷移

**GPU 必要（統合テスト、Phase 37 ではスキップ）:**
- Renderer3D, SpriteBatch, PostEffect, RayTracing 等

### 37b: 新規テストファイル一覧

各ファイルの作成手順を以下に記載。全テストは `Tests/` ディレクトリに配置。

> **注意:** `test_MathUtil.cpp`（MathUtil + Random）と `test_Crypto.cpp` と `test_Allocator.cpp` は
> **既に存在する**。以下は未カバー領域の新規テストのみ。

---

#### テスト 1: `test_Spline.cpp`

```cpp
/// @file test_Spline.cpp
/// @brief Spline 単体テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Spline.h"
#include "Math/Vector3.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `Linear_Evaluate_Start` | t=0 で最初の制御点を返す |
| `Linear_Evaluate_End` | t=1 で最後の制御点を返す |
| `Linear_Evaluate_Mid` | t=0.5 で中間点を返す（2点の場合） |
| `CatmullRom_Endpoints` | t=0 と t=1 で端点を通過 |
| `CatmullRom_Smooth` | 中間点が制御点間を滑らかに補間（2次微分連続） |
| `CubicBezier_Endpoints` | t=0 と t=1 で端点を通過 |
| `GetTotalLength_TwoPoints` | 2点間の直線距離 ≈ GetTotalLength() |
| `GetTotalLength_Positive` | 任意の点列で長さ > 0 |
| `EvaluateByDistance_Zero` | distance=0 で始点を返す |
| `EvaluateByDistance_Full` | distance=totalLength で終点を返す |
| `EvaluateByDistance_Monotonic` | distance が増加すると t も増加 |
| `FindClosestParameter` | 制御点上の点で t ≈ 期待値 |
| `Closed_Evaluate` | SetClosed(true) で t=0 ≈ t=1 |
| `Empty_GetTotalLength` | 点なしで 0 を返す |
| `SinglePoint` | 1点のみで Evaluate が同じ点を返す |
| `SetType_Changes` | SetType 後に GetType が正しい値を返す |

---

#### テスト 2: `test_Entity.cpp`

```cpp
/// @file test_Entity.cpp
/// @brief Entity/Scene 単体テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Entity.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `Scene_CreateEntity` | CreateEntity で Entity* が non-null |
| `Scene_CreateEntity_Name` | 名前が正しく設定される |
| `Scene_FindEntityByName` | FindEntity(name) で正しい Entity を返す |
| `Scene_FindEntityByID` | FindEntityByID(id) で正しい Entity を返す |
| `Scene_DestroyEntity` | DestroyEntity 後に FindEntity が nullptr |
| `Entity_AddComponent` | AddComponent<T> が non-null を返す |
| `Entity_GetComponent` | GetComponent<T> で追加したコンポーネントを取得 |
| `Entity_GetComponent_NotFound` | 未追加のコンポーネントで nullptr |
| `Entity_Transform` | GetTransform() でデフォルト Transform3D |
| `Entity_SetParent` | SetParent 後に GetParent が正しい |
| `Entity_Children` | 子エンティティが GetChildren に含まれる |
| `Entity_RemoveParent` | SetParent(nullptr) で親子関係を解除 |
| `Entity_ActiveState` | SetActive(false) 後に IsActive() == false |
| `Entity_UniqueID` | 2つの Entity の GetID() が異なる |
| `Scene_RootEntities` | 親なしエンティティが GetRootEntities に含まれる |
| `Scene_Update` | Update(dt) がクラッシュしない |
| `Scene_Clear` | Clear() 後に全 Entity が破棄される |

**注意**: Entity/Scene/Components は GPU を使わない純粋なロジックなので単体テスト可能。
ただし `MeshRendererComponent` の `model` フィールドは GPU リソースなので nullptr のままテスト。

---

#### テスト 3: `test_ActionMapping.cpp`

```cpp
/// @file test_ActionMapping.cpp
/// @brief ActionMapping 単体テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Input/ActionMapping.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `DefineAction` | DefineAction 後にアクションが存在 |
| `GetActionValue_Default` | 未定義アクションで 0.0f を返す |
| `ClearAllActions` | ClearAllActions 後に全アクションが消える |
| `KeyBinding_Structure` | InputBinding::Key(VK_SPACE) が正しいバインディング |
| `KeyAxisBinding_Structure` | InputBinding::KeyAxis(W, S) が正しいバインディング |

**注意**: `Update()` は `Keyboard`, `Mouse`, `Gamepad` の実インスタンスが必要。
モック化が困難なため、構造テストに限定する。
将来的には `Keyboard` のモックを追加して Update のテストも行う。

---

#### テスト 4: `test_NavMesh.cpp`

```cpp
/// @file test_NavMesh.cpp
/// @brief NavMesh A* パス検索テスト
#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `Build_EmptyGrid` | 空のグリッドで Build が成功 |
| `Build_WithObstacles` | 障害物ありで Build が成功 |
| `FindPath_StraightLine` | 障害物なしの直線パス |
| `FindPath_AroundObstacle` | 障害物を迂回するパス |
| `FindPath_NoPath` | 到達不可能な場合に空パスを返す |
| `FindPath_SameStartEnd` | 同じ開始/終了点で空パスまたは1点 |
| `SetWalkable_False` | 障害物セル上はパスを通らない |
| `GetCellAt` | 座標からセルインデックスへの変換 |
| `NavAgent_SetDestination` | 目的地設定が正しく保存される |
| `NavAgent_Update` | Update 後に位置が変化する |
| `NavAgent_HasReached` | 目的地到着で HasReachedDestination() == true |

---

#### テスト 5: `test_LODGroup.cpp`

```cpp
/// @file test_LODGroup.cpp
/// @brief LODGroup 選択ロジックテスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/LODGroup.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `SelectLOD_Closest` | 近距離で LOD 0 を選択 |
| `SelectLOD_Farthest` | 遠距離で最低 LOD を選択 |
| `SelectLOD_Hysteresis` | ヒステリシスバンド内で LOD が変化しない |
| `Empty_ReturnsNeg1` | LOD なしで -1 を返す |
| `SingleLOD` | LOD 1つのみで常にそれを返す |

**注意**: `SelectLOD` がカメラ距離とスクリーン占有率で判定するため、
Model* は nullptr でもバウンディング球の半径だけ設定してテスト可能かを確認。
不可能な場合はモック構造体を使用。

---

#### テスト 6: `test_SceneSerializer.cpp`

```cpp
/// @file test_SceneSerializer.cpp
/// @brief SceneSerializer JSON ラウンドトリップテスト
#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Scene.h"
#include "Core/Scene/SceneSerializer.h"
using namespace GX;
```

**テストケース:**
| テスト名 | 内容 |
|---------|------|
| `SaveLoad_EmptyScene` | 空シーンの保存/読み込みラウンドトリップ |
| `SaveLoad_SingleEntity` | 1エンティティの名前と Transform が保持される |
| `SaveLoad_Hierarchy` | 親子関係が保持される |
| `SaveLoad_Components` | コンポーネントデータが保持される |
| `SaveLoad_MultipleEntities` | 複数エンティティの順序と関係が保持される |
| `ToJsonString_Valid` | JSON 文字列が有効な JSON である |
| `FromJsonString_Invalid` | 不正な JSON で false を返す |

**注意**: Model は nullptr のままテスト。ModelLoadCallback には空のラムダを渡す。

---

### 37c: テスト CMakeLists.txt 更新

新規テストファイルを `Tests/CMakeLists.txt` の `TEST_SOURCES` リストに追加する:

```cmake
# 既存リストに追加
set(TEST_SOURCES
    test_main.cpp
    test_Vector.cpp
    test_Matrix.cpp
    test_Quaternion.cpp
    test_Color.cpp
    test_MathUtil.cpp       # 既存 (MathUtil + Random)
    test_Collision2D.cpp
    test_Collision3D.cpp
    test_Spatial.cpp
    test_Crypto.cpp         # 既存
    test_Allocator.cpp      # 既存
    # Phase 37 新規
    test_Spline.cpp
    test_Entity.cpp
    test_ActionMapping.cpp
    test_NavMesh.cpp
    test_LODGroup.cpp
    test_SceneSerializer.cpp
)
```

### 37d: テスト実行と検証

```bash
cmake -B build -S .
cmake --build build --config Debug --target GXLibTests
ctest --test-dir build --build-config Debug --verbose
```

全テストが PASS することを確認。

### 37e: 注意事項

- テストは GPU を使わない純粋なロジックテストに限定する
- `pch.h` は GXLib 本体と同じものを使用（Windows.h, DirectXMath 等が含まれる）
- `using namespace GX;` を各テストファイルの冒頭で宣言
- NavMesh/NavAgent のテストは内部状態に依存するため、API を確認してから書く
- Scene/Entity は GPU リソースを持つコンポーネント（MeshRenderer 等）を使う場合、
  model ポインタは nullptr のままテストする
- `std::vector<bool>` の問題に注意（ImGui テストではないが、Components 内で使う可能性）

---

## Phase 38: ドキュメント改善

### 目的
DocumentationAudit.md で指摘された問題（専門用語未説明、前提知識の暗黙仮定、
「なぜ」の欠如、トラブルシューティング不足）を体系的に改善する。

### 38a: 既存ドキュメント構造

```
docs/
├── tutorials/
│   ├── 01_GettingStarted.md    — 環境構築 + Hello World
│   ├── 02_2DDrawing.md         — スプライト、プリミティブ
│   ├── 03_InputAndSound.md     — キーボード、マウス、サウンド
│   ├── 04_3DRendering.md       — PBR、カメラ、ライト、ポストエフェクト
│   └── 05_GUI.md               — ウィジェット、CSS、XMLレイアウト
├── migration/
│   └── DxLibMigrationGuide.md  — DXLib からの移行ガイド
├── Glossary.md                 — 用語集
├── DocumentationAudit.md       — 監査レポート（本 Phase の入力）
└── Phase31-34_Directive.md     — 過去の指令書
```

### 38b: チュートリアル改善

各チュートリアルに以下のセクションを追加/改善:

#### 全チュートリアル共通の追加セクション

**冒頭に「前提知識」セクションを追加:**
```markdown
## 前提知識
- C++ の基礎（変数、関数、クラス、ポインタ）
- Visual Studio 2022 の基本操作
- コマンドラインの基本操作（cd, mkdir）
```

**末尾に「よくある問題」セクションを追加:**
```markdown
## よくある問題

### Q: ビルドは成功するが、実行時にシェーダーが見つからないエラーが出る
Visual Studio からデバッグ実行する場合、作業ディレクトリが exe の場所と
異なることがあります。VS_DEBUGGER_WORKING_DIRECTORY は CMake で自動設定
されますが、手動で開いた場合はプロジェクトプロパティ → デバッグ →
作業ディレクトリを `$(TargetDir)` に設定してください。

### Q: GX_Init() が -1 を返す
DirectX 12 に対応した GPU とドライバ（Windows 10 1903以降）が必要です。
GPU ドライバを最新に更新してください。

### Q: テクスチャが表示されない（白い四角になる）
テクスチャファイルのパスが正しいか確認してください。
相対パスは exe の場所からの相対です。
```

#### チュートリアル 01: Getting Started 改善

1. **WinMain の説明を追加**:
   ```markdown
   > **WinMain とは？**
   > Windows のデスクトップアプリケーションのエントリーポイント（開始地点）です。
   > コンソールアプリの `main()` に相当します。`HINSTANCE` はアプリケーションの
   > インスタンスハンドルで、Windows がアプリを識別するために使います。
   ```

2. **ダブルバッファリングの概念を説明**:
   ```markdown
   > **なぜ裏画面に描画するのか？**
   > 直接画面に描画すると、描画途中の不完全な画像が一瞬見えてしまいます
   > （ティアリング）。裏画面に描き終えてから一括で表示に切り替える
   > （ScreenFlip）ことで、滑らかな表示になります。この手法を
   > 「ダブルバッファリング」と呼びます。
   ```

3. **GXEasy パターンの推奨**:
   初心者向けに `GXEasy::App` パターンを最初に示し、低レベル API は後の章に回す。

#### チュートリアル 04: 3D Rendering 改善

1. **PBR の説明を追加**:
   ```markdown
   ## PBR（物理ベースレンダリング）とは

   PBR は、光の物理法則に基づいてマテリアルの見た目を計算する手法です。
   従来のレンダリング（Phong シェーディング等）と比べて、以下の利点があります：

   - **一貫性**: どのライティング環境でもマテリアルが自然に見える
   - **直感性**: metallic（金属度）と roughness（粗さ）の2パラメータで
     金属からプラスチックまで表現できる

   | metallic | roughness | 見た目 |
   |----------|-----------|--------|
   | 0.0 | 0.1 | 磨かれたプラスチック |
   | 0.0 | 0.9 | マットな布/木 |
   | 1.0 | 0.1 | 鏡面研磨された金属 |
   | 1.0 | 0.9 | 錆びた/粗い金属 |
   ```

2. **ポストエフェクト略語の展開**:
   ```markdown
   | 略語 | 正式名称 | 効果 |
   |------|---------|------|
   | SSAO | Screen Space Ambient Occlusion | 隅や隙間を暗くして立体感を出す |
   | SSR | Screen Space Reflections | 画面上の情報だけで反射を計算 |
   | DoF | Depth of Field | カメラのピンぼけ効果 |
   | TAA | Temporal Anti-Aliasing | 時間方向にサンプリングしてジャギーを減らす |
   | FXAA | Fast Approximate Anti-Aliasing | 軽量なエッジ滑らか化 |
   | HDR | High Dynamic Range | 明るさの表現範囲を拡大 |
   ```

### 38c: 新規チュートリアル

#### チュートリアル 06: GXEasy で始める 2D ゲーム（新規）

対象読者: C++ 初級者（GXEasy パターンで DXLib 風の簡単な API）

```markdown
# 06. GXEasy で始める 2D ゲーム

## 前提知識
- C++ の基礎（変数、if文、for文、関数）
- Visual Studio のプロジェクト作成

## このチュートリアルで学ぶこと
- GXEasy::App クラスの使い方
- 画面にテキストと図形を表示する方法
- キーボード入力の取得
- 簡単なゲームループの作り方

## Step 1: 空のウィンドウを開く
[GXEasy::App の最小コード]

## Step 2: 背景と文字を表示する
[DrawString, DrawBox]

## Step 3: キー入力で四角を動かす
[CheckHitKey, 位置変数の更新]

## Step 4: 当たり判定を追加する
[矩形 vs 矩形の当たり判定]
```

#### チュートリアル 07: 3D シーンを作る（新規）

対象読者: チュートリアル 04 完了者

```markdown
# 07. 3D シーンを作る

## このチュートリアルで学ぶこと
- Scene と Entity の使い方
- コンポーネントの追加
- カメラ操作
- ライティングの設定

## Step 1: 空のシーンを作成する
## Step 2: エンティティを追加する
## Step 3: カメラとライトを設定する
## Step 4: シーンを保存/読み込みする
```

#### チュートリアル 08: アセットパイプライン（新規）

```markdown
# 08. アセットパイプライン

## このチュートリアルで学ぶこと
- gxconv で 3D モデルを変換する
- gxpak でアセットをバンドルする
- VFS でバンドルからアセットを読み込む
- GXModelViewer でモデルを確認・編集する
```

### 38d: 関数リファレンス

`docs/api/` ディレクトリに主要 API のリファレンスページを作成。

#### 対象（優先度順）:

1. **GXEasy API** (`docs/api/GXEasy.md`)
   - GXEasy::App クラス
   - DXLib 互換関数（DrawGraph, DrawString, CheckHitKey 等）
   - FormatT テンプレート

2. **Math API** (`docs/api/Math.md`)
   - Vector2/3/4
   - Matrix4x4
   - Quaternion
   - Color
   - Spline
   - MathUtil

3. **Input API** (`docs/api/Input.md`)
   - Keyboard, Mouse, Gamepad
   - ActionMapping

4. **Audio API** (`docs/api/Audio.md`)
   - AudioManager
   - SoundPlayer, MusicPlayer
   - 3D Audio (AudioEmitter, AudioListener)

5. **Graphics API** (`docs/api/Graphics.md`)
   - Renderer3D（主要メソッドのみ）
   - Camera3D
   - Material
   - Light
   - PostEffectPipeline

6. **Scene API** (`docs/api/Scene.md`)
   - Scene, Entity
   - Components
   - SceneSerializer

#### リファレンスの形式

各関数について以下を記載:

```markdown
### DrawGraph(x, y, handle, transparent)

画像をそのままのサイズで描画します。

**引数:**
| 名前 | 型 | 説明 |
|------|-----|------|
| x | int | 描画先の左上 X 座標（ピクセル） |
| y | int | 描画先の左上 Y 座標（ピクセル） |
| handle | int | LoadGraph で取得したテクスチャハンドル |
| transparent | int | TRUE: 透過描画, FALSE: 不透過 |

**戻り値:** 0（成功）, -1（失敗）

**使用例:**
```cpp
int tex = LoadGraph("player.png");
DrawGraph(100, 200, tex, TRUE);
```

**注意:**
- handle が無効な場合は何も描画されません
- 座標は画面左上が (0, 0) です
```

### 38e: Glossary.md の拡充

既存の用語集に以下を追加:

| 用語 | 正式名称 | 説明 |
|------|---------|------|
| BLAS | Bottom-Level Acceleration Structure | DXR で個別メッシュのレイトレーシング高速化構造 |
| TLAS | Top-Level Acceleration Structure | DXR でシーン全体のレイトレーシング高速化構造 |
| GI | Global Illumination | 間接光を含むシーン全体の照明計算 |
| BRDF | Bidirectional Reflectance Distribution Function | 表面の光の反射特性を記述する関数 |
| IBL | Image-Based Lighting | 環境マップ画像を使った照明手法 |
| UTS2 | Unity Toon Shader 2.0 | Unity 用のトゥーンシェーダー実装。GXLib の Toon シェーダーの参考元 |
| LOD | Level of Detail | カメラからの距離に応じてモデルの詳細度を切り替える手法 |
| A* | A-star | 最短経路探索アルゴリズム。NavMesh で使用 |
| ECS | Entity Component System | ゲームオブジェクトをエンティティとコンポーネントで構成する設計パターン |
| VFS | Virtual File System | 物理ファイルとアーカイブを統一的に扱うファイルシステム抽象化 |
| PSO | Pipeline State Object | D3D12 のレンダリングパイプラインの状態を一括管理するオブジェクト |

### 38f: README.md 改善

- 特徴一覧の略語に括弧で日本語説明を追加
- Phase 23-35 の新機能を反映
- サンプルプロジェクト一覧を 22 個に更新
- ビルド手順のトラブルシューティングセクションを追加

### 38g: サンプル解説

各サンプルの `main.cpp` 冒頭に以下を記載（既にある程度あるが不足分を補完）:

```cpp
/// @file Samples/XXX/main.cpp
/// @brief [サンプルの1行説明]
///
/// [サンプルの詳細説明（3-5行）]
/// [学べるポイント]
/// [使用している GXLib 機能のリスト]
///
/// Controls:
///   [操作方法のリスト]
```

---

## Phase 39: 新エンジン機能（前半）— レンダリング高度化

### 目的
レンダリングパイプラインを高度化し、大規模シーンへの対応力を向上させる。

### 39a: マルチスレッドコマンド記録

#### 概要
D3D12 の最大の利点であるマルチスレッドコマンドリスト記録を活用し、
CPU バウンドな描画コール発行を並列化する。

#### アーキテクチャ

```
メインスレッド:
  BeginFrame()
  ├─ ワーカースレッド 0: シャドウパス CommandList 記録
  ├─ ワーカースレッド 1: 不透明パス CommandList 記録 (前半)
  ├─ ワーカースレッド 2: 不透明パス CommandList 記録 (後半)
  └─ メインスレッド: ポストエフェクト + UI
  ExecuteCommandLists(shadowCL, opaqueCL0, opaqueCL1, postFxCL)
  Present()
```

#### 新規ファイル
- `GXLib/Graphics/Device/ParallelCommandRecorder.h`
- `GXLib/Graphics/Device/ParallelCommandRecorder.cpp`

#### クラス設計
```cpp
namespace GX
{
    class ParallelCommandRecorder
    {
    public:
        /// @param device D3D12デバイス
        /// @param numWorkers ワーカースレッド数（0=ハードウェア並列数-1）
        void Initialize(ID3D12Device* device, uint32_t numWorkers = 0);
        void Shutdown();

        /// @brief 記録ジョブを追加
        /// @param job 引数は (ID3D12GraphicsCommandList*, uint32_t frameIndex)
        void AddRecordJob(std::function<void(ID3D12GraphicsCommandList*, uint32_t)> job);

        /// @brief 全ジョブを並列実行し、完了を待つ
        /// @return 記録済み CommandList の配列
        std::vector<ID3D12CommandList*> ExecuteAndWait(uint32_t frameIndex);

    private:
        struct WorkerThread
        {
            std::thread thread;
            CommandList cmdList;  // ワーカーごとに独立した CommandList
        };
        std::vector<WorkerThread> m_workers;
        // スレッドプール + ジョブキュー
    };
}
```

#### 実装の注意点
- **CommandAllocator**: フレーム × ワーカー数 の CommandAllocator が必要
  （CommandAllocator はスレッドセーフではないため共有不可）
- **DescriptorHeap**: GPU-visible ヒープはスレッド間で共有可能だが、
  CPU-visible ヒープの割り当てはスレッドローカルに行う
- **RootSignature/PSO**: 読み取り専用なのでスレッド間共有可能
- **リソースバリア**: 各 CommandList のバリアは独立。
  ExecuteCommandLists の順序がバリアの順序を決定する
- **pch.h**: `<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>` は既に含まれている

#### 統合方法
Renderer3D に `SetParallelMode(bool)` を追加:
```cpp
void Renderer3D::Begin(ID3D12GraphicsCommandList* cmd, uint32_t frameIndex,
                       const Camera3D& camera, float time)
{
    if (m_parallelMode && m_parallelRecorder)
    {
        // ワーカースレッドに描画を分配
    }
    else
    {
        // 従来のシングルスレッド描画
    }
}
```

### 39b: Indirect Drawing（間接描画）

#### 概要
GPU 駆動レンダリングの基盤。CPU ではなく GPU 側で描画コマンドを生成する。

#### 新規ファイル
- `GXLib/Graphics/Device/IndirectCommandBuffer.h`
- `GXLib/Graphics/Device/IndirectCommandBuffer.cpp`

#### クラス設計
```cpp
namespace GX
{
    class IndirectCommandBuffer
    {
    public:
        void Initialize(ID3D12Device* device, uint32_t maxCommands);

        /// @brief 間接描画引数を追加（CPU 側で構築）
        void AddDrawIndexedArgs(uint32_t indexCount, uint32_t instanceCount,
                                uint32_t startIndex, int32_t baseVertex,
                                uint32_t startInstance);

        /// @brief GPU で ExecuteIndirect を呼ぶ
        void Execute(ID3D12GraphicsCommandList* cmdList,
                     ID3D12CommandSignature* cmdSig);

        /// @brief コマンド数を取得
        uint32_t GetCommandCount() const;

    private:
        Buffer m_argBuffer;      // D3D12_DRAW_INDEXED_ARGUMENTS の配列
        Buffer m_countBuffer;    // コマンド数（GPU カリング時に使用）
        uint32_t m_maxCommands;
        uint32_t m_currentCount = 0;
    };
}
```

#### 用途
- Scene::Render でフラスタムカリング後の描画リストを IndirectCommandBuffer に書き込み、
  ExecuteIndirect で一括描画
- 将来的に Compute Shader でのGPUカリングと組み合わせ可能

### 39c: Async Compute

#### 概要
グラフィックスキューと並列に Compute キューで計算を行い、GPU 使用率を向上させる。

#### 対象タスク
- SSAO の Compute Shader 版
- Bloom の Compute Shader 版（ダウンサンプル+アップサンプル）
- パーティクル更新（既存の GPUParticleSystem は Graphics キューで Dispatch）
- RTGI のデノイズパス

#### 新規ファイル
- `GXLib/Graphics/Device/AsyncComputeQueue.h`
- `GXLib/Graphics/Device/AsyncComputeQueue.cpp`

#### クラス設計
```cpp
namespace GX
{
    class AsyncComputeQueue
    {
    public:
        void Initialize(ID3D12Device* device);

        /// @brief Compute ジョブをキューに追加
        void Submit(std::function<void(ID3D12GraphicsCommandList*)> computeJob);

        /// @brief GPU フェンスで同期（Graphics キューが結果を使う前に呼ぶ）
        void WaitOnGraphicsQueue(ID3D12CommandQueue* graphicsQueue);

        /// @brief 前フレームの Compute が完了しているか確認
        bool IsComplete() const;

    private:
        CommandQueue m_computeQueue;  // D3D12_COMMAND_LIST_TYPE_COMPUTE
        CommandList m_computeCmdList;
        Fence m_fence;
    };
}
```

#### 実装の注意点
- **リソース状態**: Compute キューでは `D3D12_RESOURCE_STATE_UNORDERED_ACCESS` が必要
- **フェンス同期**: Graphics → Compute → Graphics の依存関係をフェンスで管理
- **タイミング**: 前フレームの Compute 結果を今フレームの Graphics で使用（1フレーム遅延許容）

### 39d: サンプルプロジェクト

- `Samples/MultiThreadShowcase/main.cpp` — マルチスレッド描画のデモ（1000 オブジェクト）

---

## Phase 40: 新エンジン機能（後半）— ゲームプレイ機能

### 目的
ゲーム開発に直結する実用的な機能を追加する。

### 40a: Lua スクリプティング

#### 概要
ゲームロジックを C++ から分離し、ホットリロード可能な Lua スクリプトで記述できるようにする。

#### 依存ライブラリ
- **sol2** (header-only Lua C++ バインディング)
- **Lua 5.4** (FetchContent または ThirdParty に同梱)

#### 新規ファイル
- `GXLib/Script/ScriptEngine.h`
- `GXLib/Script/ScriptEngine.cpp`
- `GXLib/Script/ScriptBindings.h`
- `GXLib/Script/ScriptBindings.cpp`

#### クラス設計
```cpp
namespace GX
{
    class ScriptEngine
    {
    public:
        void Initialize();
        void Shutdown();

        /// @brief Lua ファイルを読み込んで実行
        bool ExecuteFile(const std::string& path);

        /// @brief Lua 文字列を実行
        bool ExecuteString(const std::string& code);

        /// @brief グローバル関数を呼ぶ
        template<typename Ret, typename... Args>
        Ret CallFunction(const std::string& name, Args&&... args);

        /// @brief ホットリロード（FileWatcher と連携）
        void ReloadFile(const std::string& path);

        /// @brief sol::state への直接アクセス（上級者向け）
        sol::state& GetState() { return m_lua; }

    private:
        sol::state m_lua;
    };
}
```

#### バインディング対象（ScriptBindings.cpp）

```cpp
void RegisterBindings(sol::state& lua)
{
    // Math
    lua.new_usertype<Vector2>("Vector2",
        sol::constructors<Vector2(), Vector2(float, float)>(),
        "x", &Vector2::x, "y", &Vector2::y,
        "Length", &Vector2::Length,
        "Normalized", &Vector2::Normalized,
        sol::meta_function::addition, &Vector2::operator+,
        sol::meta_function::subtraction, &Vector2::operator-
    );
    // Vector3, Color, Transform3D 等も同様

    // Input
    lua["IsKeyDown"] = [](int key) { return CheckHitKey(key) != 0; };
    lua["GetMouseX"] = []() { int x, y; GetMousePoint(&x, &y); return x; };
    lua["GetMouseY"] = []() { int x, y; GetMousePoint(&x, &y); return y; };

    // Drawing (GXEasy 互換)
    lua["DrawString"] = &DrawString;
    lua["DrawBox"] = &DrawBox;
    lua["DrawGraph"] = &DrawGraph;
    lua["LoadGraph"] = &LoadGraph;

    // Entity/Scene
    lua.new_usertype<Entity>("Entity",
        "GetName", &Entity::GetName,
        "SetPosition", [](Entity& e, float x, float y, float z) {
            e.GetTransform().SetPosition(x, y, z);
        },
        "GetPosition", [](Entity& e) -> Vector3 {
            auto p = e.GetTransform().GetPosition();
            return Vector3(p.x, p.y, p.z);
        }
    );
}
```

#### 使用例（Lua 側）
```lua
-- game.lua
function OnStart()
    player = scene:CreateEntity("Player")
    player:SetPosition(0, 1, 0)
end

function OnUpdate(dt)
    local speed = 5.0 * dt
    if IsKeyDown(KEY_W) then
        local pos = player:GetPosition()
        player:SetPosition(pos.x, pos.y, pos.z + speed)
    end
end

function OnDraw()
    DrawString(10, 10, "Hello from Lua!", 0xFFFFFFFF)
end
```

#### 実装の注意点
- **sol2**: header-only なので CMake に FetchContent で追加するか ThirdParty/ に配置
- **Lua 5.4**: C ライブラリなので `SKIP_PRECOMPILE_HEADERS ON` + `LANGUAGE C` が必要
  （lz4.c, ufbx.c と同パターン）
- **エラーハンドリング**: Lua 実行エラーは `GX::Logger::Error()` で報告
- **ホットリロード**: `FileWatcher` と連携して .lua ファイル変更時に自動リロード
- **pch.h**: sol2 のヘッダは巨大なので pch.h には入れない。ScriptEngine.cpp 内でのみインクルード

### 40b: 2D タイルマップ

#### 概要
2D ゲーム用のタイルマップシステム。Tiled エディタ (.tmx) のインポートに対応。

#### 新規ファイル
- `GXLib/Graphics/Rendering/Tilemap.h`
- `GXLib/Graphics/Rendering/Tilemap.cpp`

#### クラス設計
```cpp
namespace GX
{
    struct TileLayer
    {
        std::string name;
        int width, height;
        std::vector<int> tileIDs;  // -1 = 空
        float opacity = 1.0f;
        bool visible = true;
    };

    struct Tileset
    {
        int textureHandle;
        int tileWidth, tileHeight;
        int columns;
        int firstGID;
    };

    class Tilemap
    {
    public:
        /// @brief TMX ファイルを読み込む
        bool LoadFromTMX(const std::string& path);

        /// @brief タイルマップを描画
        void Draw(SpriteBatch& batch, const Camera2D& camera);

        /// @brief 指定座標のタイル ID を取得
        int GetTileAt(int layerIndex, int x, int y) const;

        /// @brief 指定タイルが walkable かどうか
        bool IsWalkable(int x, int y) const;

        /// @brief タイル座標 → ワールド座標
        Vector2 TileToWorld(int x, int y) const;

        /// @brief ワールド座標 → タイル座標
        void WorldToTile(float wx, float wy, int& tx, int& ty) const;

        int GetWidth() const;
        int GetHeight() const;
        int GetTileWidth() const;
        int GetTileHeight() const;
        int GetLayerCount() const;
        const TileLayer& GetLayer(int index) const;

    private:
        std::vector<TileLayer> m_layers;
        std::vector<Tileset> m_tilesets;
        int m_width = 0, m_height = 0;
        int m_tileWidth = 0, m_tileHeight = 0;
    };
}
```

#### TMX パース
- TMX は XML 形式。GXLib に既存の `XMLParser` を活用可能
- CSV 形式のタイルデータをパース
- タイルセット画像は `TextureManager::LoadTexture()` で読み込み

#### 描画最適化
- カメラの可視範囲のタイルのみ描画（カリング）
- SpriteBatch のバッチ描画を活用（同一テクスチャのタイルをまとめる）

### 40c: ルートモーション

#### 概要
アニメーションの移動量をキャラクターの Transform に反映する。
歩きアニメーションの足の滑りを防ぐ。

#### 修正ファイル
- `GXLib/Graphics/3D/Animator.h`
- `GXLib/Graphics/3D/Animator.cpp`
- `GXLib/Graphics/3D/Animation.h`

#### 追加 API
```cpp
class Animator
{
public:
    // 既存 API...

    /// @brief ルートモーションを有効化
    void SetRootMotionEnabled(bool enabled);
    bool IsRootMotionEnabled() const;

    /// @brief 今フレームのルートモーション移動量を取得
    /// Transform に加算して使用する
    XMFLOAT3 GetRootMotionDelta() const;

    /// @brief 今フレームのルートモーション回転量を取得
    Quaternion GetRootMotionRotationDelta() const;

private:
    bool m_rootMotion = false;
    XMFLOAT3 m_rootDelta = {};
    Quaternion m_rootRotDelta = Quaternion::Identity();
    XMFLOAT3 m_lastRootPos = {};
};
```

#### 実装方針
1. アニメーション更新時にルートボーン（通常は Hips）の位置変化を計算
2. ルートボーンの移動量を `m_rootDelta` に保存
3. ルートボーンの位置をアニメーション内でゼロに戻す（XZ 平面のみ）
4. ゲーム側で `GetRootMotionDelta()` を Transform に加算

```cpp
// ゲーム側の使用例
void Update(float dt)
{
    animator.Update(dt);
    if (animator.IsRootMotionEnabled())
    {
        auto delta = animator.GetRootMotionDelta();
        auto pos = transform.GetPosition();
        pos.x += delta.x;
        pos.z += delta.z;
        transform.SetPosition(pos.x, pos.y, pos.z);
    }
}
```

### 40d: アニメーションイベント

#### 概要
アニメーションの特定フレームでコールバックを発火する仕組み。
足音、攻撃ヒット判定、エフェクト発生等に使用。

#### 修正ファイル
- `GXLib/Graphics/3D/Animation.h`
- `GXLib/Graphics/3D/Animator.h`
- `GXLib/Graphics/3D/Animator.cpp`

#### 追加 API
```cpp
struct AnimationEvent
{
    float time;           // 発火時刻（秒）
    std::string name;     // イベント名
};

class Animator
{
public:
    /// @brief イベントコールバックを登録
    void SetEventCallback(std::function<void(const std::string&)> callback);

    /// @brief 特定クリップにイベントを追加
    void AddEvent(const std::string& clipName, float time, const std::string& eventName);

private:
    std::function<void(const std::string&)> m_eventCallback;
};
```

#### 使用例
```cpp
animator.SetEventCallback([](const std::string& event) {
    if (event == "footstep")
        PlaySound(footstepSound);
    else if (event == "attack_hit")
        CheckAttackCollision();
});
animator.AddEvent("Walk", 0.3f, "footstep");
animator.AddEvent("Walk", 0.8f, "footstep");
animator.AddEvent("Attack", 0.5f, "attack_hit");
```

### 40e: サンプルプロジェクト

- `Samples/LuaShowcase/main.cpp` — Lua スクリプティングデモ
- `Samples/TilemapShowcase/main.cpp` — 2D タイルマップデモ

---

## 共通ルール（全 Phase 共通）

### コーディング規約
- namespace `GX` 内に配置
- `#pragma once` + Doxygen `/// @file` / `/// @brief`
- `#include "pch.h"` を .cpp の先頭に
- DirectXMath は `using namespace DirectX;` を .cpp 内のみ
- XMFLOAT3/4/4X4 はメンバー格納用、XMVECTOR/XMMATRIX は演算用
- ComPtr で COM オブジェクト管理
- `std::unique_ptr` で所有権管理
- `constexpr` で定数定義
- エラーログは `GX::Logger::Error()`、情報ログは `GX::Logger::Info()`
- No CD3DX12 helpers — raw D3D12 structs を使用
- C++ に saturate() はない — `(std::max)(0.0f, (std::min)(1.0f, x))` を使用
- pch.h に `<sstream>`, `<unordered_set>`, `<deque>`, `<filesystem>` は **ない** — 代替を使用

### ビルド手順（各 Phase 完了時）
```bash
cmake -B build -S .
cmake --build build --config Debug
ctest --test-dir build --build-config Debug
```

### 新規ファイルの CMake 対応
- `GXLib/` 配下は `GLOB_RECURSE` で自動収集（新ディレクトリも含む）
- ただし `GXLib/AI/*.cpp` は Phase 29 で別途追加済み
- 新しい `GXLib/Script/*.cpp` は `file(GLOB_RECURSE GXLIB_SCRIPT_SOURCES GXLib/Script/*.cpp)` で追加
- `ThirdParty/` の C ファイル（Lua 等）は `SKIP_PRECOMPILE_HEADERS ON` + `LANGUAGE C`
- 新しいサンプルは `gxlib_add_sample(SampleName)` マクロ
- `Tests/CMakeLists.txt` に新テストファイルを手動追加

### MEMORY.md 更新
各 Phase 完了後に MEMORY.md の Completed Phases セクションと Common Issues を更新すること。

---

## 既知の技術的制約

| 制約 | 影響 | 回避策 |
|------|------|--------|
| pch.h に `<sstream>` なし | std::stringstream 使用不可 | std::format / std::to_string |
| pch.h に `<unordered_set>` なし | unordered_set 使用不可 | vector + sort + unique |
| pch.h に `<deque>` なし | std::deque 使用不可 | vector + リングバッファ |
| pch.h に `<filesystem>` なし | std::filesystem 使用不可 | Win32 API or `<cstdio>` std::remove |
| Windows.h min/max マクロ | std::min/max と衝突 | `(std::max)(...)` パターン |
| FormatT by-value 引数 | Args&& 使用不可 | P2905R2/MSVC14.44 制約 |
| ImTextureID = ImU64 | ポインタ直接渡し不可 | `static_cast<ImTextureID>(handle.ptr)` |
| std::vector<bool> proxy | ImGui::Checkbox 互換なし | ローカル bool にコピー |
| D3D12 Root SRV | Texture2D.Sample() 不可 | shader-visible ヒープ使用 |
| Color{1,1,1,1} 曖昧性 | int/float オーバーロード競合 | `Color(1.0f, 1.0f, 1.0f, 1.0f)` |

---

## 検証チェックリスト

各 Phase 完了時に以下を確認:

- [ ] `cmake -B build -S . && cmake --build build --config Debug` エラーゼロ
- [ ] `ctest --test-dir build --build-config Debug` 全テスト PASS
- [ ] 新規サンプルが起動してクラッシュしない
- [ ] 既存 22 サンプルが壊れていない
- [ ] GXModelViewer が起動する
- [ ] gxconv / gxpak がビルドできる
- [ ] MEMORY.md が更新されている

---

## 実装優先順序

```
Phase 36 (バグ修正) ──┐
Phase 37 (テスト)   ──┼── 並列着手可能
Phase 38 (ドキュメント)┘
        ↓
Phase 39 (レンダリング高度化) ← Phase 36 完了後
        ↓
Phase 40 (ゲームプレイ機能) ← Phase 36 完了後
```

Phase 39/40 内部の推奨順序:

**Phase 39:**
1. 39b (Indirect Drawing) — 既存パイプラインへの影響が最小
2. 39a (マルチスレッド) — アーキテクチャ変更が大きいため慎重に
3. 39c (Async Compute) — 39a の経験を活かして

**Phase 40:**
1. 40d (アニメーションイベント) — 最も軽量、すぐに効果
2. 40c (ルートモーション) — アニメーション品質向上
3. 40b (タイルマップ) — 2D ゲーム開発の基盤
4. 40a (Lua スクリプティング) — 最も大規模、外部依存あり

---

## ファイル一覧（全 Phase）

### Phase 36: 修正対象（BugReport.md 76件 + Extra 4件）
```
=== Critical ===
GXLib/GUI/Widgets/DropDown.cpp            — C-01: 配列範囲外アクセス
GXLib/GUI/Widgets/ListView.cpp            — C-02: 配列範囲外アクセス
GXLib/Graphics/Resource/TextureManager.cpp — C-03: 負インデックスアクセス
Sandbox/main.cpp                          — RT-C01: CreateGeometrySRVs() 追加
GXLib/Graphics/RayTracing/RTReflections.h  — RT-C02: ComPtr 化
GXLib/Graphics/RayTracing/RTReflections.cpp — RT-C03: ヒープスロット再設計

=== High ===
GXLib/Graphics/Rendering/TextRenderer.cpp  — H-01: vswprintf_s 引数
GXLib/Graphics/Rendering/SpriteBatch.cpp   — H-02: Map null チェック
GXLib/Graphics/Rendering/PrimitiveBatch.cpp — H-03: Map null チェック
GXLib/Graphics/PostEffect/AutoExposure.cpp — H-05: Map null チェック
GXLib/Graphics/RayTracing/RTReflections.cpp — H-06/H-08: HRESULT+null チェック
GXLib/Graphics/PostEffect/PostEffectPipeline.cpp — H-07: null バリア
GXLib/Graphics/PostEffect/SSR.cpp          — H-09: SRV バインド検証
GXLib/IO/Network/HTTPClient.cpp            — H-10: 非同期リソースリーク
GXLib/IO/Network/WebSocket.cpp             — H-11: Use-After-Free
GXLib/IO/AsyncLoader.cpp                   — H-12: レースコンディション
GXLib/Movie/MoviePlayer.cpp                — H-13: null デリファレンス
GXLib/Compat/Compat_2D.cpp                 — H-14: null+オーバーフロー
Shaders/RTReflections.hlsl                 — H-15: コメント修正, RT-H01: R16 対応
GXLib/Graphics/Resource/Texture.cpp        — H-16: 整数オーバーフロー
GXLib/Graphics/Device/BarrierBatch.h       — H-17a: 配列初期化
GXLib/GUI/Widgets/DropDown.cpp             — H-17b: 空アイテムガード
GXLib/Graphics/Rendering/FontManager.cpp   — H-18a: pixelData null
GXLib/GUI/Widgets/TextInput.cpp            — H-18b: 選択範囲境界
GXLib/GUI/Widgets/ScrollView.cpp           — H-19: ゼロ除算
GXLib/GUI/Widgets/Button.h + 7 files       — H-20: m_renderer null 統一
GXLib/Graphics/RayTracing/RTReflections.cpp — RT-H02: インスタンス上限
GXLib/Graphics/RayTracing/RTReflections.cpp — RT-H03: BLAS 連番化

=== Medium ===
GXLib/Graphics/Rendering/TextRenderer.cpp  — M-01: 改行比較
GXLib/Graphics/Rendering/FontManager.cpp   — M-02: 未初期化エントリ
GXLib/Graphics/Rendering/TextRenderer.cpp  — M-03: UV クランプ
GXLib/Graphics/RayTracing/RTReflections.cpp — M-04〜M-07, RT-M01〜RT-M05
GXLib/Graphics/PostEffect/Bloom.cpp        — M-08: エラー伝播
GXLib/Graphics/Resource/Texture.cpp        — M-09: CreateEvent
GXLib/IO/FileWatcher.cpp                   — M-10: ハンドルリーク
GXLib/IO/Crypto.cpp                        — M-11: エラーハンドリング
GXLib/IO/Archive.cpp                       — M-12: 整数オーバーフロー
GXLib/Audio/Sound.cpp                      — M-13: 読み込みエラー
GXLib/Audio/SoundPlayer.cpp                — M-14: コールバック寿命
GXLib/Physics/PhysicsWorld3D.cpp           — M-15: Shape null
GXLib/GUI/Widgets/TextInput.cpp            — M-16: off-by-one
GXLib/GUI/Widgets/TabView.cpp              — M-17: activeTab 範囲
GXLib/GUI/Widgets/DropDown.cpp             — M-18: selectedIndex
Samples/PostEffectShowcase/main.cpp        — M-19: VSync 矛盾
GXLib/GUI/Widgets/TextWidget.cpp           — M-20: 不整合
GXLib/Graphics/Rendering/SpriteBatch.cpp   — M-21: 境界チェック
GXLib/Math/Quaternion.h                    — MATH-01: ToEuler 符号
GXLib/Physics/PhysicsWorld2D.cpp           — MATH-02: 慣性モーメント
GXLib/Physics/PhysicsWorld2D.cpp           — MATH-03: AABB 回転
Shaders/RTReflections.hlsl                 — MATH-04: 法線変換
Shaders/RTReflectionComposite.hlsl         — RT-M07: コメント修正
Shaders/RTReflections.hlsl                 — RT-M06: 法線方向

=== Low ===
GXLib/Graphics/PostEffect/SSAO.cpp         — L-01: static_assert
GXLib/Graphics/PostEffect/VolumetricLight.cpp — L-02: 初期化
GXLib/Physics/MeshCollider.cpp             — L-03: 除算ゼロ
GXLib/Physics/PhysicsWorld2D.cpp           — L-04: Raycast null
GXLib/Math/Random.cpp                      — L-05: 無限ループ
GXLib/Math/Collision/Collision3D.cpp       — L-06: 除算ゼロ
GXLib/GUI/Widgets/Image.cpp                — L-07: UV 精度
GXLib/GUI/StyleSheet.cpp                   — L-08: 例外ハンドリング
Shaders/DepthOfField.hlsl                  — MATH-05: ガウス重み
Shaders/RTReflections.hlsl                 — TECH-01: ポイントライト shadow
GXLib/Physics/PhysicsWorld2D.cpp           — TECH-02: 角インパルス
GXLib/Physics/PhysicsWorld2D.cpp           — TECH-03: 摩擦速度
GXLib/Graphics/RayTracing/RTReflections.h/cpp — RT-L02: 変数名リネーム

=== Extra ===
GXLib/Core/Scene/Entity.h                  — GetComponent constexpr
GXLib/Core/Scene/Scene.cpp                 — DestroyEntity 安全性
GXLib/Graphics/Resource/DynamicBuffer.h    — Map 状態チェック
GXLib/Graphics/PostEffect/*.cpp            — null チェック全数確認
各ファイルの TODO/FIXME 項目                  — 個別対応
```

### Phase 37: 新規テスト（6 ファイル）
```
Tests/test_Spline.cpp          — 新規 (16 テスト)
Tests/test_Entity.cpp          — 新規 (17 テスト)
Tests/test_ActionMapping.cpp   — 新規 (5 テスト)
Tests/test_NavMesh.cpp         — 新規 (11 テスト)
Tests/test_LODGroup.cpp        — 新規 (5 テスト)
Tests/test_SceneSerializer.cpp — 新規 (7 テスト)
Tests/CMakeLists.txt           — 更新 (6 ファイル追加)
```
> **注意:** test_MathUtil.cpp, test_Crypto.cpp, test_Allocator.cpp は既に存在するため新規作成不要

### Phase 38: ドキュメント
```
docs/tutorials/01_GettingStarted.md  — 改善
docs/tutorials/02_2DDrawing.md       — 改善
docs/tutorials/03_InputAndSound.md   — 改善
docs/tutorials/04_3DRendering.md     — 改善
docs/tutorials/05_GUI.md             — 改善
docs/tutorials/06_GXEasy2DGame.md    — 新規
docs/tutorials/07_3DScene.md         — 新規
docs/tutorials/08_AssetPipeline.md   — 新規
docs/api/GXEasy.md                   — 新規
docs/api/Math.md                     — 新規
docs/api/Input.md                    — 新規
docs/api/Audio.md                    — 新規
docs/api/Graphics.md                 — 新規
docs/api/Scene.md                    — 新規
docs/Glossary.md                     — 拡充
README.md                            — 改善
```

### Phase 39: 新規/修正
```
GXLib/Graphics/Device/ParallelCommandRecorder.h   — 新規
GXLib/Graphics/Device/ParallelCommandRecorder.cpp  — 新規
GXLib/Graphics/Device/IndirectCommandBuffer.h      — 新規
GXLib/Graphics/Device/IndirectCommandBuffer.cpp    — 新規
GXLib/Graphics/Device/AsyncComputeQueue.h          — 新規
GXLib/Graphics/Device/AsyncComputeQueue.cpp        — 新規
GXLib/Graphics/3D/Renderer3D.h                     — 並列モード追加
GXLib/Graphics/3D/Renderer3D.cpp                   — 〃
Samples/MultiThreadShowcase/CMakeLists.txt         — 新規
Samples/MultiThreadShowcase/main.cpp               — 新規
```

### Phase 40: 新規/修正
```
GXLib/Script/ScriptEngine.h           — 新規
GXLib/Script/ScriptEngine.cpp         — 新規
GXLib/Script/ScriptBindings.h         — 新規
GXLib/Script/ScriptBindings.cpp       — 新規
GXLib/Graphics/Rendering/Tilemap.h    — 新規
GXLib/Graphics/Rendering/Tilemap.cpp  — 新規
GXLib/Graphics/3D/Animator.h          — ルートモーション+イベント追加
GXLib/Graphics/3D/Animator.cpp        — 〃
GXLib/Graphics/3D/Animation.h         — AnimationEvent 追加
GXLib/CMakeLists.txt                  — Script/ GLOB + Lua 追加
Samples/LuaShowcase/CMakeLists.txt    — 新規
Samples/LuaShowcase/main.cpp          — 新規
Samples/TilemapShowcase/CMakeLists.txt — 新規
Samples/TilemapShowcase/main.cpp      — 新規
```

## Phase 37 Directive

## Context

Phase 36（バグ修正）完了済み。現在 **151 テスト** が 11 ファイルに存在し、全パス。
本 Phase では GPU を使用しない純粋ロジックテストを **10 ファイル新規追加** し、
カバレッジを約 **260+ テスト** まで拡大する。

**重要**: この指令書は、GXLib プロジェクトの事前知識がない Claude インスタンスでも
単独で Phase 37 を完遂できるよう、全テストケースのコード骨格まで記載している。

---

## 前提条件

- **プロジェクトルート**: `C:\Users\g0190\Desktop\GXLib`
- **テストディレクトリ**: `C:\Users\g0190\Desktop\GXLib\Tests\`
- **テストフレームワーク**: Google Test v1.15.2 (FetchContent)
- **ビルド**: `cmake -B build -S .` → `cmake --build build --config Debug`
- **テスト実行**: `ctest --test-dir build --build-config Debug --output-on-failure`
- **pch.h**: GXLib 本体と共有（Windows.h, D3D12, DirectXMath, STL 含む）
- **pch.h に無いヘッダ**: `<sstream>`, `<unordered_set>` — 使用禁止
- **名前空間**: `using namespace GX;`（全テストファイル共通）

---

## 既存テスト一覧（11 ファイル / 151 テスト）

| ファイル | テスト数 | 対象 |
|---------|---------|------|
| test_main.cpp | 0 | エントリーポイント（gtest_main 使用） |
| test_Vector.cpp | 29 | Vector2/3/4 |
| test_Matrix.cpp | 11 | Matrix4x4 |
| test_Quaternion.cpp | 10 | Quaternion |
| test_Color.cpp | 10 | Color |
| test_MathUtil.cpp | 19 | MathUtil + Random |
| test_Collision2D.cpp | 19 | 2D 衝突判定 |
| test_Collision3D.cpp | 24 | 3D 衝突判定 |
| test_Spatial.cpp | 13 | Quadtree/Octree/BVH |
| test_Crypto.cpp | 6 | AES-256-CBC, SHA-256 |
| test_Allocator.cpp | 10 | PoolAllocator, FrameAllocator |

---

## テストファイルの標準パターン

```cpp
/// @file test_Xxx.cpp
/// @brief Xxx 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Path/To/Header.h"

using namespace GX;

TEST(XxxTest, MethodName_Condition)
{
    // Arrange
    // Act
    // Assert
    EXPECT_NEAR(actual, expected, 1e-5f);  // 浮動小数点
    EXPECT_EQ(actual, expected);            // 整数/bool
    EXPECT_TRUE(condition);
    EXPECT_FALSE(condition);
}
```

**命名規則:**
- ファイル名: `test_<ComponentName>.cpp`
- テストスイート名: `<ComponentName>Test`
- テストケース名: `<Method>_<Condition>` （例: `Evaluate_AtStart`）
- 浮動小数点の許容誤差: `1e-5f`（特に指定がなければ）
- `std::min/std::max` は `(std::min)(...)` パターンで呼ぶこと（NOMINMAX 対応）

---

## 新規テストファイル一覧（10 ファイル）

| # | ファイル | テスト対象 | 予想テスト数 |
|---|---------|----------|------------|
| 1 | test_Spline.cpp | Spline (3 補間タイプ) | ~18 |
| 2 | test_Transform3D.cpp | Transform3D | ~10 |
| 3 | test_Entity.cpp | Entity + Scene | ~20 |
| 4 | test_SceneSerializer.cpp | SceneSerializer JSON | ~8 |
| 5 | test_ActionMapping.cpp | ActionMapping | ~8 |
| 6 | test_NavMesh.cpp | NavMesh + NavAgent | ~14 |
| 7 | test_LODGroup.cpp | LODGroup | ~8 |
| 8 | test_Animation.cpp | AnimationClip + Skeleton | ~12 |
| 9 | test_StyleSheet.cpp | StyleSheet CSS パース | ~14 |
| 10 | test_FileSystem.cpp | VFS FileSystem | ~10 |

**合計: ~122 新規テスト → 既存 151 + 新規 122 = ~273 テスト**

---

## テスト 1: test_Spline.cpp

### ヘッダ

```cpp
/// @file test_Spline.cpp
/// @brief Spline 単体テスト — Linear/CatmullRom/CubicBezier 補間、弧長パラメータ化

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Spline.h"
#include "Math/Vector3.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Math/Spline.h
実装:   GXLib/Math/Spline.cpp

enum class SplineType { Linear, CatmullRom, CubicBezier };

class Spline {
    void AddPoint(const Vector3& point);
    void InsertPoint(int index, const Vector3& point);
    void RemovePoint(int index);
    void SetPoint(int index, const Vector3& point);
    const Vector3& GetPoint(int index) const;
    int GetPointCount() const;
    void Clear();
    void SetType(SplineType type);
    SplineType GetType() const;
    void SetClosed(bool closed);
    bool IsClosed() const;
    Vector3 Evaluate(float t) const;        // t ∈ [0, 1]
    Vector3 EvaluateTangent(float t) const;
    float GetTotalLength(int subdivisions = 64) const;
    Vector3 EvaluateByDistance(float distance, int subdivisions = 64) const;
    float FindClosestParameter(const Vector3& point, int subdivisions = 64) const;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `PointManagement_AddAndGet` | AddPoint 3 点 → GetPointCount()==3, GetPoint(i) が正しい |
| 2 | `PointManagement_InsertAndRemove` | InsertPoint(1, ...) → GetPointCount() 増加、RemovePoint(1) → 元に戻る |
| 3 | `PointManagement_Clear` | Clear() → GetPointCount()==0 |
| 4 | `SetType_Changes` | SetType(Linear) → GetType()==Linear |
| 5 | `SetClosed_Changes` | SetClosed(true) → IsClosed()==true |
| 6 | `Linear_Evaluate_Start` | 2 点 Linear、t=0 → 始点 |
| 7 | `Linear_Evaluate_End` | 2 点 Linear、t=1 → 終点 |
| 8 | `Linear_Evaluate_Mid` | 2 点 (0,0,0)→(10,0,0)、t=0.5 → (5,0,0) |
| 9 | `CatmullRom_Endpoints` | 4 点 CatmullRom、t=0 ≈ 始点、t=1 ≈ 終点 |
| 10 | `CatmullRom_Smooth` | 中間点が Linear とは異なる（曲線であること） |
| 11 | `CubicBezier_Endpoints` | 4 点 CubicBezier、t=0 = p0、t=1 = p3 |
| 12 | `GetTotalLength_TwoPoints` | (0,0,0)→(10,0,0) の長さ ≈ 10.0 |
| 13 | `GetTotalLength_Empty` | 点なし → 長さ 0 |
| 14 | `EvaluateByDistance_Zero` | distance=0 → 始点 |
| 15 | `EvaluateByDistance_Full` | distance=totalLength → 終点付近 |
| 16 | `EvaluateByDistance_Monotonic` | distance 増加 → x 座標も増加（直線上） |
| 17 | `FindClosestParameter` | 制御点上の点 → t ≈ 期待値 |
| 18 | `Closed_EvaluateWraps` | SetClosed(true)、t=0 と t=1 がほぼ同じ位置 |

### 実装上の注意

- `GetPoint()` は境界チェックなし（テストではインデックス範囲内のみアクセス）
- CubicBezier は 4 の倍数の制御点が必要（4 点 = 1 セグメント）
- GetTotalLength の subdivisions=64 はデフォルトのまま使用
- 浮動小数点比較は EXPECT_NEAR(a, b, **0.1f**) 程度のゆるい許容誤差でOK
  （弧長パラメータ化は近似のため）

---

## テスト 2: test_Transform3D.cpp

### ヘッダ

```cpp
/// @file test_Transform3D.cpp
/// @brief Transform3D 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Transform3D.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Graphics/3D/Transform3D.h

class Transform3D {
    void SetPosition(float x, float y, float z);
    void SetPosition(const XMFLOAT3& pos);
    void SetRotation(float pitch, float yaw, float roll);  // radians
    void SetRotation(const XMFLOAT3& rot);
    void SetScale(float x, float y, float z);
    void SetScale(float uniform);
    void SetScale(const XMFLOAT3& s);
    const XMFLOAT3& GetPosition() const;
    const XMFLOAT3& GetRotation() const;
    const XMFLOAT3& GetScale() const;
    XMMATRIX GetWorldMatrix() const;
    XMMATRIX GetWorldInverseTranspose() const;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `DefaultState` | デフォルト: pos=(0,0,0), rot=(0,0,0), scale=(1,1,1) |
| 2 | `SetPosition_Float3` | SetPosition(1,2,3) → GetPosition()==(1,2,3) |
| 3 | `SetPosition_XMFLOAT3` | SetPosition(XMFLOAT3{4,5,6}) → GetPosition() |
| 4 | `SetRotation_Radians` | SetRotation(0.1f, 0.2f, 0.3f) → GetRotation() |
| 5 | `SetScale_Uniform` | SetScale(2.0f) → GetScale()==(2,2,2) |
| 6 | `SetScale_NonUniform` | SetScale(1,2,3) → GetScale()==(1,2,3) |
| 7 | `WorldMatrix_IdentityTransform` | デフォルト Transform → 単位行列 |
| 8 | `WorldMatrix_TranslationOnly` | pos=(5,0,0) → 行列の _41=5 |
| 9 | `WorldMatrix_ScaleOnly` | scale=(2,2,2) → 行列の対角=2 |
| 10 | `WorldInverseTranspose_Exists` | 結果が有効な行列であること（NaN なし） |

### 実装上の注意

- `GetWorldMatrix()` は XMMATRIX を返すので、XMStoreFloat4x4 で XMFLOAT4X4 に変換して検証
- 行列の要素アクセスは `._11`, `._41` 等（DirectXMath の行ベクトル規約）
- 回転はオイラー角（ラジアン）で設定、内部で SRT 順（Scale → Rotate → Translate）

---

## テスト 3: test_Entity.cpp

### ヘッダ

```cpp
/// @file test_Entity.cpp
/// @brief Entity / Scene 単体テスト — コンポーネント管理、親子階層、シーン操作

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Entity.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Core/Scene/Entity.h, Scene.h, Components.h

enum class ComponentType : uint32_t {
    Transform, MeshRenderer, SkinnedMeshRenderer, Camera,
    Light, ParticleSystem, AudioSource, Terrain,
    RigidBody, LOD, Script, Custom, _Count
};

class Entity {
    Entity(const std::string& name = "Entity");
    const std::string& GetName() const;
    void SetName(const std::string& name);
    void SetParent(Entity* parent);
    Entity* GetParent() const;
    const std::vector<Entity*>& GetChildren() const;
    Transform3D& GetTransform();
    template<typename T> T* AddComponent();
    template<typename T> T* GetComponent() const;
    template<typename T> bool HasComponent() const;
    template<typename T> void RemoveComponent();
    bool IsActive() const;
    void SetActive(bool active);
    uint32_t GetID() const;
    void SetID(uint32_t id);
};

class Scene {
    Scene(const std::string& name = "Untitled");
    Entity* CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity* entity);
    Entity* FindEntity(const std::string& name) const;
    Entity* FindEntityByID(uint32_t id) const;
    const std::vector<std::unique_ptr<Entity>>& GetEntities() const;
    const std::vector<Entity*>& GetRootEntities() const;
    void Update(float deltaTime);
    const std::string& GetName() const;
    void SetName(const std::string& name);
    uint32_t GetEntityCount() const;
};

// Components（全て Component 基底クラスを継承、static constexpr k_Type 持ち）
struct MeshRendererComponent;      // k_Type = ComponentType::MeshRenderer
struct SkinnedMeshRendererComponent; // k_Type = ComponentType::SkinnedMeshRenderer
struct CameraComponent;            // k_Type = ComponentType::Camera
struct LightComponent;             // k_Type = ComponentType::Light
struct AudioSourceComponent;       // k_Type = ComponentType::AudioSource
struct ScriptComponent;            // k_Type = ComponentType::Script
struct LODComponent;               // k_Type = ComponentType::LOD
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `Scene_CreateEntity_NotNull` | CreateEntity → non-null |
| 2 | `Scene_CreateEntity_Name` | CreateEntity("Player") → GetName()=="Player" |
| 3 | `Scene_CreateEntity_UniqueID` | 2 つの Entity の GetID() が異なる |
| 4 | `Scene_GetEntityCount` | 3 個作成 → GetEntityCount()==3 |
| 5 | `Scene_FindEntityByName` | FindEntity("Player") → 正しい Entity* |
| 6 | `Scene_FindEntityByName_NotFound` | FindEntity("NoExist") → nullptr |
| 7 | `Scene_FindEntityByID` | FindEntityByID(id) → 正しい Entity* |
| 8 | `Scene_DestroyEntity` | DestroyEntity + Update → FindEntity==nullptr |
| 9 | `Scene_RootEntities` | 親なし Entity が GetRootEntities に含まれる |
| 10 | `Scene_SetName` | SetName("MyScene") → GetName()=="MyScene" |
| 11 | `Scene_Update_NoCrash` | Update(0.016f) がクラッシュしない |
| 12 | `Entity_DefaultTransform` | GetTransform().GetPosition() == (0,0,0) |
| 13 | `Entity_AddComponent_Camera` | AddComponent<CameraComponent>() → non-null |
| 14 | `Entity_GetComponent_Camera` | AddComponent 後に GetComponent → 同じポインタ |
| 15 | `Entity_GetComponent_NotAdded` | GetComponent<LightComponent>() → nullptr |
| 16 | `Entity_HasComponent` | AddComponent 後 HasComponent==true |
| 17 | `Entity_RemoveComponent` | RemoveComponent 後 HasComponent==false |
| 18 | `Entity_SetParent` | e2.SetParent(&e1) → e2.GetParent()==&e1, e1.GetChildren() に e2 |
| 19 | `Entity_RemoveParent` | SetParent(nullptr) → GetParent()==nullptr |
| 20 | `Entity_ActiveState` | SetActive(false) → IsActive()==false |

### 実装上の注意

- **Entity は Scene 経由で作成**（Scene が unique_ptr で所有）
- DestroyEntity は遅延削除 → Update() 呼び出し後にチェック
- `MeshRendererComponent` の model は nullptr のままテスト
- `ScriptComponent` は onUpdate/onStart/onDestroy に std::function を設定可能
  → コールバックの呼び出し確認テストも可能だが、基本テストでは省略
- Entity のコンストラクタは直接使用せず、Scene::CreateEntity() 経由が安全

---

## テスト 4: test_SceneSerializer.cpp

### ヘッダ

```cpp
/// @file test_SceneSerializer.cpp
/// @brief SceneSerializer JSON ラウンドトリップテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Scene.h"
#include "Core/Scene/SceneSerializer.h"
#include "Core/Scene/Components.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Core/Scene/SceneSerializer.h

class SceneSerializer {
    using ModelLoadCallback = std::function<Model*(const std::string& path)>;
    static bool SaveToJson(const Scene& scene, const std::string& filePath);
    static bool LoadFromJson(Scene& scene, const std::string& filePath,
                             ModelLoadCallback modelLoader = nullptr);
    static std::string ToJsonString(const Scene& scene);
    static bool FromJsonString(Scene& scene, const std::string& json,
                               ModelLoadCallback modelLoader = nullptr);
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `ToJsonString_EmptyScene` | 空シーン → 有効な JSON 文字列（`{` で始まる） |
| 2 | `RoundTrip_EmptyScene` | ToJsonString → FromJsonString → エンティティ数 0 |
| 3 | `RoundTrip_SingleEntity` | 名前 "Box" の Entity → 復元後に名前一致 |
| 4 | `RoundTrip_EntityTransform` | Position(1,2,3) 設定 → 復元後に Position 一致 |
| 5 | `RoundTrip_Hierarchy` | 親子関係 → 復元後に GetParent が一致 |
| 6 | `RoundTrip_CameraComponent` | CameraComponent 付き → 復元後に HasComponent |
| 7 | `RoundTrip_LightComponent` | LightComponent 付き → 復元後に HasComponent |
| 8 | `FromJsonString_InvalidJson` | 不正 JSON → false を返す |

### 実装上の注意

- ファイル I/O テスト (SaveToJson/LoadFromJson) は一時ファイルを使用するか、
  文字列ベースの ToJsonString/FromJsonString のみでテスト（推奨）
- ModelLoadCallback には nullptr を渡す（モデルの読み込みテストは不要）
- シリアライズ対象外のコンポーネント（LOD, Terrain 等）はテストしない
- JSON フォーマットは nlohmann/json（GXLib/ThirdParty/json.hpp）
- Transform の復元精度は EXPECT_NEAR(..., 1e-3f) で検証

---

## テスト 5: test_ActionMapping.cpp

### ヘッダ

```cpp
/// @file test_ActionMapping.cpp
/// @brief ActionMapping 構造テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Input/ActionMapping.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Input/ActionMapping.h

enum class InputBindingType { KeyboardKey, MouseButton, GamepadButton, GamepadAxis, MouseAxis };

struct InputBinding {
    InputBindingType type;
    int keyCode; float deadZone; float scale; int padIndex;
    static InputBinding Key(int vk);
    static InputBinding MouseBtn(int btn);
    static InputBinding PadBtn(int btn, int pad = 0);
    static InputBinding PadAxis(GamepadAxisId, float, float, int);
    static InputBinding KeyAxis(int vk, float s);
};

struct ActionState {
    bool pressed, triggered, released;
    float value;
};

class ActionMapping {
    void DefineAction(const std::string& name, const std::vector<InputBinding>& bindings);
    void RemoveAction(const std::string& name);
    const ActionState& GetAction(const std::string& name) const;
    bool IsActionPressed(const std::string& name) const;
    bool IsActionTriggered(const std::string& name) const;
    bool IsActionReleased(const std::string& name) const;
    float GetActionValue(const std::string& name) const;
    void Clear();
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `DefineAction_Exists` | DefineAction → GetAction が空でない ActionState を返す |
| 2 | `GetActionValue_Undefined` | 未定義アクション → 0.0f |
| 3 | `IsActionPressed_Undefined` | 未定義アクション → false |
| 4 | `RemoveAction_Removes` | RemoveAction → GetActionValue == 0.0f |
| 5 | `Clear_RemovesAll` | 3 つ定義 → Clear → 全て 0.0f |
| 6 | `KeyBinding_Factory` | InputBinding::Key(VK_SPACE) → type==KeyboardKey, keyCode==VK_SPACE |
| 7 | `KeyAxisBinding_Factory` | InputBinding::KeyAxis('W', 1.0f) → type==KeyboardKey, scale==1.0f |
| 8 | `PadBtnBinding_Factory` | InputBinding::PadBtn(0, 1) → type==GamepadButton, padIndex==1 |

### 実装上の注意

- `Update()` には Keyboard, Mouse, Gamepad の実インスタンスが必要なのでテスト不可
  → 構造テスト（定義/削除/初期値）に限定
- `s_emptyState` が GetAction() の未定義アクション用フォールバック
- InputBinding のファクトリメソッドは純粋な値構築なので完全にテスト可能

---

## テスト 6: test_NavMesh.cpp

### ヘッダ

```cpp
/// @file test_NavMesh.cpp
/// @brief NavMesh A* パス検索 + NavAgent テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/AI/NavMesh.h, NavAgent.h

class NavMesh {
    bool Build(float minX, float minZ, float maxX, float maxZ,
               float cellSize = 0.5f, float maxClimb = 0.9f, float maxSlope = 45.0f);
    void SetCellWalkable(int cellX, int cellZ, bool walkable);
    void SetCellCost(int cellX, int cellZ, float cost);
    bool FindPath(const XMFLOAT3& start, const XMFLOAT3& end,
                  std::vector<XMFLOAT3>& outPath) const;
    bool FindNearestWalkable(const XMFLOAT3& pos, XMFLOAT3& outPos) const;
    bool IsWalkable(const XMFLOAT3& pos) const;
    int GetGridWidth() const;
    int GetGridHeight() const;
    float GetCellSize() const;
    bool IsBuilt() const;
};

class NavAgent {
    void Initialize(NavMesh* mesh);
    void SetDestination(const XMFLOAT3& dest);
    void Update(float dt);
    void Stop();
    XMFLOAT3 GetPosition() const;
    void SetPosition(const XMFLOAT3& pos);
    float GetYaw() const;
    bool HasPath() const;
    bool HasReachedDestination() const;
    const std::vector<XMFLOAT3>& GetPath() const;
    int GetCurrentWaypointIndex() const;
    // Public members:
    float speed = 3.5f;
    float angularSpeed = 360.0f;
    float stoppingDistance = 0.15f;
    float height = 0.0f;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `Build_FlatGrid` | Build(0,0, 10,10, 0.5) → IsBuilt()==true |
| 2 | `Build_GridDimensions` | GetGridWidth()/GetGridHeight() が正しい |
| 3 | `IsWalkable_Default` | Build 直後 → 全セルが walkable |
| 4 | `SetCellWalkable_False` | SetCellWalkable(5,5,false) → IsWalkable({2.5,0,2.5})==false |
| 5 | `FindPath_StraightLine` | 障害物なし → パス発見、waypoints > 0 |
| 6 | `FindPath_AroundObstacle` | 中央に壁 → パスが壁を迂回 |
| 7 | `FindPath_NoPath` | 目的地を壁で完全包囲 → false |
| 8 | `FindPath_SamePoint` | start==end → パスが空 or 1 点 |
| 9 | `FindNearestWalkable` | 非 walkable 位置 → 近隣の walkable 位置を返す |
| 10 | `NavAgent_Initialize` | Initialize → HasPath()==false, HasReachedDestination()==false |
| 11 | `NavAgent_SetDestination` | SetDestination → HasPath()==true |
| 12 | `NavAgent_Update_Moves` | Update(1.0f) → GetPosition() が変化 |
| 13 | `NavAgent_HasReached` | 目的地近くに配置 → Update 後に HasReachedDestination()==true |
| 14 | `NavAgent_Stop` | Stop() → HasPath()==false |

### 実装上の注意

- Build の座標系: minX, minZ, maxX, maxZ（Y は高さ、パス検索では XZ 平面）
- セルインデックス: `cellX = (worldX - minX) / cellSize`
- NavAgent のテストでは Build 後の NavMesh を使用
- NavAgent::Update は deltaTime でスケールされる（dt=1.0f で speed=3.5 ユニット移動）
- NavAgent::HasReachedDestination は stoppingDistance(0.15f) 以内で true
- 「目的地近くに配置」テスト: SetPosition を目的地の stoppingDistance 以内に設定

---

## テスト 7: test_LODGroup.cpp

### ヘッダ

```cpp
/// @file test_LODGroup.cpp
/// @brief LODGroup LOD 選択ロジックテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/LODGroup.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Transform3D.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Graphics/3D/LODGroup.h

struct LODLevel {
    Model* model = nullptr;
    float screenPercentage = 1.0f;
};

class LODGroup {
    void AddLevel(Model* model, float screenPercentage);
    int GetLevelCount() const;
    const LODLevel& GetLevel(int index) const;
    void Clear();
    Model* SelectLOD(const Camera3D& camera, const Transform3D& transform,
                     float boundingRadius) const;
    void SetCullDistance(float distance);
    float GetCullDistance() const;
    // static constexpr float k_Hysteresis = 0.05f;
    // mutable int m_lastSelectedLevel = 0;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `Empty_ReturnsNull` | LOD なし → SelectLOD==nullptr |
| 2 | `AddLevel_Count` | 3 レベル追加 → GetLevelCount()==3 |
| 3 | `AddLevel_SortedByScreenPct` | 順不同で追加 → GetLevel(0).screenPercentage が最大 |
| 4 | `Clear_RemovesAll` | Clear → GetLevelCount()==0 |
| 5 | `SelectLOD_Close` | カメラ近距離 → LOD 0 (高詳細) の Model* |
| 6 | `SelectLOD_Far` | カメラ遠距離 → 最低 LOD の Model* |
| 7 | `SelectLOD_CullDistance` | SetCullDistance(50), カメラ 100m → nullptr |
| 8 | `SelectLOD_NoCullDistance` | SetCullDistance(0) → カリングなし |

### 実装上の注意

- **Model* は nullptr で OK**（SelectLOD 内で Model を deref しない、ただ返すだけ）
- テスト用に区別できるダミーポインタを使用:
  ```cpp
  Model* dummyLOD0 = reinterpret_cast<Model*>(0x1);
  Model* dummyLOD1 = reinterpret_cast<Model*>(0x2);
  Model* dummyLOD2 = reinterpret_cast<Model*>(0x3);
  ```
- Camera3D は SetPerspective() と SetPosition() で初期化が必要
- SelectLOD 内部で `camera.GetPosition()` と `camera.GetFovY()` を使用
- Camera3D の初期化:
  ```cpp
  Camera3D camera;
  camera.SetPerspective(XM_PIDIV4, 16.0f/9.0f, 0.1f, 1000.0f);
  camera.SetPosition(0, 0, -10);  // カメラ位置
  ```
- Transform3D で対象オブジェクトの位置を設定
- boundingRadius は正の float（例: 1.0f）

---

## テスト 8: test_Animation.cpp

### ヘッダ

```cpp
/// @file test_Animation.cpp
/// @brief AnimationClip / Skeleton / TransformTRS テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Animation.h"
#include "Graphics/3D/Skeleton.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Graphics/3D/Animation.h (AnimationClip.h), Skeleton.h

struct TransformTRS {
    XMFLOAT3 translation = {0,0,0};
    XMFLOAT4 rotation = {0,0,0,1};  // quaternion
    XMFLOAT3 scale = {1,1,1};
};

TransformTRS IdentityTRS();
TransformTRS DecomposeTRS(const XMFLOAT4X4& mat);
XMFLOAT4X4 ComposeTRS(const TransformTRS& trs);

struct AnimationChannel {
    int jointIndex = -1;
    std::vector<Keyframe<XMFLOAT3>> translationKeys;
    std::vector<Keyframe<XMFLOAT4>> rotationKeys;
    std::vector<Keyframe<XMFLOAT3>> scaleKeys;
    InterpolationType interpolation = InterpolationType::Linear;
};

class AnimationClip {
    void SetName(const std::string& name);
    const std::string& GetName() const;
    void SetDuration(float duration);
    float GetDuration() const;
    void AddChannel(const AnimationChannel& channel);
    const std::vector<AnimationChannel>& GetChannels() const;
    void SampleTRS(float time, uint32_t jointCount, TransformTRS* outPose,
                   const TransformTRS* basePose = nullptr) const;
    void Sample(float time, uint32_t jointCount, XMFLOAT4X4* outLocalTransforms) const;
};

struct Joint {
    std::string name;
    int parentIndex = -1;
    XMFLOAT4X4 inverseBindMatrix;
    XMFLOAT4X4 localTransform;
};

class Skeleton {
    void AddJoint(const Joint& joint);
    const std::vector<Joint>& GetJoints() const;
    uint32_t GetJointCount() const;
    int FindJointIndex(const std::string& name) const;
    void ComputeGlobalTransforms(const XMFLOAT4X4* local, XMFLOAT4X4* global) const;
    void ComputeBoneMatrices(const XMFLOAT4X4* global, XMFLOAT4X4* bone) const;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `IdentityTRS_Values` | IdentityTRS() → translation(0,0,0), rotation(0,0,0,1), scale(1,1,1) |
| 2 | `ComposeTRS_Identity` | ComposeTRS(IdentityTRS()) ≈ 単位行列 |
| 3 | `ComposeTRS_Translation` | translation(5,0,0) → 行列の _41=5 |
| 4 | `DecomposeTRS_RoundTrip` | ComposeTRS → DecomposeTRS → 元の TRS と一致 |
| 5 | `AnimationClip_NameAndDuration` | SetName/SetDuration → GetName/GetDuration |
| 6 | `AnimationClip_AddChannel` | AddChannel → GetChannels().size()==1 |
| 7 | `AnimationClip_SampleTRS_Static` | 1 キーフレーム → 任意の time で同じポーズ |
| 8 | `AnimationClip_SampleTRS_Interpolated` | 2 キー (t=0, t=1) → t=0.5 で中間値 |
| 9 | `Skeleton_AddJoint` | AddJoint 2 個 → GetJointCount()==2 |
| 10 | `Skeleton_FindJointIndex` | FindJointIndex("Root") → 0 |
| 11 | `Skeleton_FindJointIndex_NotFound` | FindJointIndex("NoExist") → -1 |
| 12 | `Skeleton_ComputeGlobalTransforms` | 2 ジョイント (parent→child) → child のグローバル行列が親×自身 |

### テストデータ作成ヘルパー

```cpp
// 2ジョイント Skeleton 作成ヘルパー
Skeleton CreateTestSkeleton()
{
    Skeleton skel;
    Joint root;
    root.name = "Root";
    root.parentIndex = -1;
    XMFLOAT4X4 identity;
    XMStoreFloat4x4(&identity, XMMatrixIdentity());
    root.inverseBindMatrix = identity;
    root.localTransform = identity;
    skel.AddJoint(root);

    Joint child;
    child.name = "Child";
    child.parentIndex = 0;
    child.inverseBindMatrix = identity;
    XMStoreFloat4x4(&child.localTransform, XMMatrixTranslation(0, 1, 0));
    skel.AddJoint(child);

    return skel;
}

// 単純なアニメーションクリップ作成ヘルパー
AnimationClip CreateTestClip(float duration = 1.0f)
{
    AnimationClip clip;
    clip.SetName("TestClip");
    clip.SetDuration(duration);

    AnimationChannel ch;
    ch.jointIndex = 0;
    ch.translationKeys.push_back({0.0f, {0, 0, 0}});
    ch.translationKeys.push_back({duration, {1, 0, 0}});
    ch.rotationKeys.push_back({0.0f, {0, 0, 0, 1}});
    ch.rotationKeys.push_back({duration, {0, 0, 0, 1}});
    ch.scaleKeys.push_back({0.0f, {1, 1, 1}});
    ch.scaleKeys.push_back({duration, {1, 1, 1}});
    clip.AddChannel(ch);

    return clip;
}
```

### 実装上の注意

- `Animation.h` は実際のファイル名が `AnimationClip.h` の可能性あり → 確認して正しいヘッダを include
  (`GXLib/Graphics/3D/Animation.h` または `GXLib/Graphics/3D/AnimationClip.h`)
- XMFLOAT4X4 の行列比較は各要素を EXPECT_NEAR で比較
- SampleTRS は basePose に nullptr を渡すと IdentityTRS がベースになる
- Joint の inverseBindMatrix は単位行列で OK（テスト用）

---

## テスト 9: test_StyleSheet.cpp

### ヘッダ

```cpp
/// @file test_StyleSheet.cpp
/// @brief StyleSheet CSS パースと StyleLength/StyleColor テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/StyleSheet.h"
#include "GUI/Style.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/GUI/Style.h, GXLib/GUI/StyleSheet.h

struct StyleLength {
    float value; SizeUnit unit;
    static StyleLength Px(float v);
    static StyleLength Pct(float v);
    static StyleLength Auto();
    bool IsAuto() const;
    float Resolve(float parentSize) const;
};

struct StyleColor {
    float r, g, b, a;
    StyleColor(float r, float g, float b, float a = 1.0f);
    static StyleColor FromHex(const std::string& hex);
    bool IsTransparent() const;
};

struct StyleEdges {
    float top, right, bottom, left;
    StyleEdges(float all);
    StyleEdges(float v, float h);
    StyleEdges(float t, float r, float b, float l);
    float HorizontalTotal() const;
    float VerticalTotal() const;
};

struct StyleSelector {
    static StyleSelector Parse(const std::string& str);
    int Specificity() const;
};

class StyleSheet {
    bool LoadFromString(const std::string& source);
    size_t GetRuleCount() const;
    static std::string NormalizePropertyName(const std::string& name);
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `StyleLength_Px` | StyleLength::Px(100) → value==100, unit==Px |
| 2 | `StyleLength_Pct` | StyleLength::Pct(50) → unit==Percent |
| 3 | `StyleLength_Auto` | StyleLength::Auto() → IsAuto()==true |
| 4 | `StyleLength_Resolve_Px` | Px(100).Resolve(500) == 100 |
| 5 | `StyleLength_Resolve_Pct` | Pct(50).Resolve(200) == 100 |
| 6 | `StyleColor_FromHex_RGB` | FromHex("#FF0000") → r≈1.0, g≈0, b≈0 |
| 7 | `StyleColor_FromHex_RGBA` | FromHex("#FF000080") → a≈0.5 |
| 8 | `StyleColor_IsTransparent` | StyleColor(0,0,0,0).IsTransparent()==true |
| 9 | `StyleEdges_AllSame` | StyleEdges(10) → top==right==bottom==left==10 |
| 10 | `StyleEdges_Totals` | StyleEdges(5,10,15,20) → HorizontalTotal==30, VerticalTotal==20 |
| 11 | `StyleSelector_Parse_Type` | Parse("Button") → specificity > 0 |
| 12 | `StyleSelector_Parse_Class` | Parse(".highlight") → specificity == 10 |
| 13 | `StyleSelector_Parse_ID` | Parse("#main") → specificity == 100 |
| 14 | `StyleSheet_LoadFromString` | 有効な CSS → LoadFromString==true, GetRuleCount() > 0 |

### テスト用 CSS 文字列

```cpp
const char* testCSS = R"(
    Button {
        background-color: #333333;
        color: #FFFFFF;
        font-size: 16px;
        padding: 8px 12px;
    }
    .highlight {
        color: #FFCC00;
    }
    #main {
        width: 100%;
        height: 50px;
    }
)";
```

### 実装上の注意

- StyleColor::FromHex は `#RRGGBB` と `#RRGGBBAA` の 2 形式をサポート
- NormalizePropertyName: `background-color` → `backgroundColor`（camelCase 変換）
- StyleSelector::Parse は Widget* のマッチングに使うが、テストでは Specificity のみ検証
- LoadFromString は内部でトークナイザーを使用（依存なし）

---

## テスト 10: test_FileSystem.cpp

### ヘッダ

```cpp
/// @file test_FileSystem.cpp
/// @brief VFS FileSystem テスト（モック IFileProvider 使用）

#include "pch.h"
#include <gtest/gtest.h>
#include "IO/FileSystem.h"

using namespace GX;
```

### モック IFileProvider

```cpp
/// テスト用のインメモリファイルプロバイダ
class MockFileProvider : public IFileProvider
{
public:
    std::unordered_map<std::string, std::vector<uint8_t>> files;
    int m_priority = 0;

    MockFileProvider(int priority = 0) : m_priority(priority) {}

    void AddFile(const std::string& path, const std::string& content)
    {
        files[path] = std::vector<uint8_t>(content.begin(), content.end());
    }

    bool Exists(const std::string& path) const override
    {
        return files.count(path) > 0;
    }

    FileData Read(const std::string& path) const override
    {
        FileData fd;
        auto it = files.find(path);
        if (it != files.end()) fd.data = it->second;
        return fd;
    }

    bool Write(const std::string& path, const void* data, size_t size) override
    {
        files[path] = std::vector<uint8_t>(
            static_cast<const uint8_t*>(data),
            static_cast<const uint8_t*>(data) + size);
        return true;
    }

    int Priority() const override { return m_priority; }
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `FileData_Empty` | デフォルト FileData → IsValid()==false |
| 2 | `FileData_AsString` | データあり → AsString() が正しい文字列 |
| 3 | `Mount_Exists` | Mount + AddFile → Exists==true |
| 4 | `Mount_ReadFile` | Mount + AddFile → ReadFile().AsString() が正しい |
| 5 | `Mount_WriteFile` | WriteFile → ReadFile で書いた内容が読める |
| 6 | `Mount_NotFound` | マウントなし → Exists==false |
| 7 | `Mount_MountPoint` | "assets/" にマウント → "assets/test.txt" が見つかる |
| 8 | `Priority_HigherWins` | 2 プロバイダ (priority 0, 10) → 高優先の内容が返る |
| 9 | `Unmount_Removes` | Unmount → Exists==false |
| 10 | `Clear_RemovesAll` | Clear → 全ファイル Exists==false |

### 実装上の注意

- **FileSystem はシングルトン** → テスト間で状態が残る問題がある
  → 各テストの SetUp/TearDown で `FileSystem::Instance().Clear()` を呼ぶ
  → テストフィクスチャ `TEST_F` を使用:
  ```cpp
  class FileSystemTest : public ::testing::Test {
  protected:
      void SetUp() override { FileSystem::Instance().Clear(); }
      void TearDown() override { FileSystem::Instance().Clear(); }
  };
  ```
- MockFileProvider は `std::make_shared<MockFileProvider>()` で作成
- パス正規化: バックスラッシュ → スラッシュ、先頭スラッシュ除去
- `pch.h` に `<unordered_map>` があるので MockFileProvider で使用可能

---

## CMakeLists.txt 更新

`Tests/CMakeLists.txt` の `TEST_SOURCES` リストに新規ファイルを追加:

```cmake
set(TEST_SOURCES
    test_main.cpp
    test_Vector.cpp
    test_Matrix.cpp
    test_Quaternion.cpp
    test_Color.cpp
    test_MathUtil.cpp
    test_Collision2D.cpp
    test_Collision3D.cpp
    test_Spatial.cpp
    test_Crypto.cpp
    test_Allocator.cpp
    # Phase 37 新規
    test_Spline.cpp
    test_Transform3D.cpp
    test_Entity.cpp
    test_SceneSerializer.cpp
    test_ActionMapping.cpp
    test_NavMesh.cpp
    test_LODGroup.cpp
    test_Animation.cpp
    test_StyleSheet.cpp
    test_FileSystem.cpp
)
```

---

## 実行手順

### Step 1: CMakeLists.txt 更新
`Tests/CMakeLists.txt` の TEST_SOURCES に 10 ファイルを追加（上記参照）

### Step 2: テストファイル作成（推奨順序）
以下の順に作成し、各ファイル作成後にビルド検証:

1. `test_Transform3D.cpp` — 最も単純、依存なし
2. `test_Spline.cpp` — Vector3 のみ依存
3. `test_StyleSheet.cpp` — GUI スタイル、依存なし
4. `test_FileSystem.cpp` — VFS モック、依存なし
5. `test_ActionMapping.cpp` — Input 構造テスト
6. `test_Animation.cpp` — AnimationClip + Skeleton
7. `test_Entity.cpp` — Scene/Component 統合
8. `test_SceneSerializer.cpp` — JSON ラウンドトリップ
9. `test_NavMesh.cpp` — A* パス検索
10. `test_LODGroup.cpp` — Camera3D 依存

### Step 3: ビルド & テスト

```bash
cmake -B build -S .
cmake --build build --config Debug
ctest --test-dir build --build-config Debug --output-on-failure
```

### Step 4: 全テスト PASS 確認

目標: **260+ テスト全パス**（既存 151 + 新規 ~110-120）

---

## トラブルシューティング

### Q: `#include "Graphics/3D/Animation.h"` が見つからない
実際のファイル名を確認。`AnimationClip.h` の可能性がある。
```bash
ls GXLib/Graphics/3D/Anim*
```

### Q: `std::unordered_map` が使えない
`pch.h` に `<unordered_map>` は含まれている。`<map>` は含まれていないので注意。

### Q: FileSystem シングルトンのテスト間干渉
TEST_F フィクスチャの SetUp/TearDown で `FileSystem::Instance().Clear()` を必ず呼ぶ。

### Q: Camera3D の初期化
SetPerspective() を呼ばないと FOV が 0 になり SelectLOD が正常に動作しない。
```cpp
Camera3D camera;
camera.SetPerspective(XM_PIDIV4, 16.0f/9.0f, 0.1f, 1000.0f);
camera.SetPosition(0, 0, -10);
```

### Q: `(std::min)` / `(std::max)` パターン
Windows.h の min/max マクロと競合するため、括弧で囲む:
```cpp
int clamped = (std::min)(val, maxVal);
```

### Q: NavMesh の Build 座標系
Build(minX, minZ, maxX, maxZ, cellSize) — Y は高さ方向でパス検索には使わない。
XMFLOAT3 の y=0 でテスト。

### Q: Entity は Scene 経由で作成する
Entity を直接 new しない。Scene::CreateEntity() で作成すると ID が自動割り当てされる。

### Q: AnimationClip のヘッダ名
実際のヘッダファイル名を Glob で確認:
```
GXLib/Graphics/3D/Animation.h  (AnimationClip クラスを含む可能性)
```

### Q: コンパイルエラー「ComponentType undeclared」
`#include "Core/Scene/Component.h"` を追加するか、
`Components.h` が Component.h を include しているか確認。

---

## 完了条件

1. 10 個の新規テストファイルが `Tests/` に存在する
2. `Tests/CMakeLists.txt` に全ファイルが登録されている
3. `cmake --build build --config Debug` がエラーなし
4. `ctest` で **全テスト PASS**（0 failures）
5. 新規テスト数が **100 以上**

## Phase 41-45 Directive

## Context

Phase 0-40 完了済み。エンジンは完全な 2D/3D 描画パイプライン、DXR レイトレーシング（反射＋GI）、
15+ ポストエフェクト、GUI（17 ウィジェット + CSS + XML）、物理(2D + Jolt 3D)、
オーディオ(XAudio2 + 3D空間音響)、シーングラフ/ECS、パーティクル(CPU 2D/3D + GPU Compute)、
ナビメッシュ(Grid A*)、LOD、デカール、IBL、IK(FABRIK + FootIK + LookAtIK)、
GPU インスタンシング、アクションマッピング、Lua スクリプティング(sol2)、2D タイルマップ(TMX)、
アニメーション(BlendStack/BlendTree/AnimatorStateMachine/RootMotion/AnimationEvent)、
マルチスレッドレンダリング(ParallelCommandRecorder)、Async Compute、間接描画、
GXModelViewer（ImGui Docking + 19 パネル）、アセットパイプライン（gxformat/gxconv/gxloader/gxpak）、
25 サンプルプロジェクト、273 ユニットテストを持つ成熟状態。

本指令書は **テスト拡充・不足サンプル追加・エディタ強化・パフォーマンス最適化・先進レンダリング** の
5 フェーズを網羅し、任意の Claude インスタンスが独立して各 Phase を実装できることを目的とする。

---

## 全体ロードマップ

| Phase | 名称 | 概要 | 依存 | 推定規模 |
|-------|------|------|------|----------|
| **41** | テスト拡充 | GPU不要モジュールの単体テスト大幅追加（273→450+テスト目標） | なし | 大 |
| **42** | 不足サンプル追加 | 未デモ機能の専用サンプル 8〜12 本新規作成 | なし | 大 |
| **43** | GXModelViewer シーンエディタ強化 | Undo/Redo、エンティティ生成、コンポーネント拡張、シミュレーション | Phase 41 推奨 | 大 |
| **44** | パフォーマンス最適化 | フラスタムカリング改善、リソースバリア最適化、描画ソート | なし | 中 |
| **45** | 先進レンダリング | Volumetric Clouds、Hi-Z Occlusion Culling、Virtual Texture Streaming | Phase 44 推奨 | 大 |

Phase 41/42 は並列着手可能。Phase 43 は Phase 41 完了後に着手推奨（テストで安定性確保後にエディタ変更）。
Phase 44/45 は独立して着手可能だが、Phase 44 → 45 の順序を推奨。

---

## 共通規約

### ビルド手順
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 新規ファイル追加時
- `GXLib/` 配下の `.cpp` は `GLOB_RECURSE` で自動収集されるが、新しいサブディレクトリを作った場合は `GXLib/CMakeLists.txt` に `file(GLOB_RECURSE ...)` を追加
- テストファイルは `Tests/CMakeLists.txt` に手動追加
- サンプルは `CMakeLists.txt` のルートに `gxlib_add_sample(NAME ...)` で追加
- **追加後は必ず `cmake -B build -S .` を再実行**

### コーディング規約
- C++20、名前空間 `GX`
- PCH: `pch.h`（`<sstream>` `<unordered_set>` は含まれない — 代替を使用）
- CD3DX12 ヘルパー不使用 — raw D3D12 構造体
- `Color{1, 1, 1, 1}` は曖昧 → `Color{1.0f, 1.0f, 1.0f, 1.0f}` を使用
- `std::min/max` は `(std::min)(a, b)` パターン（Windows.h マクロ回避）
- HLSL: `line` は予約語 → `gridLine` 等に変更
- `saturate()` は HLSL 専用 → C++ では `std::max(0.0f, std::min(1.0f, x))`

### テスト実行
```bash
cmake --build build --config Debug --target GXLib_Tests
cd build && ctest -C Debug --output-on-failure
```

### MEMORY.md 更新
各 Phase 完了後、`C:\Users\g0190\.claude\projects\C--Users-g0190-Desktop-GXLib\memory\MEMORY.md` の
"Completed Phases" セクションに Phase 概要を追記すること。

---

## Phase 41: テスト拡充

### 目的
現在 273 テスト / 20 テストファイルのカバレッジを大幅に拡張する。
GPU 不要で単体テスト可能な 30+ モジュールに対してテストを追加し、**450+ テスト / 35+ テストファイル** を目標とする。

### 現状分析

#### テスト済みモジュール (20ファイル / 273テスト)
| ファイル | テスト数 | カバー範囲 |
|----------|---------|-----------|
| test_Vector.cpp | 29 | Vector2/3/4 |
| test_Collision3D.cpp | 24 | AABB, Sphere, Ray, Frustum 等 |
| test_Collision2D.cpp | 19 | Circle, Rect, Line 等 |
| test_MathUtil.cpp | 19 | Lerp, Clamp, SmoothStep, Random |
| test_Spline.cpp | 18 | Linear/CatmullRom/CubicBezier |
| test_Entity.cpp | 20 | Entity, Scene, Components |
| test_StyleSheet.cpp | 14 | CSS パース、セレクタ、ルール |
| test_NavMesh.cpp | 14 | A*探索、障害物、NavAgent |
| test_Spatial.cpp | 13 | Quadtree/Octree/BVH |
| test_Animation.cpp | 12 | AnimationClip サンプリング、TRS 分解 |
| test_Matrix.cpp | 11 | 変換、逆行列、行列式 |
| test_Quaternion.cpp | 10 | Slerp, Euler, AxisAngle |
| test_Transform3D.cpp | 10 | Position/Rotation/Scale/行列 |
| test_Color.cpp | 10 | 色フォーマット、HSV、Lerp |
| test_Allocator.cpp | 10 | PoolAllocator, FrameAllocator |
| test_FileSystem.cpp | 10 | VFS, マウント、優先度 |
| test_SceneSerializer.cpp | 8 | JSON ラウンドトリップ |
| test_LODGroup.cpp | 8 | LOD選択、スクリーン占有率 |
| test_ActionMapping.cpp | 8 | バインディング、アクション、状態 |
| test_Crypto.cpp | 6 | AES-256, SHA-256, ランダム |

#### 未テストだがGPU不要でテスト可能なモジュール (優先度順)

**Priority 1 — 高価値・低労力（各 8-15 テスト想定）:**

| モジュール | ヘッダ | テスト可能な内容 |
|-----------|--------|-----------------|
| Camera2D | Graphics/Rendering/Camera2D.h | ビューポート変換、ズーム、スクロール、位置計算 |
| Camera3D | Graphics/3D/Camera3D.h | Perspective/Ortho設定、LookAt、Pitch/Yaw、ビュー行列 |
| Transform2D | Math/Transform2D.h | 位置、回転、スケール、行列合成 |
| AnimatorStateMachine | Graphics/3D/AnimatorStateMachine.h | 状態遷移、トリガー、パラメータ、条件評価 |
| BlendStack | Graphics/3D/BlendStack.h | レイヤー追加/削除、Override/Additive、ウェイト |
| BlendTree | Graphics/3D/BlendTree.h | 1D/2Dブレンド、パラメータ評価 |
| Tilemap | Graphics/Rendering/Tilemap.h | グリッド作成、タイル設定、レイヤー管理、座標変換 |
| XMLParser | GUI/XMLParser.h | XML パース、属性取得、子要素走査 |

**Priority 2 — 中価値・中労力（各 6-12 テスト想定）:**

| モジュール | ヘッダ | テスト可能な内容 |
|-----------|--------|-----------------|
| Timer | Core/Timer.h | フレーム時間、デルタ、FPS計算 |
| Logger | Core/Logger.h | メッセージフォーマット、レベルフィルタ |
| Material | Graphics/3D/Material.h | パラメータ設定、ShaderModel、デフォルト値 |
| Light | Graphics/3D/Light.h | CreateDirectional/Point/Spot、パラメータ検証 |
| Fog | Graphics/3D/Fog.h | Linear/Exponential パラメータ |
| IKSolver | Graphics/3D/IKSolver.h | FABRIK 反復、チェーン評価 |
| LookAtIK | Graphics/3D/LookAtIK.h | ターゲット追従、制約角度 |
| LayerStack | Graphics/Layer/LayerStack.h | レイヤー順序、追加/削除/検索 |
| Random | Math/Random.h | 分布、シード、方向ベクトル生成 |
| ParticleEmitter2D | Graphics/Rendering/ParticleEmitter2D.h | エミッション設定、パラメータ検証 |

**Priority 3 — 統合テスト（各 5-10 テスト想定）:**

| テスト対象 | 内容 |
|-----------|------|
| Scene + Components 統合 | 全コンポーネントタイプの追加/取得/削除ラウンドトリップ |
| AnimationClip + Events | イベントコールバック発火、ループ境界ハンドリング |
| AnimatorStateMachine + BlendStack | 状態遷移 → ブレンドレイヤー変化の整合性 |
| NavMesh + NavAgent 統合 | 複雑地形でのパス追従、障害物回避 |
| FileSystem + Archive | VFS マウント → Archive 読み込みラウンドトリップ |

---

### 41a: Camera2D テスト

**ファイル:** `Tests/test_Camera2D.cpp`

```cpp
// テスト項目 (10テスト想定):
// Camera2D_DefaultPosition         - 初期位置が (0,0)
// Camera2D_SetPosition             - SetPosition() で位置変更
// Camera2D_SetZoom                 - SetZoom() でズーム倍率変更
// Camera2D_ZoomClamp               - ズーム範囲のクランプ (最小/最大)
// Camera2D_GetPositionXY           - GetPositionX()/GetPositionY() 個別取得
// Camera2D_ViewportTransform       - ワールド→スクリーン座標変換
// Camera2D_InverseTransform        - スクリーン→ワールド座標変換
// Camera2D_VisibleRegion           - 可視領域の計算（ズーム考慮）
// Camera2D_SmoothFollow            - 追従対象への補間移動
// Camera2D_Shake                   - カメラシェイク（オフセット適用）
```

**実装手順:**
1. `Camera2D.h` を読み、公開 API を確認
2. GPU依存のメソッド（描画系）は除外し、座標変換・状態管理のみテスト
3. Google Test (`TEST()`) で各項目を実装
4. `Tests/CMakeLists.txt` に `test_Camera2D.cpp` を追加

---

### 41b: Camera3D テスト

**ファイル:** `Tests/test_Camera3D.cpp`

```cpp
// テスト項目 (12テスト想定):
// Camera3D_DefaultValues           - 初期状態の検証
// Camera3D_SetPerspective          - FOV/Aspect/Near/Far 設定
// Camera3D_SetOrthographic         - 正射影設定
// Camera3D_SetPosition             - 位置設定と取得
// Camera3D_Rotate                  - Pitch/Yaw 回転
// Camera3D_LookAt                  - LookAt で pitch/yaw 自動計算
// Camera3D_GetViewMatrix           - ビュー行列の正当性
// Camera3D_GetProjectionMatrix     - 射影行列の正当性
// Camera3D_GetViewProjection       - VP行列の合成
// Camera3D_Forward                 - 前方ベクトルの計算
// Camera3D_Right                   - 右方ベクトルの計算
// Camera3D_Up                      - 上方ベクトルの計算
```

**注意点:**
- Camera3D は DirectXMath の `XMFLOAT3`, `XMFLOAT4X4` を使用
- テストでは `EXPECT_NEAR` で浮動小数点比較（許容誤差 1e-4f）
- `Camera3D.h` の `#include` が GPU 型を含まないことを確認。含む場合はテスト対象を限定

---

### 41c: Transform2D テスト

**ファイル:** `Tests/test_Transform2D.cpp`

```cpp
// テスト項目 (8テスト想定):
// Transform2D_DefaultValues        - 位置(0,0), 回転0, スケール(1,1)
// Transform2D_SetPosition          - 位置設定/取得
// Transform2D_SetRotation          - 回転設定/取得（ラジアン）
// Transform2D_SetScale             - スケール設定/取得
// Transform2D_GetMatrix            - ワールド行列の計算
// Transform2D_ComposeTransforms    - 親子変換の合成
// Transform2D_RotateAround         - 任意点回りの回転
// Transform2D_Forward              - 前方ベクトル（回転角度ベース）
```

---

### 41d: AnimatorStateMachine テスト

**ファイル:** `Tests/test_AnimatorStateMachine.cpp`

**前提:** AnimatorStateMachine.h を読み、以下の API を確認:
- `AddState(name)`, `AddTransition(from, to, conditions)`, `SetTrigger(name)`, `SetFloat(name, value)`
- `Update(dt)`, `GetCurrentState()`, `GetTransitions()`

```cpp
// テスト項目 (15テスト想定):
// ASM_AddState                     - 状態追加と存在確認
// ASM_AddTransition                - 遷移追加（条件付き）
// ASM_SetInitialState              - 初期状態の設定
// ASM_GetCurrentState              - 現在状態の取得
// ASM_TriggerTransition            - トリガーによる状態遷移
// ASM_FloatCondition               - float パラメータ条件での遷移
// ASM_BoolCondition                - bool パラメータ条件での遷移
// ASM_TransitionDuration           - 遷移時間の検証（ブレンド中の状態）
// ASM_NoTransitionWithoutCondition - 条件未達時に遷移しないことを確認
// ASM_MultipleTransitions          - 同一状態から複数遷移先への優先度
// ASM_SelfTransition               - 自己遷移の動作
// ASM_TransitionChain              - A→B→C の連鎖遷移
// ASM_GetTransitions               - GetTransitions() の正当性
// ASM_ResetTrigger                 - トリガーが1回の遷移後にリセットされること
// ASM_InvalidState                 - 存在しない状態への遷移を試みた場合のエラーハンドリング
```

**重要:**
- AnimatorStateMachine は Skeleton/AnimationClip に依存する可能性あり
- テスト用にダミーの Skeleton（2-3 ボーン）と AnimationClip（2-3 キーフレーム）を作成するヘルパー関数を用意
- GPU リソース不要で状態遷移ロジックのみテスト可能かを `AnimatorStateMachine.h` で確認

---

### 41e: BlendStack / BlendTree テスト

**ファイル:** `Tests/test_BlendSystem.cpp`

```cpp
// BlendStack テスト (8テスト想定):
// BlendStack_AddLayer              - レイヤー追加（最大8）
// BlendStack_RemoveLayer           - レイヤー削除
// BlendStack_OverrideMode          - Override モードでベースクリップを上書き
// BlendStack_AdditiveMode          - Additive モードでベースに加算
// BlendStack_WeightBlending        - ウェイト 0.0-1.0 での補間
// BlendStack_MaxLayers             - 8レイヤー上限の検証
// BlendStack_LayerOrder            - レイヤー優先度（後勝ち）
// BlendStack_ClearAll              - 全レイヤークリア

// BlendTree テスト (8テスト想定):
// BlendTree_1DBlend                - 1Dパラメータでの2クリップブレンド
// BlendTree_1DThresholds           - 閾値境界でのブレンド比率
// BlendTree_1DExtrapolation        - パラメータが範囲外の場合
// BlendTree_2DBlend                - 2Dパラメータ (X,Y) でのブレンド
// BlendTree_2DNearestSample        - 2D最近傍サンプルの重み
// BlendTree_SetParameter           - パラメータ変更とブレンド結果の更新
// BlendTree_EmptyTree              - クリップ未設定時の動作
// BlendTree_SingleClip             - 1クリップのみ時のパススルー
```

**注意:**
- BlendStack は `AnimBlendMode`（NOT `BlendMode`）を使用 — SpriteBatch.h の `BlendMode` と名前衝突回避
- テスト用ダミークリップを共有ヘルパーで生成

---

### 41f: Tilemap テスト

**ファイル:** `Tests/test_Tilemap.cpp`

```cpp
// テスト項目 (10テスト想定):
// Tilemap_Create                   - Create(width, height, tileW, tileH) 正常動作
// Tilemap_AddLayer                 - レイヤー追加と取得
// Tilemap_GetTile                  - タイルID取得（座標指定）
// Tilemap_SetTile                  - タイルID設定
// Tilemap_OutOfBounds              - 範囲外座標でのアクセス（安全性）
// Tilemap_MultipleLayer            - 複数レイヤーの独立性
// Tilemap_WorldToTile              - ワールド座標→タイル座標変換
// Tilemap_TileToWorld              - タイル座標→ワールド座標変換
// Tilemap_GetDimensions            - 幅・高さ・タイルサイズの取得
// Tilemap_CollisionLayer           - コリジョンレイヤーの判定
```

---

### 41g: XMLParser テスト

**ファイル:** `Tests/test_XMLParser.cpp`

```cpp
// テスト項目 (10テスト想定):
// XMLParser_ParseEmpty             - 空文字列のパース
// XMLParser_ParseSingleElement     - <tag/> 単一要素
// XMLParser_ParseAttributes        - <tag attr="value"/> 属性取得
// XMLParser_ParseNested            - <parent><child/></parent> 入れ子
// XMLParser_ParseText              - <tag>テキスト</tag> テキスト内容
// XMLParser_ParseMultipleChildren  - 複数子要素
// XMLParser_GetAttribute           - 存在/非存在属性の取得
// XMLParser_ParseWithNamespace     - 名前空間付き要素
// XMLParser_InvalidXML             - 不正XMLのエラーハンドリング
// XMLParser_ParseGUILayout         - GUI レイアウト XML の実践的パース
```

---

### 41h: IKSolver / LookAtIK テスト

**ファイル:** `Tests/test_IK.cpp`

```cpp
// IKSolver テスト (8テスト想定):
// IKSolver_Create                  - ソルバー生成
// IKSolver_TwoBoneReach            - 2ボーンチェーンで到達可能なターゲット
// IKSolver_TwoBoneUnreachable      - 到達不能ターゲット（最大伸展）
// IKSolver_PoleVector              - ポールベクトルで肘/膝の方向制御
// IKSolver_MultiBoneChain          - 3ボーン以上のチェーン
// IKSolver_ConvergenceIterations   - 反復回数による収束精度
// IKSolver_ZeroLength              - ボーン長0のエッジケース
// IKSolver_TargetAtRoot            - ターゲットがルート位置の場合

// LookAtIK テスト (6テスト想定):
// LookAtIK_LookForward             - 正面ターゲットへの回転
// LookAtIK_LookBehind              - 背面ターゲット（制約角度内）
// LookAtIK_ClampAngle              - 最大回転角度のクランプ
// LookAtIK_Weight                  - ウェイト0-1の補間
// LookAtIK_UpVector                - アップベクトルの影響
// LookAtIK_SmoothTracking          - スムース追従（補間速度）
```

**注意:**
- IKSolver が Skeleton 依存かを確認。依存する場合はダミー Skeleton を生成
- LookAtIK は Transform3D ベースの回転計算がメイン — GPU不要

---

### 41i: その他の単体テスト

**ファイル:** `Tests/test_Material.cpp`
```cpp
// Material テスト (6テスト想定):
// Material_DefaultValues           - デフォルトのアルベド、ラフネス、メタリック
// Material_SetShaderModel          - ShaderModel 変更
// Material_ShaderParams            - ShaderModelParams のバイトレイアウト確認
// Material_ToonParams              - Toon UTS2 パラメータの設定と取得
// Material_Clone                   - マテリアルのコピー
// Material_OverrideFlag            - useMaterialOverride フラグ
```

**ファイル:** `Tests/test_Light.cpp`
```cpp
// Light テスト (8テスト想定):
// Light_CreateDirectional          - 方向ライト作成
// Light_CreatePoint                - ポイントライト作成
// Light_CreateSpot                 - スポットライト作成
// Light_DirectionalDirection       - 方向の正規化確認
// Light_PointRange                 - 範囲パラメータ
// Light_SpotAngle                  - 内角/外角パラメータ
// Light_Intensity                  - 強度設定
// Light_Color                      - 色設定
```

**ファイル:** `Tests/test_LayerStack.cpp`
```cpp
// LayerStack テスト (8テスト想定):
// LayerStack_AddLayer              - レイヤー追加
// LayerStack_RemoveLayer           - レイヤー削除
// LayerStack_GetByIndex            - インデックスで取得
// LayerStack_GetByName             - 名前で取得
// LayerStack_LayerOrder            - 描画順序の保証
// LayerStack_Count                 - レイヤー数
// LayerStack_Clear                 - 全クリア
// LayerStack_DuplicateName         - 重複名の処理
```

---

### 41j: Tests/CMakeLists.txt 更新

以下のファイルを追加:
```cmake
# Phase 41 新規テストファイル
set(TEST_SOURCES
    # ... 既存のテスト ...
    Tests/test_Camera2D.cpp
    Tests/test_Camera3D.cpp
    Tests/test_Transform2D.cpp
    Tests/test_AnimatorStateMachine.cpp
    Tests/test_BlendSystem.cpp
    Tests/test_Tilemap.cpp
    Tests/test_XMLParser.cpp
    Tests/test_IK.cpp
    Tests/test_Material.cpp
    Tests/test_Light.cpp
    Tests/test_LayerStack.cpp
)
```

### 41k: テスト共通ヘルパー

**ファイル:** `Tests/TestHelpers.h`

テスト全体で共有するヘルパー関数:
```cpp
#pragma once
#include <DirectXMath.h>
#include <cmath>

namespace TestHelper {

// 浮動小数点比較マクロ
constexpr float k_Epsilon = 1e-4f;

// ダミー AnimationClip 生成（3キーフレーム、線形補間）
// ダミー Skeleton 生成（2-3ボーン）
// XMFLOAT3 比較ヘルパー
inline bool NearEqual(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float eps = k_Epsilon)
{
    return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps && std::abs(a.z - b.z) < eps;
}

} // namespace TestHelper
```

### 完了基準
- [ ] 全新規テストが `ctest -C Debug` でパス
- [ ] 既存 273 テストがリグレッションなし
- [ ] テスト総数 450 以上
- [ ] Tests/CMakeLists.txt 更新済み

---

## Phase 42: 不足サンプル追加

### 目的
エンジン機能のうちデモが存在しない 17 の主要機能に対し、専用サンプルプロジェクトを作成する。
ユーザーが各機能の使い方を学べる実践的なコード例を提供する。

### 現状分析

#### サンプルが存在する機能 (25サンプル)
EasyHello, Shooting2D, Platformer2D, Walkthrough3D, GUIMenuDemo, PostEffectShowcase,
DXRShowcase, Particle2DShowcase, ParticleShowcase, IBLShowcase, InstanceShowcase,
IKShowcase, Audio3DShowcase, ActionMappingShowcase, GPUParticleShowcase,
NavmeshShowcase, SceneShowcase, TrailShowcase, SplineShowcase, WireframeShowcase,
TextLayoutShowcase, ProfilerShowcase, MultiThreadShowcase, LuaShowcase, TilemapShowcase

#### サンプルが存在しない主要機能
| 機能 | 重要度 | 実装済みAPI |
|------|--------|-----------|
| AnimatorStateMachine + BlendStack/Tree | Tier 1 | AnimatorStateMachine, BlendStack, BlendTree, AnimationEvent, RootMotion |
| Physics2D | Tier 1 | PhysicsWorld2D, RigidBody2D |
| Physics3D (Jolt) | Tier 1 | PhysicsWorld3D, RigidBody3D, PhysicsShape, MeshCollider |
| RenderLayer / MaskScreen | Tier 2 | RenderLayer, LayerStack, LayerCompositor, MaskScreen |
| AudioBus / AudioMixer | Tier 2 | AudioMixer, AudioBus |
| Decal | Tier 2 | Decal (ボックス投影 Deferred) |
| Terrain | Tier 2 | Terrain (ハイトフィールド) |
| LODGroup | Tier 2 | LODGroup (スクリーン占有率) |
| ShaderHotReload | Tier 3 | ShaderLibrary, ShaderHotReload |
| Network (TCP/UDP/WebSocket) | Tier 3 | TCPSocket, UDPSocket, HTTPClient, WebSocket |
| MoviePlayer | Tier 3 | MoviePlayer (Media Foundation) |
| FileSystem / Archive | Tier 3 | FileSystem, Archive, ArchiveFileProvider, PakFileProvider |
| PointShadowMap | Tier 3 | PointShadowMap |
| Skeletal Animation (.gxmd + .gxan) | Tier 3 | GxmdModelLoader, AnimationClip, Animator |
| Camera2D (dedicated) | Tier 3 | Camera2D (ズーム、追従) |
| Collision Debug Viz | Tier 3 | Collision2D/3D + PrimitiveBatch3D |

### 必須実装サンプル（Tier 1-2: 8本）

---

#### 42a: AnimationBlendingShowcase

**目的:** AnimatorStateMachine + BlendStack + BlendTree + AnimationEvent + RootMotion の統合デモ

**ファイル:** `Samples/AnimationBlendingShowcase/main.cpp`

**仕様:**
- .gxmd スキンドモデルを読み込み（アセットが無い場合はプロシージャル生成の簡易スキンドメッシュ）
- AnimatorStateMachine で Idle → Walk → Run → Jump の 4 状態を定義
- BlendStack で上半身/下半身の 2 レイヤーブレンド
- BlendTree 1D で Walk/Run のスピードパラメータブレンド
- AnimationEvent でフットステップタイミングにエフェクト発火
- RootMotion で移動デルタを Transform に適用
- HUD に現在の状態、ブレンドウェイト、RootMotion デルタを表示

**コントロール:**
- WASD: 移動パラメータ変更（BlendTree パラメータ）
- Space: ジャンプ（トリガー発火）
- 1-4: 状態直接切り替え
- Tab: RootMotion ON/OFF 切り替え
- ESC: 終了

**実装手順:**
1. `Samples/AnimationBlendingShowcase/main.cpp` 作成
2. `CMakeLists.txt` に `gxlib_add_sample(NAME AnimationBlendingShowcase)` 追加
3. GXEasy::App を継承、3D シーン + HUD 構成
4. ダミーアニメーションクリップを手動生成（アセットが無い場合）:
   - Idle: T-pose 微動（呼吸モーション）
   - Walk: 2秒ループ、ルートモーション XZ オフセット付き
   - Run: 1秒ループ、より大きいルートモーション
   - Jump: 0.5秒ワンショット
5. AnimatorStateMachine 設定:
   ```cpp
   auto& sm = animator.GetStateMachine();
   sm.AddState("Idle");
   sm.AddState("Walk");
   sm.AddState("Run");
   sm.AddState("Jump");
   sm.AddTransition("Idle", "Walk", /* speed > 0.1 */);
   sm.AddTransition("Walk", "Run", /* speed > 0.7 */);
   sm.AddTransition("Walk", "Idle", /* speed < 0.1 */);
   sm.AddTransition("Run", "Walk", /* speed < 0.7 */);
   sm.AddTransition("*", "Jump", /* trigger: "jump" */);
   sm.AddTransition("Jump", "Idle", /* exitTime */);
   ```
6. BlendStack 設定:
   ```cpp
   blendStack.AddLayer("Base", AnimBlendMode::Override, 1.0f);
   blendStack.AddLayer("UpperBody", AnimBlendMode::Additive, 0.5f);
   ```
7. AnimationEvent 設定:
   ```cpp
   walkClip.AddEvent(0.25f, "footstep_left");
   walkClip.AddEvent(0.75f, "footstep_right");
   animator.SetEventCallback([](const std::string& name) {
       // エフェクト発火 or サウンド再生
   });
   ```

**注意:**
- `.gxmd`/`.gxan` アセットが利用可能であればそれを使用
- 利用不可の場合、手動でキーフレームを設定した AnimationClip を生成
- 3Dシーンのため Renderer3D + PostEffectPipeline 必須
- HDR パイプライン必須（R16G16B16A16_FLOAT）

---

#### 42b: Physics2DShowcase

**目的:** PhysicsWorld2D + RigidBody2D の物理シミュレーションデモ

**ファイル:** `Samples/Physics2DShowcase/main.cpp`

**仕様:**
- 画面下部に地面（静的ボディ）
- クリックで動的ボックスを生成（重力で落下）
- 複数ボックスが衝突して積み重なる
- 摩擦と反発係数のリアルタイムスライダー
- PrimitiveBatch で物理ボディの可視化

**コントロール:**
- LMB: ボックス生成
- RMB: 円形ボディ生成
- R: リセット（全ボディ削除）
- ↑/↓: 重力の強さ調整
- 1-3: 反発係数切り替え (0.0 / 0.5 / 0.9)
- ESC: 終了

**実装手順:**
1. `PhysicsWorld2D` と `RigidBody2D` の API を確認
2. 静的地面 + 壁をボディとして配置
3. マウスクリック位置に動的ボディを追加
4. `Update()` で `PhysicsWorld2D::Step(dt)` を呼び出し
5. `Draw()` で各ボディの位置/回転を取得し `PrimitiveBatch` で描画
6. HUD にボディ数、FPS、物理パラメータ表示

---

#### 42c: Physics3DShowcase

**目的:** PhysicsWorld3D (Jolt) の 3D 物理シミュレーションデモ

**ファイル:** `Samples/Physics3DShowcase/main.cpp`

**仕様:**
- 地面（静的メッシュ）
- 球・ボックス・カプセルの動的ボディを生成
- ドミノ倒しのプリセットシーン
- ラグドール風の連結ボディ（constraint）
- PrimitiveBatch3D でコリジョン形状のワイヤフレーム表示

**コントロール:**
- LMB: 球を射出（カメラ前方）
- 1: ドミノプリセット
- 2: ボックスタワープリセット
- 3: ラグドールプリセット
- R: リセット
- F: カメラ追従モード切り替え
- ESC: 終了

**実装手順:**
1. `PhysicsWorld3D.h` と `PhysicsShape.h` の API を確認
2. Jolt 初期化 → PhysicsWorld3D::Initialize()
3. 地面: 静的 PhysicsShape::Box (100x1x100)
4. ドミノ: 20個の薄いボックスを等間隔配置
5. `Update()` で `PhysicsWorld3D::Step(dt)` → 各ボディの Transform 取得
6. `Draw()` で Renderer3D + PrimitiveBatch3D（ワイヤフレーム）

**注意:**
- PhysicsWorld3D は Jolt Physics 依存 — ビルド時にリンク済み
- `PhysicsWorld3D.cpp` を読み、Step/CreateBody/GetTransform の API を確認

---

#### 42d: RenderLayerShowcase

**目的:** RenderLayer + LayerStack + LayerCompositor + MaskScreen のデモ

**ファイル:** `Samples/RenderLayerShowcase/main.cpp`

**仕様:**
- 3つのレンダーレイヤー: 背景(Sky)、メインシーン(3D)、UI(2D)
- MaskScreen で円形マスクを適用してメインシーンをマスキング
- LayerCompositor で合成順序を制御
- レイヤーの表示/非表示をトグル

**コントロール:**
- 1/2/3: 各レイヤーの表示/非表示トグル
- M: マスクの ON/OFF
- ↑/↓: マスク半径の調整
- WASD: マスク位置の移動
- ESC: 終了

**実装手順:**
1. `RenderLayer.h`, `LayerStack.h`, `LayerCompositor.h`, `MaskScreen.h` の API 確認
2. LayerStack に 3 レイヤーを追加（描画順序指定）
3. 各レイヤーに描画内容を設定:
   - Layer 0 (Sky): Skybox or グラデーション背景
   - Layer 1 (Scene): 3D オブジェクト
   - Layer 2 (UI): 2D テキスト/図形
4. MaskScreen で Layer 1 にマスク適用
5. LayerCompositor で最終合成

---

#### 42e: DecalShowcase

**目的:** Decal システム（ボックス投影 Deferred Decal）のデモ

**ファイル:** `Samples/DecalShowcase/main.cpp`

**仕様:**
- 3D シーン（地面 + 壁 + キューブ複数）
- クリック位置にデカール（弾痕/ペイント）を配置
- デカールが曲面にも正しく投影されることを示す
- 複数デカールの重ね描画
- デカールのフェードアウト（生存時間）

**コントロール:**
- LMB: デカール配置（カメラレイキャスト）
- 1/2/3: デカールテクスチャ切り替え（弾痕/血痕/ペイント）
- R: 全デカール削除
- ESC: 終了

**注意:**
- Decal は front-face culling（裏面描画）、depth write OFF
- 深度バッファからワールド位置を再構築
- テクスチャが必要 — 手動でプロシージャル生成 or SoftImage で白丸テクスチャを動的生成

---

#### 42f: TerrainShowcase

**目的:** Terrain ハイトフィールドレンダリングのデモ

**ファイル:** `Samples/TerrainShowcase/main.cpp`

**仕様:**
- プロシージャルハイトマップ生成（Perlin ノイズ風 sin/cos 合成）
- Terrain の描画（ライティング + テクスチャ）
- Camera3D でフライスルー
- 高度に応じた色分け（水/草/岩/雪）
- LODGroup でカメラ距離に応じたメッシュ解像度切り替え

**コントロール:**
- WASD: カメラ移動
- マウス: カメラ回転
- ↑/↓: 地形の起伏度調整
- L: LOD 可視化（ワイヤフレームで LOD レベル色分け）
- ESC: 終了

---

#### 42g: LODShowcase

**目的:** LODGroup のスクリーン占有率ベース LOD 切り替えデモ

**ファイル:** `Samples/LODShowcase/main.cpp`

**仕様:**
- 同一オブジェクトの 3 LOD レベル（High/Mid/Low ポリゴン）
- オブジェクトを遠近に配置（格子状 or 一列）
- カメラの距離で LOD が自動切り替わるのを可視化
- LOD レベルを色で表示（赤=High, 黄=Mid, 青=Low）
- HUD にポリゴン数、LOD レベル分布を表示

**コントロール:**
- WASD + マウス: カメラ移動
- H: ヒステリシスバンドの ON/OFF
- 1-3: LOD 強制切り替え
- Tab: LOD 可視化色の ON/OFF
- ESC: 終了

**実装手順:**
1. `LODGroup.h` の API 確認: `AddLOD(mesh, screenPercentage)`, `SelectLOD(camera, transform)`
2. 3段階のメッシュを MeshGenerator で生成:
   - LOD 0: CreateSphere(1.0, 32, 32) — 高ポリ
   - LOD 1: CreateSphere(1.0, 16, 16) — 中ポリ
   - LOD 2: CreateSphere(1.0, 8, 8) — 低ポリ
3. LODGroup に登録:
   ```cpp
   lodGroup.AddLOD(meshHigh, 0.3f);   // 30%以上で高ポリ
   lodGroup.AddLOD(meshMid,  0.1f);   // 10%以上で中ポリ
   lodGroup.AddLOD(meshLow,  0.0f);   // それ以下で低ポリ
   ```
4. 格子状にオブジェクトを配置、Update で LOD 選択、Draw で描画

---

#### 42h: AudioMixerShowcase

**目的:** AudioBus + AudioMixer の多トラックミキシングデモ

**ファイル:** `Samples/AudioMixerShowcase/main.cpp`

**仕様:**
- 3つのオーディオバス: BGM, SFX, Voice
- 各バスの音量を個別制御
- マスターボリューム
- バスごとのミュート/ソロ
- サイン波生成でデモサウンド（外部ファイル不要）
- HUD にバスレベルメーター表示

**コントロール:**
- 1/2/3: 各バスのミュート切り替え
- ↑/↓: 選択バスの音量調整
- Tab: バス選択切り替え
- M: マスターミュート
- ESC: 終了

---

### 42i-42l: 追加サンプル（Tier 3, 余力があれば実装）

#### 42i: ShaderHotReloadShowcase
- シェーダーファイルを動的に変更 → 自動再コンパイル → リアルタイム反映
- `ShaderLibrary` + `ShaderHotReload` + `FileWatcher` の連携

#### 42j: PointShadowShowcase
- ポイントライトの全方位シャドウマップ
- キューブマップ6面レンダリングの可視化

#### 42k: SkeletalAnimShowcase
- `.gxmd` + `.gxan` を読み込みスキンドメッシュアニメーション再生
- Animator の Play/Pause/Speed 制御

#### 42l: CollisionDebugShowcase
- Collision2D/3D のデバッグ可視化
- Quadtree/Octree/BVH のノード表示
- レイキャストの可視化

### CMakeLists.txt 更新

```cmake
# Phase 42 新規サンプル (Tier 1-2)
gxlib_add_sample(NAME AnimationBlendingShowcase)
gxlib_add_sample(NAME Physics2DShowcase)
gxlib_add_sample(NAME Physics3DShowcase)
gxlib_add_sample(NAME RenderLayerShowcase)
gxlib_add_sample(NAME DecalShowcase)
gxlib_add_sample(NAME TerrainShowcase)
gxlib_add_sample(NAME LODShowcase)
gxlib_add_sample(NAME AudioMixerShowcase)

# Phase 42 追加サンプル (Tier 3, optional)
# gxlib_add_sample(NAME ShaderHotReloadShowcase)
# gxlib_add_sample(NAME PointShadowShowcase)
# gxlib_add_sample(NAME SkeletalAnimShowcase)
# gxlib_add_sample(NAME CollisionDebugShowcase)
```

### 完了基準
- [ ] Tier 1-2 の 8 サンプルがビルド・実行可能
- [ ] 各サンプルにファイル先頭コメント（目的、コントロール説明）
- [ ] CMakeLists.txt 更新済み
- [ ] 既存サンプルのビルドにリグレッションなし

---

## Phase 43: GXModelViewer シーンエディタ強化

### 目的
GXModelViewer を「モデルビューア」から「シーンエディタ」に進化させる。
インポートのみのエンティティ管理から、作成・編集・シミュレーションが可能な統合ツールへ。

### 現状分析

**現在できること (Phase 32完了時点):**
- モデルインポート (FBX/glTF/OBJ/GXMD) + ドラッグ&ドロップ
- エンティティ選択/削除/リネーム
- Transform 編集 (ImGuizmo: 移動/回転/スケール)
- マテリアルオーバーライド + シェーダーモデルパラメータ編集
- アニメーション再生 (Timeline, AnimatorStateMachine 可視化)
- ライティング (16灯, Directional/Point/Spot)
- ポストエフェクト (13種パラメータ制御)
- シーン保存/読み込み (JSON)
- 19 パネル体制

**現在できないこと (=今回のスコープ):**
- 空エンティティの作成
- プリミティブ形状の追加
- Undo/Redo
- ドラッグ&ドロップでの親子関係変更
- エンティティ複製
- マルチセレクト
- コンポーネントの動的追加/削除
- 物理シミュレーション プレビュー
- テクスチャのマテリアルへの割り当て UI

### ソースファイル構成

```
GXModelViewer/
├── main.cpp                        // WinMain
├── GXModelViewerApp.h/cpp          // メインアプリ (1934行)
├── ModelExporter.h/cpp             // GXMD/GXAN エクスポート
├── InfiniteGrid.h/cpp              // グリッド描画
├── Panels/ (19パネル)
│   ├── SceneHierarchyPanel.h/cpp
│   ├── PropertyPanel.h/cpp
│   ├── LightingPanel.h/cpp
│   ├── PostEffectPanel.h/cpp
│   ├── TimelinePanel.h/cpp
│   ├── AnimatorPanel.h/cpp
│   ├── BlendTreeEditor.h/cpp
│   ├── SkyboxPanel.h/cpp
│   ├── TerrainPanel.h/cpp
│   ├── PerformancePanel.h/cpp
│   ├── LogPanel.h/cpp
│   ├── ModelInfoPanel.h/cpp
│   ├── SkeletonPanel.h/cpp
│   ├── TextureBrowser.h/cpp
│   ├── AssetBrowserPanel.h/cpp
│   ├── IBLPanel.h/cpp
│   ├── ParticlePanel.h/cpp
│   ├── IKPanel.h/cpp
│   └── AudioPanel.h/cpp
└── Scene/
    ├── EditorMetadata.h
    ├── SceneGraph.h/cpp            // フラットエンティティ管理
    └── SceneSerializer.h/cpp       // JSON保存/読み込み
```

---

### 43a: Undo/Redo システム

**目的:** コマンドパターンによる操作の取り消し/やり直し

**新規ファイル:**
- `GXModelViewer/UndoSystem.h`
- `GXModelViewer/UndoSystem.cpp`

**設計:**
```cpp
// 抽象コマンドインターフェース
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

// Undoスタック管理
class UndoSystem {
public:
    void Execute(std::unique_ptr<ICommand> cmd);  // 実行+スタックにプッシュ
    void Undo();                                   // 最後のコマンドを取り消し
    void Redo();                                   // 最後のUndoをやり直し
    bool CanUndo() const;
    bool CanRedo() const;
    const std::string& GetUndoDescription() const;
    const std::string& GetRedoDescription() const;
    void Clear();

private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;
    static constexpr int k_MaxUndoLevels = 100;
};
```

**具体コマンド（優先度順）:**

```cpp
// 1. Transform 変更
class TransformCommand : public ICommand {
    int m_entityIndex;
    Transform3D m_oldTransform, m_newTransform;
    // Execute: entity.transform = m_newTransform
    // Undo: entity.transform = m_oldTransform
};

// 2. エンティティ追加/削除
class AddEntityCommand : public ICommand { /* ... */ };
class RemoveEntityCommand : public ICommand { /* ... */ };

// 3. マテリアル変更
class MaterialChangeCommand : public ICommand { /* ... */ };

// 4. 親子関係変更
class ReparentCommand : public ICommand { /* ... */ };

// 5. リネーム
class RenameCommand : public ICommand { /* ... */ };
```

**統合ポイント:**
- ImGuizmo のドラッグ終了時に `TransformCommand` を発行
- SceneHierarchyPanel の操作に対応する Command を発行
- PropertyPanel の変更に対応する Command を発行
- **ショートカット:** Ctrl+Z = Undo, Ctrl+Y = Redo

**注意:**
- GXModelViewerApp のメンバーに `UndoSystem m_undoSystem` を追加
- ImGuizmo の `ImGuizmo::IsUsing()` が `false` になった時点で Transform の差分をコマンド化
- Undo 時に GPU リソースの整合性を保証（モデル削除の Undo ではモデルを再読み込み）

---

### 43b: エンティティ生成 & プリミティブ追加

**目的:** インポートなしで空エンティティやプリミティブ形状を追加

**変更ファイル:**
- `GXModelViewer/Panels/SceneHierarchyPanel.cpp` — メニュー追加
- `GXModelViewer/GXModelViewerApp.cpp` — 生成ロジック

**追加メニュー項目（SceneHierarchyPanel のコンテキストメニュー）:**
```
右クリック → 追加
  ├── 空のエンティティ
  ├── ──────────────
  ├── キューブ
  ├── スフィア
  ├── プレーン
  ├── カプセル
  ├── シリンダー
  └── コーン
```

**プリミティブ生成:**
```cpp
void GXModelViewerApp::CreatePrimitive(const std::string& name, PrimitiveType type)
{
    int idx = m_sceneGraph.AddEntity(name);
    auto* entity = m_sceneGraph.GetEntity(idx);

    GX::MeshData meshData;
    switch (type) {
        case PrimitiveType::Cube:    meshData = GX::MeshGenerator::CreateBox(1, 1, 1); break;
        case PrimitiveType::Sphere:  meshData = GX::MeshGenerator::CreateSphere(0.5f, 32, 32); break;
        case PrimitiveType::Plane:   meshData = GX::MeshGenerator::CreatePlane(10, 10, 1, 1); break;
        case PrimitiveType::Capsule: meshData = GX::MeshGenerator::CreateCapsule(0.5f, 1.0f, 32); break;
        // ...
    }

    entity->ownedModel = std::make_unique<GX::Model>();
    entity->ownedModel->CreateFromMeshData(m_renderer, meshData);
    entity->model = entity->ownedModel.get();

    m_undoSystem.Execute(std::make_unique<AddEntityCommand>(/* ... */));
}
```

**注意:**
- `MeshGenerator` の API を確認 — `CreateBox`, `CreateSphere`, `CreatePlane` が存在するか
- 存在しない場合は `MeshData` を直接構築するヘルパーを作成
- 生成されたプリミティブのデフォルトマテリアル: 灰色 PBR (albedo=0.5, roughness=0.5, metallic=0.0)

---

### 43c: エンティティ複製 & マルチセレクト

**目的:** エンティティの複製とマルチセレクトで効率的なシーン構築

**複製 (Ctrl+D):**
```cpp
void GXModelViewerApp::DuplicateEntity(int sourceIdx)
{
    auto* src = m_sceneGraph.GetEntity(sourceIdx);
    if (!src) return;

    int newIdx = m_sceneGraph.AddEntity(src->name + " (Copy)");
    auto* dst = m_sceneGraph.GetEntity(newIdx);

    dst->transform = src->transform;
    dst->transform.SetPosition(
        src->transform.GetPosition().x + 1.0f,
        src->transform.GetPosition().y,
        src->transform.GetPosition().z
    );
    dst->materialOverride = src->materialOverride;
    dst->useMaterialOverride = src->useMaterialOverride;

    if (src->ownedModel) {
        // モデルの共有参照（メモリ節約、同じメッシュを指す）
        dst->model = src->model;
    }

    m_undoSystem.Execute(std::make_unique<AddEntityCommand>(/* ... */));
}
```

**マルチセレクト:**
- `SceneGraph` に `std::vector<int> selectedEntities` を追加（単一 `selectedEntity` を置換）
- Ctrl+クリック: 選択に追加/解除
- Shift+クリック: 範囲選択
- PropertyPanel: マルチセレクト時は共通プロパティのみ表示
- ImGuizmo: マルチセレクト時は中心位置にギズモ表示、全選択エンティティに変換適用

---

### 43d: ドラッグ&ドロップ親子関係変更

**目的:** SceneHierarchyPanel でドラッグ&ドロップによる親子関係変更

**変更ファイル:** `GXModelViewer/Panels/SceneHierarchyPanel.cpp`

**実装:**
```cpp
// ImGui のドラッグ&ドロップ API を使用
if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
{
    ImGui::SetDragDropPayload("ENTITY_INDEX", &entityIndex, sizeof(int));
    ImGui::Text("Move: %s", entity->name.c_str());
    ImGui::EndDragDropSource();
}

if (ImGui::BeginDragDropTarget())
{
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_INDEX"))
    {
        int draggedIdx = *(const int*)payload->Data;
        // 循環参照チェック: draggedIdx が targetIdx の祖先でないことを確認
        if (!IsAncestor(draggedIdx, targetIdx))
        {
            m_undoSystem.Execute(std::make_unique<ReparentCommand>(
                draggedIdx, oldParent, targetIdx));
        }
    }
    ImGui::EndDragDropTarget();
}
```

**循環参照防止:**
```cpp
bool SceneGraph::IsAncestor(int candidateIdx, int entityIdx) const
{
    int current = entityIdx;
    while (current >= 0) {
        if (current == candidateIdx) return true;
        current = m_entities[current].parentIndex;
    }
    return false;
}
```

---

### 43e: テクスチャ割り当て UI

**目的:** PropertyPanel でマテリアルのテクスチャスロットにテクスチャを割り当て可能にする

**変更ファイル:** `GXModelViewer/Panels/PropertyPanel.cpp`

**UI:**
```
Material Override
├── Albedo Map:     [albedo.png] [Browse...] [Clear]
├── Normal Map:     [none]       [Browse...] [Clear]
├── Roughness Map:  [none]       [Browse...] [Clear]
├── Metallic Map:   [none]       [Browse...] [Clear]
└── Emissive Map:   [none]       [Browse...] [Clear]
```

**実装:**
- テクスチャスロットごとに `ImGuiFileDialog` でファイル選択
- `TextureManager::LoadTexture(path)` でテクスチャをロード
- マテリアルの対応するテクスチャハンドルを更新
- テクスチャブラウザからのドラッグ&ドロップも対応

---

### 43f: シミュレーションモード

**目的:** シーン全体のシミュレーション（物理+アニメーション+スクリプト）をプレビュー

**UI:**
```
[▶ Play] [⏸ Pause] [⏹ Stop]    Simulation Mode: ON
```

**設計:**
- Play: 全エンティティのスナップショットを保存 → PhysicsWorld3D 初期化 → シミュレーション開始
- Pause: シミュレーションを一時停止
- Stop: スナップショットから全エンティティを復元
- シミュレーション中は Transform 編集を無効化

**状態管理:**
```cpp
enum class SimulationState { Editing, Playing, Paused };

struct SceneSnapshot {
    std::vector<Transform3D> transforms;
    std::vector<std::string> names;
    // ... 必要なエンティティ状態
};

class SimulationManager {
    SimulationState m_state = SimulationState::Editing;
    SceneSnapshot m_snapshot;

    void Play(SceneGraph& scene);   // スナップショット保存+開始
    void Pause();
    void Stop(SceneGraph& scene);   // スナップショット復元
    void Update(float dt, SceneGraph& scene);  // 物理+アニメ更新
};
```

---

### 完了基準
- [ ] Ctrl+Z / Ctrl+Y で Undo/Redo 動作
- [ ] 空エンティティ & プリミティブ（6種）の追加
- [ ] Ctrl+D でエンティティ複製
- [ ] ドラッグ&ドロップで親子関係変更
- [ ] テクスチャ割り当て UI 動作
- [ ] Play/Pause/Stop でシミュレーションモード動作
- [ ] 既存の全 19 パネルにリグレッションなし

---

## Phase 44: パフォーマンス最適化

### 目的
レンダリングパイプラインの効率を向上させ、大規模シーンでのフレームレートを改善する。

### 現状分析

**既存の最適化:**
- GPU インスタンシング (k_InstancingThreshold=4, Scene 自動バッチング)
- ParallelCommandRecorder (マルチスレッドコマンド記録)
- IndirectCommandBuffer (GPU 間接描画)
- AsyncComputeQueue (非同期コンピュート)
- DynamicBuffer (リングバッファ方式)
- Entity BoundsInfo + フラスタムカリング (Scene/Entity)
- BVH/Octree/Quadtree (空間加速構造)
- GPUProfiler (タイムスタンプベース計測)

**最適化可能な領域:**

| 領域 | 現状 | 改善案 | 期待効果 |
|------|------|--------|---------|
| フラスタムカリング | Entity 単位の AABB カリング | Octree ベースの階層カリング | 大規模シーンで 2-3x |
| 描画ソート | 順序なし描画 | マテリアル→深度ソート | PSO 切り替え削減 50%+ |
| リソースバリア | 個別バリア発行 | BarrierBatch による一括発行 | API コール削減 |
| 定数バッファ | オブジェクトごとに CBV バインド | SSBO/StructuredBuffer | ルートシグネチャ効率化 |
| テクスチャバインド | 描画ごとにディスクリプタ設定 | Bindless テクスチャ | バインドコスト削減 |
| ポストエフェクト | 各エフェクトが個別パス | マルチパス融合 | RT 切り替え削減 |

---

### 44a: 描画ソート & PSO 切り替え最適化

**変更ファイル:** `GXLib/Graphics/3D/Renderer3D.h`, `Renderer3D.cpp`

**現状:**
- `DrawModel()` は呼ばれた順に描画
- サブメッシュごとにマテリアル→PSO の切り替えが発生
- 同じ PSO のオブジェクトが交互に描画されると切り替えコストが累積

**改善:**
```cpp
// 描画キュー構造体
struct DrawCommand {
    const Model* model;
    Transform3D transform;
    uint64_t sortKey;  // PSO ID << 48 | Material ID << 32 | Depth
};

// ソート基準: PSO → マテリアル → 前後 (不透明: front-to-back)
void Renderer3D::SortDrawCommands()
{
    std::sort(m_drawQueue.begin(), m_drawQueue.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            return a.sortKey < b.sortKey;
        });
}
```

**ソートキー構成:**
- Bits 63-48: PSO ハンドル (不透明/透明/シャドウ)
- Bits 47-32: マテリアルハンドル
- Bits 31-0: カメラからの深度 (uint32_t にエンコード)
  - 不透明: front-to-back (early-Z 活用)
  - 透明: back-to-front (正しいブレンド)

**実装手順:**
1. `Renderer3D` に `std::vector<DrawCommand> m_drawQueue` を追加
2. `DrawModel()` を描画キューへの追加に変更（実描画は後段）
3. `FlushDrawCommands()` でソート → 一括描画
4. `EndFrame()` から `FlushDrawCommands()` を呼び出し

---

### 44b: BarrierBatch 活用

**変更ファイル:** `GXLib/Graphics/Device/BarrierBatch.h` (既存), 各使用箇所

**現状:**
- `BarrierBatch` クラスが存在するが、多くの箇所で個別バリアを発行
- `RenderTarget::TransitionTo()` は内部で `ResourceBarrier(1, &barrier)` を呼ぶ

**改善:**
- ポストエフェクトチェーンでの連続バリアを BarrierBatch で一括化
- Render → PostEffect 間のバリアを統合

```cpp
// Before (各エフェクトが個別にバリア発行):
hdrRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
bloomRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

// After (一括バリア):
BarrierBatch batch;
batch.Add(hdrRT.GetResource(), oldState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
batch.Add(bloomRT.GetResource(), oldState, D3D12_RESOURCE_STATE_RENDER_TARGET);
batch.Flush(cmdList);
```

**対象箇所:**
1. `PostEffectPipeline::Resolve()` — ピンポン RT の状態遷移
2. `Renderer3D::BeginFrame()` / `EndFrame()` — シャドウ→メイン→ポストのバリア
3. `RTReflections::Dispatch()` / `RTGI::Dispatch()` — UAV バリア

---

### 44c: Octree ベース階層フラスタムカリング

**変更ファイル:**
- `GXLib/Core/Scene/Scene.h`, `Scene.cpp`

**現状:**
- Entity ごとに `BoundsInfo` で AABB フラスタムカリング
- 全エンティティを線形走査

**改善:**
- シーン内の全エンティティを Octree に登録
- フラスタムクエリで可視エンティティのみを取得
- エンティティの移動時に Octree を更新

```cpp
class Scene {
    // 既存
    std::vector<Entity*> m_entities;

    // 追加
    GX::Octree<Entity*> m_spatialIndex;
    bool m_spatialIndexDirty = true;

    void RebuildSpatialIndex();
    void QueryVisibleEntities(const GX::Frustum& frustum,
                              std::vector<Entity*>& visible) const;
};
```

**更新タイミング:**
- エンティティ追加/削除時: `m_spatialIndexDirty = true`
- Transform 変更時: `m_spatialIndexDirty = true`
- `RenderInternal()` 冒頭で dirty ならリビルド

---

### 44d: GPU プロファイリング統合

**目的:** GPUProfiler の計測データをレンダリング最適化の意思決定に活用

**変更ファイル:** `GXLib/Graphics/Device/GPUProfiler.h`, `GPUProfiler.cpp`

**追加機能:**
```cpp
struct ProfilingStats {
    float shadowPassMs;
    float mainPassMs;
    float postEffectMs;
    float uiPassMs;
    float totalFrameMs;
    int drawCallCount;
    int triangleCount;
    int psoSwitchCount;
};

// Renderer3D に統計カウンタを追加
class Renderer3D {
    int m_drawCallCount = 0;
    int m_psoSwitchCount = 0;
    // ... BeginFrame() でリセット、DrawIndexedInstanced() ごとにインクリメント
};
```

---

### 完了基準
- [ ] 描画ソートにより PSO 切り替え回数 50%+ 削減
- [ ] BarrierBatch でリソースバリアの一括発行
- [ ] Octree フラスタムカリングで 1000+ エンティティシーンのフレーム時間改善
- [ ] GPU プロファイリング統計が ProfilerOverlay に表示
- [ ] 既存サンプル・テストにリグレッションなし

---

## Phase 45: 先進レンダリング

### 目的
レンダリングパイプラインに先進的な手法を追加し、視覚品質と効率を向上させる。

### 前提
Phase 44 の最適化基盤（描画ソート、BarrierBatch、空間加速構造）が完了していること。

---

### 45a: Hi-Z Occlusion Culling

**目的:** 深度バッファの階層的ミップマップを使い、遮蔽されたオブジェクトを GPU 側で事前にカリング

**新規ファイル:**
- `GXLib/Graphics/3D/HiZBuffer.h`
- `GXLib/Graphics/3D/HiZBuffer.cpp`
- `Shaders/HiZGenerate.hlsl` — Hi-Z ミップ生成 Compute Shader
- `Shaders/HiZCull.hlsl` — Hi-Z カリング Compute Shader

**設計:**

```
[深度バッファ] → [Hi-Z Generate CS] → [Hi-Z ミップチェーン]
                                          ↓
[バウンディングボックスバッファ] → [Hi-Z Cull CS] → [可視フラグバッファ]
                                                      ↓
                                             [IndirectCommandBuffer]
                                                      ↓
                                             [ExecuteIndirect描画]
```

**Hi-Z Generate (Compute Shader):**
```hlsl
// 深度バッファの各ミップレベルを生成
// ミップN+1 の各ピクセル = ミップN の 2x2 ピクセルの最大深度
[numthreads(8, 8, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    float d00 = srcMip[tid.xy * 2 + uint2(0, 0)];
    float d01 = srcMip[tid.xy * 2 + uint2(0, 1)];
    float d10 = srcMip[tid.xy * 2 + uint2(1, 0)];
    float d11 = srcMip[tid.xy * 2 + uint2(1, 1)];
    dstMip[tid.xy] = max(max(d00, d01), max(d10, d11));
}
```

**Hi-Z Cull (Compute Shader):**
```hlsl
// 各オブジェクトのスクリーン空間 AABB を Hi-Z とテスト
// 結果を IndirectCommandBuffer のカウンタに反映
[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    AABB aabb = objectBounds[tid];
    float4 screenAABB = ProjectToScreen(aabb, viewProj);
    int mipLevel = CalculateMipLevel(screenAABB);
    float hiZDepth = SampleHiZ(screenAABB.center, mipLevel);

    if (screenAABB.nearZ <= hiZDepth) // 可視
    {
        uint idx;
        InterlockedAdd(visibleCount[0], 1, idx);
        visibleCommands[idx] = drawCommands[tid];
    }
}
```

**C++ 側:**
```cpp
class HiZBuffer {
public:
    bool Initialize(GraphicsDevice& device, uint32_t width, uint32_t height);
    void GenerateHiZ(CommandList& cmdList, DepthBuffer& depthBuffer);
    void CullObjects(CommandList& cmdList,
                     const Buffer& boundsBuf, uint32_t objectCount,
                     const IndirectCommandBuffer& indirectBuf);
    ID3D12Resource* GetHiZTexture() const;

private:
    Texture m_hiZTexture;  // R32_FLOAT with mip chain
    PipelineState m_generatePSO;
    PipelineState m_cullPSO;
    RootSignature m_rootSig;
    uint32_t m_mipLevels;
};
```

**統合ポイント:**
1. `Renderer3D::BeginFrame()` 後に前フレームの深度から Hi-Z 生成
2. Scene の全エンティティの AABB を GPU バッファにアップロード
3. Hi-Z Cull CS で可視オブジェクトを選別
4. `ExecuteIndirect()` で可視オブジェクトのみ描画

---

### 45b: Volumetric Clouds

**目的:** レイマーチベースのボリュメトリッククラウド

**新規ファイル:**
- `GXLib/Graphics/3D/VolumetricClouds.h`
- `GXLib/Graphics/3D/VolumetricClouds.cpp`
- `Shaders/VolumetricClouds.hlsl`

**設計:**
- レイマーチ（SV_VertexID フルスクリーン三角形 → PS でマーチ）
- ワーリーノイズ + パーリンノイズで雲密度
- ビールランバート法則で光散乱
- 時間経過で雲が流れる（風パラメータ）
- Temporal reprojection でノイズ削減

**パラメータ:**
```cpp
struct CloudParams {
    float cloudLayerBottom = 1500.0f;   // 雲底高度
    float cloudLayerTop = 3000.0f;      // 雲頂高度
    float coverage = 0.5f;              // 雲の被覆率 0-1
    float density = 0.05f;              // 密度乗数
    float windSpeed = 10.0f;            // 風速 (m/s)
    DirectX::XMFLOAT3 windDirection = {1, 0, 0}; // 風向き
    int marchSteps = 64;                // レイマーチステップ数
    int lightSteps = 6;                 // ライトマーチステップ数
    float silverLiningIntensity = 0.5f; // シルバーライニング強度
};
```

**ポストエフェクトパイプラインへの統合:**
- Skybox の後、シーン描画の前（または後）にクラウドパスを挿入
- 深度バッファを使ってシーンとの交差を計算（雲の手前にあるオブジェクトは遮蔽）

---

### 45c: Bindless テクスチャ

**目的:** ディスクリプタヒープ全体を一度にバインドし、テクスチャインデックスでアクセス

**変更ファイル:**
- `GXLib/Graphics/Device/DescriptorHeap.h`, `.cpp`
- `GXLib/Graphics/3D/Renderer3D.h`, `.cpp`
- `Shaders/PBR.hlsl` (修正)

**設計:**
```hlsl
// HLSL: Bindless テクスチャアクセス
Texture2D textures[] : register(t0, space1);  // Unbounded array
SamplerState linearSampler : register(s0);

// マテリアルの定数バッファにテクスチャインデックスを格納
cbuffer MaterialCB : register(b1)
{
    int albedoTexIndex;    // textures[albedoTexIndex].Sample(...)
    int normalTexIndex;
    int roughnessTexIndex;
    int metallicTexIndex;
    // ...
};
```

**C++ 側:**
```cpp
// TextureManager がグローバル SRV ヒープを管理
// ハンドル = ヒープ内のインデックス → シェーダーに渡す
class TextureManager {
    // 既存のハンドルがそのまま SRV インデックスになる
    int LoadTexture(const std::wstring& path);  // returns heap index
};
```

**注意:**
- D3D12 の `D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE` が必要
- SM 6.0+ が必要（既に dxcompiler を使用しているので問題なし）
- フォールバック: Bindless 非対応 GPU 向けに従来のバインド方式を残す

---

### 45d: Screen Space Contact Shadows

**目的:** 小さなオブジェクト（草、小石等）のセルフシャドウをスクリーンスペースで近似

**新規ファイル:**
- `GXLib/Graphics/PostEffect/ContactShadows.h`
- `GXLib/Graphics/PostEffect/ContactShadows.cpp`
- `Shaders/ContactShadows.hlsl`

**設計:**
- 各ピクセルからライト方向へスクリーンスペースでレイマーチ
- 深度バッファとの交差でシャドウ判定
- CSM が表現できない細部のシャドウを補完

**パラメータ:**
```cpp
struct ContactShadowParams {
    float maxDistance = 0.3f;   // マーチ最大距離 (ビュースペース)
    int stepCount = 16;         // マーチステップ数
    float thickness = 0.05f;   // 厚み閾値
    float intensity = 0.5f;    // シャドウ強度
};
```

**パイプライン統合:**
- SSAO の直後に配置（共に深度・法線を使用）
- 出力: シャドウマスクテクスチャ（R8_UNORM）
- ライティングパスでシャドウマスクを乗算

---

### 45e: Temporal Super Resolution (簡易版)

**目的:** TAA の拡張としてアップスケーリングを実現（内部解像度を下げてパフォーマンス向上）

**変更ファイル:**
- `GXLib/Graphics/PostEffect/TAA.h`, `TAA.cpp`

**設計:**
- 内部解像度を 75% に下げて描画
- TAA のテンポラル蓄積でアップスケール
- モーションベクトルによるリプロジェクション
- シャープニングフィルタで品質回復

**パラメータ追加:**
```cpp
float internalResolutionScale = 0.75f;  // 内部解像度スケール (0.5-1.0)
float sharpness = 0.5f;                 // シャープニング強度
```

---

### 完了基準
- [ ] Hi-Z Occlusion Culling で 1000+ オブジェクトシーンの描画コール 30%+ 削減
- [ ] Volumetric Clouds がリアルタイムで描画（30fps 以上維持）
- [ ] Bindless テクスチャでマテリアル切り替えコスト削減
- [ ] Contact Shadows で CSM 補完のセルフシャドウ
- [ ] 既存サンプル・テストにリグレッションなし

---

## 付録

### A. ファイル命名規則

| 種類 | パターン | 例 |
|------|---------|-----|
| ヘッダ | PascalCase.h | `HiZBuffer.h` |
| 実装 | PascalCase.cpp | `HiZBuffer.cpp` |
| シェーダー | PascalCase.hlsl | `VolumetricClouds.hlsl` |
| インクルードシェーダー | PascalCase.hlsli | `ShaderModelCommon.hlsli` |
| テスト | test_PascalCase.cpp | `test_Camera3D.cpp` |
| サンプル | SampleName/main.cpp | `Physics3DShowcase/main.cpp` |

### B. サンプルテンプレート

```cpp
/// @file Samples/XXXShowcase/main.cpp
/// @brief XXX のデモ。
///
/// 説明文。
///
/// Controls:
///   WASD       - 移動
///   ESC        - 終了
#include "GXEasy.h"
// #include 必要なヘッダ

class XXXShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: XXX";
        config.width = 1280;
        config.height = 720;
        config.bgR = 20; config.bgG = 20; config.bgB = 30;
        return config;
    }

    void Start() override { /* 初期化 */ }
    void Update(float dt) override { /* 更新 */ }
    void Draw() override { /* 描画 */ }

private:
    // メンバ変数
};

GX_EASY_APP(XXXShowcaseApp)
```

### C. 3D サンプルテンプレート（Renderer3D 使用時）

```cpp
/// @file Samples/XXX3DShowcase/main.cpp
#include "GXEasy.h"
#include "Compat/CompatContext.h"
// 追加 #include

class XXX3DShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: XXX 3D";
        config.width = 1280;
        config.height = 720;
        config.bgR = 10; config.bgG = 10; config.bgB = 15;
        config.enable3D = true;  // 3D レンダラー有効化
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& r = ctx.renderer3D;
        auto& c = ctx.camera3D;
        auto& p = ctx.postEffectPipeline;

        // ポストエフェクト設定
        p.SetTonemappingMode(GX::TonemappingMode::ACES);
        p.SetExposure(1.0f);
        p.GetBloom().SetEnabled(true);
        p.GetSSAO().SetEnabled(true);
        p.SetFXAAEnabled(true);

        // ライティング
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional(
            { 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        r.SetLights(lights, 1, { 0.08f, 0.08f, 0.08f });

        // カメラ
        float aspect = static_cast<float>(ctx.swapChain.GetWidth())
                     / ctx.swapChain.GetHeight();
        c.SetPerspective(DirectX::XM_PIDIV4, aspect, 0.1f, 500.0f);
        c.SetPosition(0.0f, 5.0f, -10.0f);
        c.LookAt({ 0.0f, 0.0f, 0.0f });

        // メッシュ/モデル読み込み
        // ...
    }

    void Update(float dt) override
    {
        // 入力処理、ロジック更新
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& r = ctx.renderer3D;
        auto& c = ctx.camera3D;
        auto& p = ctx.postEffectPipeline;
        auto& db = ctx.depthBuffer;

        r.BeginFrame(ctx.cmdList, c);
        // r.DrawModel(...);
        r.EndFrame();

        db.TransitionTo(ctx.cmdList, D3D12_RESOURCE_STATE_DEPTH_READ);
        p.Resolve(ctx.cmdList, ctx.swapChain, db, c);
        db.TransitionTo(ctx.cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // 2D HUD (SpriteBatch / TextRenderer)
    }

private:
    // メンバ変数
};

GX_EASY_APP(XXX3DShowcaseApp)
```

### D. テストテンプレート

```cpp
/// @file Tests/test_XXX.cpp
#include <gtest/gtest.h>
#include "対象ヘッダ.h"

// ヘルパー
namespace {
    constexpr float k_Eps = 1e-4f;
}

TEST(XXX, DefaultValues)
{
    GX::XXX obj;
    EXPECT_EQ(obj.GetSomething(), expectedValue);
}

TEST(XXX, SomeOperation)
{
    GX::XXX obj;
    obj.DoSomething(param);
    EXPECT_NEAR(obj.GetResult(), expected, k_Eps);
}
```

### E. Phase 間の依存関係図

```
Phase 41 (テスト拡充) ─────────────────┐
                                        ├──→ Phase 43 (エディタ強化)
Phase 42 (サンプル追加) ──── 独立 ───── │
                                        │
Phase 44 (パフォーマンス最適化) ────────┼──→ Phase 45 (先進レンダリング)
```

### F. 各 Phase の開始前チェックリスト

1. `git status` で未コミット変更がないことを確認
2. `cmake --build build --config Debug` でビルドが通ることを確認
3. `ctest -C Debug` で全テストがパスすることを確認
4. `MEMORY.md` で前 Phase の完了が記録されていることを確認
5. 本指令書の対象 Phase セクションを精読

### G. 各 Phase の完了後チェックリスト

1. 全ファイルがビルドできること
2. 既存テストがリグレッションなしでパス
3. 新規テストが全パス
4. 新規サンプルが実行可能
5. `MEMORY.md` に Phase 完了を記録
6. コミットメッセージに Phase 番号と概要を含める

---

*本指令書は Phase 36-40 指令書 (`docs/Phase36-40_Directive.md`) の後継であり、
GXLib エンジンの成熟フェーズ（品質・安定化・高度化）をカバーする。*


---

# Part VII: フェーズ完了ログ

## Phase 0 Summary

## 概要

DXライブラリの完全上位互換フレームワーク「GXLib」のPhase 0として、
DirectX 12ベースの基盤を構築し、**色付き三角形（Hello Triangle）の描画**に成功した。

---

## 技術仕様

| 項目 | 内容 |
|------|------|
| 言語 | C++20 |
| ビルドシステム | CMake（VS2022 v145ツールセット） |
| シェーダーコンパイラ | Windows SDK DXC (dxcompiler.dll) |
| 名前空間 | `GX::` |
| 命名規則 | クラス/メソッド: PascalCase, メンバ: m_camelCase |
| PCH | pch.h（Windows + DX12 + STL + DirectXMath） |
| コンパイルフラグ | `/utf-8`（日本語コメント対応） |

---

## 作成ファイル一覧（30ファイル）

### ビルド構成（5ファイル）

| ファイル | 役割 |
|----------|------|
| `CMakeLists.txt` | ルートCMake（C++20、/utf-8、サブプロジェクト統合） |
| `GXLib/CMakeLists.txt` | エンジンライブラリ（STATIC、PCH、DX12リンク） |
| `Sandbox/CMakeLists.txt` | テストアプリ（WIN32 EXE、シェーダーコピー） |
| `GXLib/pch.h` | プリコンパイルドヘッダー |
| `GXLib/pch.cpp` | PCH生成用ソース |

### Core（8ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `Logger` | Logger.h/cpp | ログレベル別出力（Info/Warn/Error）、OutputDebugString + printf |
| `Timer` | Timer.h/cpp | QueryPerformanceCounterベース高精度タイマー、デルタタイム・FPS計算 |
| `Window` | Window.h/cpp | Win32ウィンドウ作成、WndProc、メッセージループ、リサイズコールバック |
| `Application` | Application.h/cpp | Initialize → Run → Shutdown ライフサイクル、FPSタイトル表示 |

### Graphics/Device（12ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `GraphicsDevice` | GraphicsDevice.h/cpp | DXGIFactory → Adapter列挙 → D3D12Device作成、デバッグレイヤー |
| `Fence` | Fence.h/cpp | ID3D12Fence + Event、Signal/WaitForValue/WaitForGPU |
| `CommandQueue` | CommandQueue.h/cpp | ID3D12CommandQueue、ExecuteCommandLists、Flush |
| `DescriptorHeap` | DescriptorHeap.h/cpp | RTV/DSV/CBV_SRV_UAVヒープ管理、Allocate/GetHandle |
| `SwapChain` | SwapChain.h/cpp | IDXGISwapChain4、ダブルバッファリング、Present、Resize |
| `CommandList` | CommandList.h/cpp | ID3D12GraphicsCommandList + Allocator×2、Reset/Close |

### Graphics/Pipeline（6ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `Shader` | Shader.h/cpp | DXC (IDxcCompiler3) によるHLSLコンパイル（VS/PS） |
| `RootSignatureBuilder` | RootSignature.h/cpp | ビルダーパターンでルートシグネチャ構築 |
| `PipelineStateBuilder` | PipelineState.h/cpp | ビルダーパターンでGraphics PSO構築 |

### Graphics/Resource（2ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `Buffer` | Buffer.h/cpp | 頂点/インデックスバッファ作成（UPLOADヒープ） |

### Shaders（1ファイル）

| ファイル | 役割 |
|----------|------|
| `Shaders/HelloTriangle.hlsl` | 頂点シェーダー（VSMain）+ ピクセルシェーダー（PSMain） |

### Sandbox（1ファイル）

| ファイル | 役割 |
|----------|------|
| `Sandbox/main.cpp` | WinMain、全システム初期化、三角形描画ループ |

---

## ディレクトリ構成

```
GXLib/
├── CMakeLists.txt
├── GXLib/
│   ├── CMakeLists.txt
│   ├── pch.h
│   ├── pch.cpp
│   ├── Core/
│   │   ├── Application.h / .cpp
│   │   ├── Logger.h / .cpp
│   │   ├── Timer.h / .cpp
│   │   └── Window.h / .cpp
│   └── Graphics/
│       ├── Device/
│       │   ├── GraphicsDevice.h / .cpp
│       │   ├── Fence.h / .cpp
│       │   ├── CommandQueue.h / .cpp
│       │   ├── CommandList.h / .cpp
│       │   ├── DescriptorHeap.h / .cpp
│       │   └── SwapChain.h / .cpp
│       ├── Pipeline/
│       │   ├── Shader.h / .cpp
│       │   ├── RootSignature.h / .cpp
│       │   └── PipelineState.h / .cpp
│       └── Resource/
│           └── Buffer.h / .cpp
├── Sandbox/
│   ├── CMakeLists.txt
│   └── main.cpp
└── Shaders/
    └── HelloTriangle.hlsl
```

---

## 描画パイプライン

Sandbox/main.cppでの描画フロー：

```
1. Application初期化 → Window作成（1280x720）
2. GraphicsDevice初期化（デバッグレイヤー有効）
3. CommandQueue + CommandList作成
4. SwapChain作成（ダブルバッファリング、FLIP_DISCARD）
5. DXCでシェーダーコンパイル（HelloTriangle.hlsl）
6. RootSignature作成（パラメータなし、IA入力のみ）
7. PipelineState作成（VS + PS + InputLayout + CullNone + DepthOff）
8. 頂点バッファ作成（3頂点: 赤・青・緑）

メインループ:
  Timer.Tick() → ProcessMessages()
  → CommandList.Reset()
  → ResourceBarrier(PRESENT → RENDER_TARGET)
  → ClearRenderTargetView(ダークブルー)
  → SetViewport/Scissor → SetRootSignature → SetPSO
  → IASetVertexBuffers → DrawInstanced(3, 1)
  → ResourceBarrier(RENDER_TARGET → PRESENT)
  → CommandList.Close() → ExecuteCommandLists()
  → SwapChain.Present() → Fence.Signal()
```

---

## ビルド方法

```bash
# CMake構成
cmake -B build -G "Visual Studio 17 2022" -A x64

# ビルド（Debug）
cmake --build build --config Debug

# 実行
build\Sandbox\Debug\Sandbox.exe
```

---

## 実行結果

- ウィンドウが画面中央に1280x720で表示される
- ダークブルーの背景に赤・青・緑のグラデーション三角形が描画される
- タイトルバーにFPSが1秒ごとに更新表示される
- ESCキーで終了
- ウィンドウリサイズ時にSwapChainが自動リサイズ

---

## コメントスタイル

全クラスに初学者向け日本語コメントを付与：

```cpp
/// @brief GPU同期用フェンスクラス
///
/// 【初学者向け解説】
/// CPUとGPUは非同期で動作します。CPUがGPUに描画コマンドを送っても、
/// GPUがすぐに処理を完了するとは限りません。
///
/// フェンスは「GPUの作業完了を待つ」ための仕組みです。
/// イメージとしては、レストランで注文した料理が「できたよ！」と
/// 知らせてくれるベルのようなものです。
```

---

## Phase 0 完了チェックリスト

- [x] CMakeプロジェクト構成
- [x] Win32ウィンドウ作成・メッセージループ
- [x] D3D12デバイス初期化（Factory, Device, CommandQueue）
- [x] SwapChain作成（ダブルバッファリング）
- [x] DescriptorHeap管理クラス
- [x] FenceによるCPU-GPU同期
- [x] CommandAllocator / CommandList管理
- [x] パイプラインステート基盤（RootSignatureビルダー、PSOビルダー）
- [x] DXCによるシェーダーコンパイル
- [x] 三角形描画（Hello Triangle）
- [x] フレームタイミング・FPS制御
- [x] ログシステム
- [x] 全クラスに初学者向け日本語コメント
- [x] Debugビルド成功（エラー・警告なし）

## Phase 1 Summary

## 概要

Phase 0（Hello Triangle）で構築したDirectX 12基盤の上に、
DXLib互換の**2D描画エンジン**を実装した。
SpriteBatch（テクスチャ付きスプライト描画）とPrimitiveBatch（基本図形描画）を中心に、
テクスチャ管理・ソフトウェアイメージ・カメラ・アニメーション・レンダーターゲットを完成させた。

---

## 技術仕様（Phase 0からの追加分）

| 項目 | 内容 |
|------|------|
| テクスチャ読み込み | stb_image.h（バンドル済み、ヘッダーオンリー） |
| 対応画像形式 | PNG, JPG, BMP, TGA 等（stb_image準拠） |
| スプライトバッチ容量 | 最大 4,096 スプライト/バッチ |
| プリミティブバッチ容量 | 最大 4,096×3 三角形頂点 + 4,096×2 線分頂点 |
| テクスチャ管理上限 | 256 テクスチャ |
| ブレンドモード | Alpha / Add / Sub / Mul / Screen / None（6種） |
| 座標系 | 左上原点、Y軸下向き（DXLib互換） |

---

## 作成ファイル一覧（Phase 1で追加: 24ファイル）

### Graphics/Resource（12ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `DynamicBuffer` | DynamicBuffer.h/cpp | フレーム書き換え用UPLOADバッファ（ダブルバッファリング対応） |
| `Texture` | Texture.h/cpp | GPU テクスチャ（stb_image読み込み → UPLOADステージング → DEFAULTヒープ → SRV作成） |
| `TextureManager` | TextureManager.h/cpp | ハンドルベースのテクスチャ管理（パスキャッシュ、フリーリスト、UV矩形） |
| `SoftImage` | SoftImage.h/cpp | CPUメモリ上のピクセル操作（DXLib LoadSoftImage/DrawPixelSoftImage互換） |
| `RenderTarget` | RenderTarget.h/cpp | オフスクリーンレンダーターゲット（RTV + SRV、DXLib MakeScreen互換） |
| `DepthBuffer` | DepthBuffer.h/cpp | 深度バッファ基盤（Phase 1では未使用、将来の3D対応用） |

### Graphics/Rendering（10ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `SpriteBatch` | SpriteBatch.h/cpp | 2Dスプライトバッチ描画（DrawGraph系5種 + ブレンド6種 + 描画色） |
| `PrimitiveBatch` | PrimitiveBatch.h/cpp | 基本図形描画（線分、矩形、円、三角形、楕円、1ピクセル） |
| `Camera2D` | Camera2D.h/cpp | 2Dカメラ（位置・ズーム・回転 → ビュープロジェクション行列） |
| `SpriteSheet` | SpriteSheet.h/cpp | 画像分割読み込み（DXLib LoadDivGraph互換） |
| `Animation2D` | Animation2D.h/cpp | フレームベースアニメーション（ループ、速度、タイマー管理） |

### Shaders（2ファイル）

| ファイル | 役割 |
|----------|------|
| `Shaders/Sprite.hlsl` | スプライト描画用（正射影変換 + テクスチャサンプリング × 頂点カラー） |
| `Shaders/Primitive.hlsl` | プリミティブ描画用（正射影変換 + 頂点カラーのみ） |

### ThirdParty（1ファイル）

| ファイル | 役割 |
|----------|------|
| `GXLib/ThirdParty/stb_image.h` | ヘッダーオンリー画像ローダー（STB_IMAGE_IMPLEMENTATION はTexture.cppで定義） |

### Sandbox（更新: 1ファイル）

| ファイル | 役割 |
|----------|------|
| `Sandbox/main.cpp` | Phase 1テストアプリ（SpriteBatch + PrimitiveBatch + SoftImageによる動的テクスチャ） |

---

## ディレクトリ構成（Phase 1完了時点）

```
GXLib/
├── CMakeLists.txt
├── Phase0_Summary.md
├── Phase1_Summary.md
├── GXLib/
│   ├── CMakeLists.txt
│   ├── pch.h
│   ├── pch.cpp
│   ├── Core/
│   │   ├── Application.h / .cpp
│   │   ├── Logger.h / .cpp
│   │   ├── Timer.h / .cpp
│   │   └── Window.h / .cpp
│   ├── Graphics/
│   │   ├── Device/
│   │   │   ├── GraphicsDevice.h / .cpp
│   │   │   ├── Fence.h / .cpp
│   │   │   ├── CommandQueue.h / .cpp
│   │   │   ├── CommandList.h / .cpp
│   │   │   ├── DescriptorHeap.h / .cpp
│   │   │   └── SwapChain.h / .cpp
│   │   ├── Pipeline/
│   │   │   ├── Shader.h / .cpp
│   │   │   ├── RootSignature.h / .cpp
│   │   │   └── PipelineState.h / .cpp
│   │   ├── Resource/                    ← Phase 1で拡張
│   │   │   ├── Buffer.h / .cpp          (Phase 0)
│   │   │   ├── DynamicBuffer.h / .cpp   (NEW)
│   │   │   ├── Texture.h / .cpp         (NEW)
│   │   │   ├── TextureManager.h / .cpp  (NEW)
│   │   │   ├── SoftImage.h / .cpp       (NEW)
│   │   │   ├── RenderTarget.h / .cpp    (NEW)
│   │   │   └── DepthBuffer.h / .cpp     (NEW)
│   │   └── Rendering/                   ← Phase 1で新設
│   │       ├── SpriteBatch.h / .cpp     (NEW)
│   │       ├── PrimitiveBatch.h / .cpp  (NEW)
│   │       ├── Camera2D.h / .cpp        (NEW)
│   │       ├── SpriteSheet.h / .cpp     (NEW)
│   │       └── Animation2D.h / .cpp     (NEW)
│   └── ThirdParty/
│       └── stb_image.h                  (NEW)
├── Sandbox/
│   ├── CMakeLists.txt
│   └── main.cpp                         (UPDATED)
└── Shaders/
    ├── HelloTriangle.hlsl               (Phase 0)
    ├── Sprite.hlsl                      (NEW)
    └── Primitive.hlsl                   (NEW)
```

---

## 描画パイプライン

### スプライト描画フロー

```
1. SpriteBatch.Begin(cmdList, frameIndex)
   → 定数バッファに正射影行列を書き込み
   → ディスクリプタヒープをバインド

2. DrawGraph / DrawRotaGraph / DrawExtendGraph 等
   → 4頂点（Position, UV, Color）をバッチ内バッファに蓄積
   → テクスチャハンドルやブレンドモードが変わるとFlush()

3. SpriteBatch.End()
   → 残りの頂点をFlush()
   → Flush():
      RootSignature設定 → PSO設定（ブレンドモード別）
      → 頂点バッファ（DynamicBuffer）書き込み
      → インデックスバッファ（事前生成、0-1-2, 2-3-0 パターン）
      → SRVテーブルにテクスチャをバインド
      → DrawIndexedInstanced(spriteCount × 6, 1)
```

### プリミティブ描画フロー

```
1. PrimitiveBatch.Begin(cmdList, frameIndex)
   → 定数バッファに正射影行列を書き込み

2. DrawBox / DrawCircle / DrawTriangle 等
   → 塗りつぶし: 三角形頂点バッファに蓄積
   → アウトライン: 線分頂点バッファに蓄積
   → 円/楕円: 扇形に三角形分割（segments引数で精度指定）

3. PrimitiveBatch.End()
   → FlushTriangles():
      三角形用PSO + 頂点バッファ → Draw(triVertexCount)
   → FlushLines():
      線分用PSO（TOPOLOGY_LINELIST）+ 頂点バッファ → Draw(lineVertexCount)
```

### シェーダー座標変換（共通）

```hlsl
// 正射影行列: スクリーン座標(px) → クリップ座標(-1〜+1)
// DirectXMathはrow-major、HLSL cbufferはcolumn-majorなので
// mul(matrix, vector)形式で暗黙転置により正しく変換される
output.pos = mul(projectionMatrix, float4(input.pos, 0.0f, 1.0f));
```

---

## 主要クラスのAPI

### SpriteBatch（DXLib DrawGraph系互換）

```cpp
// 初期化
bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                uint32_t screenWidth, uint32_t screenHeight);

// 描画サイクル
void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);
void DrawGraph(float x, float y, int handle, bool transFlag = true);
void DrawRotaGraph(float cx, float cy, float extRate, float angle,
                   int handle, bool transFlag = true);
void DrawExtendGraph(float x1, float y1, float x2, float y2,
                     int handle, bool transFlag = true);
void DrawModiGraph(float x1, float y1, float x2, float y2,
                   float x3, float y3, float x4, float y4,
                   int handle, bool transFlag = true);
void DrawRectGraph(float x, float y, int srcX, int srcY, int w, int h,
                   int handle, bool transFlag = true);
void End();

// 描画設定
void SetBlendMode(BlendMode mode);     // Alpha, Add, Sub, Mul, Screen, None
void SetDrawColor(float r, float g, float b, float a = 1.0f);
void SetScreenSize(uint32_t width, uint32_t height);
void SetProjectionMatrix(const XMMATRIX& matrix);  // Camera2D用
void ResetProjectionMatrix();

// テクスチャ管理
TextureManager& GetTextureManager();
```

### PrimitiveBatch（DXLib DrawLine/DrawBox系互換）

```cpp
// 初期化
bool Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight);

// 描画サイクル
void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);
void DrawLine(float x1, float y1, float x2, float y2, uint32_t color, int thickness = 1);
void DrawBox(float x1, float y1, float x2, float y2, uint32_t color, bool fillFlag);
void DrawCircle(float cx, float cy, float r, uint32_t color, bool fillFlag, int segments = 32);
void DrawTriangle(float x1, float y1, float x2, float y2,
                  float x3, float y3, uint32_t color, bool fillFlag);
void DrawOval(float cx, float cy, float rx, float ry, uint32_t color,
              bool fillFlag, int segments = 32);
void DrawPixel(float x, float y, uint32_t color);
void End();

// 設定
void SetScreenSize(uint32_t width, uint32_t height);
void SetProjectionMatrix(const XMMATRIX& matrix);
void ResetProjectionMatrix();
```

### TextureManager（DXLib LoadGraph互換）

```cpp
bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
int  LoadTexture(const std::wstring& filePath);            // → ハンドル（キャッシュ付き）
int  CreateTextureFromMemory(const void* pixels, uint32_t w, uint32_t h);
Texture* GetTexture(int handle);
void ReleaseTexture(int handle);
int  CreateRegionHandles(int baseHandle, int allNum,       // スプライトシート分割用
                         int xNum, int yNum, int xSize, int ySize);
```

### SoftImage（DXLib MakeSoftImage/DrawPixelSoftImage互換）

```cpp
bool Create(uint32_t width, uint32_t height);
bool LoadFromFile(const std::wstring& filePath);
uint32_t GetPixel(int x, int y) const;       // 0xAARRGGBB
void DrawPixel(int x, int y, uint32_t color); // 0xAARRGGBB
void Clear(uint32_t color = 0x00000000);
int  CreateTexture(TextureManager& textureManager);  // GPUにアップロード
```

### Camera2D

```cpp
void SetPosition(float x, float y);
void SetZoom(float scale);         // 1.0 = 等倍
void SetRotation(float angle);     // ラジアン
XMMATRIX GetViewProjectionMatrix(uint32_t screenWidth, uint32_t screenHeight) const;
```

### SpriteSheet（DXLib LoadDivGraph互換）

```cpp
static bool LoadDivGraph(TextureManager& textureManager,
                         const std::wstring& filePath,
                         int allNum, int xNum, int yNum,
                         int xSize, int ySize,
                         int* handleArray);
```

### Animation2D

```cpp
void AddFrames(const int* handles, int count, float frameDuration);
void Update(float deltaTime);
int  GetCurrentHandle() const;
void SetLoop(bool loop);
void SetSpeed(float speed);
void Reset();
bool IsFinished() const;
```

---

## 設計パターン

| パターン | 適用箇所 | 説明 |
|----------|----------|------|
| バッチレンダリング | SpriteBatch, PrimitiveBatch | 頂点データをCPU側に蓄積し、End()で一括描画。DrawCall数を最小化 |
| ダブルバッファリング | DynamicBuffer | GPUが前フレームのバッファを読み中に、CPUが次フレームに書き込み |
| ハンドルベース管理 | TextureManager | `int`ハンドルで間接参照。DXLibと同じAPI体験 |
| フリーリスト | TextureManager, DescriptorHeap | 解放されたハンドル/インデックスを再利用 |
| パスキャッシュ | TextureManager | 同一パスの二重読み込みを防止（`unordered_map`） |
| UV矩形ハンドル | TextureRegion | 1枚のテクスチャに対し複数のUV矩形ハンドルを発行（スプライトシート対応） |
| PSO配列 | SpriteBatch | ブレンドモード6種をPSO配列で保持し、モード変更時にPSO切替 |
| 三角形/線分分離 | PrimitiveBatch | TOPOLOGY_TRIANGLELIST用とTOPOLOGY_LINELIST用で別バッファ・別PSO |

---

## GPUリソースのアップロードフロー

```
=== テクスチャアップロード ===
stb_image (CPU) → ピクセルデータ
                → UPLOADヒープにステージングバッファ作成
                → DEFAULTヒープ（GPU専用）にテクスチャリソース作成
                → コマンドリスト: CopyTextureRegion(staging → texture)
                → フェンス待ち: GPU側コピー完了を保証
                → SRV作成 → シェーダーからアクセス可能に

=== 動的頂点データ ===
CPU (DrawGraph等) → DynamicBuffer[frameIndex].Map() で直接書き込み
                  → Unmap()
                  → IASetVertexBuffers で GPU にバインド
                  ※ UPLOADヒープなのでコピー不要（CPU/GPU共有メモリ）
```

---

## テストアプリ（Sandbox/main.cpp）

Phase 1の全機能を検証するデモ：

### プリミティブ描画テスト
| 描画内容 | API | 色/設定 |
|----------|-----|---------|
| 塗りつぶし矩形 | `DrawBox(50,50, 200,150, ..., true)` | 赤 (0xFFFF4444) |
| アウトライン矩形 | `DrawBox(220,50, 370,150, ..., false)` | 緑 (0xFF44FF44) |
| 塗りつぶし円 | `DrawCircle(480,100, 50, ..., true)` | 青 (0xFF4444FF) |
| アウトライン円 | `DrawCircle(600,100, 50, ..., false)` | 黄 (0xFFFFFF44) |
| 塗りつぶし三角形 | `DrawTriangle(700,50, 750,150, 650,150, ..., true)` | マゼンタ (0xFFFF44FF) |
| 水平線 | `DrawLine(50,180, 750,180, ...)` | 白 (0xFFFFFFFF) |
| 塗りつぶし楕円 | `DrawOval(400,250, 100,50, ..., true)` | シアン (0xFF44FFFF) |
| アニメーション線 | `DrawLine(400,300, sinf(t)*300+400, 350, ...)` | オレンジ (0xFFFF8800) |

### スプライト描画テスト
| 描画内容 | API | 備考 |
|----------|-----|------|
| 通常描画 | `DrawGraph(50, 400, handle)` | 64x64チェッカーパターン（SoftImageで動的生成） |
| 拡大描画 | `DrawExtendGraph(150,400, 310,528, handle)` | 160x128に引き伸ばし |
| 回転描画 | `DrawRotaGraph(450,464, 2.0, totalTime, handle)` | 2倍拡大＋時間回転 |
| 加算ブレンド | `SetBlendMode(Add) → DrawGraph(600,400, handle)` | 発光効果 |
| 色付き描画 | `SetDrawColor(1,0.5,0.5,0.8) → DrawGraph(700,400, handle)` | 赤みがかった半透明 |

### テストテクスチャ生成
```
SoftImageで64x64のチェッカーパターンを動的生成：
- 8x8ピクセル単位の格子
- 白 (0xFFFFFFFF) と 青 (0xFF4488CC) の交互
- CreateTexture()でGPUにアップロード
```

---

## 修正した不具合

### プロジェクション行列の二重転置問題

**症状**: 矩形・円・スプライトが一切表示されず、線分だけが画面中央付近に極小サイズで斜めに表示される。

**原因**: `SpriteBatch.cpp`と`PrimitiveBatch.cpp`で定数バッファ書き込み前に`XMMatrixTranspose()`を呼んでいた。

| レイヤー | 格納形式 |
|---------|---------|
| DirectXMath (`XMMATRIX`) | row-major |
| HLSL `cbuffer`（デフォルト） | column-major |

Row-majorデータをcolumn-majorとして読み込む時点で**暗黙的に転置**される。
HLSL側で`mul(projectionMatrix, float4(pos, 0, 1))`を使う場合、この暗黙転置だけで正しい変換結果になる。
C++側で追加の`XMMatrixTranspose()`を行うと**二重転置**となり、`w`成分が座標依存の値になる。
perspective divideで全座標が極小の一点に潰れ、描画が消失していた。

**計算例**（頂点 (400, 300)、1280x720画面）:

| ケース | clip座標 | 結果 |
|-------|----------|------|
| 正しい（転置なし） | (-0.375, 0.167, 0, **1.0**) | 画面内に正常表示 |
| 二重転置 | (0.625, -0.833, 0, **-99**) | perspective divideで消失 |

**修正内容**（2ファイル、各1行）:

`PrimitiveBatch.cpp`:
```cpp
// 変更前
XMMATRIX transposed = XMMatrixTranspose(proj);
memcpy(cbData, &transposed, sizeof(XMMATRIX));

// 変更後
memcpy(cbData, &proj, sizeof(XMMATRIX));
```

`SpriteBatch.cpp`:
```cpp
// 変更前
XMMATRIX transposed = XMMatrixTranspose(proj);
memcpy(cbData, &transposed, sizeof(XMMATRIX));

// 変更後
memcpy(cbData, &proj, sizeof(XMMATRIX));
```

**教訓**: DirectXMath + HLSL column-major cbuffer + `mul(matrix, vector)`の組み合わせでは、C++側で`XMMatrixTranspose()`を呼ぶ必要はない。暗黙転置が自動的に正しい変換を行う。

---

## ビルド方法

```bash
# CMake構成
cmake -B build -G "Visual Studio 17 2022" -A x64

# ビルド（Debug）
cmake --build build --config Debug

# 実行
build\Sandbox\Debug\Sandbox.exe
```

---

## 実行結果

- ウィンドウが画面中央に1280x720で表示される
- ダークブルーの背景に以下が描画される：
  - 画面上部: 赤い塗りつぶし矩形、緑のアウトライン矩形、青い塗りつぶし円、黄色のアウトライン円、マゼンタの三角形
  - 中段: 白い水平線、シアンの楕円、オレンジのアニメーション線（sin波で往復）
  - 画面下部: チェッカーパターンスプライト5種（通常、拡大、回転アニメ、加算ブレンド、色付き半透明）
- タイトルバーにFPSが1秒ごとに更新表示される
- ESCキーで終了
- ウィンドウリサイズ時にSwapChain + SpriteBatch + PrimitiveBatchが自動リサイズ

---

## Phase 1 完了チェックリスト

- [x] DynamicBuffer（ダブルバッファリング対応UPLOADバッファ）
- [x] Texture（stb_imageによるファイル読み込み + GPU転送 + SRV作成）
- [x] TextureManager（intハンドル管理、パスキャッシュ、フリーリスト）
- [x] SoftImage（CPUピクセル操作 → GPU転送）
- [x] RenderTarget（オフスクリーン描画）
- [x] DepthBuffer（基盤のみ、将来用）
- [x] SpriteBatch（DrawGraph / DrawRotaGraph / DrawExtendGraph / DrawModiGraph / DrawRectGraph）
- [x] PrimitiveBatch（DrawLine / DrawBox / DrawCircle / DrawTriangle / DrawOval / DrawPixel）
- [x] ブレンドモード6種（Alpha / Add / Sub / Mul / Screen / None）
- [x] Camera2D（位置・ズーム・回転 → ビュープロジェクション行列）
- [x] SpriteSheet（LoadDivGraph互換の画像分割読み込み）
- [x] Animation2D（フレームベースアニメーション）
- [x] Sprite.hlsl / Primitive.hlsl（正射影変換シェーダー）
- [x] テストアプリ（プリミティブ + スプライト + ブレンドモード + 動的テクスチャ）
- [x] 二重転置バグ修正（XMMatrixTranspose削除）
- [x] 全クラスに初学者向け日本語コメント
- [x] Debugビルド成功（エラー・警告なし）

## Phase 2 Summary

## 概要

Phase 2 の完了条件「音が鳴り、操作可能な2Dゲームが作れる」を達成。
入力・音声・テキスト描画の3機能を追加し、インタラクティブなデモを実装した。

---

## Sub-Phase 2a: Input System

### 新規ファイル (8ファイル)
| ファイル | 役割 |
|----------|------|
| `GXLib/Input/Keyboard.h` | キーボード入力クラス定義 |
| `GXLib/Input/Keyboard.cpp` | 256キー状態管理、WM_KEYDOWN/KEYUP処理、press/trigger/release判定 |
| `GXLib/Input/Mouse.h` | マウス入力クラス定義 |
| `GXLib/Input/Mouse.cpp` | 座標・デルタ・3ボタン・ホイール管理、WM_MOUSEMOVE等処理 |
| `GXLib/Input/Gamepad.h` | ゲームパッドクラス定義（XInput） |
| `GXLib/Input/Gamepad.cpp` | 4コントローラ対応、スティック/トリガーのデッドゾーン処理 |
| `GXLib/Input/InputManager.h` | 統合入力マネージャー定義 |
| `GXLib/Input/InputManager.cpp` | Keyboard/Mouse/Gamepad統合、DXLib互換API（CheckHitKey等） |

### 変更ファイル
- **`GXLib/Core/Window.h`** — `AddMessageCallback()` メソッド追加、`m_messageCallbacks` ベクタ追加
- **`GXLib/Core/Window.cpp`** — WndProc内でコールバックリストをループ呼出、ESCハードコード削除

### 設計ポイント
- ポーリングモデル: 毎フレーム `Update()` で前フレーム状態を保存、現在状態を確定
- Windowメッセージコールバック: `AddMessageCallback()` で Input がメッセージを受信
- Gamepadデッドゾーン: 0.24（スティック）、0.12（トリガー）で再マッピング

---

## Sub-Phase 2b: Audio System

### 新規ファイル (10ファイル)
| ファイル | 役割 |
|----------|------|
| `GXLib/Audio/AudioDevice.h` | XAudio2エンジンクラス定義 |
| `GXLib/Audio/AudioDevice.cpp` | CoInitializeEx → XAudio2Create → CreateMasteringVoice |
| `GXLib/Audio/Sound.h` | WAVデータ保持クラス定義 |
| `GXLib/Audio/Sound.cpp` | WAVファイルパーサー（RIFF/fmt/dataチャンク解析） |
| `GXLib/Audio/SoundPlayer.h` | SE再生クラス定義 |
| `GXLib/Audio/SoundPlayer.cpp` | Play毎に新規SourceVoice作成、コールバックで自動解放 |
| `GXLib/Audio/MusicPlayer.h` | BGM再生クラス定義 |
| `GXLib/Audio/MusicPlayer.cpp` | 単一Voice、ループ再生、Pause/Resume、FadeIn/FadeOut |
| `GXLib/Audio/AudioManager.h` | ハンドルベース管理クラス定義 |
| `GXLib/Audio/AudioManager.cpp` | LoadSound→ハンドル、PlaySound/PlayMusic、フェード更新 |

### 設計ポイント
- TextureManagerと同じハンドル+freelistパターン
- SE: 同時複数再生対応（毎回SourceVoice生成、OnStreamEndコールバックで解放）
- BGM: 単一Voice、`XAUDIO2_LOOP_INFINITE` でループ、`Update()` で音量フェード補間
- WAVパーサー: RIFF→fmt→dataの順にチャンクを走査、不明チャンクはスキップ

---

## Sub-Phase 2c: Text Rendering

### 新規ファイル (4ファイル)
| ファイル | 役割 |
|----------|------|
| `GXLib/Graphics/Rendering/FontManager.h` | フォントマネージャー定義（GlyphInfo構造体含む） |
| `GXLib/Graphics/Rendering/FontManager.cpp` | DirectWriteラスタライズ → WIC Bitmap → 1024x1024テクスチャアトラス |
| `GXLib/Graphics/Rendering/TextRenderer.h` | テキストレンダラー定義 |
| `GXLib/Graphics/Rendering/TextRenderer.cpp` | SpriteBatch::DrawRectGraphでグリフ描画、DrawString/DrawFormatString |

### 設計ポイント
- フォントアトラス: 1024x1024ピクセル、ASCII(32-126)を初期化時に一括ラスタライズ
- オンデマンド: 未知文字（日本語等）は初回アクセス時にラスタライズ＆アトラス再アップロード
- ラスタライズパイプライン: DirectWrite TextLayout → D2D WicBitmapRenderTarget → ピクセル読出し → BGRA→RGBA変換 → アトラスにコピー → TextureManager::CreateTextureFromMemory
- TextRenderer: 文字列をグリフ単位でSpriteBatch::DrawRectGraphに変換、色はSetDrawColorで指定

---

## Sub-Phase 2d: Integration Demo

### 変更ファイル
- **`Sandbox/main.cpp`** — Phase 2全機能を使用するインタラクティブデモ

### デモ機能
- **入力**: WASD/矢印キーでスプライト移動、マウスクリックでテレポート
- **ゲームパッド**: 左スティックで移動、Aボタンで効果音
- **SE**: スペースキーで矩形波SE再生（プログラム生成WAV）
- **BGM**: C-E-Gコードのサイン波がループ再生（プログラム生成WAV）
- **テキスト**: FPS、プレイヤー座標、マウス座標、ゲームパッド状態、操作説明を描画
- **ミュート**: Mキーでマスターボリューム切り替え
- **ESC**: アプリ終了

### テスト用アセット
- WAVファイルはプログラムで動的生成（外部ファイル不要）
  - `test_se.wav`: 880Hz矩形波、0.2秒、減衰エンベロープ
  - `test_bgm.wav`: C-E-Gサイン波コード、4秒

---

## インフラ変更

### `GXLib/pch.h`
- `#include <xaudio2.h>` 追加
- `#include <dwrite.h>` 追加
- `#include <d2d1.h>` 追加
- `#include <fstream>` 追加

### `GXLib/CMakeLists.txt`
- GLOB_RECURSE に `Input/*.cpp`, `Input/*.h`, `Audio/*.cpp`, `Audio/*.h` 追加
- リンクライブラリ追加: `xinput.lib`, `xaudio2.lib`, `ole32.lib`, `dwrite.lib`, `d2d1.lib`, `windowscodecs.lib`

---

## ファイル構成（Phase 2で追加）

```
GXLib/
├── Input/
│   ├── Keyboard.h / .cpp
│   ├── Mouse.h / .cpp
│   ├── Gamepad.h / .cpp
│   └── InputManager.h / .cpp
├── Audio/
│   ├── AudioDevice.h / .cpp
│   ├── Sound.h / .cpp
│   ├── SoundPlayer.h / .cpp
│   ├── MusicPlayer.h / .cpp
│   └── AudioManager.h / .cpp
└── Graphics/Rendering/
    ├── FontManager.h / .cpp
    └── TextRenderer.h / .cpp
```

合計: **新規22ファイル** + **変更4ファイル**（Window.h/cpp, pch.h, CMakeLists.txt, Sandbox/main.cpp）

---

## 再利用パターン

| パターン | 適用先 | 参照元 |
|----------|--------|--------|
| ハンドル+freelist管理 | AudioManager, FontManager | TextureManager |
| Begin/Draw/End バッチ | TextRenderer → SpriteBatch | SpriteBatch |
| Window コールバック | InputManager → Window | Window::SetResizeCallback |
| CreateTextureFromMemory | FontManager アトラス | TextureManager |

## Phase 3 Summary

## Overview

Phase 3 implemented a complete 3D rendering engine on top of the Phase 2 infrastructure (2D drawing, text, input, audio). The engine supports Physically Based Rendering (PBR), Cascaded Shadow Maps (CSM), procedural skybox, fog, glTF 2.0 model loading, and skeletal animation.

---

## Sub-Phases

### Phase 3a: Basic 3D Pipeline
- **Vertex3D.h** — 3D PBR vertex format (position, normal, texcoord, tangent)
- **Transform3D.h/cpp** — Position/Rotation/Scale with world matrix computation
- **Camera3D.h/cpp** — Perspective camera with FPS-style movement
- **MeshData.h/cpp** — CPU-side mesh data + MeshGenerator (Box, Sphere, Plane)
- **Renderer3D.h/cpp** — Core 3D renderer with per-object/per-frame constant buffers
- **PBR.hlsl** — Cook-Torrance BRDF with directional/point/spot lights
- **PBRCommon.hlsli** — Shared PBR math (GGX NDF, Smith geometry, Fresnel)
- **LightingUtils.hlsli** — Light evaluation functions

### Phase 3b: Shadow Mapping
- **ShadowMap.h/cpp** — Basic shadow map (depth-only render target)
- **CascadedShadowMap.h/cpp** — 4-cascade shadow maps for large scenes
- **ShadowUtils.hlsli** — PCF soft shadows, cascade selection
- **ShadowDepth.hlsl** — Shadow depth pass vertex shader
- Shadow pipeline: separate root signature, front-face culling, depth bias

### Phase 3c: Material System & Texture Maps
- **Material.h/cpp** — PBR material (albedo, normal, metallic-roughness, AO, emissive)
- **MaterialManager** — Handle-based material management with freelist
- Texture map binding via descriptor tables (t0-t4)
- Material flags for optional texture maps (HAS_ALBEDO_MAP etc.)

### Phase 3d: glTF Model Loading & Skeletal Animation
- **Model.h** — Model container (GPUMesh + sub-meshes + skeleton + animations)
- **Mesh.h/cpp** — GPU mesh with sub-mesh support
- **ModelLoader.h/cpp** — glTF 2.0 loader using cgltf.h
- **Skeleton.h/cpp** — Joint hierarchy with inverse bind matrices
- **AnimationClip.h/cpp** — Keyframe animation (translation, rotation, scale)
- **AnimationPlayer.h/cpp** — Animation playback with bone matrix computation
- **cgltf.h** — Third-party header-only glTF parser

### Phase 3e: Environment & Effects
- **Skybox.h/cpp** — Procedural skybox (sky gradient + sun disk + corona + haze)
- **Skybox.hlsl** — Sky rendering shader
- **Fog.h** — Fog constants (Linear / Exp / Exp2 modes)
- **PrimitiveBatch3D.h/cpp** — Debug wireframe primitives (lines, boxes, spheres, grid)
- **Primitive3D.hlsl** — 3D line rendering shader
- **Terrain.h/cpp** — Heightmap-based terrain generation and rendering

---

## Architecture

### Rendering Pipeline (per frame)
```
1. Shadow Pass (per cascade 0-3)
   - Render scene depth from light's perspective
   - Depth-only output, front-face culling + depth bias
2. Main Pass
   - Clear backbuffer + depth
   - Skybox (depth write OFF, z=w trick)
   - PBR scene rendering (with CSM shadows + fog)
   - Debug primitives (wireframe, alpha blend)
   - 2D text overlay
3. Present
```

### Constant Buffer Layout
| Register | Name | Contents |
|----------|------|----------|
| b0 | ObjectConstants | World matrix, WorldInverseTranspose |
| b1 | FrameConstants | View/Proj/VP, Camera pos, Time, LightVP[4], CascadeSplits, Shadow params, Fog params |
| b2 | LightConstants | LightData[16], AmbientColor, NumLights |
| b3 | MaterialConstants | Albedo, Metallic, Roughness, AO, Emissive factors + flags |
| b4 | BoneConstants | BoneMatrices[128] (skinned models only) |

### Texture Slots
| Register | Usage |
|----------|-------|
| t0-t4 | Material textures (albedo, normal, metrough, AO, emissive) |
| t8-t11 | Shadow maps (4 cascades) |
| s0 | Linear wrap sampler |
| s2 | Shadow comparison sampler |

---

## File List

### C++ Files (GXLib/Graphics/3D/)
| File | Lines | Description |
|------|-------|-------------|
| Vertex3D.h | ~40 | PBR vertex format + input layout |
| Transform3D.h/cpp | ~120 | 3D transform with world matrix |
| Camera3D.h/cpp | ~180 | FPS camera |
| MeshData.h/cpp | ~350 | Mesh generation (Box, Sphere, Plane) |
| Mesh.h/cpp | ~100 | GPU mesh with sub-meshes |
| Model.h | ~60 | Model container |
| ModelLoader.h/cpp | ~500 | glTF 2.0 loader |
| Skeleton.h/cpp | ~80 | Joint hierarchy |
| AnimationClip.h/cpp | ~100 | Keyframe animation data |
| AnimationPlayer.h/cpp | ~150 | Animation playback |
| Light.h/cpp | ~80 | Light types (Directional, Point, Spot) |
| Material.h/cpp | ~120 | PBR material + manager |
| Fog.h | ~30 | Fog constants |
| ShadowMap.h/cpp | ~100 | Depth-only render target |
| CascadedShadowMap.h/cpp | ~250 | 4-cascade shadow system |
| Skybox.h/cpp | ~140 | Procedural skybox |
| PrimitiveBatch3D.h/cpp | ~190 | Debug wireframes |
| Terrain.h/cpp | ~150 | Heightmap terrain |
| Renderer3D.h/cpp | ~550 | Main 3D renderer |

### Shader Files (Shaders/)
| File | Description |
|------|-------------|
| PBR.hlsl | Cook-Torrance BRDF + lighting + CSM + fog |
| PBRCommon.hlsli | NDF, Geometry, Fresnel functions |
| LightingUtils.hlsli | EvaluateLight for all light types |
| ShadowUtils.hlsli | ComputeCascadedShadow with PCF |
| ShadowDepth.hlsl | Shadow pass VS (depth only) |
| Skybox.hlsl | Procedural sky + sun |
| Primitive3D.hlsl | 3D line rendering |

### Third-Party
| File | Description |
|------|-------------|
| cgltf.h | glTF 2.0 parser (header-only) |

---

## Key Design Decisions

1. **PBR Cook-Torrance BRDF** — Industry-standard physically based shading model
2. **Cascaded Shadow Maps (4 cascades)** — Good shadow quality at all distances
3. **Ring buffer for object constants** — Efficient per-object CB updates without per-draw allocation
4. **Shared SRV heap** — TextureManager's SRV heap also hosts shadow map SRVs
5. **Procedural skybox** — No cubemap textures needed, gradient + sun computed in shader
6. **Inline tonemapping** — PBR.hlsl and Skybox.hlsl currently contain Reinhard tonemapping + gamma correction (to be moved to post-effect pipeline in Phase 4)

## Known Limitations (addressed in Phase 4)

- Tonemapping is inline in PBR.hlsl/Skybox.hlsl (no HDR pipeline)
- No post-processing effects (bloom, FXAA, SSAO, etc.)
- Backbuffer is R8G8B8A8_UNORM (LDR), bright specular highlights are clamped after tonemapping

## Phase 4a Summary

## Overview

Phase 4aでは、シーン描画をHDR浮動小数点レンダーターゲット (R16G16B16A16_FLOAT) に切り替え、ポストエフェクトパイプラインの基盤を構築した。トーンマッピング（Reinhard/ACES/Uncharted2）をフルスクリーン三角形パスとして実装し、LDRバックバッファへ出力する。

## 新規ファイル

| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | ポストエフェクトパイプライン管理クラス |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | HDR RT作成、BeginScene/EndScene/Resolve、トーンマッピングPSO |
| `Shaders/Fullscreen.hlsli` | SV_VertexIDベースのフルスクリーン三角形VS（VB不要） |
| `Shaders/Tonemapping.hlsl` | 3種トーンマッピング + ガンマ補正PS |

## 変更ファイル

| File | Change |
|------|--------|
| `GXLib/Graphics/Resource/RenderTarget.h/cpp` | リソースステート管理 (m_currentState, TransitionTo()) 追加 |
| `Shaders/PBR.hlsl` (L241-245) | インラインReinhard+ガンマ削除、HDRリニア値出力 |
| `Shaders/Skybox.hlsl` (L64-68) | 同上 |
| `GXLib/Graphics/3D/Renderer3D.cpp` | PBR PSO → R16G16B16A16_FLOAT |
| `GXLib/Graphics/3D/Skybox.cpp` | Skybox PSO → R16G16B16A16_FLOAT |
| `GXLib/Graphics/3D/PrimitiveBatch3D.cpp` | PrimitiveBatch3D PSO → R16G16B16A16_FLOAT |
| `Sandbox/main.cpp` | HDR RT描画→PostEffect→Tonemapping→LDRフロー、1/2/3キーでトーンマップ切替、+/-で露出調整 |

## 描画フロー

```
Shadow Pass (x4 cascades)
  → HDR RT Clear + DSV Clear
  → Skybox (HDR)
  → PBR 3D Scene (HDR)
  → Debug Primitives (HDR)
  → EndScene (HDR RT → SRV)
  → Backbuffer → RENDER_TARGET
  → Tonemapping (HDR SRV → LDR Backbuffer)
  → 2D Text Overlay (LDR)
  → Backbuffer → PRESENT
```

## 操作

| Key | Action |
|-----|--------|
| 1 | Reinhard tonemapping |
| 2 | ACES Filmic tonemapping |
| 3 | Uncharted2 tonemapping |
| +/- | Exposure調整 |

## Phase 4b Summary

## Overview
HDRシーンの明るい部分から光の滲み(Bloom)エフェクトを生成するポストエフェクトを実装。

## New Files

| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/Bloom.h` | Bloomエフェクトクラス定義 |
| `GXLib/Graphics/PostEffect/Bloom.cpp` | Bloom実装（Threshold→Downsample→Blur→Upsample→Composite） |
| `Shaders/Bloom.hlsl` | Bloomシェーダー（PSThreshold, PSDownsample, PSGaussianBlurH/V, PSAdditive） |

## Architecture

### Processing Pipeline
1. **Threshold**: HDRシーンから閾値以上の明るい部分を抽出 → mipRT[0]
2. **Downsample**: 5段階のダウンサンプル (1/2 → 1/4 → 1/8 → 1/16 → 1/32)
3. **Gaussian Blur**: 各レベルで水平/垂直の9タップGaussianブラー
4. **Upsample**: 小→大の順にアディティブ合成（mipRT[4]→mipRT[3]→...→mipRT[0]）
5. **Composite**: HDRシーンをdestにコピーし、mipRT[0]をアディティブブレンドで合成

### Key Design Decisions
- **Additive Blend PSO**: D3D12では1つのCBV_SRV_UAVヒープしかバインドできないため、2テクスチャ合成を避け、Additive Blend PSOで描画する方式を採用
- **DynamicBuffer frame index 0 固定**: Bloom全パスは1フレーム内で連続実行されるため、frame index 0のみ使用
- **Gaussian weights**: `{0.227027, 0.194596, 0.121622, 0.054054, 0.016216}` (9タップ)

### Parameters
- `threshold` (default: 1.0): 輝度閾値。これ以上の明るさのピクセルのみBloom対象
- `intensity` (default: 0.5): Bloom合成時の強度

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | Bloom メンバ追加、GetBloom() アクセサ |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | Bloom.Execute() をResolve()内で呼び出し、hdrPingPongRTをdestとして使用 |
| `GXLib/Graphics/Pipeline/PipelineState.h/cpp` | SetAdditiveBlend() メソッド追加 |
| `Sandbox/main.cpp` | キー4でBloomトグル、HUDにBloom状態表示 |

## Verification
- Build: OK
- Runtime: Bloom ON/OFFで明るい部分にグロー効果が確認できた

## Phase 4cde Summary

## Overview
3つのポストエフェクトを追加:
- **FXAA** (Fast Approximate Anti-Aliasing): トーンマッピング後のLDR画像に適用するアンチエイリアシング
- **Vignette + Chromatic Aberration**: 画面端の周辺減光 + R/G/Bチャンネルの色収差
- **Color Grading**: HDR空間でのカラーグレーディング（露出・コントラスト・彩度・色温度）

## New Files

| File | Description |
|------|-------------|
| `Shaders/FXAA.hlsl` | FXAA 3.11 Quality実装（輝度ベースのエッジ検出 + サブピクセルAA） |
| `Shaders/Vignette.hlsl` | ビネット（距離ベース周辺減光） + 色収差（R/G/B異UVサンプリング） |
| `Shaders/ColorGrading.hlsl` | 露出補正、コントラスト、彩度、色温度調整 |

## Architecture

### Effect Chain (Updated)
```
HDR Scene → [Bloom] → [ColorGrading(HDR)] → [Tonemapping(HDR→LDR)]
          → [FXAA(LDR)] → [Vignette+ChromAb(LDR)] → Backbuffer
```

### Ping-Pong RT Strategy
- **HDR**: `m_hdrRT` と `m_hdrPingPongRT` でHDRエフェクトをピンポン
- **LDR**: `m_ldrRT[0]` と `m_ldrRT[1]` でLDRエフェクトをピンポン
- **最終パス最適化**: チェーンの最後のエフェクトは中間RTを介さず直接バックバッファRTVに描画

### DrawFullscreenToRTV
バックバッファへの直接描画用ヘルパー関数を追加。`DrawFullscreen()`は`RenderTarget&`を要求するため、生の`D3D12_CPU_DESCRIPTOR_HANDLE`でバックバッファに直接描画する`DrawFullscreenToRTV()`を追加。

### FXAA Details
- 周囲9点(N,S,W,E,NW,NE,SW,SE,M)の輝度を計算
- 輝度差が閾値以下なら早期リターン（エッジなし）
- エッジ方向（水平/垂直）を判定
- サブピクセルAAとエッジに沿ったブレンドを適用

### Vignette Details
- `smoothstep(radius, radius+0.4, dist*1.414)` で距離ベースの減光
- `lerp(1, vignette, intensity)` でintensity=0でエフェクト無効化
- 色収差: UV中心からの方向に沿って R/B チャンネルをオフセット

### Color Grading Details
- HDR空間で適用（トーンマッピング前）
- 露出: `exp2(exposure)` で乗算
- コントラスト: `(color - 0.5) * contrast + 0.5` で0.5を基準にスケーリング
- 彩度: 輝度とのlerp
- 色温度: R/Bチャンネルのバランス調整（簡易版）

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | FXAA/Vignette/ColorGrading用のPSO, CB, パラメータ追加。LDR RT[2]追加。DrawFullscreenToRTV()追加 |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | 全エフェクトチェーン実装。Resolve()で最後のパスを直接バックバッファに描画する最適化 |
| `Sandbox/main.cpp` | キー5:FXAA, 6:Vignette, 7:ColorGrading トグル。HUDに全エフェクト状態表示 |

## Parameters

| Effect | Parameter | Default | Range |
|--------|-----------|---------|-------|
| FXAA | qualitySubpix | 0.75 | 0-1 |
| FXAA | edgeThreshold | 0.166 | 0.063-0.333 |
| Vignette | intensity | 0.5 | 0-1 |
| Vignette | radius | 0.8 | 0-1 |
| ChromAb | chromaticStrength | 0.003 | 0-0.01 |
| ColorGrading | contrast | 1.0 | 0.5-2.0 |
| ColorGrading | saturation | 1.0 | 0-2.0 |
| ColorGrading | temperature | 0.0 | -1 to 1 |

## Verification
- Build: OK (全ファイルコンパイル成功)
- Runtime: 各エフェクトのON/OFF切替が期待通り動作すること（要実行確認）

## Phase 4fg Summary

## Overview
2つのサブフェーズを完了:
- **Phase 4f**: SSAO (Screen Space Ambient Occlusion) - Hemisphere SSAO with bilateral blur
- **Phase 4g**: All-light shadows (Spot + Point shadow maps) + Cylinder winding order fix

## Phase 4f: SSAO

### Implementation
- **Algorithm**: Hemisphere SSAO (64 samples per pixel)
- **Noise**: Hash-based rotation (no noise texture needed)
- **Blur**: Bilateral blur (depth-aware, preserves edges)
- **Composite**: Multiply blend onto HDR scene
- **Input**: Depth buffer SRV + Camera projection/inverse-projection matrices

### New Files
| File | Description |
|------|-------------|
| `Shaders/SSAO.hlsl` | SSAO generation (hemisphere sampling) + bilateral blur + composite passes |
| `GXLib/Graphics/PostEffect/SSAO.h` | SSAO class declaration |
| `GXLib/Graphics/PostEffect/SSAO.cpp` | SSAO initialization, PSO creation, render passes |

### Architecture
- DepthBuffer has `CreateWithOwnSRV()` for shader-visible SRV heap
- DepthBuffer has `TransitionTo()` for explicit state tracking
- PostEffectPipeline::Resolve() takes DepthBuffer & Camera3D args for SSAO
- SSAO uses own half-res RenderTarget for AO map
- 3-pass: Generate AO → Bilateral Blur → Multiply Composite onto scene

### Parameters
| Parameter | Default | Range |
|-----------|---------|-------|
| radius | 0.5 | 0.1-2.0 |
| bias | 0.025 | 0-0.1 |
| intensity | 1.0 | 0-3.0 |
| sampleCount | 64 | 16-128 |

## Phase 4g: All-Light Shadows

### Spot Shadow
- Resolution: 2048x2048
- Single depth-only pass with spot light view/projection matrix
- PCF filtering in PBR shader
- SRV slot: t12

### Point Shadow
- Resolution: 1024x1024 per face
- **Texture2DArray** with 6 slices (one per cube face)
- Own DSV heap with 6 DSV descriptors
- 6 render passes (one per face), face selected by dominant axis
- View matrices: +X, -X, +Y, -Y, +Z, -Z with 90-degree FOV
- PCF filtering with face selection by dominant axis in PBR shader
- SRV slot: t13 (Texture2DArray)

### Shadow CB Layout
- FrameConstants: 1008 bytes total
  - CSM: 528 bytes (4 cascades × 132 bytes)
  - Spot: 80 bytes (ViewProj + params)
  - Point: 400 bytes (6 face ViewProj matrices + params)
- Shadow pass CB: 11 slots × 256 bytes each
  - Slots 0-3: CSM cascades
  - Slot 4: Spot shadow
  - Slots 5-10: Point shadow faces

### SRV Layout in PBR Shader
| Slot | Content |
|------|---------|
| t8-t11 | CSM shadow maps (4 cascades) |
| t12 | Spot shadow map |
| t13 | Point shadow map (Texture2DArray, 6 slices) |

## Cylinder Winding Order Fix

### Problem
`CreateCylinder()` had reversed triangle winding for side faces and bottom cap. With `D3D12_CULL_MODE_BACK`, exterior faces were culled, showing interior faces with inverted lighting.

### Verification Method
Computed screen-space signed areas comparing with working CreateBox:
- Box right face: signed area < 0 = front-facing from outside (correct)
- Cylinder side face: signed area > 0 = back-facing from outside (WRONG)
- Cylinder bottom cap: back-facing from below (WRONG)
- Cylinder top cap: front-facing from above (correct)

### Fix (MeshData.cpp)
**Side faces** - swapped 2nd and 3rd vertex of each triangle:
```cpp
// Before: base+j, base+j+ringCount, base+j+1
// After:  base+j, base+j+1, base+j+ringCount

// Before: base+j+1, base+j+ringCount, base+j+ringCount+1
// After:  base+j+1, base+j+ringCount+1, base+j+ringCount
```

**Bottom cap** - swapped 2nd and 3rd vertex:
```cpp
// Before: center, j+1, j+2
// After:  center, j+2, j+1
```

**Top cap** - no change needed (already correct)

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/SSAO.h` | New: SSAO class |
| `GXLib/Graphics/PostEffect/SSAO.cpp` | New: SSAO implementation |
| `Shaders/SSAO.hlsl` | New: SSAO shader (generate + blur + composite) |
| `GXLib/Graphics/Resource/DepthBuffer.h` | Added CreateWithOwnSRV(), TransitionTo(), GetSRVGPUHandle() |
| `GXLib/Graphics/Resource/DepthBuffer.cpp` | Own SRV heap, state tracking |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | Added SSAO member, Resolve() signature change |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | SSAO integration in effect chain |
| `GXLib/Graphics/3D/Renderer3D.h/cpp` | Spot + Point shadow map rendering passes |
| `GXLib/Graphics/3D/PointShadowMap.h/cpp` | New: Point shadow map (6-face Texture2DArray) |
| `GXLib/Graphics/3D/Light.h` | SpotLight/PointLight shadow params |
| `Shaders/PBR.hlsl` | Spot + Point shadow sampling, face selection |
| `Shaders/Shadow.hlsl` | Shadow pass vertex shader |
| `GXLib/Graphics/3D/MeshData.cpp` | Cylinder winding order fix (sides + bottom cap) |
| `Sandbox/main.cpp` | SSAO toggle (key 8), shadow demo scene |

## Effect Chain (Updated)
```
HDR Scene → [SSAO(multiply)] → [Bloom] → [ColorGrading(HDR)]
          → [Tonemapping(HDR→LDR)] → [FXAA(LDR)]
          → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Verification
- Build: OK
- SSAO: Hemisphere sampling produces visible darkening in corners/crevices
- Spot Shadow: 2048x2048, PCF filtered
- Point Shadow: 6-face 1024x1024 Texture2DArray, dominant-axis face selection
- Cylinder: Correct winding - sides and bottom visible with proper lighting

## Phase 4h Summary

## Overview
被写界深度 (Depth of Field) ポストエフェクトを実装。
フォーカス距離から離れた領域をぼかし、カメラのレンズ効果をシミュレートする。

## Algorithm
1. **CoC生成**: 深度バッファ → ビュー空間Z復元 → フォーカス距離からのずれ → Circle of Confusion (R16_FLOAT)
2. **ブラー**: CoC加重ガウシアンブラー (H/V分離, half-res)
3. **合成**: CoC値でシャープHDRとブラーHDRをlerp

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/DepthOfField.h` | DoF class declaration |
| `GXLib/Graphics/PostEffect/DepthOfField.cpp` | DoF initialization, 3-pass execution |
| `Shaders/DepthOfField.hlsl` | CoC generation + blur (H/V) + composite shaders |

## Architecture
- 3-pass pipeline: CoC → BlurH → BlurV → Composite
- CoC map: R16_FLOAT full-resolution
- Blur: half-resolution HDR RTs for performance
- Composite: 3-SRV combined heap (sharp + blurred + CoC)
- CopyDescriptors で shader-visible ヒープにSRVをまとめる
- DoF/MotionBlur共通パターン: 専用 DescriptorHeap で複数テクスチャをバインド

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| focalDistance | 10.0 | フォーカス距離 (ビュー空間Z) |
| focalRange | 5.0 | フォーカス鮮明範囲 |
| bokehRadius | 8.0 | ボケの最大半径 (ピクセル) |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/DepthOfField.h` | New |
| `GXLib/Graphics/PostEffect/DepthOfField.cpp` | New |
| `Shaders/DepthOfField.hlsl` | New |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | DoF member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | DoF integration (after Bloom, before ColorGrading) |
| `Sandbox/main.cpp` | 0キーでDoFトグル、F/Gキーでフォーカス距離調整 |

## Effect Chain Position
```
HDR Scene → [SSAO(multiply)] → [Bloom] → [DoF] → [ColorGrading(HDR)]
          → [Tonemapping(HDR→LDR)] → [FXAA(LDR)]
          → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Verification
- Build: OK
- DoF ON/OFF: 0キーでトグル動作確認
- フォーカス距離調整: F/Gキーで遠近のボケが変化
- Half-res blur: パフォーマンス良好
- D3D12 Debug Layer: エラーなし

## Phase 4i Summary

## Overview
カメラベースのモーションブラーポストエフェクトを実装。
深度再投影方式により、カメラ移動に応じたブラー効果を生成する。

## Algorithm
1. 深度バッファからワールド座標を再構成 (invViewProjection)
2. 前フレームのVP行列でスクリーン座標に再投影
3. 現在位置と前フレーム位置の差分から速度ベクトルを計算
4. 速度方向にHDRシーンをブラー (N samples)
5. スカイボックス (depth >= 1.0) はブラーをスキップ

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/MotionBlur.h` | MotionBlur class declaration |
| `GXLib/Graphics/PostEffect/MotionBlur.cpp` | MotionBlur initialization, execution |
| `Shaders/MotionBlur.hlsl` | Depth reprojection + velocity blur shader |

## Architecture
- DoFと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots × 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理 (Bloom/DoFの後、ColorGradingの前)

## Key Design Decision: UpdatePreviousVP のタイミング
- `UpdatePreviousVP()` は `Execute()` の**後**に呼ぶ必要がある
- 先に呼ぶと今フレームのVPで上書きされ、速度=0になりブラーが効かない
- PostEffectPipeline::Resolve() 内で Execute() 後に呼び出し

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| intensity | 1.0 | ブラー強度 |
| sampleCount | 16 | ブラーサンプル数 |
| enabled | false | デフォルトOFF (パフォーマンス考慮) |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/MotionBlur.h` | New |
| `GXLib/Graphics/PostEffect/MotionBlur.cpp` | New |
| `Shaders/MotionBlur.hlsl` | New |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | MotionBlur member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | MotionBlur integration (after DoF) |
| `Sandbox/main.cpp` | Bキーでトグル、HUD表示追加 |

## Effect Chain Position
```
HDR Scene → [SSAO(multiply)] → [Bloom] → [DoF] → [MotionBlur]
          → [ColorGrading(HDR)] → [Tonemapping(HDR→LDR)]
          → [FXAA(LDR)] → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Lessons Learned
- **UpdatePreviousVP timing**: Execute後に呼ぶ。先に呼ぶと速度=0になる
- **スカイボックス除外**: depth >= 1.0 をチェックしてブラーをスキップ
- **速度クランプ**: 最大速度を画面の10%に制限し、極端なブラーを防止
- **2-SRV専用ヒープ**: D3D12ではCBV_SRV_UAVヒープは1つしかバインドできないため、複数テクスチャ使用時は専用ヒープにまとめる

## Verification
- Build: OK
- Bキーでトグル: ON/OFF切り替え動作確認
- カメラ移動時: 速度に応じたブラー効果
- 静止時: ブラーなし (速度ベクトル ≈ 0)
- スカイボックス: ブラーされない
- D3D12 Debug Layer: エラーなし

## Phase 4j Summary

## Overview
スクリーン空間レイマーチングによる反射ポストエフェクトを実装。
Forward+レンダリング（GBuffer無し）環境向けに、深度勾配から法線を再構成する方式。

## Algorithm
1. 深度バッファからビュー空間位置を再構成
2. ±8pxカーネルで深度勾配からビュー空間法線を再構成（中央/片側差分のスムーズブレンド）
3. 反射方向を計算し、レイをスクリーン空間に射影
4. スクリーン空間DDAレイマーチング（1ピクセル/ステップ）
5. ビュー空間Z比較で深度バッファとの交差判定
6. バイナリ精緻化（8反復）でヒット位置を精密化
7. フレネル効果 + エッジフェード + 距離フェード + エッジ信頼度で反射強度を制御

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/SSR.h` | SSR class declaration, SSRConstants struct |
| `GXLib/Graphics/PostEffect/SSR.cpp` | SSR initialization, SRV heap, execution |
| `Shaders/SSR.hlsl` | Screen-space DDA ray marching shader |

## Architecture
- DoF/MotionBlurと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots × 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理（SSAO後、Bloom前）
- 定数バッファ: 224B (3行列 + 8パラメータ) → 256-align

## Key Design Decisions

### Screen-space DDA Ray Marching
- ビュー空間ステッピングでは画面上の不均一なカバレッジでゴースティングが発生
- スクリーン空間DDA（1ピクセル/ステップ）で均一なサンプリングを実現
- 射影深度の線形補間で透視投影正確な深度比較

### 法線再構成（±8px カーネル）
- GBufferが無いため深度勾配から法線を近似
- ±1px: ポリゴンファセットが見える（階段状アーティファクト）
- ±16px 2レベル: カーネル切替境界にアウトラインが出る
- ±8px スムーズブレンド: 中央差分と片側差分をビュー空間Z重みで滑らかに混合

### ビュー空間Zエッジ検出
- 深度バッファ値での固定閾値は遠距離で破綻（非線形圧縮のため）
- ビュー空間Z座標で比較（距離非依存、閾値0.3ユニット）
- エッジ信頼度を法線再構成と統一（±8pxサンプルから算出）

### フレネル反射
- F0 = 0.3（可視性のため高めに設定）
- Schlick近似: `F = F0 + (1-F0) * (1-NdotV)^5`
- グレージング角ほど強い反射

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| maxDistance | 30.0 | レイの最大距離 (view-space) |
| stepSize | 0.15 | 未使用（DDA方式では自動計算） |
| maxSteps | 200 | 最大ステップ数 |
| thickness | 0.15 | ヒット判定の厚み (view-space units) |
| intensity | 1.0 | 反射強度 |
| enabled | false | デフォルトOFF (パフォーマンス考慮) |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | SSR member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | SSR integration (after SSAO, before Bloom) |
| `Sandbox/main.cpp` | Rキーでトグル、HUD表示追加、SSRデモオブジェクト(ミラーウォール+カラフル球体) |

## Effect Chain Position
```
HDR Scene → [SSAO(multiply)] → [SSR] → [Bloom] → [DoF] → [MotionBlur]
          → [ColorGrading(HDR)] → [Tonemapping(HDR→LDR)]
          → [FXAA(LDR)] → [Vignette+ChromAb(LDR)] → Backbuffer
```

## Lessons Learned
- **Screen-space DDA > View-space stepping**: ビュー空間ステッピングは画面カバレッジが不均一でゴースト/トレイルが発生する。スクリーン空間DDA（1px/step）が正解
- **法線カーネルサイズ**: 小さすぎ(±1px)→ポリゴン見える、大きすぎ(±16px)→アウトライン。±8pxが妥協点
- **2レベルカーネルのハード切替は禁物**: 16px→2pxのハード切替で枠線が出る。lerp()でスムーズに混合すべき
- **深度バッファの非線形性**: 深度バッファ値での固定閾値は遠距離で破綻する。ビュー空間Zで比較すれば距離非依存
- **ViewZFromDepth()**: invProjection の (2,2)/(3,2) 成分だけで安価にビュー空間Zを取得可能
- **バイナリ精緻化**: 8反復でサブピクセル精度のヒット位置を取得。視覚的品質が大幅向上
- **エッジ信頼度の統一**: 法線再構成とエッジ検出を同じ±8pxサンプルから計算することで、範囲の不一致によるアーティファクトを防止

## Verification
- Build: OK
- Rキーでトグル: ON/OFF切り替え動作確認
- ミラーウォール: カラフル球体の反射が見える
- 床面: グレージング角で反射が強くなる（フレネル効果）
- 画面端: フェードアウトする
- スカイボックス: SSR対象外 (depth >= 1.0)
- D3D12 Debug Layer: エラーなし

## Phase 4k Summary

## Overview
深度バッファから法線を再構成し、Sobelエッジ検出でアウトラインを合成するポストエフェクト。
Forward+レンダリング（GBuffer無し）環境向け。トゥーン/NPR表現に活用可能。

## Algorithm
1. 3x3近傍の深度をビュー空間Zに変換
2. Sobelオペレータで深度エッジを検出（距離非依存）
3. 中心+4近傍で法線を再構成し、`1 - dot(center, neighbor)` の最大値で法線エッジを検出
4. `edge = max(depthFactor, normalFactor) * intensity`
5. `lerp(sceneColor, lineColor, edge)` で合成
6. スカイボックス (depth >= 1.0) はスキップ

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/OutlineEffect.h` | OutlineEffect class, OutlineConstants struct (112B) |
| `GXLib/Graphics/PostEffect/OutlineEffect.cpp` | Initialization, SRV heap, execution (SSR pattern) |
| `Shaders/Outline.hlsl` | Sobel depth edge + normal edge detection shader |

## Architecture
- SSR/MotionBlurと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots x 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理（MotionBlur後、ColorGrading前）
- 定数バッファ: 112B (invProjection 64B + params + lineColor) -> 256-align

## Key Design Decisions

### Sobel Depth Edge (View-space Z)
- 深度バッファ値ではなくビュー空間Zに変換してからSobelを適用
- 距離に依存しないエッジ検出（遠距離でもアウトラインの太さが一定）
- Sobelカーネル: 標準3x3 (Gx/Gy)

### Normal Edge Detection
- 中心ピクセルの法線と4近傍の法線を再構成
- `1 - dot(normalCenter, normalNeighbor)` で法線差を計算
- 4方向の最大値をエッジ値とする
- スカイボックス近傍はスキップ（depth >= 1.0）

### Normal Reconstruction
- SSRと同じ片側差分選択方式（±1px）
- 深度差が小さい側を選択し、エッジ跨ぎを防止
- `cross(dx, dy)` で法線計算、カメラ方向に修正

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| depthThreshold | 0.5 | ビュー空間Z差のSobelエッジ閾値 |
| normalThreshold | 0.3 | 法線ドット積エッジ閾値 |
| intensity | 1.0 | アウトライン強度 |
| lineColor | (0,0,0,1) | アウトライン色（黒） |
| enabled | false | デフォルトOFF |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | OutlineEffect member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | Outline integration (after MotionBlur, before ColorGrading) |
| `Sandbox/main.cpp` | Oキーでトグル、HUD表示追加(Y=260)、ShadowDebug行をY=285に移動 |

## Effect Chain Position
```
HDR Scene -> [SSAO(multiply)] -> [SSR] -> [Bloom] -> [DoF] -> [MotionBlur] -> [Outline]
          -> [ColorGrading(HDR)] -> [Tonemapping(HDR->LDR)]
          -> [FXAA(LDR)] -> [Vignette+ChromAb(LDR)] -> Backbuffer
```

**配置理由:**
- MotionBlur後: アウトラインがブラーされない（幾何学的エッジは常にシャープ）
- ColorGrading前: アウトライン色もカラーグレーディング+トーンマッピングの影響を受ける

## Verification
- Build: OK (cmake -B build && cmake --build build --config Debug)
- Oキーでトグル: ON/OFF切り替え
- 球体・箱・柱のシルエットにアウトラインが表示される
- 箱の面境界（法線変化）にもエッジが検出される
- スカイボックス: アウトライン対象外 (depth >= 1.0)
- 遠距離でもアウトライン太さが一定（ビュー空間Z使用）

## Phase 4l Summary

## Overview
GPU Gems 3 のスクリーン空間ラディアルブラー方式で、ディレクショナルライトからのゴッドレイ（光の筋）を合成するポストエフェクト。
太陽のスクリーン位置を基点に、深度バッファで遮蔽判定しながらラディアルブラーを適用する。

## Algorithm
1. CPU側: ライト逆方向の遠方点 (`camPos - lightDir*1000`) をVP行列で射影 → NDC → UV変換
2. CPU側: ビュー空間Zで前方チェック + 画面外フェードアウトで太陽可視性を計算
3. GPU側: ピクセルUVから太陽スクリーン位置へ向かうレイを計算
4. GPU側: ハッシュノイズでピクセル毎にレイ開始位置をジッター（バンディング軽減）
5. GPU側: `numSamples` ステップでレイマーチ、各ステップで深度をリニアサンプリング
6. GPU側: `smoothstep(0.99, 1.0, depth)` で遮蔽判定（スカイ=光、オブジェクト=遮蔽）
7. GPU側: `weight * illuminationDecay` でステップ毎の寄与を蓄積、`decay` で減衰
8. GPU側: 最終色 = `godRay * exposure * intensity * sunVisible * lightColor`
9. シーンに加算合成: `result = sceneColor.rgb + finalGodRay`

## New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/VolumetricLight.h` | VolumetricLight class, VolumetricLightConstants struct (48B) |
| `GXLib/Graphics/PostEffect/VolumetricLight.cpp` | Initialization, SRV heap, sun position calculation, execution |
| `Shaders/VolumetricLight.hlsl` | Radial blur god ray shader with jitter + linear depth sampling |

## Architecture
- SSR/MotionBlur/OutlineEffectと同じ 2-SRV 専用ヒープパターン (scene + depth)
- SRVヒープ: 2 slots x 2 frames = 4 slots
- RS: b0(CB) + DescTable(t0=scene, t1=depth) + s0(linear) + s1(point)
- HDR空間で処理（SSR後、Bloom前）
- 定数バッファ: 48B (sunScreenPos 8B + params 24B + lightColor 12B + sunVisible 4B) -> 256-align

## Key Design Decisions

### Sun Screen Position (CPU側)
- 太陽ワールド位置 = `camPos - normalize(lightDir) * 1000` (光の逆方向の遠方)
- VP行列で射影 → NDC → UV変換 (`x*0.5+0.5`, `-y*0.5+0.5`)
- PostEffectPipelineはRenderer3Dにアクセスできないため、ライト方向はSetLightDirection()で外部設定

### Sun Visibility
- ビュー空間Zで前方チェック (z <= 0 → visible = 0)
- 画面内(距離~0.7)は100%、画面外は距離に応じてフェードアウト、距離2.0以上で0
- UpdateSunInfo()を毎フレーム呼出し（enabled状態に関係なく）

### Jitter + Linear Sampling (品質改善)
- 初期実装ではバンディング（ジャギー）が発生
- **ハッシュノイズジッター**: ピクセル毎にレイ開始位置をランダムオフセット → 規則的バンディングをノイズに変換
- **リニア深度サンプリング**: PointSamplerからLinearSamplerに変更 → オブジェクトエッジで自然なブレンド
- **smoothstep遷移**: binary判定からsmoothstep(0.99, 1.0)に変更 → 滑らかな遮蔽遷移

### C++ saturate()問題
- `saturate()` はHLSLイントリンシックで、C++では使用不可
- `(std::max)(0.0f, (std::min)(1.0f, value))` で代替

## Parameters
| Parameter | Default | Description |
|-----------|---------|-------------|
| density | 1.0 | 散乱密度（レイが太陽位置まで伸びる） |
| decay | 0.97 | 穏やかな減衰（画面の約60%まで可視） |
| weight | 0.04 | 控えめなサンプル寄与で過度な明るさを防止 |
| exposure | 0.35 | 保守的な値でシーンが洗い流されない |
| numSamples | 96 | 品質/パフォーマンスのバランス |
| intensity | 1.0 | ニュートラル、調整しやすい |
| lightColor | (1.0, 0.98, 0.95) | 暖色系の太陽光 |
| lightDirection | (0.3, -1.0, 0.5) | デフォルトライト方向 |
| enabled | false | パフォーマンス考慮でデフォルトOFF |

## Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | VolumetricLight member + accessor added |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | VolumetricLight integration (after SSR, before Bloom); UpdateSunInfo() called every frame |
| `Sandbox/main.cpp` | Vキーでトグル、SetLightDirection/Color初期設定、HUD表示追加(Y=285 GodRay状態+SunUV+Visible)、ShadowDebug行をY=310に移動、ヘルプに"V: GodRays"追加 |

## Effect Chain Position
```
HDR Scene -> [SSAO(multiply)] -> [SSR] -> [VolumetricLight] -> [Bloom] -> [DoF] -> [MotionBlur] -> [Outline]
          -> [ColorGrading(HDR)] -> [Tonemapping(HDR->LDR)]
          -> [FXAA(LDR)] -> [Vignette+ChromAb(LDR)] -> Backbuffer
```

**配置理由:**
- SSR後: 反射は既に計算済み
- Bloom前: ゴッドレイの明るい光がBloomに自然に寄与する

## Verification
- Build: OK (cmake -B build && cmake --build build --config Debug)
- Vキーでトグル: ON/OFF切り替え
- カメラを太陽方向に向けるとゴッドレイが表示される
- オブジェクトのシルエットによりレイが遮蔽される
- カメラが太陽の反対方向を向くとエフェクトがフェードアウト
- Bloom ON時にゴッドレイがBloomで光る
- ジッター+リニアサンプリングにより、バンディングやジャギーが抑制されている
- HUDにSunUV/Visible情報がリアルタイム表示される

## Phase 4mno Summary

## Overview
Phase 4 の最終3タスクを実装。Temporal Anti-Aliasing (TAA)、自動露出 (Auto-Exposure)、JSON設定ファイルの読み書き。
これにより Phase 4 の完了条件「全エフェクトがON/OFFでき、ビジュアルが劇的に向上する。JSON設定ファイルで制御可能」を達成。

---

## Phase 4m: TAA (Temporal Anti-Aliasing)

### Algorithm
1. Halton(2,3) 数列で8サンプルサイクルのサブピクセルジッターを生成
2. ジッターをプロジェクション行列の _31, _32 に加算 (`Camera3D::GetJitteredProjectionMatrix`)
3. Renderer3D が jittered VP でシーンをレンダリング（シャドウパスは非ジッター）
4. 深度バッファからリプロジェクション: 現フレーム逆VP → ワールド → 前フレームVP → historyUV
5. 3x3近傍の variance clipping (mu +/- gamma*sigma, gamma=1.0) で履歴をクランプ
6. lerp(current, clampedHistory, blendFactor=0.9) でブレンド
7. 画面外の historyUV は current をそのまま使用
8. TAA出力を historyRT に CopyResource で保存

### New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/TAA.h` | TAA class, TAAConstants struct (160B), Halton generator |
| `GXLib/Graphics/PostEffect/TAA.cpp` | 3-SRV dedicated heap (scene+history+depth), Execute with history copy |
| `Shaders/TAA.hlsl` | Variance clipping + depth reprojection + unjittered sampling |

### Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/3D/Camera3D.h` | SetJitter/ClearJitter/GetJitter/GetJitteredProjectionMatrix + m_jitterOffset member |
| `GXLib/Graphics/3D/Camera3D.cpp` | GetJitteredProjectionMatrix implementation (proj._31/_32 offset) |
| `GXLib/Graphics/3D/Renderer3D.cpp` | FrameConstants uses jittered VP (line 521-522) |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | TAA member + accessor, BeginScene signature: Camera3D& (non-const) |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | TAA init/execute/OnResize, BeginScene applies jitter, Resolve integrates TAA after Outline |
| `Sandbox/main.cpp` | T key toggle, BeginScene(camera) call, HUD Y=310, help updated |

### Key Design Decisions
- **BeginScene signature change**: `const Camera3D&` → `Camera3D&` (non-const) for jitter application
- **Non-jittered VP for TAA CB**: TAA uses `camera.GetViewProjectionMatrix()` (non-jittered) for reprojection
- **3-SRV heap**: scene + history + depth, 3 slots x 2 frames = 6 slots
- **First frame handling**: CopyResource src→dest→history without shader execution
- **Effect chain position**: After Outline, before ColorGrading (all HDR spatial effects complete)

---

## Phase 4n: AutoExposure (Eye Adaptation)

### Algorithm
1. HDR scene → 256x256 log luminance RT (R16_FLOAT): `log(max(luminance, 0.0001))`
2. Downsample chain: 256 → 64 → 16 → 4 → 1 (bilinear sampling, 4 passes)
3. 1x1 RT → readback buffer copy (2-frame ring buffer, no GPU stall)
4. CPU reads previous frame's readback: half-float → float conversion
5. targetExposure = keyValue (0.18) / exp(avgLogLuminance)
6. Exponential smoothing: current += (target - current) * (1 - exp(-speed * dt))
7. Clamp to [minExposure, maxExposure]

### New Files
| File | Description |
|------|-------------|
| `GXLib/Graphics/PostEffect/AutoExposure.h` | AutoExposure class with downsample chain + readback ring buffer |
| `GXLib/Graphics/PostEffect/AutoExposure.cpp` | PS-based log-luminance, 4-pass downsample, readback, adaptation |
| `Shaders/AutoExposure.hlsl` | PSLogLuminance (HDR→log(lum)), PSDownsample (bilinear average) |

### Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | AutoExposure member + accessor, Resolve deltaTime param |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | AutoExposure init/OnResize, ComputeExposure before tonemapping |
| `Sandbox/main.cpp` | X key toggle, Resolve(deltaTime) call, HUD Y=335, help updated |

### Key Design Decisions
- **PS-based (no Compute Shader)**: CSインフラ不要、既存パイプラインに統合しやすい
- **2-frame readback ring buffer**: GPU stall 回避、2フレーム遅延は視覚的に問題なし
- **Half-float CPU conversion**: R16_FLOAT readback → manual IEEE 754 half→float conversion
- **Fixed-size downsample chain**: 256→1 は固定サイズなので OnResize 不要
- **Integration point**: トーンマッピング直前で `exposure = ComputeExposure(...)` を呼び出し

---

## Phase 4o: JSON Settings

### New Files
| File | Description |
|------|-------------|
| `GXLib/ThirdParty/json.hpp` | nlohmann/json v3.11.3 single-header (MIT license) |
| `GXLib/Graphics/PostEffect/PostEffectSettings.h` | PostEffectSettings::Load/Save static methods |
| `GXLib/Graphics/PostEffect/PostEffectSettings.cpp` | Full JSON serialization for all 13 effects |

### Modified Files
| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.h` | LoadSettings/SaveSettings methods, const accessors for all effects |
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | LoadSettings/SaveSettings delegation to PostEffectSettings |
| `Sandbox/main.cpp` | F12 key save, startup load from post_effects.json |

### JSON Format
```json
{
  "postEffects": {
    "tonemapping": { "mode": "ACES", "exposure": 1.0 },
    "bloom": { "enabled": true, "threshold": 1.0, "intensity": 0.5 },
    "fxaa": { "enabled": true },
    "ssao": { "enabled": false, "radius": 0.5, "bias": 0.025, "power": 2.0 },
    "dof": { "enabled": false, "focalDistance": 10.0, "focalRange": 5.0, "bokehRadius": 4.0 },
    "motionBlur": { "enabled": false, "intensity": 1.0, "sampleCount": 16 },
    "ssr": { "enabled": false, "maxSteps": 200, "intensity": 1.0 },
    "outline": { "enabled": false, "depthThreshold": 0.5, "normalThreshold": 0.3, "intensity": 1.0 },
    "taa": { "enabled": false, "blendFactor": 0.9 },
    "autoExposure": { "enabled": false, "speed": 1.5, "min": 0.1, "max": 10.0, "keyValue": 0.18 },
    "volumetricLight": { "enabled": false, "intensity": 1.0, "density": 1.0, "decay": 0.97 },
    "vignette": { "enabled": false, "intensity": 0.5, "chromaticAberration": 0.003 },
    "colorGrading": { "enabled": false, "contrast": 1.0, "saturation": 1.0, "temperature": 0.0 }
  }
}
```

---

## Final Effect Chain (Phase 4 Complete)
```
HDR Scene -> [SSAO(multiply)] -> [SSR] -> [VolumetricLight] -> [Bloom] -> [DoF]
          -> [MotionBlur] -> [Outline] -> [TAA] -> [ColorGrading(HDR)]
          -> [AutoExposure] -> [Tonemapping(HDR->LDR)]
          -> [FXAA(LDR)] -> [Vignette+ChromAb(LDR)] -> Backbuffer
```

## Phase 4 Completion Status
All 13 post-effects from Framework Plan §3.4 implemented:
- Bloom, Tonemapping (3 modes), HDR, SSAO, DoF, MotionBlur, ColorGrading
- FXAA, TAA, Vignette, ChromaticAberration, SSR, VolumetricLight, OutlineEffect
- AutoExposure (Eye Adaptation)
- JSON settings file (post_effects.json) with F12 save / startup load

**Phase 4 complete. Next: Phase 5 (描画レイヤーシステム).**

## Verification
- Build: OK (cmake -B build -S . && cmake --build build --config Debug)
- All effects toggleable via keyboard (1-9, 0, B, R, O, V, T, X)
- F12 saves post_effects.json, startup loads it
- No regressions in existing effects

## Phase 5 Summary

## Overview
描画レイヤーシステムを実装。レイヤー単位でのRT管理、Z-order合成、ブレンドモード、マスク機能を提供。
Scene(PostFX付き) + UI(2Dオーバーレイ) の2レイヤー構成で動作確認。

## Completion Condition
> 背景→ゲーム→UIが独立レイヤーで管理され合成される → **達成**

---

## New Files

| File | Description |
|------|-------------|
| `GXLib/Graphics/Layer/RenderLayer.h/cpp` | レイヤークラス (RGBA8 LDR RT, Z-order, visibility, opacity, blendMode, mask) |
| `GXLib/Graphics/Layer/LayerStack.h/cpp` | レイヤー管理 (Create/Get/Remove, Z-order sort, OnResize) |
| `GXLib/Graphics/Layer/LayerCompositor.h/cpp` | Z-order順合成 (6 blend PSOs + mask PSOs, 2-SRV dedicated heap) |
| `GXLib/Graphics/Layer/MaskScreen.h/cpp` | DXLib互換マスク (RGBA8 RT, R channel mask, rect/circle draw via VB) |
| `Shaders/LayerComposite.hlsl` | レイヤー合成シェーダー (texture sample + opacity + mask) |
| `Shaders/MaskDraw.hlsl` | マスク描画シェーダー (orthographic projection, fill value) |

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp` | Resolve outputs to any RTV handle (scene layer RT) |
| `Sandbox/main.cpp` | Scene+UI 2-layer構成, L key mask demo toggle |

## Architecture

```
Scene Layer (Z:0, PostFX=true)  ─┐
                                  ├→ LayerCompositor → Backbuffer → Present
UI Layer (Z:1000, Alpha blend)  ─┘
```

- PostEffectPipeline.Resolve() → Scene Layer RT (not backbuffer directly)
- 2D overlay (SpriteBatch/TextRenderer) → UI Layer RT
- LayerCompositor: Z-order ascending, per-layer blend PSO selection

## Key Design Decisions
- **LDR RGBA8 per-layer RT**: PostFX chain handles HDR→LDR, layers are all LDR
- **6 blend modes**: Alpha, Add, Sub, Mul, Screen, None — each has own PSO
- **Mask via separate layer**: MaskScreen wraps a RenderLayer, R channel = mask value
- **2-SRV dedicated heap for masked compositing**: layer texture + mask texture
- **Fullscreen triangle**: SV_VertexID-based, no VB (consistent with PostEffect approach)

## Verification
- Build: OK
- Scene layer with PostFX + UI layer with text/GUI composited correctly
- L key toggles mask demo (rect + circle mask on UI layer)

## Phase 6ab Summary

## Overview
GUI基盤システム（Widget, UIContext, UIRenderer, Style）とCSSスタイルシートパーサーを実装。
Panel, TextWidget, Button の3ウィジェットで動作確認。CSSによるスタイル外部定義でコード削減。

## Completion Condition
> XMLとCSSでメニュー・HUDが構築でき、C++でイベント処理可能 → **CSS部分 達成** (XMLは6cで実装予定)

---

## Phase 6a: GUI Core Foundation

### New Files

| File | Description |
|------|-------------|
| `GXLib/GUI/Widget.h` | 基底クラス (WidgetType, LayoutRect, UIEvent, ツリー構造, FindById) |
| `GXLib/GUI/Widget.cpp` | AddChild, RemoveChild, FindById, OnEvent, Update, Render |
| `GXLib/GUI/Style.h` | StyleLength, StyleColor, StyleEdges, TextAlign, FlexDirection等, Style構造体 |
| `GXLib/GUI/UIContext.h/cpp` | ウィジェットツリー管理, Flexboxレイアウト, 3フェーズイベントディスパッチ, フォーカス |
| `GXLib/GUI/UIRenderer.h/cpp` | UIRectBatch(SDF角丸矩形) + SpriteBatch + TextRenderer + ScissorStack統合 |
| `GXLib/GUI/Widgets/Panel.h/cpp` | コンテナウィジェット (子要素のレイアウトコンテナ) |
| `GXLib/GUI/Widgets/TextWidget.h/cpp` | テキスト表示ウィジェット (intrinsic size対応) |
| `GXLib/GUI/Widgets/Button.h/cpp` | ボタン (hover/pressed/disabled スタイル, onClick) |
| `Shaders/UIRect.hlsl` | SDF角丸矩形シェーダー (背景+枠線+影, CB=144bytes) |

### Architecture

```
Widget (base)
  ├─ Panel     (container, children layout)
  ├─ TextWidget(text display, intrinsic size)
  └─ Button    (hover/pressed/disabled states, onClick callback)

UIContext
  ├─ Event dispatch: Capture → Target → Bubble (3-phase)
  ├─ Hit test: reverse child order (Z-order)
  ├─ Layout: Flexbox (MeasureWidget bottom-up → LayoutWidget top-down)
  └─ Focus management

UIRenderer
  ├─ UIRectBatch: SDF rounded rect (1 draw/rect, CB=144bytes, per-rect VB)
  ├─ SpriteBatch: texture rendering
  ├─ TextRenderer: font rendering
  └─ ScissorStack: nested clipping
```

### Key Design Decisions
- **UIRectBatch**: SDF角丸矩形シェーダー。背景+枠線+影を1ドローコールで描画
- **3フェーズイベント**: DOM準拠のCapture→Target→Bubble
- **Flexbox**: MeasureWidget(ボトムアップ測定) → LayoutWidget(トップダウン配置)
- **DynamicBuffer multi-cycle fix**: per-frame offset counters で同一フレーム内上書き防止

---

## Phase 6b: StyleSheet Parser (.css)

### New Files

| File | Description |
|------|-------------|
| `GXLib/GUI/StyleSheet.h` | PseudoClass, StyleSelector, StyleProperty, StyleRule, StyleSheet クラス |
| `GXLib/GUI/StyleSheet.cpp` | Tokenizer, Parser, NormalizePropertyName, ApplyProperty, Cascade, ApplyToTree |
| `Assets/ui/menu.css` | サンプルスタイルシート (メインメニュー, CSS標準kebab-case) |

### Modified Files

| File | Changes |
|------|---------|
| `GXLib/GUI/UIContext.h/cpp` | SetStyleSheet(), ComputeLayout()先頭でApplyToTree() |
| `Sandbox/CMakeLists.txt` | Assets ディレクトリのビルド後コピー追加 |
| `Sandbox/main.cpp` | StyleSheet使用に書き換え (~100行 → ~40行) |

### Architecture

```
.css File → Tokenizer → Parser → StyleRules[]
                                       ↓
ComputeLayout() → ApplyToTree() → Cascade(specificity sort) → Style applied
                                       ↓
                                  LayoutWidget() → Render()
```

### CSS Parser Details

**Tokenizer**: 単一パス文字スキャナ
- Token種類: Ident, Hash(#xxx), Dot, Colon, LBrace, RBrace, Semicolon, Number, Percent, String, Eof
- `/* */` ブロックコメント, `//` 行コメント対応

**Parser**: `stylesheet = rule*`, `rule = selector '{' declaration* '}'`

**Selector Types**: `#id`, `.class`, `Type`, `:pseudo`
- Specificity: id=100, class=10, type=1
- Pseudo: hover, pressed, disabled, focused

**Property Normalization**:
- kebab-case → camelCase 自動変換 (flex-direction → flexDirection)
- CSS標準エイリアス: border-radius → cornerRadius, background-color → backgroundColor

**Cascade**:
1. マッチするルール収集
2. specificity昇順 → sourceOrder昇順ソート
3. pseudo=None → computedStyle
4. pseudo=Hover/Pressed/Disabled → Button の状態スタイル

### Key Design Decisions
- **毎フレーム ApplyToTree**: ComputeLayout() で毎フレーム適用 → CSSホットリロード可能
- **CSS標準形式**: kebab-case プロパティ名 + NormalizePropertyName で内部camelCase変換
- **`<sstream>` 不使用**: pch.h に `<sstream>` がないため手動文字列パース

---

## Verification
- Build: OK (Debug)
- メインメニュー表示: CSS定義の色・配置・角丸・影・枠線が正しく反映
- ボタンホバー/プレス: :hover/:pressed 擬似クラスのスタイル変化動作
- カスケード: #btnExit(id) が .menuButton(class) を正しくオーバーライド
- コード削減: Sandbox/main.cpp のGUI構築 ~100行 → ~40行

## Phase 6c Summary

## Overview
XMLファイルからウィジェットツリーを宣言的に定義する仕組みを実装。
C++のウィジェット構築コード (~40行) を XML + CSS + C++ イベントバインディング (~15行) に置き換え。

## Completion Condition
> XMLとCSSでメニュー・HUDが構築でき、C++でイベント処理可能 → **XML部分 達成**

---

## New Files

| File | Description |
|------|-------------|
| `GXLib/GUI/XMLParser.h` | XMLNode struct + XMLDocument class (DOM) |
| `GXLib/GUI/XMLParser.cpp` | 再帰降下XMLパーサー (BOM, comments, `<?xml?>`, entities, self-closing) |
| `GXLib/GUI/GUILoader.h` | GUILoader class (font/event registration, BuildFromFile/BuildFromDocument) |
| `GXLib/GUI/GUILoader.cpp` | BuildWidget (tag→widget mapping, font/event resolution, inline style) + Utf8ToWstring |
| `Assets/ui/menu.xml` | 宣言的メニューUI (root panel, menu panel, title text, 3 buttons) |

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/GUI/StyleSheet.h` | `ApplyProperty()` と `NormalizePropertyName()` を private → public static に移動 |
| `Sandbox/main.cpp` | GUILoader使用に書き換え (~40行 → ~15行), タイトル "Phase6c [XML Parser]" |

## Architecture

```
menu.xml → XMLDocument → XMLNode tree
                              ↓
GUILoader.BuildWidget() → Widget tree
  ├─ tag → widget type (Panel/Text/Button, unknown → Panel fallback)
  ├─ id/class/enabled/visible → widget properties
  ├─ font → ResolveFontHandle (registered name → int handle)
  ├─ onClick → m_eventMap lookup → widget.onClick
  ├─ text attr or text content → Utf8ToWstring → SetText
  ├─ other attrs → StyleSheet::ApplyProperty (inline style)
  └─ children → recursive BuildWidget
```

### XML Parser Details

**再帰降下パーサー** (トークナイザ不要、文字単位走査):
- UTF-8 BOM (0xEF 0xBB 0xBF) スキップ
- `<?xml ... ?>` 宣言スキップ
- `<!-- ... -->` コメントスキップ
- `<Tag attr="val">...</Tag>` 通常要素
- `<Tag attr="val" />` 自己閉じ
- `<Text>content</Text>` テキストコンテント
- 属性クォート: `"..."` と `'...'` 両対応
- XML エンティティ: `&amp;` `&lt;` `&gt;` `&quot;` `&apos;`
- タグ不一致: `GX_LOG_ERROR` + 位置ヒント

### GUILoader Details

**C++ Usage:**
```cpp
GX::GUI::GUILoader loader;
loader.SetRenderer(&g_uiRenderer);
loader.RegisterFont("default", g_guiFontHandle);
loader.RegisterFont("large", g_guiFontLarge);
loader.RegisterEvent("onStartGame", []() { GX_LOG_INFO("Start!"); });
loader.RegisterEvent("onOpenOptions", []() { GX_LOG_INFO("Options!"); });
loader.RegisterEvent("onExit", []() { PostQuitMessage(0); });

auto root = loader.BuildFromFile("Assets/ui/menu.xml");
```

## Key Design Decisions
- **自前XMLパーサー**: 外部依存なし、軽量 (pugixml/RapidXML不要)
- **StyleSheet public static**: ApplyProperty/NormalizePropertyName をGUILoaderから利用可能に
- **`<unordered_set>` 不使用**: pch.hに含まれないため、inline比較で代替
- **テキスト内容**: text属性 > テキストコンテント (属性優先)
- **未知タグ → Panel fallback**: エラーではなく警告 + フォールバック

## Error Handling

| Situation | Behavior |
|-----------|----------|
| XML file not found | `GX_LOG_ERROR`, return nullptr |
| Invalid XML (unclosed tag etc.) | `GX_LOG_ERROR` + position hint, return nullptr |
| Unknown tag name | `GX_LOG_WARN`, Panel fallback |
| Unknown font name | `GX_LOG_WARN`, handle=-1 |
| Unregistered event name | `GX_LOG_WARN`, onClick not set |
| Unknown entity | Passed through as-is |

## Verification
- Build: OK (Debug)
- 見た目: Phase 6b と同一のメニュー表示 (U キーでトグル)
- イベント: ボタンクリックで LOG 出力 + Exit で終了
- ホバー/プレス: CSS :hover/:pressed がXML構築ツリーでも動作
- コード削減: main.cpp GUI構築 ~40行 → ~15行

## Phase 7 Summary

## Overview
ファイルシステム抽象化 (VFS)、暗号化アーカイブ (.gxarc)、非同期アセットロード、ファイル変更監視、
TCP/UDP/HTTP/WebSocket ネットワーク、Media Foundation 動画再生を実装。

## Completion Condition
> 暗号化アーカイブからのアセット読込、HTTP通信、動画再生が動作 → **達成**

---

## Sub-phases

| Sub-phase | Content |
|-----------|---------|
| 7a | FileSystem + PhysicalFileProvider (VFS) |
| 7b | Archive + AES-256 + LZ4 compression |
| 7c | AsyncLoader + FileWatcher |
| 7d | TCP/UDP Socket + HTTP Client + WebSocket |
| 7e | MoviePlayer (Media Foundation) |
| 7f | Integration demo + VFS migration |

## New Files

| File | Description |
|------|-------------|
| `GXLib/IO/FileSystem.h/cpp` | VFS singleton, mount-based provider routing, path normalization |
| `GXLib/IO/PhysicalFileProvider.h/cpp` | Disk-backed IFileProvider |
| `GXLib/IO/Crypto.h/cpp` | AES-256-CBC, SHA-256, random bytes (Windows BCrypt API) |
| `GXLib/IO/Archive.h/cpp` | Custom .gxarc format (LZ4 + AES-256), ArchiveWriter |
| `GXLib/IO/ArchiveFileProvider.h/cpp` | Read-only IFileProvider backed by Archive (priority=100) |
| `GXLib/IO/AsyncLoader.h/cpp` | Background thread file loading, main-thread callback dispatch |
| `GXLib/IO/FileWatcher.h/cpp` | ReadDirectoryChangesW + OVERLAPPED async, change notification |
| `GXLib/IO/Network/TCPSocket.h/cpp` | Winsock2 TCP client |
| `GXLib/IO/Network/UDPSocket.h/cpp` | Winsock2 UDP socket |
| `GXLib/IO/Network/HTTPClient.h/cpp` | WinHTTP sync+async HTTP client |
| `GXLib/IO/Network/WebSocket.h/cpp` | WinHTTP WebSocket client |
| `GXLib/Movie/MoviePlayer.h/cpp` | Media Foundation video decode → texture |
| `GXLib/ThirdParty/lz4.h` | LZ4 compression header (BSD license) |
| `GXLib/ThirdParty/lz4.c` | LZ4 compression source |

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/CMakeLists.txt` | Added IO/*.cpp, Movie/*.cpp to GLOB_RECURSE; lz4.c manual append (LANGUAGE C + SKIP_PRECOMPILE_HEADERS); linked bcrypt, ws2_32, winhttp, mfplat, mfreadwrite, mf, mfuuid |
| `GXLib/pch.h` | Added `<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>` |
| `GXLib/GUI/XMLParser.cpp` | VFS-first LoadFromFile with ifstream fallback |
| `GXLib/GUI/StyleSheet.cpp` | VFS-first LoadFromFile with ifstream fallback |
| `Sandbox/main.cpp` | VFS init, archive demo, async HTTP GET, movie player, Phase 7 status display |

## Architecture

### VFS (Virtual File System)
```
FileSystem (singleton)
  ├── Mount("", PhysicalFileProvider("./"))     priority=0
  └── Mount("", ArchiveFileProvider("game.gxarc"))  priority=100
      ↓ ReadFile("Assets/texture.png")
      → Try highest priority first → Archive → Physical → FileData
```

### Archive Format (.gxarc)
```
[Magic: "GXARC\0\0\0" 8B] [TOC Header 16B] [TOC Data (AES-256)] [File Data (LZ4)]
```

### Network Stack
- TCPSocket / UDPSocket: Winsock2
- HTTPClient: WinHTTP (sync + async via detached thread)
- WebSocket: WinHTTP WebSocket API (receive thread + message queue)

### MoviePlayer
- IMFSourceReader → MFVideoFormat_RGB32 → BGRA→RGBA flip → TextureManager::CreateTextureFromMemory

## Key Design Decisions
- **VFS fallback pattern**: Try FileSystem first, fall back to direct I/O (backward compatible)
- **LZ4 as C source**: Compiled with LANGUAGE C + SKIP_PRECOMPILE_HEADERS to avoid PCH conflicts
- **Archive priority=100**: Archives take precedence over physical files
- **Async pattern**: Worker thread + mutex + completed queue + Update() on main thread
- **FileWatcher**: OVERLAPPED async ReadDirectoryChangesW with stop event

## Issues Encountered
- **LZ4 linker error**: CMake classified lz4.c as `<None Include>` instead of `<ClCompile Include>`. Fixed by adding `LANGUAGE C` to set_source_files_properties.

## Verification
- Build: OK
- VFS: ON (PhysicalFileProvider mounted)
- Archive: OK (encrypted archive created and verified)
- HTTP: 200 OK (232 bytes from httpbin.org)
- Movie: N/A (no test video, controls ready F5/F6)

## Phase 8 Summary

## Overview
GXLib独自の数学ライブラリ、2D/3D衝突判定、空間分割構造、2D/3Dの物理エンジンを実装。
数学型 (Vector2/3/4, Matrix4x4, Quaternion, Color) は DirectXMath の XMFLOAT 系をゼロオーバーヘッドで継承し、
既存の DirectXMath コードとの暗黙的な相互変換を維持しつつ、演算子オーバーロードや便利メソッドを追加。
衝突判定は SAT (分離軸定理) や Moller-Trumbore レイ-三角形交差などのアルゴリズムを実装。
空間分割は Quadtree/Octree/BVH のテンプレートヘッダーオンリーライブラリとして提供。
2D物理はカスタムインパルスベースエンジン、3D物理は Jolt Physics v5.3.0 を PIMPL パターンでラップし統合。

## Completion Condition
> Vector/Matrix/Quaternion数学ライブラリ、2D/3Dコリジョン、空間分割、2Dカスタム物理、3D Jolt物理が動作 → **達成**

---

## Sub-phases

| Sub-phase | Content |
|-----------|---------|
| 8a | Math types (Vector2/3/4, Matrix4x4, Quaternion, Color, MathUtil, Random) |
| 8b | Collision2D / Collision3D (AABB, Circle, Sphere, OBB, SAT, Moller-Trumbore, Raycast) |
| 8c | Spatial structures (Quadtree, Octree, BVH with SAH split) |
| 8d | Physics2D (RigidBody2D, PhysicsWorld2D — custom impulse-based engine) |
| 8e | Physics3D (PhysicsWorld3D — Jolt Physics v5.3.0 PIMPL wrapper) |

## New Files

| File | Description |
|------|-------------|
| `GXLib/Math/MathUtil.h` | 定数 (PI, TAU, EPSILON)、Clamp, Lerp, SmoothStep, 角度変換などのユーティリティ関数群 |
| `GXLib/Math/Vector2.h` | 2Dベクトル (XMFLOAT2継承)、演算子, Length, Dot, Cross, Lerp, Min/Max |
| `GXLib/Math/Vector3.h` | 3Dベクトル (XMFLOAT3継承)、Cross, Reflect, Transform, TransformNormal |
| `GXLib/Math/Vector4.h` | 4Dベクトル (XMFLOAT4継承)、Vector3+W コンストラクタ |
| `GXLib/Math/Matrix4x4.h` | 4x4行列 (XMFLOAT4X4継承)、ToXMMATRIX/FromXMMATRIX、Translation/Scaling/Rotation/LookAt/Perspective |
| `GXLib/Math/Quaternion.h` | クォータニオン (XMFLOAT4継承)、Slerp, FromAxisAngle, FromEuler, ToEuler, RotateVector |
| `GXLib/Math/Color.h` | RGBA色 (float4)、uint32_t/uint8_t コンストラクタ, HSV変換, プリセット色 |
| `GXLib/Math/Random.h/cpp` | Mersenne Twister 乱数、Int/Float範囲指定, PointInCircle/Sphere, Direction2D/3D, Global() |
| `GXLib/Math/Collision/Collision2D.h` | 2D形状 (AABB2D, Circle, Line2D, Polygon2D)、交差判定, Raycast, Sweep |
| `GXLib/Math/Collision/Collision3D.h` | 3D形状 (AABB3D, Sphere, Ray, Plane, Frustum, OBB, Triangle)、SAT, Moller-Trumbore, Raycast |
| `GXLib/Math/Collision/Quadtree.h` | テンプレート四分木 (AABB/Circle クエリ, GetPotentialPairs) |
| `GXLib/Math/Collision/Octree.h` | テンプレート八分木 (AABB/Sphere/Frustum クエリ, GetPotentialPairs) |
| `GXLib/Math/Collision/BVH.h` | テンプレートBVH (SAH分割, AABB/Rayクエリ, 最近傍Raycast) |
| `GXLib/Physics/RigidBody2D.h` | 2D剛体 (位置, 速度, 質量, コライダー, BodyType, トリガー, レイヤー) |
| `GXLib/Physics/PhysicsWorld2D.h/cpp` | 2D物理ワールド (重力, ブロードフェーズ, インパルス応答, 摩擦, レイキャスト, コールバック) |
| `GXLib/Physics/PhysicsWorld3D.h/cpp` | 3D物理ワールド (Jolt Physics PIMPL, ボディ管理, シェイプ作成, レイキャスト) |
| `GXLib/Physics/PhysicsShape.h` | 物理シェイプハンドル (Jolt内部シェイプ参照のvoid*ラッパー) |

## Modified Files

| File | Changes |
|------|---------|
| `CMakeLists.txt` | Jolt Physics を FetchContent (v5.3.0), USE_STATIC_MSVC_RUNTIME_LIBRARY OFF |
| `GXLib/CMakeLists.txt` | Math/*.cpp, Physics/*.cpp を GLOB_RECURSE に追加; Jolt リンク |
| `GXLib/pch.h` | `<random>`, `<functional>` 追加 (Random用 mt19937, PhysicsWorld コールバック用) |
| `Sandbox/main.cpp` | 2D/3D物理デモ統合, Phase 8 ステータス表示 |

## Architecture

### Math Types (DirectXMath継承)
```
XMFLOAT2 ← Vector2  (演算子 +,-,*,/、Length, Dot, Cross2D, Lerp, Min/Max)
XMFLOAT3 ← Vector3  (Cross3D, Reflect, Transform, TransformNormal, 方向定数)
XMFLOAT4 ← Vector4  (Vector3+W ctor)
XMFLOAT4 ← Quaternion (Slerp, NLerp, FromAxisAngle/Euler, ToEuler, RotateVector)
XMFLOAT4X4 ← Matrix4x4 (ToXMMATRIX/FromXMMATRIX, Translation/Scaling/Rotation/LookAt/Perspective)

Color (独立struct, float r/g/b/a)
  ├── float/uint32_t/uint8_t ctors
  ├── HSV変換 (FromHSV/ToHSV)
  └── プリセット色 (White, Black, Red, Green, Blue, Yellow, Cyan, Magenta, Transparent)

MathUtil: PI, TAU, EPSILON, Clamp, Lerp, InverseLerp, Remap, SmoothStep, SmootherStep,
          DegreesToRadians/RadiansToDegrees, NormalizeAngle, IsPowerOfTwo, NextPowerOfTwo

Random (Mersenne Twister mt19937):
  ├── Int/Float (範囲指定)
  ├── Vector2InRange/Vector3InRange
  ├── PointInCircle/PointInSphere (rejection sampling, 均一分布)
  ├── Direction2D (angle-based) / Direction3D (Marsaglia method)
  └── Global() (static singleton)
```

### Collision Detection (2D)
```
形状: AABB2D, Circle, Line2D, Polygon2D (winding number)
判定: TestAABBvsAABB, TestCirclevsCircle, TestAABBvsCircle
      TestPointIn{AABB,Circle,Polygon}, TestLinevs{AABB,Circle,Line}
交差: IntersectAABBvsAABB → HitResult2D (point, normal, depth)
      IntersectCirclevsCircle, IntersectAABBvsCircle
Ray:  Raycast2D (AABB: parametric slab, Circle: quadratic)
Sweep: SweepCirclevsCircle (相対速度→レイキャスト)
```

### Collision Detection (3D)
```
形状: AABB3D, Sphere, Ray, Plane, Frustum (6-plane), OBB (center+halfExtents+axes), Triangle
判定: TestSphereVsSphere, TestAABBVsAABB, TestSphereVsAABB, TestOBBVsOBB (SAT 15軸)
      TestFrustumVs{Sphere,AABB,Point}
交差: IntersectSphereVsSphere, IntersectSphereVsAABB → HitResult3D
Ray:  RaycastSphere, RaycastAABB (slab method), RaycastPlane
      RaycastTriangle (Moller-Trumbore, u/v barycentric output)
      RaycastOBB (OBB→ローカル空間変換→AABB raycast)
Sweep: SweepSphereVsSphere
Closest: ClosestPointOn{AABB, Triangle, Line}
```

### Spatial Partitioning
```
Quadtree<T> (2D空間分割):
  ├── AABB2D bounds, maxDepth=8, maxObjects=8
  ├── 4分割 (NW, NE, SW, SE)
  ├── Query(AABB2D), Query(Circle)
  └── GetPotentialPairs (ancestor propagation)

Octree<T> (3D空間分割):
  ├── AABB3D bounds, maxDepth=8, maxObjects=8
  ├── 8分割 (bit-mask octant selection)
  ├── Query(AABB3D), Query(Sphere), Query(Frustum)
  └── GetPotentialPairs

BVH<T> (Bounding Volume Hierarchy):
  ├── Build: SAH (Surface Area Heuristic) 分割
  │   └── 3軸ソート → 全分割点のコスト評価 → 最小コスト選択
  ├── Query(AABB3D), Query(Ray)
  └── Raycast → 最近傍ヒット (early out by closestT)
```

### 2D Physics (Custom Engine)
```
PhysicsWorld2D
  ├── IntegrateBodies(dt)
  │   ├── 重力適用
  │   ├── 蓄積力/トルク適用 (F * invMass * dt)
  │   ├── 減衰 (linear/angular damping)
  │   └── 位置/回転の積分
  ├── BroadPhase → O(n^2) AABB overlap + レイヤーフィルタ
  ├── NarrowPhase → Circle/Circle, AABB/AABB, AABB/Circle 判定
  └── ResolveCollision
      ├── 位置補正 (Baumgarte-style, percent=0.8, slop=0.01)
      ├── インパルス応答 (反発係数 e = min(a,b))
      └── 摩擦 (Coulomb friction, mu = sqrt(a*b))

RigidBody2D: position, velocity, mass, restitution, friction, damping
             BodyType (Static/Dynamic/Kinematic), ColliderShape2D (Circle/AABB)
             isTrigger, userData, layer bitmask, ApplyForce/Impulse/Torque
```

### 3D Physics (Jolt Physics PIMPL Wrapper)
```
PhysicsWorld3D (PIMPL: struct Impl)
  ├── Initialize(maxBodies) → RegisterDefaultAllocator, Factory, RegisterTypes
  │   ├── TempAllocatorImpl (32MB)
  │   ├── JobSystemThreadPool (hardware_concurrency - 1 threads)
  │   └── PhysicsSystem::Init (maxBodies, maxBodyPairs, maxContactConstraints)
  ├── Shape creation: Box, Sphere, Capsule, Mesh → PhysicsShape (void* → JPH::ShapeRefC*)
  ├── Body management: AddBody(shape, settings) → PhysicsBodyID
  │   ├── MotionType → JPH::EMotionType mapping
  │   ├── CalculateInertia (mass override)
  │   └── Friction, restitution, damping settings
  ├── Body manipulation: Set/Get Position/Rotation/Velocity, ApplyForce/Impulse/Torque
  ├── Step(dt) → PhysicsSystem::Update
  ├── Raycast → NarrowPhaseQuery::CastRay → RaycastResult
  └── Callbacks: onContactAdded, onContactRemoved (ContactListenerImpl)

Jolt内部クラス (anonymous namespace):
  ├── BPLayerInterface (2-layer: NON_MOVING, MOVING)
  ├── ObjectVsBroadPhaseFilter (Static → Moving のみ衝突)
  ├── ObjectLayerPairFilter (Static-Static ペアをスキップ)
  └── ContactListenerImpl → PhysicsWorld3D callbacks

変換ヘルパー:
  ToJolt(Vector3→Vec3, Quaternion→Quat)
  FromJoltV(Vec3→Vector3), FromJoltR(RVec3→Vector3), FromJoltQ(Quat→Quaternion)
```

## Key Design Decisions
- **DirectXMath継承 (ゼロオーバーヘッド)**: Vector2/3/4, Matrix4x4, Quaternion を XMFLOAT 系から継承し、メモリレイアウト同一でキャスト不要の相互変換を実現
- **ヘッダーオンリー数学ライブラリ**: Random.cpp のみ .cpp (mt19937 statics)。他は全てインラインで最適化を促進
- **Color の複数コンストラクタ**: float, uint32_t, uint8_t の3種類を提供。ただし `Color(1, 0, 0)` のような整数リテラルで曖昧性が発生するため、float版は `1.0f` と明示する必要がある
- **SAH (Surface Area Heuristic) BVH分割**: 全3軸の全分割点を総当たり評価し、最小表面積コストの分割を選択。小規模オブジェクトセットで十分な品質
- **O(n^2) 2Dブロードフェーズ**: 小〜中規模ボディ数を想定し、シンプルなペアワイズAABB判定を採用。大規模時は Quadtree を組み合わせ可能
- **Jolt PIMPL パターン**: PhysicsWorld3D.cpp のみに Jolt ヘッダーを include し、公開ヘッダーからの依存を完全に排除。PhysicsShape は void* ハンドルで抽象化
- **Jolt FetchContent v5.3.0**: CMake FetchContent で自動取得。USE_STATIC_MSVC_RUNTIME_LIBRARY=OFF で GXLib の /MDd と互換性を確保
- **2D物理の画面座標系対応**: Y-down の画面座標系では重力Yを正の値に設定 (物理学の慣例と逆)
- **テンプレートヘッダーオンリー空間分割**: Quadtree/Octree/BVH を全てテンプレートクラスとしてヘッダーに実装。型パラメータで任意のオブジェクト型を格納可能
- **Moller-Trumbore レイ-三角形交差**: バリセントリック座標 (u, v) を出力し、テクスチャ座標補間やヒット判定に活用可能
- **OBB vs OBB (SAT 15軸)**: 3+3+9=15軸の分離軸テストで OBB 同士の正確な交差判定を実現
- **Random: rejection sampling**: PointInCircle/PointInSphere は棄却法で均一分布を保証、Direction3D は Marsaglia 法で球面上の均一方向を生成

## Issues Encountered
- **Jolt USE_STATIC_MSVC_RUNTIME_LIBRARY**: デフォルトONだと /MT でリンクされ、GXLib の /MDd と衝突。FetchContent 時に OFF を明示して解決。
- **Jolt RegisterDefaultAllocator() 必須**: v5.3.0 では Reallocate 関数ポインタが追加されており、個別関数設定ではなく RegisterDefaultAllocator() を使用する必要があった。
- **JPH::RVec3 型の曖昧性**: Jolt のダブルプレシジョンビルドでは RVec3 が DVec3 になる場合があり、FromJoltV/FromJoltR/FromJoltQ と変換ヘルパーを分離して対応。
- **CalculateInertia の mass アサーション**: mMassPropertiesOverride.mMass に質量を設定しないと mass>0 のアサーションが発生。OverrideMassProperties と mMass をセットで設定して解決。
- **TempAllocatorImpl サイズ不足**: maxBodies が大きい場合、デフォルトの 10MB では不足。32MB に増量して安定化。
- **maxBodyPairs/maxContactConstraints 上限**: maxBodies に比例して設定すると過大になるため、上限値 (65536/10240) でキャップ。
- **Color コンストラクタ曖昧性**: `Color(1, 0, 0)` が int→uint8_t と int→float の両方に一致。明示的な `1.0f` リテラル使用で回避。
- **GetNumBroadPhaseLayers 戻り値型**: Jolt が `JPH::uint` を期待するが、`uint` と書くと未定義。`JPH::uint` を明示して解決。

## Verification
- Build: OK
- Math types: Vector2/3/4, Matrix4x4, Quaternion, Color 全演算正常動作
- Collision2D: AABB/Circle/Line/Polygon 判定+交差+Raycast 正常
- Collision3D: Sphere/AABB/OBB/Frustum/Triangle 判定+Raycast (Moller-Trumbore) 正常
- Spatial: Quadtree/Octree/BVH のInsert/Query正常動作
- Physics2D: 重力落下、衝突応答 (インパルス+摩擦)、トリガーコールバック正常
- Physics3D: Jolt初期化OK、ボディ追加/シミュレーション/レイキャスト正常動作

## Phase 9 Summary

## Overview
DXLib互換の簡易API (CompatContext シングルトン + グローバル関数) を実装し、
`#include "GXLib.h"` のみで2D/3D/入力/音声/フォント/数学関数を使用可能にした。
また ShaderLibrary (コンパイルキャッシュ + バリアント管理) と ShaderHotReload
(ファイル変更検知 → デバウンス → GPU Flush → PSO再構築) を統合し、
全18レンダラーの PSO Rebuilder 登録によるランタイムシェーダー差し替えを実現。
コンパイルエラー時はエラーメッセージをオーバーレイ表示する仕組みも整備した。

## Completion Condition
> DXLib互換APIで既存サンプルが動作 → **達成**

---

## Sub-phases

| Sub-phase | Content |
|-----------|---------|
| 9a | CompatContext シングルトン + 基本API (GX_Init/GX_End, ProcessMessage, ClearDrawScreen, ScreenFlip, SetGraphMode, etc.) |
| 9b | 描画API (LoadGraph, DrawGraph, DrawRotaGraph, DrawExtendGraph, DrawRectGraph, DrawModiGraph, DrawLine, DrawBox, DrawCircle, DrawTriangle, DrawOval, DrawPixel, SetDrawBlendMode, SetDrawBright) |
| 9c | 入力/音声/3D API (CheckHitKey, GetHitKeyStateAll, GetMouseInput, GetJoypadInputState, LoadSoundMem, PlaySoundMem, PlayMusic, LoadModel, DrawModel, SetCameraPositionAndTarget, etc.) |
| 9d | ShaderLibrary (ShaderKey ハッシュキャッシュ, バリアント管理, PSORebuilder 登録/呼び出し) |
| 9e | ShaderHotReload + PSO Rebuilder統合 (FileWatcher → デバウンス → GPU Flush → InvalidateFile → PSO再構築, F9 トグル) |
| 9f | エラーオーバーレイ (Shader.m_lastError, ShaderLibrary.GetLastError, ShaderHotReload.GetErrorMessage → 画面表示) |

## New Files

| File | Description |
|------|-------------|
| `GXLib/Compat/GXLib.h` | 公開ヘッダー ― この1ファイルで全簡易APIを使用可能。関数宣言 + CompatTypes.h include |
| `GXLib/Compat/CompatContext.h` | 内部シングルトン。全GXLibサブシステム (GraphicsDevice, SpriteBatch, PrimitiveBatch, FontManager, TextRenderer, InputManager, AudioManager, Renderer3D, Camera3D, PostEffectPipeline) を保持 |
| `GXLib/Compat/CompatContext.cpp` | Initialize (全サブシステム初期化), Shutdown, BeginFrame/EndFrame, EnsureSpriteBatch/EnsurePrimitiveBatch/FlushAll, AllocateModelHandle |
| `GXLib/Compat/CompatTypes.h` | 互換定数 (GX_SCREEN_*, GX_BLENDMODE_*, GX_PLAYTYPE_*, GX_FONTTYPE_*) + KEY_INPUT_* (DIKコード体系) + PAD_INPUT_* + VECTOR/MATRIX/COLOR_U8 型 |
| `GXLib/Compat/Compat_System.cpp` | GX_Init, GX_End, ProcessMessage, SetMainWindowText, ChangeWindowMode, SetGraphMode, GetColor, GetNowCount, SetDrawScreen, ClearDrawScreen, ScreenFlip, SetBackgroundColor |
| `GXLib/Compat/Compat_2D.cpp` | LoadGraph, DeleteGraph, LoadDivGraph, GetGraphSize, DrawGraph, DrawRotaGraph, DrawExtendGraph, DrawRectGraph, DrawModiGraph, DrawLine, DrawBox, DrawCircle, DrawTriangle, DrawOval, DrawPixel, SetDrawBlendMode, SetDrawBright |
| `GXLib/Compat/Compat_Font.cpp` | DrawString, DrawFormatString, GetDrawStringWidth, CreateFontToHandle, DeleteFontToHandle, DrawStringToHandle, DrawFormatStringToHandle, GetDrawStringWidthToHandle |
| `GXLib/Compat/Compat_Input.cpp` | DIK→VK 256エントリ変換テーブル, CheckHitKey, GetHitKeyStateAll, GetMouseInput, GetMousePoint, GetMouseWheelRotVol, GetJoypadInputState |
| `GXLib/Compat/Compat_Sound.cpp` | LoadSoundMem, PlaySoundMem, StopSoundMem, DeleteSoundMem, ChangeVolumeSoundMem, CheckSoundMem, PlayMusic, StopMusic, CheckMusic |
| `GXLib/Compat/Compat_3D.cpp` | SetCameraPositionAndTarget, SetCameraNearFar, LoadModel, DeleteModel, DrawModel, SetModelPosition, SetModelScale, SetModelRotation |
| `GXLib/Compat/Compat_Math.cpp` | VGet, VAdd, VSub, VScale, VDot, VCross, VNorm, VSize, MGetIdent, MMult, MGetRotX/Y/Z, MGetTranslate |
| `GXLib/Graphics/Pipeline/ShaderLibrary.h` | ShaderKey (file+entry+target+defines), ShaderKeyHasher, ShaderLibrary シングルトン (キャッシュ + PSORebuilder登録) |
| `GXLib/Graphics/Pipeline/ShaderLibrary.cpp` | GetShader/GetShaderVariant (キャッシュヒット or コンパイル), RegisterPSORebuilder, InvalidateFile (.hlsli→全クリア, .hlsl→該当のみ), NormalizePath |
| `GXLib/Graphics/Pipeline/ShaderHotReload.h` | ShaderHotReload シングルトン (FileWatcher + pending queue + debounce timer) |
| `GXLib/Graphics/Pipeline/ShaderHotReload.cpp` | Initialize (Shaders/ 監視), OnShaderFileChanged (重複排除→pending), Update (debounce→Flush GPU→InvalidateFile), IsShaderFile (.hlsl/.hlsli判定) |

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/CMakeLists.txt` | Compat/*.cpp を GLOB_RECURSE に追加 |
| `GXLib/Graphics/Pipeline/Shader.h` | CompileFromFile の defines 付きオーバーロード追加, m_lastError メンバー追加 |
| `GXLib/Graphics/Pipeline/Shader.cpp` | defines ベクトルを -D NAME=VALUE DXC引数に変換するコンパイルパス追加 |
| 各レンダラー (18箇所) | RegisterPSORebuilder() 呼び出し追加 (Initialize 内の CreatePipelines 後) |
| Group B PostEffects | CreatePipelines() を Initialize() から分離 (PSO 生成のみ独立化、RootSignature は Initialize に残留) |
| `Sandbox/main.cpp` | ShaderHotReload 統合, F9 トグル, エラーオーバーレイ表示 |

## Architecture

### CompatContext Singleton

```
CompatContext (GX_Internal namespace)
  ├── Application           ← ウィンドウ + タイマー
  ├── GraphicsDevice        ← ID3D12Device
  ├── CommandQueue          ← ID3D12CommandQueue
  ├── CommandList           ← ID3D12GraphicsCommandList
  ├── SwapChain             ← IDXGISwapChain4
  ├── SpriteBatch           ← 2Dスプライト描画
  ├── PrimitiveBatch        ← 2Dプリミティブ描画
  ├── FontManager           ← フォントアトラス管理
  ├── TextRenderer          ← テキスト描画
  ├── InputManager          ← Keyboard + Mouse + Gamepad
  ├── AudioManager          ← XAudio2 SE/BGM
  ├── Renderer3D            ← PBR 3Dレンダラー
  ├── Camera3D              ← 3Dカメラ
  ├── PostEffectPipeline    ← ポストエフェクトチェーン
  ├── ModelLoader           ← glTF/GLBローダー
  └── models[]              ← ModelEntry (Model + Transform3D) + フリーリスト
```

### ActiveBatch Auto-Switching

```
DrawGraph()  → EnsureSpriteBatch() → [PrimBatch active? → End()] → SpriteBatch.Begin()
DrawBox()    → EnsurePrimitiveBatch() → [SpriteBatch active? → End()] → PrimBatch.Begin()
ScreenFlip() → FlushAll() → 現在のバッチを End()

ActiveBatch enum: None / Sprite / Primitive
```

### DIK → VK Conversion Table

```
s_dikToVK[256] — DirectInput DIK_* → Win32 VK_* 静的テーブル
  DIK 0x01 (ESCAPE)  → VK_ESCAPE
  DIK 0x1C (RETURN)  → VK_RETURN
  DIK 0x39 (SPACE)   → VK_SPACE
  DIK 0xC8 (UP)      → VK_UP
  ... (全キーボードキー対応)

CheckHitKey(dikCode) → s_dikToVK[dikCode] → InputManager.GetKeyboard().IsKeyDown(vk)
GetHitKeyStateAll(buf) → 全256エントリをスキャンしてbuf[dik] = 1/0
```

### ShaderLibrary Hash Cache

```
ShaderKey { filePath, entryPoint, target, defines[] }
    ↓ ShaderKeyHasher (boost風 hash_combine)
unordered_map<ShaderKey, ShaderBlob>
    ↓ GetShader() / GetShaderVariant()
    ├── Cache HIT  → return cached blob
    └── Cache MISS → Shader::CompileFromFile() → cache → return
```

### ShaderHotReload Pipeline

```
FileWatcher (Shaders/)
    ↓ OnShaderFileChanged() [worker thread]
    ↓ mutex → m_pendingChanges (重複排除)
    ↓
Update() [main thread, 毎フレーム]
    ├── FileWatcher.Update() (コールバック発火)
    ├── pending あり → m_debounceTimer = 0.3s
    ├── timer > 0 → return (デバウンス中)
    └── timer <= 0 → リロード実行
         ├── CommandQueue.Flush() (GPU完了待ち)
         ├── ShaderLibrary.InvalidateFile(path)
         │    ├── .hlsli → 全キャッシュクリア + 全Rebuilder呼び出し
         │    └── .hlsl  → 該当エントリ削除 + 該当Rebuilder呼び出し
         ├── 成功 → m_errorMessage.clear()
         └── 失敗 → m_errorMessage = compiler error (オーバーレイ表示用)
```

### PSO Rebuilder Registration (18 Renderers)

```
Initialize() 内で RegisterPSORebuilder() を呼び出し:

 1. SpriteBatch           10. SSR
 2. PrimitiveBatch        11. DepthOfField
 3. Renderer3D (main)     12. MotionBlur
 4. Renderer3D (shadow)   13. OutlineEffect
 5. Skybox                14. VolumetricLight
 6. PrimitiveBatch3D      15. TAA
 7. Bloom                 16. AutoExposure
 8. PostEffectPipeline ×4 17. MaskScreen
 9. SSAO                  18. LayerCompositor
     +  UIRenderer

各 rebuilder lambda: this→CreatePipelines(device)
```

## Key Design Decisions

- **GX_ prefix (not DxLib_)**: DXLibとの名前衝突を回避しつつ、API形状は互換性を維持。`GX_Init()`/`GX_End()` だが `DrawGraph()`/`DrawBox()` 等の描画関数はDXLibと同名
- **ActiveBatch auto-switching**: SpriteBatch/PrimitiveBatch の Begin/End をユーザーから隠蔽。描画関数呼び出し時に自動的に必要なバッチに切り替え (PSO切替コスト最小化)
- **DIK→VK静的テーブル**: DXLib が DirectInput ベースのキーコード (DIK_*) を使用するため、Win32 VK コードへの変換テーブルを用意。遅延初期化で初回のみ構築
- **volume 0-255 → 0.0-1.0 線形変換**: DXLib互換の整数音量をGXLib内部の浮動小数点音量に変換
- **color GetColor → 0xFFRRGGBB**: アルファ=0xFF固定、DXLib互換の RGB 整数値からパック形式へ変換
- **ShaderLibrary singleton**: 全シェーダーコンパイルを一元管理。キャッシュにより同一シェーダーの再コンパイルを防止
- **ShaderKey hash**: filePath + entryPoint + target + defines のタプルをハッシュキーとして使用。boost 風 hash_combine で衝突を低減
- **.hlsli 全クリア方針**: include依存グラフの追跡は複雑なため、.hlsli 変更時は全キャッシュクリア + 全 PSORebuilder 呼び出しで安全性を確保
- **デバウンス 0.3秒**: エディタの保存時に発生する複数回の FileWatcher イベントを1回のリロードにまとめる
- **GPU Flush before PSO rebuild**: PSO 置換前に CommandQueue.Flush() で GPU の描画完了を待ち、使用中の PSO を安全に差し替え
- **Group B PostEffects: CreatePipelines() 分離**: PSO 再構築コールバックから呼び出すため、Initialize() から PSO 生成部分のみを独立関数化。RootSignature 生成は Initialize() に残留

## Issues Encountered

- **ActiveBatch 切替忘れ**: SpriteBatch と PrimitiveBatch を同フレーム内で混在させた場合に描画が崩れた。EnsureSpriteBatch/EnsurePrimitiveBatch の自動フラッシュで解決
- **DIK コード体系の差異**: DXLib は DirectInput (DIK_*) ベースだが GXLib は Win32 メッセージ (VK_*) ベース。256エントリの静的変換テーブルで吸収
- **GX_PLAYTYPE_LOOP → PlayMusic**: DXLib では PlaySoundMem に LOOP フラグを渡すと BGM ループ再生になるが、GXLib は SE/BGM が別管理。playType で分岐して適切な API に委譲
- **ShaderLibrary mutex**: ホットリロードの OnShaderFileChanged はワーカースレッドから呼ばれるため、キャッシュアクセスに mutex が必須
- **PSO Rebuilder 登録タイミング**: Initialize() 内で CreatePipelines() 完了後に登録する必要がある。先に登録するとコールバック内で未初期化の RootSignature を参照してクラッシュ
- **Group B PostEffects の CreatePipelines 分離**: SSAO/DoF/MotionBlur/SSR/Outline/VolumetricLight/TAA/AutoExposure は Initialize() 内で RootSignature と PSO を一括生成していたため、PSO のみを再構築する CreatePipelines() を抽出する必要があった

## Verification
- Build: OK
- Compat API: GX_Init/GX_End, 2D描画 (Sprite+Primitive), テキスト描画, 入力 (DIK変換), 音声 (SE/BGM), 3D (カメラ+モデル) — 全関数動作確認
- ShaderLibrary: キャッシュヒット/ミス動作確認
- ShaderHotReload: .hlsl 変更 → デバウンス → PSO再構築 → 即時反映確認
- エラーオーバーレイ: 意図的にシェーダー構文エラー → エラーメッセージ画面表示 → 修正後自動復帰確認
- F9 トグル: ホットリロードステータス表示 ON/OFF 確認

## Phase 10a Summary

## Overview
D3D12 Query Heap (TIMESTAMP) を使用したGPUタイムスタンプ プロファイラ、フレーム単位リニアアロケータ、
固定サイズプールアロケータ、バッチバリア発行ユーティリティを実装。
ダブルバッファリングリードバックによりGPUストールを回避しつつ、フレーム毎のGPU負荷を
スコープ単位で計測可能。P キーでHUDオーバーレイをトグル表示。

## Completion Condition
> GPUプロファイラでボトルネック計測が可能 → **達成**

---

## Sub-phases

| Sub-phase | Content |
|-----------|---------|
| 10a-1 | GPUProfiler (D3D12 Timestamp Query + HUD overlay) |
| 10a-2 | FrameAllocator + PoolAllocator (メモリ最適化ユーティリティ) |
| 10a-3 | BarrierBatch (バッチバリア発行ユーティリティ) |

## New Files

| File | Description |
|------|-------------|
| `GXLib/Graphics/Device/GPUProfiler.h` | GPUプロファイラ ヘッダー (シングルトン、RAII GPUProfileScope、マクロ) |
| `GXLib/Graphics/Device/GPUProfiler.cpp` | GPUプロファイラ実装 (Query Heap作成、リードバック、スコープ計測) |
| `GXLib/Core/FrameAllocator.h` | フレーム単位リニアアロケータ (ヘッダーオンリー) |
| `GXLib/Core/PoolAllocator.h` | 固定サイズプールアロケータ (テンプレート、ヘッダーオンリー) |
| `GXLib/Graphics/Device/BarrierBatch.h` | バッチバリア発行ユーティリティ (テンプレート、ヘッダーオンリー) |

## Modified Files

| File | Changes |
|------|---------|
| `Sandbox/main.cpp` | P キーで GPUProfiler トグル、HUDオーバーレイ描画、BeginFrame/EndFrame 統合 |

## Architecture

### GPUProfiler
```
D3D12 Query Heap (TIMESTAMP, k_MaxTimestamps=256)
  ├── BeginFrame()  → 前フレーム結果リードバック + フレーム開始タイムスタンプ
  ├── BeginScope()  → スコープ開始タイムスタンプ (EndQuery)
  ├── EndScope()    → スコープ終了タイムスタンプ (EndQuery)
  └── EndFrame()    → フレーム終了タイムスタンプ + ResolveQueryData → Readback Buffer

Readback Buffer[0] ←→ Readback Buffer[1]  (ダブルバッファリング)
  Frame N: 書き込み → Frame N+2: CPU読み取り (GPUストール回避)
```

- **シングルトンパターン**: `GPUProfiler::Instance()` で唯一のインスタンスにアクセス
- **ダブルバッファリング**: `k_BufferCount=2` のリードバックバッファで、GPU が書き込み中のバッファとは別のバッファから CPU が読み取る
- **タイムスタンプ周波数**: `ID3D12CommandQueue::GetTimestampFrequency()` で取得、ticks → ms 変換
- **スコープ計測**: `BeginScope`/`EndScope` のペアで任意区間を計測。後方検索で最後の未終了スコープを閉じる（ネスト対応）
- **RAII ヘルパー**: `GPUProfileScope` がコンストラクタで `BeginScope`、デストラクタで `EndScope` を呼ぶ
- **マクロ**: `GX_GPU_PROFILE_SCOPE(cmdList, name)` で簡潔にスコープ計測
- **HUD オーバーレイ**: P キーでトグル（デフォルト OFF）、フレームGPU時間 + 各スコープの ms を表示
- **オーバーフロー保護**: `k_MaxTimestamps=256`（128スコープ分）超過時は最後のスロットを再利用

### FrameAllocator
```
[                    1MB Buffer (default)                    ]
 ↑ m_offset=0 (Reset)
 ├── Allocate(64B) → [####]  offset=64
 ├── Allocate(128B, align=16) → [pad][########]  offset=208
 └── ... O(1) bump allocation ...
 Frame End → Reset() → offset=0 (一括解放)
```

- **リニア（バンプ）アロケータ**: ポインタを進めるだけの O(1) 確保
- **実アドレスベースアラインメント**: `(base + offset + alignment - 1) & ~(alignment - 1)` で正確なアラインメント
- **フレーム開始時 Reset()**: 個別 Free() 不要、ダングリングポインタのリスクなし
- **テンプレート Allocate\<T\>()**: `alignof(T)` を自動適用する型安全バージョン
- **キャッシュフレンドリー**: 連続メモリ配置により局所性を確保
- **デフォルト 1MB**: ソートキー配列、一時バッファ等のフレーム内一時データに適用

### PoolAllocator
```
Block 0: [Node0|Node1|Node2|...|Node63]  (BlockSize=64)
Block 1: [Node0|Node1|...|Node63]         (オンデマンド拡張)
           ↓ FreeList: Node→Node→Node→nullptr
```

- **固定サイズオブジェクトプール**: Widget, Sound, Particle 等の大量生成・破棄向け
- **フリーリスト方式**: 未使用スロットをリンクドリストで管理、Allocate/Free ともに O(1)
- **Union ノード**: `Node` は `next` ポインタと `storage[sizeof(T)]` の共用体（メモリ節約）
- **オンデマンドブロック拡張**: フリーリスト枯渇時に `BlockSize` 個分の新ブロックを確保
- **New/Delete ヘルパー**: placement new + デストラクタ呼び出し付きの型安全インターフェース
- **ヒープ断片化防止**: 同サイズオブジェクトの確保・解放を局所化

### BarrierBatch
```
BarrierBatch<16> batch(cmdList);
  batch.Transition(rt, RENDER_TARGET, PIXEL_SHADER_RESOURCE);
  batch.Transition(ds, DEPTH_WRITE, DEPTH_READ);
  batch.UAV(uavBuffer);
  // デストラクタで自動 Flush → 1回の ResourceBarrier(3, barriers)
```

- **テンプレートバッチ**: `BarrierBatch<N=16>` でスタック配列サイズを指定
- **自動フラッシュ**: 容量超過時およびデストラクタで自動 `ResourceBarrier()` 呼び出し
- **Transition + UAV**: 2種類のバリアをサポート、`before == after` はスキップ
- **GPU パイプライン効率化**: 複数バリアを1回の API 呼び出しにまとめることで同期コスト削減

## Key Design Decisions
- **ダブルバッファリングリードバック**: GPU が ResolveQueryData で書き込み中のバッファを CPU が読まないよう、2フレーム遅延で結果を取得。WaitForValue 完了後なのでデータの整合性は保証される
- **AllocTimestamp() のオーバーフロー対策**: 256 クエリを超えた場合は最後のスロットを再利用し、クラッシュを防止
- **EndScope の後方検索**: スコープのネスト（LIFO順序）に対応するため、最後の未終了エントリを逆順で検索
- **FrameAllocator をヘッダーオンリーに**: インライン化による最大のパフォーマンスを確保（Allocate は数命令のバンプ操作のみ）
- **PoolAllocator の Union ノード**: 未使用スロットでは next ポインタ、使用中はオブジェクトデータとして同じメモリ領域を共用
- **BarrierBatch のデストラクタ自動フラッシュ**: スコープ終了時に確実にバリアが発行され、書き忘れを防止
- **P キーでトグル（デフォルト OFF）**: リリースビルドでもオーバーヘッドなし（m_enabled チェックで即 return）

## Issues Encountered
- **FrameAllocator アラインメント不具合**: 初期実装ではオフセットベースのアラインメント `(m_offset + alignment - 1) & ~(alignment - 1)` を使用していたが、バッファの先頭アドレスがアラインメント境界にない場合に正しくアラインされない問題が発生。実アドレスベース `(base + m_offset + alignment - 1) & ~(alignment - 1)` に修正して解決

## Verification
- Build: OK
- GPUProfiler: P キーで HUD オーバーレイ表示、スコープ毎の GPU 時間（ms）を確認
- FrameAllocator: アラインメント + 容量超過テスト通過
- PoolAllocator: Allocate/Free/New/Delete サイクルテスト通過
- BarrierBatch: バッチ発行 + 自動フラッシュ動作確認
- Tests: 151/151 pass

## Phase 10c Summary

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

## Phase 10 Remaining Work

## 方向性 (Knowledges準拠)
- 核心: 「DXライブラリは見た目がショボいし機能が足りない。DirectX 12で完全上位互換を作る」
- 優先順位: 正しく動く > パフォーマンス > コードの美しさ
- DXLib互換と使いやすさを最優先（過度な抽象化は避ける）

## 完成条件 (G1〜G5) の達成状況

| # | 完成条件 | 状態 | 根拠 |
|---|---------|------|------|
| G1 | DXライブラリの全APIカテゴリを網羅 | 達成 | Phase1〜9 Summary |
| G2 | ポストエフェクトパイプライン標準搭載 | 達成 | Phase4 Summary (13エフェクト) |
| G3 | 描画レイヤーシステム動作 | 達成 | Phase5 Summary |
| G4 | XMLベースGUIシステム | 達成 | Phase6 Summary |
| G5 | サンプルプロジェクト群動作 | 達成 | Phase10c Summary (5サンプル) |

---

## 作業手順と結果 (Directive 8.4 準拠)

### Task 1: ビルド検証 — 完了 (2026-02-10)
```
cmake -B build -S .
cmake --build build --config Debug
```
- GXLib.lib / Sandbox.exe / GXLibTests.exe / 5サンプル が全てビルド成功

### Task 2: テスト実行 — 完了 (2026-02-10)
```
ctest --test-dir build --build-config Debug
```
- 151/151 パス
- FrameAllocator のアライン問題を修正後、再テストで全通過

### Task 3: 欠落 Phase Summary 作成 — 完了
- Phase8_Summary.md / Phase9_Summary.md / Phase10a_Summary.md / Phase10c_Summary.md を追加

### Task 4: Project Directive 更新 — 完了
- `Knowledges/GXLib Project Directive.md` 8.4 に「完了日/期間/学び/申し送り」を追記

### Task 5: Doxygen 生成 — 任意
- Doxyfile は設定済み。生成は必要時に実施。

---

## Phase 10 項目の最終状況

| 項目 | 状態 | 備考 |
|------|------|------|
| メモリアロケータ (Pool, Frame) | 済 | FrameAllocator.h, PoolAllocator.h |
| リソースバリア最適化 | 済 | BarrierBatch.h |
| GPUタイムスタンプ・プロファイラ | 済 | Phase10a |
| 描画コールバッチング最適化 | 済 | SpriteBatch/PrimitiveBatch |
| Google Test ユニットテスト | 済 | 151 tests |
| APIリファレンス (HTML) | 済 | 1120エントリ, 137クラス |
| Doxygen設定 | 済 | Doxyfile |
| チュートリアル | 済 | 5本 |
| DXLib移行ガイド | 済 | docs/migration/DxLibMigrationGuide.md |
| サンプルプロジェクト | 済 | 5サンプル |
| README.md | 済 | 224行 |

---

## 将来の改善タスク (Phase10+ / 非ブロッカー)
- マルチスレッド CommandList 記録
- テクスチャストリーミング (mip 分割 + LRU)
- GPU回帰テスト (スクリーンショット比較)
- メモリリーク検出 (CRT Debug Heap + Live Objects)
- D3D12 Debug Layer の詳細オプション化

---

## 変更履歴
- 2026-02-10: FrameAllocator アライン修正、テスト全通過、Phase Summary 補完、Directive更新


---

# Part VIII: レポート・分析

## Bug Report

**Date:** 2026-02-17
**Scope:** Full project scan + Ray Tracing 詳細調査 + 計算式・手法検証

---

## 全体サマリー

| セクション | Critical | High | Medium | Low/Info | 合計 |
|---|---|---|---|---|---|
| 全体スキャン | 3 | 21 | 21 | 8 | 53 |
| RT 詳細調査 | 3 | 3 | 7 | 5 | 18 |
| 計算式・手法検証 | 0 | 0 | 7 | 3 | 10 |
| **合計（重複除く）** | **6** | **24** | **32** | **14** | **76** |

> **注:** H-04（Skeleton 行列乗算順序）は計算式検証で正しいことが確認されたため除外済み。

---

## Critical

### C-01: DropDown - 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:178`
- **Category:** Buffer Overflow
- **Description:** `RenderSelf()` 内のラムダで `wideItems[i]` にアクセスするが、`m_items` と `m_wideItems` のサイズが異なる場合（`SetItems()` レンダリング中呼び出し等）、範囲外アクセスが発生する。

### C-02: ListView - 配列の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/ListView.cpp:134`
- **Category:** Buffer Overflow
- **Description:** `m_wideItems[i]` にアクセスする際、`i` は `m_items.size()` で制限されるが、`m_wideItems` のサイズが同一であることを検証していない。サイズ不一致時に範囲外アクセスが発生。

### C-03: TextureManager::CreateRegionHandles - 負インデックスで配列アクセス
- **File:** `GXLib/Graphics/Resource/TextureManager.cpp:184-187`
- **Category:** Out-of-Bounds Access
- **Description:** `AllocateHandle()` が失敗時に返す値を検証せずに `m_entries[handle]` にアクセス。負のインデックスで vector にアクセスするため未定義動作。ループ内の `if (firstHandle == -1)` は最初の呼び出しのみチェックしており、AllocateHandle の失敗チェックではない。

---

## High

### H-01: TextRenderer - vswprintf_s のバッファサイズ引数欠落
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:157`
- **Category:** Buffer Overflow
- **Description:** `vswprintf_s(buffer, format, args)` で第2引数のバッファサイズ（1024）が欠落。セキュア版 `vswprintf_s` は4引数必須。バッファオーバーフローのリスク。

### H-02: SpriteBatch::Begin - Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:199`
- **Category:** Null Pointer Dereference
- **Description:** `m_mappedVertices = static_cast<SpriteVertex*>(m_vertexBuffer.Map(frameIndex))` の戻り値が null の場合、後続の `AddQuad()` で null ポインタ参照。

### H-03: PrimitiveBatch::Begin - Map の戻り値未チェック
- **File:** `GXLib/Graphics/Rendering/PrimitiveBatch.cpp:133-134`
- **Category:** Null Pointer Dereference
- **Description:** `m_mappedTriVertices` と `m_mappedLineVertices` の両方が、`Map()` 失敗時に null のまま描画関数で使用される。

### ~~H-04: Skeleton - 行列乗算の順序誤り~~ **【誤検出 — 正しいことを確認済み】**
- **File:** `GXLib/Graphics/3D/Skeleton.cpp:35`
- **Category:** ~~Math Error~~ → **False Positive**
- **Description:** `XMMATRIX bone = invBind * global;` は DirectX の行ベクトル規約（v × M）で正しい順序。`invBindPose × currentGlobalTransform` はスケルタルアニメーションの標準的な骨行列合成公式であり、バグではない。計算式検証レポート（後述）で検証確認済み。

### H-05: AutoExposure - マップドポインタの未検証デリファレンス
- **File:** `GXLib/Graphics/PostEffect/AutoExposure.cpp:231`
- **Category:** Null Pointer Dereference
- **Description:** `Map()` 成功後、`mapped` ポインタを null チェックせずに `*reinterpret_cast<const uint16_t*>(mapped)` でデリファレンス。

### H-06: RTReflections::OnResize - HRESULT 未チェック
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:574-577`
- **Category:** Missing Error Handling
- **Description:** `CreateCommittedResource` の戻り値を確認していない。リソース作成失敗時 `m_halfResUAV` が null のまま後続で使用される。

### H-07: PostEffectPipeline - null リソースへの UAV バリア
- **File:** `GXLib/Graphics/PostEffect/PostEffectPipeline.cpp:440-446`
- **Category:** Null Pointer Dereference
- **Description:** `m_halfResUAV.Get()` が null の場合（H-06 の結果）、null リソースに対して UAV バリアが発行される。

### H-08: RTReflections - m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **Category:** Null Pointer Dereference
- **Description:** `m_normalRT` が null の場合、`CreateShaderResourceView` に nullptr が渡される。一部コードパスでは null チェックがあるが、全パスでは不統一。

### H-09: SSR - normalRT の SRV バインド未検証
- **File:** `GXLib/Graphics/PostEffect/SSR.cpp:138`
- **Category:** Resource Binding Mismatch
- **Description:** `UpdateSRVHeap` で normalRT の SRV を常に作成するが、バインド状態の検証が欠如しており、他システムによる変更でシェーダーリソースの不整合が発生する可能性。

### H-10: HTTPClient - 非同期操作のリソースリーク
- **File:** `GXLib/IO/Network/HTTPClient.cpp:195-210`
- **Category:** Resource Leak / Use-After-Free
- **Description:** `GetAsync()`/`PostAsync()` でスレッドが `.detach()` で起動されるが、HTTPClient 破棄時にスレッドが実行中の場合、キャプチャした `this` ポインタがダングリングになる。

### H-11: WebSocket - ReceiveLoop の Use-After-Free
- **File:** `GXLib/IO/Network/WebSocket.cpp:115-250`
- **Category:** Use-After-Free
- **Description:** `Close()` で `m_running=false` 設定後にスレッドを join するが、`ReceiveLoop()` が `m_hWebSocket` アクセス中に `Close()` でハンドルが無効化される競合。mutex による同期が必要。

### H-12: AsyncLoader - 破棄時のレースコンディション
- **File:** `GXLib/IO/AsyncLoader.cpp:12-21`
- **Category:** Race Condition
- **Description:** デストラクタで `m_running=false` 後に `notify_one()` するが、ワーカースレッドの `m_completedQueue` / `m_statusMap` アクセスと同時実行される競合。

### H-13: MoviePlayer - null ポインタデリファレンス
- **File:** `GXLib/Movie/MoviePlayer.cpp:104, 288-299`
- **Category:** Null Pointer Dereference
- **Description:** `pOutputType->Release()` が null チェックなし。`Close()` 後に `m_texManager` が nullptr のまま使用される可能性。

### H-14: Compat_2D LoadDivGraph - null ポインタ・バッファオーバーフロー
- **File:** `GXLib/Compat/Compat_2D.cpp:61-81`
- **Category:** Null Pointer / Buffer Overflow
- **Description:** `handleBuf` ポインタの null チェックが欠如。また `allNum` の妥当性検証なし。大きな値で配列外書き込みが発生。

### H-15: RTReflections.hlsl - cbuffer コメントとアクセスの不一致
- **File:** `Shaders/RTReflections.hlsl:40-44, 238-239`
- **Category:** Shader Resource Mismatch
- **Description:** `g_InstanceRoughnessGeom` は `.x=roughness, .y=geometryIndex` のみ文書化されているが、シェーダーで `.z`(texIdx) と `.w`(hasTexture) にもアクセス。C++ 側で `.z/.w` が未設定の場合、未初期化データが使用される。

### H-16: Texture - 大サイズテクスチャでの整数オーバーフロー
- **File:** `GXLib/Graphics/Resource/Texture.cpp:104-106, 246-248`
- **Category:** Integer Overflow
- **Description:** `rowPitch * height` の計算で uint32_t オーバーフローが発生する可能性。例: 16384x16384 テクスチャで `65536 * 16384 = 1GB` が uint32_t 上限を超え、バッファサイズが不足してバッファオーバーフロー。

### H-17: BarrierBatch - m_barriers 配列の未初期化
- **File:** `GXLib/Graphics/Device/BarrierBatch.h:22, 62-64`
- **Category:** Uninitialized Data
- **Description:** `m_count` は 0 に初期化されるが、`m_barriers[]` 配列はゼロクリアされない。バリアを Add して Flush する正常フローでは問題ないが、`m_count` が不正な値を持った場合、ガベージデータが GPU に送られる。

### H-18: FontManager - pixelData の null チェック欠如
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:313`
- **Category:** Null Pointer Dereference
- **Description:** `lock->GetDataPointer(&bufferSize, &pixelData)` 後、`pixelData` が null でないことを確認せずに memcpy ループで使用。

### H-17: DropDown::OnEvent - 空アイテム時の範囲外アクセス
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:61`
- **Category:** Out-of-Bounds
- **Description:** `SetItems()` で空ベクトルを設定した場合、`m_selectedIndex=0` が残り、`onValueChanged(m_items[m_selectedIndex])` で範囲外アクセス。

### H-18: TextInput::DeleteSelection - 選択範囲の境界値不正
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:93`
- **Category:** Logic Error
- **Description:** `m_selStart`/`m_selEnd` が不正な値の場合、`erase(s, e-s)` に負の値が渡される可能性。

### H-19: ScrollView - ゼロ除算リスク
- **File:** `GXLib/GUI/Widgets/ScrollView.cpp:25`
- **Category:** Division by Zero
- **Description:** `viewH` が 0 または負の場合、スクロール計算のクランプロジックが破綻。

### H-20: 複数ウィジェット - m_renderer の null チェック不統一
- **Files:** `Button.h, TextWidget.h, CheckBox.h, DropDown.h, ListView.h, RadioButton.h, TabView.h, TextInput.h`
- **Category:** Null Pointer Dereference
- **Description:** 複数のウィジェットで `m_renderer` を null チェックなしで使用するコードパスが存在。一部のメソッドではチェックしているが不統一。

---

## Medium

### M-01: TextRenderer - 改行文字の比較誤り
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:101`
- **Category:** Logic Error
- **Description:** `DrawStringTransformed()` で `L'\\n'`（エスケープされた文字列）と比較しており、実際の改行文字 `L'\n'` を正しく検出できない。`DrawString()` の line 41 では正しく `L'\n'` と比較。

### M-02: FontManager - 未初期化エントリへのアクセス
- **File:** `GXLib/Graphics/Rendering/FontManager.cpp:410`
- **Category:** Uninitialized Data
- **Description:** `AllocateHandle()` がフリーリストからハンドルを再利用する場合、`m_entries[handle]` が未初期化の `FontEntry` を返す可能性。

### M-03: TextRenderer - テクスチャ座標のオーバーフロー
- **File:** `GXLib/Graphics/Rendering/TextRenderer.cpp:65`
- **Category:** Integer Overflow
- **Description:** `glyph->u0 * FontManager::k_AtlasSize` が float→int キャスト時に k_AtlasSize(2048) と等しい場合、テクスチャ範囲外アクセス。クランプ処理なし。

### M-04: RTReflections - 冗長なリソース状態遷移
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:306-309, 466-469`
- **Category:** Logic Error
- **Description:** `srcHDR`, `depth`, `m_normalRT` が NON_PIXEL_SHADER_RESOURCE に遷移後、PIXEL_SHADER_RESOURCE に再遷移するが、状態チェックなし。冗長な GPU コマンド。

### M-05: RTReflections::BuildBLAS - エラーハンドリング不足
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:131-142`
- **Category:** Missing Error Handling
- **Description:** `BuildBLAS` の戻り値 `idx` が範囲外の場合、`m_blasGeometry.resize(idx + 1)` でギャップのあるデフォルト初期化エントリが生成される。

### M-06: RTAccelerationStructure - ストライド検証不足
- **File:** `GXLib/Graphics/RayTracing/RTAccelerationStructure.cpp:42`
- **Category:** Integer Overflow
- **Description:** `VertexBuffer.StrideInBytes` が 0 の場合や vertexCount との積でオーバーフローする場合、無効な GPU 操作が発生。

### M-07: RTReflections - テクスチャスロットオーバーフロー
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:180-185`
- **Category:** Buffer Overflow
- **Description:** 32 テクスチャを超えて追加された場合、ディスクリプタヒープサイズ（40+32=72）を超える SRV 作成が発生する可能性。

### M-08: Bloom::OnResize - エラー伝播なし
- **File:** `GXLib/Graphics/PostEffect/Bloom.cpp:296-299`
- **Category:** Missing Error Handling
- **Description:** `CreateMipRenderTargets` が void 型のため、`RenderTarget::Create` 失敗時のエラーが伝播されない。無効なレンダーターゲットで処理続行。

### M-09: Texture - CreateEvent の戻り値未チェック
- **File:** `GXLib/Graphics/Resource/Texture.cpp:213, 356`
- **Category:** Missing Error Handling
- **Description:** `CreateEvent()` が NULL を返した場合のエラーハンドリングが欠如。`WaitForSingleObject` に NULL ハンドルが渡される。

### M-10: FileWatcher - イベントハンドルリーク
- **File:** `GXLib/IO/FileWatcher.cpp:42, 57`
- **Category:** Resource Leak
- **Description:** `CreateEventA()` の戻り値を確認せず、NULL でも `ReadDirectoryChangesW()` に渡される。

### M-11: Crypto - 不統一なエラーハンドリング
- **File:** `GXLib/IO/Crypto.cpp:17-30, 83-93`
- **Category:** Error Handling
- **Description:** `Encrypt()` と `Decrypt()` でエラーログの出力が不統一。一部パスで `hAlg` のクリーンアップが欠落。

### M-12: Archive - 整数オーバーフロー
- **File:** `GXLib/IO/Archive.cpp:62`
- **Category:** Integer Overflow
- **Description:** `tocSize` が極端に大きい場合、`resize()` でメモリ不足が発生するが、上限チェックがない。不正なアーカイブファイルで DoS 可能。

### M-13: Sound - ファイル読み込みエラーハンドリング不足
- **File:** `GXLib/Audio/Sound.cpp:44, 60-61, 68-72`
- **Category:** Uninitialized Data
- **Description:** 複数の `file.read()` 呼び出しで読み取り成功を確認していない。トランケートされた WAV ファイルで部分的に読み取られたデータが有効として処理される。

### M-14: SoundPlayer - コールバックの寿命管理
- **File:** `GXLib/Audio/SoundPlayer.cpp:38-47`
- **Category:** Dangling Pointer
- **Description:** `VoiceCallback` の所有権が `m_activeVoices` に移動されるが、外部からボイスが破棄された場合コールバックがダングリングに。

### M-15: PhysicsWorld3D - シェイプ作成の null チェック不足
- **File:** `GXLib/Physics/PhysicsWorld3D.cpp:286, 295, 304`
- **Category:** Null Pointer Dereference
- **Description:** `new JPH::ShapeRefC(...)` の結果を検証せずに割り当て。Jolt 側で例外が発生した場合のハンドリングなし。

### M-16: TextInput - ループ条件の off-by-one
- **File:** `GXLib/GUI/Widgets/TextInput.cpp:203`
- **Category:** Logic Error
- **Description:** `for (int i = 1; i <= static_cast<int>(display.size()); ++i)` で `<=` 使用。空文字列時に不正なアクセス。

### M-17: TabView - activeTab の範囲検証なし
- **File:** `GXLib/GUI/Widgets/TabView.cpp:40`
- **Category:** Logic Error
- **Description:** `m_activeTab` が -1 や children.size() 以上の場合、全子要素が非表示になる。範囲検証なし。

### M-18: DropDown::SetItems - selectedIndex の検証不正
- **File:** `GXLib/GUI/Widgets/DropDown.cpp:26-27`
- **Category:** Off-by-One
- **Description:** `m_selectedIndex >= items.size()` でリセットするが、`items` が空の場合 `m_selectedIndex=0` が設定され不正。

### M-19: PostEffectShowcase - VSync と TargetFps の矛盾
- **File:** `Samples/PostEffectShowcase/main.cpp:44-45`
- **Category:** Logic Error
- **Description:** `config.vsync = true` と `config.targetFps = 240` が同時設定。VSync 有効時は `targetFps` が無視されるため、設定が矛盾。

### M-20: TextWidget - GetIntrinsicHeight のフォールバック不整合
- **File:** `GXLib/GUI/Widgets/TextWidget.cpp:15`
- **Category:** Logic Error
- **Description:** `m_fontHandle < 0` の場合 `computedStyle.fontSize` を返すが、`GetIntrinsicWidth()` は同条件で `0.0f` を返す。不整合な挙動。

### M-21: SpriteBatch - AddQuad の境界チェック不正
- **File:** `GXLib/Graphics/Rendering/SpriteBatch.cpp:246`
- **Category:** Logic Error
- **Description:** `m_vertexWriteOffset + m_spriteCount >= k_MaxSprites` のチェックは、現在追加中の4頂点を考慮していない。境界条件で1スプライト分オーバーする可能性。

---

## Low

### L-01: SSAO - カーネルサイズ0で除算ゼロ
- **File:** `GXLib/Graphics/PostEffect/SSAO.cpp`
- **Category:** Division by Zero
- **Description:** `k_KernelSize == 0` の場合、カーネルスケール計算で除算ゼロ。実質的にはコンパイル時定数のため発生しにくい。

### L-02: VolumetricLight - 未初期化 XMFLOAT3
- **File:** `GXLib/Graphics/PostEffect/VolumetricLight.cpp:126-127`
- **Category:** Uninitialized Variable
- **Description:** `sunNDC` が `XMStoreFloat3` で条件的に割り当て。Transform が無効データを返した場合、未初期化値が使用される。

### L-03: MeshCollider - 除算ゼロ
- **File:** `GXLib/Physics/MeshCollider.cpp:50, 79`
- **Category:** Division by Zero
- **Description:** `1.0f / weld` で `weld=0` のガードはあるが、負の値が通過する。`step = vertices.size() / maxPoints` で `step=0` の可能性。

### L-04: PhysicsWorld2D - Raycast 出力ポインタ未検証
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:260-262`
- **Category:** Null Pointer Dereference
- **Description:** `outBody`, `outPoint`, `outNormal` を null チェックせずデリファレンス。

### L-05: Random - 無限ループリスク
- **File:** `GXLib/Math/Random.cpp:54-98`
- **Category:** Infinite Loop
- **Description:** Rejection sampling の `for(;;)` ループに最大試行回数の制限なし。乱数生成が壊れた場合に無限ループ。

### L-06: Collision3D::ClosestPointOnLine - 除算ゼロ
- **File:** `GXLib/Math/Collision/Collision3D.cpp:426`
- **Category:** Division by Zero
- **Description:** `ab.Dot(ab)` が除算に使用されるが、線分長ゼロ（同一点）の場合 infinity/NaN が発生。

### L-07: Image Widget - UV オフセットの浮動小数点精度
- **File:** `GXLib/GUI/Widgets/Image.cpp:16-19`
- **Category:** Float Precision
- **Description:** `std::fmod()` で UV オフセットをラップするが、連続更新で浮動小数点誤差が蓄積。

### L-08: StyleSheet::ParseLength - 例外ハンドリング欠如
- **File:** `GXLib/GUI/StyleSheet.cpp:557`
- **Category:** Exception Handling
- **Description:** `std::stof()` が不正フォーマットで例外を投げるが、try-catch がない。

---

## Priority Fix Order

1. **Critical (C-01, C-02):** GUI の配列範囲外アクセス - クラッシュ直結
2. **High (H-01):** vswprintf_s のバッファサイズ - セキュリティ脆弱性
3. **High (H-10, H-11, H-12):** スレッド安全性 - 非同期クラッシュ
4. **High (H-02, H-03, H-05):** null チェック - 初期化失敗時のクラッシュ
5. **High (H-15):** シェーダー cbuffer 不一致 - レンダリング不正
6. **Medium (M-01):** 改行文字の比較誤り - テキスト描画不正

---
---

# Ray Tracing 詳細調査レポート

**Date:** 2026-02-17
**Scope:** GXLib/Graphics/RayTracing/, Shaders/RTReflections*.hlsl, 統合ポイント (PostEffectPipeline, Sandbox, DXRShowcase)

## 概要

レイトレーシングサブシステム全体を C++ コード・HLSL シェーダー・統合ポイントの 3 軸で精査した結果、**Critical 3件、High 3件、Medium 7件、Low/Info 5件** の不整合を検出。

---

## RT-Critical

### RT-C01: Sandbox で CreateGeometrySRVs() が呼ばれていない
- **File:** `Sandbox/main.cpp`
- **Category:** Missing Initialization
- **Description:** BLAS 構築後に `CreateGeometrySRVs()` が呼ばれていない。Dispatch ヒープのスロット [8..39]（ジオメトリ VB/IB SRV）および [40..71]（アルベドテクスチャ SRV）が初期化されないまま DispatchRays が実行される。
- **Impact:** ClosestHit シェーダーで `g_GeometryBuffers[geomIdx*2]` アクセス時にゴミデータまたは無効ディスクリプタが参照される。GPU ハング・クラッシュの原因。
- **Verification:** `Samples/DXRShowcase/main.cpp:459` では正しく `CreateGeometrySRVs()` が呼ばれている。Sandbox には該当呼び出しが存在しない（grep 確認済み）。
- **Fix:** BLAS 構築 + GPU フラッシュ後に `g_rtReflections->CreateGeometrySRVs();` を追加。

### RT-C02: リソースポインタの寿命管理不備（Use-After-Free リスク）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h:184-196`
- **Category:** Memory Safety
- **Description:** `m_textureResources` と `m_blasGeometry` が `std::vector<ID3D12Resource*>`（生ポインタ）で保持されている。アプリケーション側でテクスチャやメッシュが解放・再作成された場合、ダングリングポインタが残る。
- **Impact:** シェーダーが解放済み GPU メモリを読み取り → GPU ハング、TDR、不正レンダリング。
- **Fix:** `ComPtr<ID3D12Resource>` に変更するか、フレーム開始時に参照を再取得する設計に変更。

### RT-C03: ディスクリプタヒープ スロット衝突（frameIndex >= 2 の場合）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:341`
- **Category:** Descriptor Heap Collision
- **Description:** `uint32_t heapBase = frameIndex * 4;` でフレーム毎のディスクリプタ位置を計算。ジオメトリ SRV は固定スロット 8 から開始。
  - Frame 0: heapBase=0（スロット 0-3） → OK
  - Frame 1: heapBase=4（スロット 4-7） → OK
  - Frame 2: heapBase=8（スロット 8-11） → **ジオメトリスロットと衝突**
  - Frame 3: heapBase=12（スロット 12-15） → **衝突**
- **Risk:** 現在 `k_BufferCount=2` のため frameIndex は 0 か 1 で実質問題ないが、バッファ数変更時に即座に破綻する脆弱な設計。
- **Fix:** フレーム毎スロットをジオメトリ/テクスチャスロットの後に配置するか、専用ヒープで分離。

---

## RT-High

### RT-H01: R16_UINT インデックスバッファ非対応（シェーダー側ハードコード）
- **File:** `Shaders/RTReflections.hlsl:193-196`
- **Category:** Format Compatibility
- **Description:** ClosestHit シェーダーでインデックスバッファのロードが R32_UINT 前提でハードコード：
  ```hlsl
  uint i0 = ib.Load(primIdx * 12 + 0);   // 4バイト×3 = 12バイト/三角形
  uint i1 = ib.Load(primIdx * 12 + 4);
  uint i2 = ib.Load(primIdx * 12 + 8);
  ```
  しかし C++ 側 `RTReflections::BuildBLAS()` は `DXGI_FORMAT_R16_UINT` も受け付ける。R16_UINT 使用時はストライド 6 バイト/三角形になるため、完全に誤ったインデックスがロードされる。
- **Impact:** R16_UINT メッシュの反射が完全に壊れる（不正な三角形参照 → ゴミジオメトリ）。
- **Fix:** (a) BuildBLAS で R32_UINT のみ許可するか、(b) インデックスフォーマットを cbuffer でシェーダーに渡して分岐。

### RT-H02: AddInstance() にインスタンス数上限チェックなし
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:165-203`
- **Category:** Buffer Overflow
- **Description:** `AddInstance()` は `m_instanceData.push_back()` するが、`k_MaxInstances=512` を超えた場合のガードがない。GPU 側 cbuffer は 512 要素固定（`g_InstanceAlbedoMetallic[512]`）のため、512 を超える `InstanceIndex()` でシェーダーが配列外アクセス。
- **Impact:** シェーダー内での未定義動作（GPU ハング、不正レンダリング）。
- **Fix:** `if (m_instanceData.size() >= k_MaxInstances) { LOG_WARN(...); return; }`

### RT-H03: BLAS ジオメトリ配列のギャップ（不連続インデックス）
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:138-140`
- **Category:** Array Integrity
- **Description:** `BuildBLAS` の戻り値 `idx` が不連続の場合、`m_blasGeometry.resize(idx + 1)` でギャップ要素がデフォルト初期化される。`CreateGeometrySRVs()` はギャップ位置で null リソースの SRV を作成する。
- **Impact:** ClosestHit で `geomIdx` がギャップ位置を指すと、null SRV からの読み取りが発生。

---

## RT-Medium

### RT-M01: 深度バッファ状態遷移の非対称性
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:307, 467, 547`
- **Category:** Resource State
- **Description:** 深度バッファの状態遷移フロー：
  1. L307: `NON_PIXEL_SHADER_RESOURCE` に遷移（Dispatch 用）
  2. L467: `PIXEL_SHADER_RESOURCE` に遷移（Composite 用）
  3. L547: `DEPTH_WRITE` に遷移（復元）
  呼び出し元（PostEffectPipeline）が深度を `PIXEL_SHADER_RESOURCE` として期待している場合、`DEPTH_WRITE` で返却されるため不整合。

### RT-M02: OnResize() でディスクリプタヒープ未更新
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:550-580`
- **Category:** Resize Handling
- **Description:** `OnResize()` で `m_halfResUAV` は再作成されるが、`m_dispatchHeap` と `m_compositeHeap` は更新されない。次の `Execute()` でディスクリプタが再作成されるため通常動作するが、リサイズ直後の描画で旧ディスクリプタが使用されるリスク。

### RT-M03: 深度 SRV フォーマットのハードコード
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:354`
- **Category:** Format Assumption
- **Description:** `srvDesc.Format = DXGI_FORMAT_R32_FLOAT` が固定値。`DepthBuffer` の実際のフォーマット（`D24_UNORM_S8_UINT` 等）と不一致の場合、SRV 作成失敗または不正データ。
- **Fix:** `depth.GetResource()->GetDesc().Format` からフォーマットを取得。

### RT-M04: テクスチャスロットの SRV がフレーム跨ぎで残留
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:156-163`
- **Category:** Stale Descriptor
- **Description:** `BeginFrame()` で `m_textureLookup` と `m_textureResources` はクリアされるが、ディスパッチヒープのスロット [40..71] にある SRV ディスクリプタは上書きされない。同一スロットに異なるテクスチャが割り当てられた場合、旧フレームの SRV が参照される可能性。

### RT-M05: m_normalRT の null チェック不完全
- **File:** `GXLib/Graphics/RayTracing/RTReflections.cpp:308-360`
- **Category:** Null Safety
- **Description:** `m_normalRT` が null の場合、L360 で `nullptr` が `CreateShaderResourceView` に渡される。D3D12 は null SRV を許容するが、シェーダー側（`g_Normal.SampleLevel()`）では null チェックなく使用される。

### RT-M06: ClosestHit の法線方向保証なし
- **File:** `Shaders/RTReflections.hlsl:221-228`
- **Category:** Shader Logic
- **Description:** 補間頂点法線 `N` は表面向きであることが保証されていない。ジオメトリック法線 `Ng` には表裏チェック（L227-228）があるが、シェーディング法線 `N` にはない。裏面ヒット時に BRDF 計算が不正になる。
- **Mitigation:** L310-312 の grazing angle 補正で部分的に対処されているが、完全ではない。

### RT-M07: Composite パスの alpha コメント誤記
- **File:** `Shaders/RTReflectionComposite.hlsl:89`
- **Category:** Documentation
- **Description:** コメント「alpha = フレネル × ヒット種別重み」だが、実際は alpha はヒット種別のみ（Miss=0.5, ClosestHit=1.0）。フレネルは BRDF 計算で RGB に含まれる。

---

## RT-Low/Info

### RT-L01: cbuffer コメントの不完全さ（既報 H-15 の詳細）
- **File:** `Shaders/RTReflections.hlsl:40-44`
- **Description:** `g_InstanceRoughnessGeom` のコメントが `.x=roughness, .y=geometryIndex` のみ。実際は `.z=texIdx, .w=hasTexture` も使用されている。C++ 側（`RTReflections.cpp:201`）では全 4 成分を設定済み。

### RT-L02: 半解像度変数名が実態と不一致
- **File:** `GXLib/Graphics/RayTracing/RTReflections.h/cpp`
- **Description:** `m_halfResUAV`, `m_halfWidth`, `m_halfHeight` が実際にはフル解像度で使用されている。MEMORY.md にも「フル解像度 dispatch」と記載あり。

### RT-L03: SetCommandList4() が毎フレーム呼び出し
- **File:** `Sandbox/main.cpp, PostEffectPipeline.h:129`
- **Description:** `SetCommandList4()` は初期化時に 1 回で十分だが、毎フレーム呼ばれている。パフォーマンス影響は微小だが設計として冗長。

### RT-L04: HDR クランプ値の固定
- **File:** `Shaders/RTReflections.hlsl:340`
- **Description:** `color = min(color, 5.0)` で HDR 値を 5.0 にクランプ。ACES Tonemap で ACES(5.0)≈0.96 のため妥当だが、定数化されておらずトーンマッパー変更時に調整が必要。

### RT-L05: ポイントライトのインスタンスマスク未対応
- **File:** `Shaders/RTReflections.hlsl:325-338`
- **Description:** ClosestHit でポイントライトは全インスタンスに一律適用。TLAS インスタンスマスクによるライティング除外に非対応。デモ用途では問題ないが、プロダクション用途では制限。

---

## RT 修正優先順位

1. **RT-C01 (CRITICAL):** Sandbox の `CreateGeometrySRVs()` 追加 — 即座にクラッシュ回避
2. **RT-C02 (CRITICAL):** リソースポインタを `ComPtr` 化 — Use-After-Free 防止
3. **RT-H01 (HIGH):** R16_UINT インデックス対応 or 制限 — メッシュ互換性確保
4. **RT-H02 (HIGH):** AddInstance の上限チェック追加 — バッファオーバーフロー防止
5. **RT-C03 (CRITICAL):** ディスクリプタヒープスロット設計見直し — 将来の拡張性確保
6. **RT-H03 (HIGH):** BLAS インデックスの連続性保証 — null SRV 回避
7. **RT-M01〜M07:** 中優先度の修正（深度状態遷移、フォーマット検証、法線安全性等）

---
---

# 計算式・手法 検証レポート

**Date:** 2026-02-17
**Scope:** PBR/BRDF 数式、3D 数学（行列・クォータニオン）、物理シミュレーション、DXR レイトレーシング、ポストエフェクト

## 概要

プロジェクト全体の計算式と使用手法を検証した結果、**数式バグ 5件、手法上の問題 3件、品質制限 2件** を検出。PBR BRDF（GGX NDF, Schlick Fresnel, Smith Geometry, Cook-Torrance）およびレンダリングパイプライン（シェーダーテーブル、BLAS/TLAS 構築、リソースバインディング、cbuffer レイアウト）は全て正確であることを確認。

---

## 検証済み（正確）

以下の数式・手法は全て正しいことを検証済み：

| 対象 | ファイル | 結果 |
|---|---|---|
| GGX NDF (Trowbridge-Reitz) | `PBRCommon.hlsli:12-23` | alpha=roughness^2 の Disney 規約、除算ゼロガード付き ✓ |
| Schlick Fresnel | `PBRCommon.hlsli:47-50` | F0 + (1-F0)(1-cosθ)^5、saturate ガード ✓ |
| Smith Geometry (Schlick-GGX) | `PBRCommon.hlsli:28-42` | k=(r+1)^2/8 は直接光用で正しい ✓ |
| Cook-Torrance BRDF | `PBRCommon.hlsli:55-80` | DGF/(4·NdotV·NdotL) + ε ✓ |
| Lambert Diffuse | `PBRCommon.hlsli:76` | albedo/π 正規化 ✓ |
| エネルギー保存 | `PBRCommon.hlsli:73-74` | kD=(1-kS)(1-metallic) ✓ |
| 法線マッピング TBN | `PBR.hlsl:200-205` | float3x3(T,B,N) + mul(normalMap,TBN) ✓ |
| Specular Anti-Aliasing | `PBR.hlsl:220-226` | Tokuyoshi & Kaplanyan 2019 方式 ✓ |
| ワールド座標復元 | `RTReflections.hlsl:83-89` | UV→NDC→clip→invVP→perspDiv ✓ |
| 重心座標補間 | `RTReflections.hlsl:214-219` | DXR BuiltInTriangleIntersectionAttributes 準拠 ✓ |
| シェーダーテーブル配置 | `RTPipeline.cpp:202-237` | AlignUp(32,32)=32, テーブル64B境界 ✓ |
| DispatchRays 記述子 | `RTPipeline.cpp:250-261` | Stride, Size, StartAddress 全正確 ✓ |
| BLAS/TLAS 構築 | `RTAccelerationStructure.cpp` | PrebuildInfo, UAV バリア, Transform 転置 ✓ |
| ルートシグネチャ←→HLSL | `RTPipeline.cpp + RTReflections.hlsl` | 全7パラメータ一致 ✓ |
| cbuffer レイアウト | `RTReflections.h:22-44 + .hlsl:14-37` | 320B C++/HLSL 完全一致 ✓ |
| SSAO サンプリング | `SSAO.hlsl` | 半球サンプル、レンジチェック、bilateral blur ✓ |
| Bloom Karis Average | `Bloom.hlsl:38-56` | w=1/(1+lum) ファイアフライ抑制 ✓ |
| TAA 分散クリッピング | `TAA.hlsl:60-85` | Salvi 2016 / Karis 2014 方式 ✓ |
| Motion Blur リプロジェクション | `MotionBlur.hlsl:33-44` | UV→world→prevClip→prevUV ✓ |
| Transform3D SRT 合成 | `Transform3D.cpp:14` | S*R*T（行ベクトル規約） ✓ |
| Skeleton 骨行列 | `Skeleton.cpp:35` | invBind*global（行ベクトル規約で正しい） ✓ |
| CSM フラスタム計算 | `CascadedShadowMap.cpp:60-82` | tanHalfFov → ビュー空間コーナー ✓ |
| Collision3D SAT (OBB) | `Collision3D.cpp:91-148` | 15軸テスト (Ericson 準拠) ✓ |
| Moller-Trumbore | `Collision3D.cpp:240-269` | レイ-三角形交差 ✓ |
| 自動露出 | `AutoExposure.hlsl + .cpp` | log輝度→exp逆変換→指数平滑 ✓ |

---

## MATH-BUG: 数式バグ

### MATH-01: Quaternion::ToEuler() の符号誤り
- **File:** `GXLib/Math/Quaternion.h:88-101`
- **Severity:** Medium
- **Description:** `XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)` は回転順 Z×Y×X（外的 XYZ）を適用する。この規約に対応するオイラー角抽出公式は：
  ```
  sinP = 2(wx + yz)   // 正しい
  ```
  しかし実装では：
  ```cpp
  float sinP = 2.0f * (w * x - y * z);  // 符号が逆
  ```
  同様に yaw（L95: `w*y + z*x` → 正しくは `w*y - z*x`）と roll（L99: `w*z + x*y` → 正しくは `w*z - x*y`）も符号が反転。`FromEuler() → ToEuler()` のラウンドトリップが不正確になる。
- **Impact:** Quaternion からオイラー角を取得する全てのコードパスに影響。デバッグ表示やエディタ用途で使用されている場合、誤った角度が表示される。

### MATH-02: PhysicsWorld2D — 角トルクに質量の逆数を使用（慣性モーメントの逆数が正しい）
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:65`
- **Severity:** Medium-High
- **Description:**
  ```cpp
  body->angularVelocity += body->m_torqueAccum * (body->InverseMass() * dt);
  ```
  正しい物理式は `角加速度 = トルク / 慣性モーメント(I)` であり、`トルク / 質量(m)` ではない。
  - 円形: `I = 0.5 × m × r²`
  - 矩形: `I = (1/12) × m × (w² + h²)`

  `1/mass` を使用すると、大きな物体の回転が速すぎ、小さな物体の回転が遅すぎる不正確な挙動となる。

### MATH-03: PhysicsWorld2D — AABB ブロードフェーズが回転を無視
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:91-96`
- **Severity:** Medium
- **Description:**
  ```cpp
  return {
      body.position - body.shape.halfExtents,
      body.position + body.shape.halfExtents
  };
  ```
  矩形ボディの AABB 計算で `body.rotation` を考慮していない。回転した矩形の AABB は、4隅を回転してから min/max を取る必要がある。回転したボディは正しくない衝突判定範囲を持ち、本来衝突すべきペアが見逃される。

### MATH-04: RTReflections ClosestHit — 法線変換に ObjectToWorld3x4 を直接使用
- **File:** `Shaders/RTReflections.hlsl:222`
- **Severity:** Low-Medium
- **Description:**
  ```hlsl
  float3 N = normalize(mul((float3x3)ObjectToWorld3x4(), normalObj));
  ```
  非一様スケーリング時、法線の正しい変換には逆転置行列 `WorldToObject3x4()` の転置（= `mul(normalObj, (float3x3)WorldToObject3x4())`）を使用する必要がある。現在の実装では、例えば `scale={2,1,1}` のインスタンスで法線が歪む。
  - 一様スケールのみの場合は問題なし。
  - **参考:** 4gamer 記事でも言及されている通り、DXR の反射はラスタライズ結果と視覚的一貫性を保つ必要があり、法線の不正確さは反射品質に直結する。

### MATH-05: DepthOfField — ガウスカーネル重みの正規化不正
- **File:** `Shaders/DepthOfField.hlsl:74-82`
- **Severity:** Low
- **Description:** ガウス重みの合計：
  ```
  0.1963 + 2×(0.1745 + 0.1217 + 0.0667 + 0.0287 + 0.0097 + 0.0026)
  = 0.1963 + 2×0.4039 = 0.1963 + 0.8078 = 1.0041
  ```
  重みが 1.0 より 0.41% 大きく、ぼかし適用時に画像が微妙に明るくなる。

---

## TECH-ISSUE: 手法上の問題

### TECH-01: RTReflections — ポイントライトに太陽シャドウを再利用
- **File:** `Shaders/RTReflections.hlsl:336`
- **Severity:** Medium
- **Description:**
  ```hlsl
  color += brdfP * pointLightColor * pointLightIntensity * NdotLp * atten * shadow;
  ```
  `shadow` 変数は太陽方向のシャドウレイ結果（L282-306）。ポイントライトの遮蔽は太陽の遮蔽とは無関係であり、ポイントライトから見通しが効く表面でも太陽が遮蔽されていればポイントライトも暗くなってしまう。
  - **正しい実装:** ポイントライト用に別途シャドウレイ（ヒットポイント→ポイントライト方向）を発射するか、`shadow = 1.0` でポイントライトに影なしとする。
  - **参考:** 4gamer 記事の Futuremark アプローチ「レイトレを極力行わない」方針では、シャドウレイ本数を最小限にする戦略が推奨されているが、異なる光源のシャドウ結果を共有することは物理的に不正確。

### TECH-02: PhysicsWorld2D — 衝突解決に角インパルスなし
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:161-217`
- **Severity:** Medium
- **Description:** `ResolveCollision()` は法線・摩擦インパルスを線形速度にのみ適用し、角速度への影響を計算していない。物理的には、接触点が重心から離れている場合、衝撃は `ω += (r × J) / I` の角運動量変化を生じるべき。現在の実装ではオフセンター衝突でもオブジェクトが回転しない。

### TECH-03: PhysicsWorld2D — 摩擦計算にインパルス適用前の相対速度を使用
- **File:** `GXLib/Physics/PhysicsWorld2D.cpp:206`
- **Severity:** Low
- **Description:** `relVel`（L182 で計算）は法線インパルス適用前の値。L197-198 でインパルス適用後の速度を使用すべき。実際には誤差は小さく、多くの簡易物理エンジンで同様の簡略化が行われている。

---

## QUALITY: 品質制限（バグではないが改善余地あり）

### QUAL-01: TAA — HDR ブレンド前のトーンマップ未実装
- **File:** `Shaders/TAA.hlsl`
- **Description:** 現在のブレンドはリニア HDR 空間で行われる。プロダクション TAA（Karis 2014）では `color/(1+luminance)` 変換後にブレンドし逆変換する手法が標準。高輝度ハイライトでゴースティングが発生する可能性。

### QUAL-02: CSM — 対数分割未使用
- **File:** `GXLib/Graphics/3D/CascadedShadowMap.cpp:44-48`
- **Description:** カスケード分割は手動比率 `{0.05, 0.15, 0.4, 1.0}` による線形分割。Practical Split Scheme (PSSM) の対数/線形ブレンド（`C_log^i * (1-λ) + C_lin^i * λ`）を使用すると、近距離の影解像度が向上する。

---

## DXR 手法の検証（参考記事との比較）

4gamer 記事（Futuremark GDC 2018）では「レイトレーシングを極力行わないこと」が最適解として提唱されており、反射取得のみに DXR を活用する戦略が紹介されている。GXLib の DXR 実装はこの方針に沿っており：

| 項目 | GXLib 実装 | 評価 |
|---|---|---|
| レイ使用目的 | 反射 + シャドウレイのみ | ✓ 記事方針に合致 |
| ラスタライズ主体 | PBR パスは従来ラスタライズ | ✓ 正しいアプローチ |
| BLAS ビルドフラグ | PREFER_FAST_TRACE | ✓ 静的ジオメトリに適切 |
| TLAS ビルドフラグ | PREFER_FAST_BUILD | ✓ 毎フレーム再構築に適切 |
| シャドウレイ最適化 | ACCEPT_FIRST_HIT + SKIP_CLOSEST_HIT | ✓ 標準的な高速化手法 |
| 解像度 | フル解像度 dispatch | △ 記事はフル HD で約200万本が限界と指摘。半解像度の検討余地あり |

---

## 修正優先順位（計算式・手法）

1. **MATH-02 (M-H):** PhysicsWorld2D 角トルク — 物理シミュレーション全体に影響
2. **TECH-01 (M):** ポイントライトシャドウ — レンダリング品質に直結
3. **MATH-03 (M):** AABB ブロードフェーズ回転無視 — 衝突検出の信頼性
4. **TECH-02 (M):** 衝突解決の角インパルス欠如 — 物理リアリズム
5. **MATH-01 (M):** Quaternion::ToEuler() 符号 — 使用箇所次第で影響
6. **MATH-04 (L-M):** RT 法線変換 — 非一様スケール時のみ顕在化
7. **MATH-05 (L):** DoF ガウス重み — 0.4% の輝度増加（ほぼ不可視）
8. **TECH-03 (L):** 摩擦の相対速度 — 一般的な簡略化
9. **QUAL-01〜02:** 品質改善（TAA トーンマップ、CSM 対数分割）

## Project Analysis (2026-02-11)

## 1. Project Overview and Philosophy

GXLib is an ambitious C++20 game development framework designed as a complete, superior replacement for the existing DxLib, specifically targeting DirectX 12. Its core philosophy emphasizes functionality, performance, and ease of use, while avoiding excessive abstraction. The project prioritizes creating a correctly functioning system over immediate optimization, and maintains strong compatibility with DxLib's API where appropriate to ease migration. The development process is documented through detailed phase summaries, reflecting a structured and iterative approach.

## 2. Core Technologies and Standards

*   **Language**: C++20
*   **API**: DirectX 12
*   **Build System**: CMake (targeting Visual Studio 2022 v145 toolset)
*   **Shader Compiler**: Windows SDK DXC (dxcompiler.dll)
*   **Platform**: Windows
*   **Coding Standards**:
    *   **Namespace**: `GX::` (often nested, e.g., `GX::GUI::`)
    *   **Naming Conventions**: PascalCase for classes, methods, and types; `m_camelCase` for member variables.
    *   **Character Encoding**: `/utf-8` for source files (supports Japanese comments).
    *   **PCH**: `pch.h` for common headers (Windows, DX12, STL, DirectXMath).

## 3. Architectural Overview

GXLib is structured into several modular subsystems, each responsible for a specific aspect of the engine. Key modules include Core, Graphics (further subdivided into Device, Pipeline, Resource, Rendering, 3D, Layer, PostEffect), GUI, Input, Audio, IO, Math, and Physics. A compatibility layer provides DxLib-like APIs, and a comprehensive ShaderLibrary and ShaderHotReload system support shader management and runtime iteration.

## 4. Key Features and Implementations

### Core
*   **Application Lifecycle**: `Application` class manages `Initialize` → `Run` → `Shutdown`.
*   **Logging**: `Logger` with level-based output (Info/Warn/Error).
*   **Timing**: `Timer` using `QueryPerformanceCounter` for high-precision delta time and FPS calculation.
*   **Window Management**: `Window` class for Win32 window creation, message handling, and resize callbacks.

### Graphics
*   **DirectX 12 Foundation**: `GraphicsDevice`, `CommandQueue`, `SwapChain`, `DescriptorHeap`, `Fence`, `CommandList` for low-level D3D12 operations.
*   **Shader Management**: `Shader` class for HLSL compilation via DXC, `RootSignatureBuilder`, `PipelineStateBuilder`.
*   **Resource Management**:
    *   **Buffers**: `Buffer` (vertex/index) and `DynamicBuffer` (UPLOAD heap for CPU-GPU shared data).
    *   **Textures**: `Texture` (stb_image for loading, GPU transfer), `TextureManager` (handle-based, caching).
    *   **Render Targets**: `RenderTarget` (off-screen rendering), `DepthBuffer`.
    *   **Soft Image**: `SoftImage` for CPU-side pixel manipulation.
*   **2D Rendering**:
    *   `SpriteBatch`: Efficient batched sprite rendering with various blend modes.
    *   `PrimitiveBatch`: Batched drawing of basic shapes (lines, boxes, circles, triangles).
    *   `Camera2D`: 2D camera with position, zoom, and rotation.
    *   `SpriteSheet`, `Animation2D`: For sprite animation.
*   **3D Rendering**:
    *   **PBR**: Physically Based Rendering with Cook-Torrance BRDF.
    *   **Lighting**: Directional, Point, Spot lights.
    *   **Shadows**: Cascaded Shadow Maps (CSM), Spot Shadow Maps, Point Shadow Maps (Texture2DArray).
    *   **Models**: glTF 2.0 loader (`cgltf.h`), skeletal animation (`Skeleton`, `AnimationClip`, `AnimationPlayer`).
    *   `Skybox`: Procedural skybox rendering.
    *   `Fog`: Linear, Exp, Exp2 modes.
    *   `PrimitiveBatch3D`: Debug wireframe primitives.
    *   `Terrain`: Heightmap-based terrain.
*   **Post-Processing Pipeline**: Comprehensive HDR-enabled chain with ping-pong render targets and LDR output.
    *   `HDR Pipeline`: R16G16B16A16_FLOAT render targets.
    *   `Tonemapping`: Reinhard, ACES, Uncharted2 (movable to post-effect pipeline).
    *   `Bloom`, `FXAA`, `Vignette`, `Chromatic Aberration`, `Color Grading`.
    *   `SSAO` (Screen Space Ambient Occlusion) with bilateral blur.
    *   `Depth of Field` (DoF) with CoC generation and Gaussian blur.
    *   `Motion Blur` (camera-based, depth reprojection).
    *   `Screen Space Reflections` (SSR) with ray marching and normal reconstruction from depth.
    *   `OutlineEffect` (Sobel edge detection from depth/normal).
    *   `Volumetric Light` (God Rays) using radial blur and depth occlusion.
    *   `TAA` (Temporal Anti-Aliasing) with Halton sequence jitter and variance clipping.
    *   `AutoExposure` (Eye Adaptation) using log luminance and exponential smoothing.
*   **Rendering Layers**: `RenderLayer` system for managing independent LDR layers (e.g., Scene, UI) with Z-order composition, blend modes, and masking. `LayerCompositor` for final output.

### GUI
*   **Widget System**: Base `Widget` class with tree structure, event handling (Capture → Target → Bubble), layout (`Flexbox`).
*   **UI Elements**: `Panel`, `TextWidget`, `Button`, `Slider`, `CheckBox`, `RadioButton`, `Dialog`, `TextInput`, `Image`, etc.
*   **Styling**: `StyleSheet` for CSS parsing (kebab-case to camelCase), supporting pseudo-classes (`:hover`, `:pressed`).
*   **Rendering**: `UIRenderer` integrates SDF rounded rectangles (`UIRectBatch`), `SpriteBatch`, `TextRenderer`, and scissor stacking.
*   **Declarative UI**: `XMLParser` and `GUILoader` for defining UI layouts from XML files, binding to C++ events.

### Input
*   **Unified Input Management**: `InputManager` integrates `Keyboard`, `Mouse`, and `Gamepad` (XInput).
*   **Polling Model**: Updates input states each frame, supporting `press`, `trigger`, `release`.
*   **Event-driven**: Window message callbacks for input processing.

### Audio
*   **XAudio2 Backend**: `AudioDevice` for XAudio2 engine initialization.
*   **Sound Management**: `Sound` (WAV data), `SoundPlayer` (SE playback with multiple voices), `MusicPlayer` (BGM with looping, fade-in/out).
*   `AudioManager`: Handle-based management for sounds and music.

### IO
*   **Virtual File System (VFS)**: Mount-based `FileSystem` with `IFileProvider` interface, allowing priority-based access to physical files or archives.
*   **Encrypted Archives**: Custom `.gxarc` format with LZ4 compression and AES-256 encryption.
*   **Asynchronous Loading**: `AsyncLoader` for background asset loading.
*   **File Watching**: `FileWatcher` for monitoring file system changes.
*   **Networking**: `TCPSocket`, `UDPSocket`, `HTTPClient` (WinHTTP), `WebSocket` (WinHTTP WebSocket API).

### Math / Physics
*   **Custom Math Library**: `Vector2/3/4`, `Matrix4x4`, `Quaternion`, `Color` types inheriting from DirectXMath's `XMFLOAT` types for zero-overhead interoperability, with operator overloads and utility methods. `MathUtil` for common functions, `Random` (Mersenne Twister).
*   **2D/3D Collision Detection**: `Collision2D`/`Collision3D` with various shapes (AABB, Circle, Sphere, OBB, Triangle), SAT (Separating Axis Theorem), Moller-Trumbore ray-triangle intersection, raycasting.
*   **Spatial Partitioning**: Template-based `Quadtree`, `Octree`, `BVH` (with SAH splits) for efficient spatial queries.
*   **2D Physics**: Custom impulse-based `PhysicsWorld2D` with `RigidBody2D` (gravity, collision response, friction).
*   **3D Physics**: Integration of Jolt Physics v5.3.0 via PIMPL wrapper (`PhysicsWorld3D`) for robust 3D simulation, body/shape management, and raycasting.

### DXLib Compatibility Layer
*   `GXLib.h`: A single include provides a set of global functions mimicking DxLib's API for 2D/3D drawing, input, audio, and font functions.
*   `CompatContext`: Singleton managing the internal GXLib subsystems used by the compatibility layer.
*   `ActiveBatch` auto-switching for `SpriteBatch`/`PrimitiveBatch` to optimize draw calls.

### Developer Tools
*   **Shader Hot Reload**: Monitors shader file changes, debounces, flushes GPU, and triggers PSO rebuilds at runtime (`F9` toggle). Provides error overlay for compilation failures.
*   **PSO Rebuilder**: A mechanism for various renderers to rebuild their Pipeline State Objects dynamically.
*   **JSON Settings**: `nlohmann/json` integration for loading/saving post-effect configurations to `post_effects.json` (`F12` save).

## 5. Strengths of the Project

*   **Comprehensive Features**: Covers a vast range of engine functionalities from low-level graphics to high-level physics, GUI, and networking.
*   **Modern API Usage**: Leverages DirectX 12 and C++20 for performance and modern programming practices.
*   **Modular and Layered Architecture**: Promotes maintainability and extensibility.
*   **Detailed Documentation**: Extensive `PhaseX_Summary.md` files provide clear insights into design decisions, issues, and verification for each stage of development.
*   **Developer Experience**: Features like Shader Hot Reload, DxLib compatibility, and JSON settings significantly enhance iteration speed and ease of use.
*   **Performance Focus**: Batch rendering, double-buffering for dynamic resources, optimized allocators (FrameAllocator, PoolAllocator), and careful post-effect chain management demonstrate a strong emphasis on performance.
*   **Robust Physics**: Integration of Jolt Physics for 3D and a custom impulse-based 2D engine provide solid physics foundations.

## 6. Future Work / Known Limitations

As per `Phase10_RemainingWork.md`:
*   Multi-threaded CommandList recording.
*   Texture streaming (mip splitting + LRU).
*   GPU regression testing (screenshot comparison).
*   Memory leak detection (CRT Debug Heap + Live Objects).
*   Further D3D12 Debug Layer options.

## 7. Coding Conventions and Style

*   **Namespace**: Strictly adheres to `GX::` namespace, often with sub-namespaces for modules (e.g., `GX::GUI`).
*   **Naming**: PascalCase for class, struct, enum names, and public methods. `m_camelCase` for member variables.
*   **Comments**: Extensive use of Japanese comments, especially "beginner-friendly explanations" within classes, indicating a focus on clarity and education. The `/utf-8` compile option supports this.
*   **C++ Modernity**: Embraces C++20 features, smart pointers, and RAII where appropriate.

---
**Analysis Date**: 2026-02-11

## Project Analysis (2026-02-21)

**日付:** 2026-02-21
**前回分析:** 2026-02-11 (ProjectAnalyze_2026-02-11.md)
**前回バグレポート:** 2026-02-17 (BugReport.md)

---

## 1. 未コミット変更の全体像

最終コミット `fa53e0e` (Fix FBX法線反転) 以降、**59ファイル変更 (+11,249/-861行)** + **新規11ファイル** が未コミット。

| 変更範囲 | ファイル数 | 追加行数(概算) | 内容 |
|----------|----------|------------|------|
| GXLib本体 (Graphics/3D, Resource, IO) | 17 modified + 2 new | ~1,200 | Toon/Outline/Viewer API/FBX修正/PakFileProvider |
| Shaders | 4 modified + 1 new | ~350 | UTS2 Toon, ToonOutline, ShadowUtils, ShaderModelCommon, InfiniteGrid |
| gxformat/gxconv/gxloader | 5 modified | ~250 | ShaderModelParams UTS2化, FBX IBM修正, ワイド文字パス |
| GXModelViewer | 22 modified + 8 new | ~1,600 (独自コード) | Docking/Viewport RT/ギズモ/シャドウ/ピッキング/パネル群 |
| ImGui ThirdParty (docking branch) | 12 modified | ~8,240 | master → docking branch 移行 |

---

## 2. 未コミット変更の機能別分類

### A. GXLib本体への変更（エンジンライブラリ）

#### A-1. UTS2ベースToonシェーダー再構築（Phase 20-21）
- **ShaderModelParams** (`gxformat/shader_model.h`): 旧ramp/band方式を削除、UTS2方式に置換。3ゾーンシェード (shadeColor/shade2ndColor, baseColorStep/baseShadeFeather, shadeColorStep/shade1st2ndFeather)。Phong/SS/CC領域を7エイリアスアクセサで再利用 (rimLightDirMask等)。256B維持。
- **ShaderModelGPUParams** (`ShaderModelConstants.h`): GPU cbuffer配置をUTS2対応。ConvertToGPUParamsにToon分岐追加。
- **ShaderModelCommon.hlsli**: cbuffer変数名をUTS2対応に変更。7つの`#define`マクロでToonエイリアス定義。GetLightShadow()修正。
- **Toon.hlsl**: 完全リライト。halfLambert+CSM統合、片側ランプ3ゾーン遷移、UTS2スペキュラ(power→threshold変換)、UTS2リム(insideMaskリマッピング+方向マスク)。
- **ShadowUtils.hlsli**: k_ShadowSoftness追加、ポイントシャドウ3x3→5x5 PCFアップグレード、キューブフェイス境界マージンクランプ。

#### A-2. スムース法線転写Toonアウトライン（Phase 19）
- **Mesh.h/cpp**: `CreateSmoothNormalBuffer()` (slot 1 VB, stride 12)、`ComputeSmoothNormals()` (位置量子化0.0001→unordered_mapグループ化→法線平均化)。
- **Vertex3D.h**: `k_Vertex3DPBROutlineLayout` / `k_Vertex3DSkinnedOutlineLayout` (slot 0=既存 + slot 1=SMOOTHNORMAL)。
- **ShaderRegistry.cpp/h**: アウトラインPSOに新レイアウト適用、DepthBias(500,0,2)追加。
- **ToonOutline.hlsl**: 完全リライト。スムース法線ベースの押し出し（ハイブリッド：凸領域はscaleDir、凹領域はスムース法線）。ワールド空間でposH.wスケール。UTS2距離減衰、シャドウ統合、baseColor^2暗化。
- **ModelLoader.cpp**: glTF/FBXローダーにスムース法線計算を追加。
- **GxmdModelLoader.cpp**: GXMDローダーにもスムース法線計算を追加。

#### A-3. Viewer/Editor向けAPI拡張（Phase 17）
- **Animator.h/cpp**: `GetCurrentClip/GetCurrentTime/SetCurrentTime/GetSpeed/SetSpeed/IsPaused`、ルートモーションロック (`SetLockRootPosition/Rotation`)。
- **AnimatorStateMachine.h**: `GetTransitions()` 公開。
- **Camera3D.h/cpp**: `SetPitch(float)` / `SetYaw(float)` 追加（オービットカメラ用）。
- **Renderer3D.h/cpp**: `SetWireframeMode(bool)`（ワイヤフレームPSO 2種）、`SetMaterialOverride/ClearMaterialOverride`、`DrawModel/DrawSkinnedModel(submeshVisibility)`。Toonアウトライン2スロットVBバインド。シャドウDepthBias強化。
- **TextureManager.h/cpp**: `GetFilePath(int handle)` 追加。

#### A-4. FBXインポート改善
- **ModelLoader.cpp** (+408行): `FbxAxisSystem::ConvertScene()` 廃止→手動Z反転RH-to-LH変換。ルートボーンに`EvaluateGlobalTransform()`使用。UV事前キャッシュ（FBX SDKバグ回避）。5段階テクスチャパス検索。全プロパティフォールバック。非スキンメッシュのノード変換適用。
- **gxconv/fbx_importer.cpp** (+113行): `space_conversion=ADJUST_TRANSFORMS`。ルートボーン `node_to_world` 使用。IBM転置修正。ルートボーンアニメに `parentWorld*localMat` 合成。

#### A-5. GXPAK VFS統合（Phase 18）
- **PakFileProvider.h/cpp** (新規): `IFileProvider` 実装、`gxloader::PakLoader` 使用、Priority=100。

#### A-6. ワイド文字パスローダー
- **gxloader model_loader.h/cpp**: `LoadGxmdW(wstring)` 追加。
- **gxloader anim_loader.h/cpp**: `LoadGxanW(wstring)` 追加。

### B. GXModelViewer変更

#### B-1. ImGui Docking移行 + DockSpaceレイアウト（Phase 22）
- ImGui master → docking branch 1.92.6（+8,240行）
- `DockSpaceOverViewport()` によるドッキング有効化
- 3Dシーン → m_viewportRT(LDR) → `ImGui::Image()` 表示
- ビューポートウィンドウサイズ駆動のリサイズ（SwapChainはOSウィンドウに依存）
- タブ化: Inspector(Properties/ModelInfo/Skeleton)、Rendering(Lighting/PostEffect/Skybox)

#### B-2. 大規模機能追加（Phase 17拡張）
- **ImGuizmoギズモ統合**: Translate/Rotate/Scale、ワールド/ローカル切替、スナップ、ビューポートツールバー
- **CSMシャドウパス**: 4カスケード+スポット+ポイントシャドウ完全統合
- **InfiniteGrid** (新規3ファイル): シェーダーベース無限グリッド、Y=0面、軸カラー、距離フェード
- **ビューポートピッキング**: レイキャスト→AABB→最近接ヒット、CPUスキニングAABB
- **ボーン可視化**: 関節スフィア+ボーンライン+選択ボーン軸表示
- **AABB可視化**: OBBワイヤフレーム描画
- **D&Dインポート**: WM_DROPFILES + ImGuiドラッグドロップ
- **アニメーションインポート**: .gxan/.fbx/.gltf/.glb対応、ボーン名リマッピング
- **キーボードショートカット**: Space/F/W/T/E/R/L/B
- **オービットカメラ改善**: ビューポート限定入力、ギズモ排他、選択ボーン追従

#### B-3. 新規パネル（Phase 17）
- **AssetBrowserPanel**: ファイル/フォルダブラウザ、D&D対応
- **ModelInfoPanel**: 頂点数/三角形数/VB,IBサイズ/AABB/ボーン数/アニメ一覧
- **SkeletonPanel**: ボーン階層ツリー、選択ボーンTRS表示、ワールド座標、IBM表示

#### B-4. パネル改善
- **PropertyPanel**: ギズモセクション、レンダリングセクション、サブメッシュ可視性、UTS2全パラメータエディタ
- **SceneHierarchyPanel**: スキンドエンティティのボーンツリー展開
- **TimelinePanel**: 完全リライト（トランスポート制御、スクラブ、速度プリセット、ルートロック）
- **AnimatorPanel**: 遷移リンク描画
- **各パネル**: DrawContent()メソッド追加（タブ埋め込み対応）
- **ModelExporter**: TextureManager引数追加（テクスチャパス保存）

---

## 3. 完成条件 (G1-G5) 達成状況

| # | 完成条件 | 状態 | 備考 |
|---|---------|------|------|
| G1 | DXライブラリの全APIカテゴリを網羅 | **達成** | Phase 1-9で全カテゴリカバー |
| G2 | ポストエフェクトパイプライン標準搭載 | **達成** | 13エフェクト + JSON設定 |
| G3 | 描画レイヤーシステム動作 | **達成** | RenderLayer + LayerCompositor + MaskScreen |
| G4 | XMLベースGUIシステム | **達成** | 16種Widget + XML/CSS + C++バインド |
| G5 | サンプルプロジェクト群動作 | **達成** | 5サンプル |

---

## 4. Directive計画 vs 現状のギャップ分析

### 4.1 Directiveに記載あり・未実装の機能

| カテゴリ | 機能 | Directive記載箇所 | 現状 |
|---------|------|-----------------|------|
| 3D | **インスタンシング描画** | 3.3 モデル描画 | **未実装** — DrawModel は1体ずつ |
| 3D | **LODシステム** | 3.3 地形 / Phase 3 | **未実装** — 固定LODのみ |
| 3D | **IBL (Image Based Lighting)** | 3.3 環境マップ | **未実装** — Skyboxはあるがイラディアンスマップ/BRDF LUT なし |
| 3D | **モーフターゲット** | Phase 3 glTFローダー | **未実装** |
| 3D | **Area Light** | 3.3 ライティング | **未実装** |
| Shader | **コンピュートシェーダーパイプライン** | 3.5 | **部分的** — PostEffect内で使用されるがユーザーAPI未公開 |
| Sound | **3Dサウンド (X3DAudio/HRTF)** | 3.6 | **未実装** |
| Sound | **オーディオミキサー (バス/エフェクトチェーン)** | 3.6 | **未実装** |
| Sound | **リアルタイムエフェクト (リバーブ/EQ)** | 3.6 | **未実装** |
| Input | **タッチ入力** | 3.7 | **未実装** |
| Input | **アクションマッピング** | 3.7 | **未実装** |
| Input | **入力バッファリング (格ゲー向け)** | 3.7 | **未実装** |
| Text | **SDFフォントレンダリング** | 3.8 | **DirectWrite方式** — SDF未実装 |
| Text | **リッチテキスト (色・サイズ混在)** | 3.8 | **未実装** |
| Text | **BMFont対応** | 3.8 | **未実装** |
| Core | **DPI対応 (Per-Monitor V2)** | 3.1 | **未実装** |
| Core | **マルチウィンドウ** | 3.1 | **未実装** |
| Core | **EventBus / Delegate** | 5章 ディレクトリ構成 | **未実装** |
| Core | **ConfigManager (JSON/INI)** | 5章 ディレクトリ構成 | **PostEffectSettings のみ** |
| Compat | **DXLib互換API完全網羅** | 3.1-3.12 全項目 | **部分的** — 主要API対応済み、全関数網羅は未検証 |

### 4.2 Directiveに記載なし・追加実装済みの機能

| 機能 | Phase | 備考 |
|------|-------|------|
| DXR レイトレーシング反射 | 11 | RTAccelerationStructure/RTPipeline/RTReflections |
| DXR RTGI グローバルイルミネーション | 12 | 半解像度GI+テンポラル蓄積+A-Trous空間フィルタ |
| アセットパイプライン (gxformat/gxconv/gxloader/gxpak) | 13 | OBJ/FBX/glTF→GXMD/GXAN, LZ4圧縮PAK |
| ShaderRegistry + シェーダーモデルPSO | 14 | 12 PSO (6モデル×2バリアント) |
| アニメーションブレンドシステム | 15 | BlendStack/BlendTree/AnimatorStateMachine |
| GXModelViewer (ImGui 3Dモデルビューア) | 16-22 | 12+パネル, docking, ギズモ, シャドウ |
| UTS2 Toonシェーダー | 18-21 | 3ゾーンシェード, スムース法線アウトライン |
| GXPAK VFS統合 | 18 | PakFileProvider |

---

## 5. 既知バグ状況 (BugReport.md, 2026-02-17)

| 重要度 | 件数 | 修正済み | 未修正 | 主要な未修正項目 |
|--------|------|---------|--------|----------------|
| Critical | 6 | 0 | 6 | DropDown/ListView配列外, TextureManager負インデックス, RT-C01〜C03 |
| High | 24 | 0 | 24 | vswprintf_sバッファサイズ, Map null未チェック, スレッド安全性 |
| Medium | 32 | 0 | 32 | TextRenderer改行, 各種エラーハンドリング |
| Low | 14 | 0 | 14 | 除算ゼロガード, float精度 |
| **合計** | **76** | **0** | **76** | |

### 計算式バグ (MATH-01〜05)
- MATH-01: Quaternion::ToEuler() 符号誤り — **未修正**
- MATH-02: PhysicsWorld2D 角トルクに質量逆数を使用 — **未修正**
- MATH-03: PhysicsWorld2D AABB回転無視 — **未修正**
- MATH-04: RT ClosestHit 法線変換で非一様スケール未対応 — **未修正**
- MATH-05: DoF ガウス重み正規化0.4%超過 — **未修正**

---

## 6. DxLib置き換えの視点で「次にやるべきこと」

**Directiveの核心:** 「DXライブラリの完全上位互換フレームワークをDirectX 12ベースでゼロから構築する」

### 6.1 DxLib互換で不足している機能 (優先度: 高→低)

| 優先度 | 機能 | DxLib該当API | 理由 |
|--------|------|-------------|------|
| **高** | インスタンシング描画 | `MV1DrawModel`(大量描画時) | Directiveに明記。同一メッシュ大量描画で必須 |
| **高** | 3Dサウンド + ミキサー | `Set3DPositionSoundMem`, AudioMixer | Directiveに明記。3Dゲームで必須 |
| **高** | アクションマッピング | (DxLibになし/新規) | Directiveに明記。入力設定ファイルで再マップ |
| **中** | IBL (Image Based Lighting) | (DxLibになし/新規) | Directive 3.3 環境マップ。PBR品質向上の鍵 |
| **中** | SDFフォントレンダリング | `DrawString`の上位互換 | Directive 3.8。現在DirectWrite方式→SDF化で品質・スケーラビリティ向上 |
| **中** | タッチ入力 | `GetTouchInputNum` | Directive 3.7。Windowsタブレット/Surface対応 |
| **中** | リッチテキスト / BMFont | (DxLibになし/新規) | Directive 3.8。ゲーム内テキスト表現力 |
| **低** | DPI対応 | (DxLibになし/新規) | Directive 3.1。マルチモニタ対応 |
| **低** | LODシステム | 地形・モデル | Directive Phase 3。大規模シーンで必須 |

### 6.2 Directiveに明記されていないが価値のある追加機能

| 機能 | DxLib互換性の観点 | 理由 |
|------|-----------------|------|
| **パーティクルシステム** | DxLibにはない→新規差別化 | 2D/3Dエフェクト表現。GPUパーティクル(Compute)で大量描画 |
| **IK (逆運動学)** | DxLibにはない→新規差別化 | 足接地・視線追従。アニメーション品質向上 |

---

## 7. ビルド状態

```
cmake --build build --config Debug → 成功 (2026-02-21 確認)
```

- GXLib.lib: OK
- Sandbox.exe: OK
- GXModelViewer.exe: OK
- 5 Samples: 未確認（CMake再生成が必要な可能性）

---

## 8. 推奨アクション

### 即座に実施すべき
1. **未コミット変更のコミット** — 59ファイル+11新規ファイルが未コミット。Phase 17-22 相当の大量変更がリスクにさらされている

### DxLib置き換えとして次に取り組むべき
2. **インスタンシング描画** — Directiveに明記、3Dゲーム必須
3. **3Dサウンド + オーディオミキサー** — Directiveに明記、3Dゲーム必須
4. **アクションマッピング** — Directiveに明記、入力管理の標準化
5. **パーティクルシステム** — DxLibにない新規差別化機能、表現力大幅向上

### バグ修正（上位のみ）
6. **Critical 6件** — クラッシュ直結のため早期修正推奨
7. **MATH-02** (PhysicsWorld2D角トルク) — 物理シミュレーション全体に影響

## Documentation Audit

**調査日:** 2026-02-17 (更新: 2026-02-21)
**参考記事:** [開発者が書くチュートリアルは読みにくい (Gigazine)](https://gigazine.net/news/20250926-developer-read/)

## 記事の要旨

アニー・ミューラー氏のブログ記事が指摘する「開発者が書くチュートリアルの問題」:

1. **専門用語の無説明使用** — 初心者が解説を読むために、さらに別の解説が必要になる
2. **前提知識の暗黙の仮定** — 「当然知っているはず」と思い込んで説明を省略する
3. **曖昧なコマンド・手順説明** — 実装段階で「何をすればいいか分からない」状態に陥る
4. **「知識の呪い」** — 専門家が他者も同じ知識を持っていると無意識に思い込む
5. **エコシステム成熟化の弊害** — 分野が確立されると基礎知識が省略されがちになる

---

## 調査対象

| ドキュメント | ファイル |
|---|---|
| README | `README.md` |
| チュートリアル 01-05 | `docs/tutorials/01_GettingStarted.md` 〜 `05_GUI.md` |
| DXLib 移行ガイド | `docs/migration/DxLibMigrationGuide.md` |
| サンプル README | `Samples/*/README.md` |

---

## 問題分析

### 問題 1: 専門用語の無説明使用

**深刻度: 高**

ドキュメント全体で、専門用語が説明なしに使われている箇所が多数存在する。

#### チュートリアル 01 (Getting Started)

| 箇所 | 問題 |
|---|---|
| `int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)` | Win32 API の型が説明なし。初心者はこの関数シグネチャだけで混乱する |
| `SetDrawScreen(GX_SCREEN_BACK)` | 「裏画面描画」とコメントしているが、なぜ裏画面に描画するのか(ダブルバッファリングの概念)が説明されていない |
| `cmdList.Begin(frameIndex)` / `cmdQueue.Execute()` | コマンドリスト・コマンドキューという D3D12 の基本概念が未説明 |
| `cmdList.Get()->ClearRenderTargetView(rtv, clearColor, 0, nullptr)` | 生の D3D12 API 呼び出しが何をしているか説明なし |
| `auto frameIndex = swapChain.GetCurrentBackBufferIndex()` | バックバッファインデックスとは何か未説明 |

#### チュートリアル 04 (3D Rendering)

| 箇所 | 問題 |
|---|---|
| PBR (物理ベースレンダリング) | 名前は書いてあるが、PBR が何か、従来レンダリングと何が違うかの説明がない |
| `metallic = 0.0f`, `roughness = 0.5f` | パラメータの意味（コメントあり）は書いてあるが、なぜこの値を選ぶかの指針がない |
| `TonemapMode::ACES` | ACES とは何か、他のモード (Reinhard, Uncharted2) との違いが不明 |
| CSM (カスケードシャドウマップ) | 名前だけ登場。何をするものか説明なし |
| `SetFov(GX::MathUtil::PI / 4.0f)` | FOV (視野角) の概念と 45 度を選ぶ理由が未説明 |
| SSAO, SSR, DoF, TAA 等 | 略語の展開すらない（リストの日本語名はあるが意味の説明なし） |

#### README.md

| 箇所 | 問題 |
|---|---|
| 特徴一覧 | PBR, CSM, SSAO, SSR, TAA, HDR, FXAA, VFS, BVH, SAH 等の略語が大量に列挙され、知らない人には暗号に見える |

---

### 問題 2: 前提知識の暗黙の仮定

**深刻度: 高**

#### 読者レベルの想定が不明確

チュートリアルの対象読者が明記されていない。以下の知識が暗黙的に要求されている:

- **C++ の基礎知識** — ラムダ式、`auto`、`unique_ptr` が説明なく使われている
- **Win32 API** — `WinMain`, `HINSTANCE`, `PostQuitMessage` が前提
- **CMake** — ビルド手順で cmake コマンドを示すが、CMake 自体の説明なし
- **DirectX / GPU** — コマンドリスト、スワップチェーン、レンダーターゲットが前提知識
- **Web 技術 (CSS/XML)** — GUI チュートリアルで Flexbox, セレクタ構文が前提知識

#### 具体例

```cpp
// チュートリアル 05 より
loader.RegisterEvent("btnStart", [](GX::Widget&) {
    StartGame();
});
```
ラムダ式を知らない読者にとって `[](GX::Widget&) { ... }` は意味不明。

```cpp
// チュートリアル 03 より
int mouse = GetMouseInput();
if (mouse & MOUSE_INPUT_LEFT) { /* 左クリック中 */ }
```
ビットフラグ演算（`&` 演算子のビット単位 AND 的用法）が説明なし。

---

### 問題 3: 関数パラメータの説明不足

**深刻度: 中**

コード例で関数の引数の意味が分かりにくい箇所が多い。

#### チュートリアル 02 (2D Drawing)

```cpp
DrawRotaGraph(640, 480, 2.0, 0.5, texHandle, TRUE);
```
- `640, 480` = 座標？
- `2.0` = 拡大率？
- `0.5` = 回転角度（ラジアン）？度？
- `TRUE` = 何を有効にしている？

コメントは「拡大・回転描画」のみ。個々の引数の意味は読者の推測に委ねられている。

```cpp
GX::MeshData sphere = GX::MeshData::CreateSphere(0.5f, 32, 16);
```
- `0.5f` = 半径（コメントなし）
- `32` = ?
- `16` = ?

分割数であることは経験者しか分からない。

---

### 問題 4: 「なぜ」の欠如

**深刻度: 高**

コード例は「何をするか」は示しているが、「なぜそうするか」「なぜその値を選ぶか」の説明が全般的に欠落している。

| 箇所 | 不足している説明 |
|---|---|
| `SetDrawScreen(GX_SCREEN_BACK)` | なぜ裏画面に描画する必要があるか |
| `ScreenFlip()` | フリップが必要な理由（ティアリング防止等） |
| `SetCameraNearFar(0.1f, 1000.0f)` | なぜ 0.1 と 1000 なのか。値を変えると何が起きるか |
| `light.intensity = 3.0f` | なぜ 3.0 か。1.0 や 10.0 だとどうなるか |
| `postFX.SetBloomThreshold(1.0f)` | しきい値 1.0 の意味。HDR 値との関係 |
| `camera.SetAspectRatio(1280.0f / 960.0f)` | アスペクト比とは何か、なぜ計算で求めるか |

---

### 問題 5: エラーケース・トラブルシューティングの欠如

**深刻度: 中**

#### 記載されていない事項

- ビルドが失敗した場合の対処法
- `GX_Init()` が `-1` を返す原因と対処
- シェーダーファイルが見つからない場合のエラー（VS_DEBUGGER_WORKING_DIRECTORY 問題）
- GPU が DirectX 12 に非対応の場合の挙動
- テクスチャファイルが存在しない場合
- サウンドファイルが WAV 以外の場合

開発中にメモリ（MEMORY.md）に記録された「Common Issues」には多数の落とし穴が挙げられているが、チュートリアルや公開ドキュメントにはこれらが一切反映されていない。

---

### 問題 6: 移行ガイドの DXLib 前提

**深刻度: 低〜中**

DXLib 移行ガイドは DXLib ユーザーを対象としているため、DXLib の前提知識は許容範囲。ただし以下の点は問題:

- `DIK_* コード互換` — DIK (DirectInput Key) の説明なし
- 非互換項目は列挙されているが、代替手段や回避方法の説明が不足
- 実際の移行前後を比較する「完全なコード例」がない

---

### 問題 7: サンプル README の情報不足

**深刻度: 低**

サンプルの README は最小限（操作方法 + ビルドコマンドのみ）で、以下が不足:

- サンプルのスクリーンショット
- コードの主要部分の解説
- 学べるポイントの説明
- 必要なアセットファイルの説明

---

## 問題の該当度まとめ

| 記事の指摘 | GXLib での該当状況 | 深刻度 |
|---|---|---|
| 専門用語の無説明使用 | **該当する** — D3D12 用語、ポストエフェクト略語、PBR 用語等が多数 | 高 |
| 前提知識の暗黙の仮定 | **該当する** — C++, Win32, CMake, DirectX, CSS が暗黙の前提 | 高 |
| 曖昧なコマンド説明 | **部分的に該当** — 関数パラメータの意味が推測頼り | 中 |
| 「知識の呪い」 | **該当する** — 「なぜ」の説明が全般的に欠落 | 高 |
| エコシステム成熟化の弊害 | **該当する** — 開発メモには詳細な知見があるが公開ドキュメントに未反映 | 中 |

---

## 改善提案

### 1. 対象読者の明記

各チュートリアルの冒頭に前提知識を明記する。

```markdown
## 前提知識
- C++ の基礎（変数、関数、クラス、ポインタ）
- Visual Studio の基本操作
- コマンドラインの基本操作
```

### 2. 用語集 (Glossary) の作成

略語・専門用語をまとめた用語集ページを作成し、チュートリアル内からリンクする。

```markdown
| 用語 | 正式名称 | 説明 |
|---|---|---|
| PBR | Physically Based Rendering | 物理法則に基づく現実的なライティング手法 |
| SSAO | Screen Space Ambient Occlusion | 画面空間で環境光の遮蔽を近似する手法 |
| CSM | Cascaded Shadow Maps | 距離に応じてシャドウマップの精度を段階分けする手法 |
```

### 3. 関数パラメータの説明追加

コード例の引数にインラインコメントか、直後に引数表を追加する。

```cpp
DrawRotaGraph(
    640, 480,       // 描画中心座標 (px)
    2.0,            // 拡大率 (1.0 = 等倍)
    0.5,            // 回転角度 (ラジアン, 約28.6度)
    texHandle,      // テクスチャハンドル
    TRUE);          // 透過描画 (TRUE = アルファ値を使用)
```

### 4. 「なぜ」の説明を追加

各コード例の前後に、設計意図や値の選定理由を簡潔に記述する。

```markdown
> **なぜ裏画面に描画するのか？**
> 直接画面に描画すると、描画途中の不完全な画像が見えてしまいます（ティアリング）。
> 裏画面に描き終えてから一括表示（ScreenFlip）することで、滑らかな表示になります。
```

### 5. トラブルシューティングセクションの追加

各チュートリアルの末尾に「よくある問題」を追加する。

```markdown
## よくある問題

### Q: ビルドは成功するが、実行時にシェーダーが見つからないエラーが出る
Visual Studio からデバッグ実行する場合、作業ディレクトリが exe の場所と
異なることがあります。プロジェクトプロパティ → デバッグ → 作業ディレクトリを
`$(TargetDir)` に設定してください。

### Q: GX_Init() が -1 を返す
DirectX 12 に対応した GPU とドライバが必要です。...
```

### 6. サンプルコードに段階的な解説を追加

「完成コード → コピペ」ではなく、段階的に構築する形に変更する。

```markdown
### ステップ 1: ウィンドウを開く
まずは空のウィンドウを表示するだけのコードです。
[コード + 説明]

### ステップ 2: 背景色を変更する
ClearDrawScreen で画面を青く塗りつぶしてみましょう。
[コード + 説明]

### ステップ 3: テキストを表示する
[コード + 説明]
```

---

## 結論

GXLib のドキュメントは、記事が指摘する「開発者が書く読みにくいチュートリアル」の特徴に**複数該当**している。特に以下の 3 点が顕著:

1. **専門用語の説明不足** — D3D12/グラフィックスの専門用語が前提知識として扱われている
2. **「なぜ」の欠如** — コードが「何をするか」は書いてあるが「なぜそうするか」が書かれていない
3. **開発メモと公開ドキュメントの乖離** — MEMORY.md に蓄積された実践的知見が公開ドキュメントに反映されていない

ドキュメントの構造自体（チュートリアル 5 本 + 移行ガイド + サンプル）は良好であり、改善の余地は大きい。上記の提案を順次適用することで、初心者にもアクセスしやすいドキュメントに改善できる。

---

## 2026-02-21 更新: Phase 11〜22 反映状況

### 追加済み項目

以下の Phase 11〜22 の機能がドキュメントに反映された:

| Phase | 機能 | 反映先 |
|---|---|---|
| 11 | DXR レイトレーシング反射 (RTReflections) | index.html (ページ+サイドバー), README, Glossary, チュートリアル04, 移行ガイド |
| 12 | DXR RTGI (グローバルイルミネーション) | index.html (ページ+サイドバー), README, Glossary, チュートリアル04 |
| 13 | アセットパイプライン (gxformat/gxconv/gxloader/gxpak) | index.html (4ページ+サイドバー), README, Glossary, チュートリアル04, 移行ガイド |
| 14 | ShaderRegistry + シェーダーモデル PSO | index.html (2ページ+サイドバー), README, Glossary, チュートリアル04 |
| 15 | アニメーションブレンド (BlendStack/BlendTree/ASM) | index.html (4ページ+サイドバー), README, Glossary, チュートリアル04 |
| 16-17 | GXModelViewer | README (プロジェクト構成), 移行ガイド |
| 18 | マテリアルオーバーライド + Toon 強化 + GXPAK VFS | index.html (PakFileProvider ページ), README |
| 19-21 | スムース法線 Toon アウトライン + UTS2 | index.html (ShaderRegistry 内), README, Glossary, チュートリアル04 |
| 22 | ImGui Docking 移行 | README, Glossary |

### 残課題

- GXModelViewer の操作方法チュートリアルは未作成（新規チュートリアル 06 が望ましい）
- index.html の JS detail データに Phase 14-22 の詳細説明/サンプルコードが未追加
- サンプル README の Phase 11+ 新機能への言及が不足

## Phase 40 Bug Fix Session

Phase 38-40（ドキュメント改善・レンダリング高度化・ゲームプレイ機能）実装後のビルドエラー修正とランタイムバグ修正の記録。

---

## 1. ビルドエラー修正

### Tilemap.cpp — LoadTexture 引数型不一致
- `TextureManager::LoadTexture()` は `const std::wstring&` を要求するが `std::string` を渡していた
- `std::wstring wTexPath(texPath.begin(), texPath.end())` で変換を追加

### ScriptBindings.cpp — 存在しないAPI / 型不一致
- `Color::ToARGB()` は存在しない → `Color::ToRGBA()` に修正
- `LoadTexture` も同様に `std::wstring` 変換を追加

### TilemapShowcase/main.cpp — 多数のAPI名間違い
| 誤 | 正 |
|---|---|
| `ctx.primitiveBatch` | `ctx.primBatch` |
| `ctx.defaultFont` | `ctx.defaultFontHandle` |
| `GX::TString` / `GX::FormatT` | `TString` / `FormatT`（グローバル） |
| `app.Run()` | `GXEasy::Run(app, app.GetConfig())` |
| `TEXT("{}...")` + `std::format` | `std::format(L"...")` or `FormatT(TEXT(...))` |

### LuaShowcase/main.cpp — API名間違い
- `ctx.textureManager` → `ctx.renderer3D.GetTextureManager()`
- `ctx.defaultFont` → `ctx.defaultFontHandle`
- `app.Run()` → `GXEasy::Run(app, app.GetConfig())`

### MultiThreadShowcase/main.cpp — API名間違い + 未定義メソッド
- `DrawModel(GPUMesh, Transform3D, Material)` は存在しない → `SetMaterial()` + `DrawMesh()` に分離
- `Setup3D()` メソッド定義が欠落 → PostEffect/Camera/Light初期化を含む実装を追加
- `app.Run()` → `GXEasy::Run(app, app.GetConfig())`

---

## 2. ランタイムクラッシュ修正

### TilemapShowcase — Access violation in PrimitiveBatch::DrawBox (0xc0000005)
- **原因**: `PrimitiveBatch` が Begin されていない状態で `DrawBox` を呼び出した
- **修正**: `ctx.EnsurePrimitiveBatch()` をタイル描画ループ前に追加、`ctx.EnsureSpriteBatch()` をテキスト描画前に追加

### LuaShowcase — Access violation in SpriteBatch::AddQuad (0xc0000005)
- **原因**: `SpriteBatch` が Begin されていない状態で `TextRenderer::DrawString` を呼び出した
- **修正**: `ctx.EnsureSpriteBatch()` を描画前に追加（後に DxLib 互換関数に全面移行）

### MultiThreadShowcase — Access violation in Renderer3D::SetMaterial (0xc0000005)
- **原因**: 3Dレンダリングパイプラインが未初期化（`renderer3D.Begin()` 未呼出）
- **修正**: `FlushAll` → `BeginScene` → `renderer3D.Begin` → 描画 → `End` → `EndScene` → depth遷移 → `Resolve` の正規パイプラインを実装

### MultiThreadShowcase — D3D12 例外 (0x00087a) in SetMaterial
- **原因**: `k_MaxObjectsPerFrame = 512` に対し 1001 オブジェクト（1000キューブ＋1床）を描画し、マテリアル定数バッファがオーバーフロー
- **修正**: `k_ObjectCount` を 1000 → 500 に削減（501 < 512）

### MultiThreadShowcase — DrawString 型不一致 (C2664)
- **原因**: `std::wstring::c_str()` (wchar_t*) を DxLib 互換 `DrawString` (TCHAR* = char*) に渡した
- **修正**: `FormatT(TEXT(...))` パターンに変更

---

## 3. 入力が効かない問題

### TilemapShowcase — 矢印キー / Z/X キーが反応しない
- **原因**: `Keyboard::IsKeyDown()` は **Win32 VK コード** を受け取るが、**DirectInput スキャンコード** (0xC8=DIK_UP 等) を渡していた。DIK→VK 変換は DxLib 互換 `CheckHitKey()` 内でのみ行われる
- **修正箇所**:

| ファイル | 誤（DIKコード） | 正（VKコード） |
|---|---|---|
| TilemapShowcase | `0xC8, 0xD0, 0xCB, 0xCD` | `VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT` |
| TilemapShowcase | `0x2C, 0x2D` | `'Z', 'X'` |
| MultiThreadShowcase | `0x02` | `'1'` |
| ScriptBindings.cpp | `KEY_UP=0xC8` 等 | `KEY_UP=VK_UP` 等 |

---

## 4. サンプル内容が伝わらない問題（全面書き直し）

### LuaShowcase — 何も描画されない
- **問題**: Lua の `OnDraw` がコメントだけで何も描画せず、プレイヤー座標も取得・可視化されていなかった
- **修正**:
  - `ScriptEngine` に `GetGlobalFloat()` / `GetGlobalInt()` を新規追加（PIMPL越しにLuaグローバル変数を読み取る）
  - Lua側: `OnUpdate` で座標をグローバル変数 `_px`/`_py`/`_ps` にエクスポート
  - C++側: `GetGlobalFloat("_px")` で座標取得 → 青い矩形＋十字マーク＋ラベルを描画
  - 背景グリッド + HUD（Lua状態・座標表示）追加
  - 全描画を DxLib 互換関数（`DrawBox`/`DrawLine`/`DrawString`）で統一

### MultiThreadShowcase — 3D描画パイプライン欠落 + カメラ操作なし
- **問題**: `renderer3D.Begin()` が呼ばれず何も描画されない。ヘッダに書かれた WASD/マウス操作のコードが存在しない
- **修正**:
  - Walkthrough3D と同じ 3D 描画パイプライン（BeginScene→Begin→描画→End→EndScene→Resolve）を実装
  - WASD/QE カメラ移動 + 右クリックマウスルック（Walkthrough3D パターン）を実装
  - 500個の虹色キューブが波打つアニメーション
  - FPS 表示 + 操作ガイド HUD

---

## 5. 変更ファイル一覧

| ファイル | 変更種別 |
|---|---|
| `GXLib/Graphics/Rendering/Tilemap.cpp` | wstring変換追加 |
| `GXLib/Script/ScriptBindings.cpp` | ToARGB→ToRGBA, wstring変換, VKコード修正 |
| `GXLib/Script/ScriptEngine.h` | `GetGlobalFloat`/`GetGlobalInt` 追加 |
| `GXLib/Script/ScriptEngine.cpp` | 上記の実装 + スタブ |
| `Samples/TilemapShowcase/main.cpp` | API名修正, EnsureBatch追加, VKコード修正 |
| `Samples/LuaShowcase/main.cpp` | 全面書き直し（可視化・入力対応） |
| `Samples/MultiThreadShowcase/main.cpp` | 全面書き直し（3Dパイプライン・カメラ操作） |

---

## 6. 学んだ教訓

1. **`Keyboard::IsKeyDown()` は VK コード**、DxLib 互換 `CheckHitKey()` は DIK コード — 直接 `Keyboard` を使う場合は VK を渡すこと
2. **3Dサンプルは必ず** `FlushAll` → `BeginScene` → `renderer3D.Begin` → 描画 → `End` → `EndScene` → depth遷移 → `Resolve` のフルパイプラインが必要
3. **2D描画前に** `EnsureSpriteBatch()` / `EnsurePrimitiveBatch()` が必須（DxLib 互換関数は内部で呼ぶので不要）
4. **`k_MaxObjectsPerFrame = 512`** — DrawMesh/SetMaterial の呼び出し回数は 512 以内に収めること
5. **DxLib 互換 `DrawString`** は `TCHAR*`（_MBCS=char*）→ `std::wstring` は渡せない。`FormatT(TEXT(...))` を使う
6. **サンプルは DxLib 互換関数を使うのが安全** — `DrawBox`/`DrawLine`/`DrawString` は内部で Ensure を呼ぶのでクラッシュしにくい

