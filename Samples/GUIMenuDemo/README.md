# GUIMenuDemo

XML + CSS で UI を構築する GUI メニューデモです。
17種類のウィジェットの使い方を確認できます。

## このサンプルで学べること

- XML によるウィジェットツリーの定義
- CSS によるスタイル指定（色、サイズ、レイアウト、擬似クラス）
- Flexbox レイアウト（要素の並び方向、配置、間隔）
- C++ からのイベント登録（ボタンクリック、スライダー値変更等）
- デザイン解像度によるGUIスケーリング
- ホットリロード（F5 で XML/CSS を再読み込み）

## コードのポイント

- **宣言的UI**: XML でウィジェット構造を定義し、CSS でスタイルを分離
- **イベント登録**: `loader.RegisterEvent("id", callback)` でクリック処理を登録
- **毎フレームレイアウト**: `ComputeLayout()` を毎フレーム呼んでウィンドウリサイズに対応
- **デバッグ表示**: F2 キーでウィジェットの境界ボックスを可視化

## 操作

- クリック: UI 操作
- F5: UI 再読み込み（XML/CSS のホットリロード）
- F2: レイアウトデバッグ表示
- ESC: 戻る/終了

## 主要ファイル

| ファイル | 内容 |
|---------|------|
| `Assets/ui/guimenu_demo.xml` | ウィジェットツリー定義 |
| `Assets/ui/guimenu_demo.css` | スタイル定義 |
| `main.cpp` | イベント登録・レイアウト・描画処理 |

## ビルド/実行

```bash
cmake -B build -S .    # ルートで実行
cmake --build build --config Debug --target GUIMenuDemo
```

`build/` 配下に生成される `GUIMenuDemo.exe` を実行してください。

## 関連チュートリアル

- [05_GUI](../../docs/tutorials/05_GUI.md) — XML + CSS による GUI 構築
- [01_GettingStarted](../../docs/tutorials/01_GettingStarted.md) — ビルドの基本
