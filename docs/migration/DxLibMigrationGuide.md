# DXLib → GXLib 移行ガイド

## 概要

GXLib は DXLib 互換の簡易 API を提供しています。
`Compat/GXLib.h` をインクルードすることで、DXLib に似た関数群を使用できます。

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
DxLib_Init();
DxLib_End();

// 変更後 (GXLib)
GX_Init();
GX_End();
```

### 3. 定数名の変更

| DXLib | GXLib | 説明 |
|---|---|---|
| `DX_SCREEN_BACK` | `GX_SCREEN_BACK` | 裏画面 |
| `DX_SCREEN_FRONT` | `GX_SCREEN_FRONT` | 表画面 |
| `DX_BLENDMODE_NOBLEND` | `GX_BLENDMODE_NOBLEND` | ブレンドなし |
| `DX_BLENDMODE_ALPHA` | `GX_BLENDMODE_ALPHA` | アルファブレンド |
| `DX_BLENDMODE_ADD` | `GX_BLENDMODE_ADD` | 加算合成 |
| `DX_BLENDMODE_SUB` | `GX_BLENDMODE_SUB` | 減算合成 |
| `DX_BLENDMODE_MUL` | `GX_BLENDMODE_MUL` | 乗算合成 |
| `DX_PLAYTYPE_NORMAL` | `GX_PLAYTYPE_NORMAL` | 通常再生 |
| `DX_PLAYTYPE_BACK` | `GX_PLAYTYPE_BACK` | バックグラウンド再生 |
| `DX_PLAYTYPE_LOOP` | `GX_PLAYTYPE_LOOP` | ループ再生 |
| `DX_FONTTYPE_NORMAL` | `GX_FONTTYPE_NORMAL` | 通常フォント |
| `DX_INPUT_PAD1` | `GX_INPUT_PAD1` | パッド1 |

### 4. 関数対応表

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
| `SetDrawBlendMode()` | `SetDrawBlendMode()` | 定数名が異なる |
| `SetDrawBright()` | `SetDrawBright()` | 同名 |

#### テキスト

| DXLib | GXLib | 備考 |
|---|---|---|
| `DrawString()` | `DrawString()` | ASCII のみ |
| `DrawFormatString()` | `DrawFormatString()` | ASCII のみ |
| `GetDrawStringWidth()` | `GetDrawStringWidth()` | 同名 |
| `CreateFontToHandle()` | `CreateFontToHandle()` | 同名 |
| `DeleteFontToHandle()` | `DeleteFontToHandle()` | 同名 |
| `DrawStringToHandle()` | `DrawStringToHandle()` | ASCII のみ |
| `DrawFormatStringToHandle()` | `DrawFormatStringToHandle()` | ASCII のみ |
| `GetDrawStringWidthToHandle()` | `GetDrawStringWidthToHandle()` | 同名 |

#### 入力

| DXLib | GXLib | 備考 |
|---|---|---|
| `CheckHitKey()` | `CheckHitKey()` | DIK_* コード互換 |
| `GetHitKeyStateAll()` | `GetHitKeyStateAll()` | 同名 |
| `GetMouseInput()` | `GetMouseInput()` | 同名 |
| `GetMousePoint()` | `GetMousePoint()` | 同名 |
| `GetMouseWheelRotVol()` | `GetMouseWheelRotVol()` | 同名 |
| `GetJoypadInputState()` | `GetJoypadInputState()` | 同名 |

#### サウンド

| DXLib | GXLib | 備考 |
|---|---|---|
| `LoadSoundMem()` | `LoadSoundMem()` | 同名 |
| `PlaySoundMem()` | `PlaySoundMem()` | 定数名が異なる |
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
| `MV1LoadModel()` | `LoadModel()` | 関数名変更 |
| `MV1DeleteModel()` | `DeleteModel()` | 関数名変更 |
| `MV1DrawModel()` | `DrawModel()` | 関数名変更 |
| `MV1SetPosition()` | `SetModelPosition()` | 関数名変更 |
| `MV1SetScale()` | `SetModelScale()` | 関数名変更 |
| `MV1SetRotationXYZ()` | `SetModelRotation()` | 関数名変更 |
| `SetCameraPositionAndTarget_UpVecY()` | `SetCameraPositionAndTarget()` | Up固定(Y) |
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

### フォント
- GXLib のフォントは **ASCII (32-126) のみ対応**
- 日本語テキストの描画は未対応
- Unicode 文字を描画すると実行時エラーが発生する可能性あり

### カラー値
- DXLib: `GetColor()` は内部形式のカラー値を返す
- GXLib: `GetColor()` は `0xFFRRGGBB` 形式を返す

### 3D モデル形式
- DXLib: `.x`, `.mv1` 形式に対応
- GXLib: **glTF (.gltf/.glb)** のみ対応

### サウンド形式
- DXLib: `.mp3`, `.ogg`, `.wav` 等に広く対応
- GXLib: `.wav` のみ対応（XAudio2 ベース）

### 描画方式
- DXLib: Direct3D 9/11 ベース（即時モード描画）
- GXLib: **Direct3D 12 ベース**（コマンドリスト、バッチ描画）
- 描画順序やタイミングが異なる場合がある

### 未実装の DXLib 関数
以下の DXLib 関数は GXLib に互換実装がありません:
- `SetTransColor()` — 透過色指定
- `SetDrawArea()` — 描画領域制限
- `SaveDrawScreen()` — スクリーンショット保存
- `MakeScreen()` — オフスクリーン描画
- `GetDrawScreenGraph()` — 画面キャプチャ
- `SetFontSize()` — デフォルトフォントサイズ変更
- ネットワーク関連関数（DXLib の HTTP は未対応、GXLib はネイティブ API のみ）

## GXLib 固有の機能

DXLib にない GXLib 独自機能:
- **HDR レンダリング + ポストエフェクト** (Bloom, SSAO, SSR, DoF, TAA 等)
- **PBR マテリアル** (メタリック/ラフネス)
- **レンダーレイヤーシステム** (Scene + UI の分離合成)
- **XML + CSS GUI** (宣言的UIシステム)
- **VFS (仮想ファイルシステム)** + アーカイブ (AES-256 暗号化)
- **シェーダーホットリロード** (F9キー)
- **GPU プロファイラ** (Pキー)
- **Jolt Physics 3D** (物理シミュレーション)
- **WebSocket / HTTP クライアント**
