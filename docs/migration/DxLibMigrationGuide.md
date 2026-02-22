# DXLib → GXLib 移行ガイド

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
