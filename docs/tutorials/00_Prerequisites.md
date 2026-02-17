# 00 - 前提知識ガイド

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
