# Phase 2: Text, Input, Sound — 実装サマリー

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
