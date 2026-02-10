# Phase 9 Summary: DXLib互換レイヤー + Shader Hot Reload

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
