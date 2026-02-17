# EasyHello

GXLib の最小構成サンプル。DXLib 互換 API (Compat API) を使ったシンプルなプログラムです。

## このサンプルで学べること

- GXLib の Compat API による最小限のウィンドウ表示
- メインループ（ProcessMessage → ClearDrawScreen → 描画 → ScreenFlip）の流れ
- キーボード入力による図形の移動
- `GetColor()` を使った色指定

## コードのポイント

- **メインループの基本形**: `while (ProcessMessage() == 0)` でウィンドウが閉じるまで繰り返し
- **ダブルバッファリング**: `SetDrawScreen(GX_SCREEN_BACK)` で裏画面に描画し `ScreenFlip()` で表示
- **入力処理**: `CheckHitKey()` で押されているキーを判定して円を移動

## 操作

- 矢印キー: 円を移動
- ESC: 終了

## ビルド/実行

```bash
cmake -B build -S .    # ルートで実行
cmake --build build --config Debug --target EasyHello
```

`build/` 配下に生成される `EasyHello.exe` を実行してください。

## 関連チュートリアル

- [01_GettingStarted](../../docs/tutorials/01_GettingStarted.md) — ビルドとHello Window
- [02_Drawing2D](../../docs/tutorials/02_Drawing2D.md) — 描画の基本
