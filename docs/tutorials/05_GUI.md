# 05 - GUI System

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

## 次のステップ

- [DXLib 移行ガイド](../migration/DxLibMigrationGuide.md) — DXLib からの移行方法
- [API リファレンス](../index.html) — 全 GUI ウィジェットの API 詳細
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認
