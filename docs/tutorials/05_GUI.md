# 05 - GUI System

XML + CSS による宣言的 GUI の構築方法を解説します。

## 概要

GXLib の GUI システムは Web 技術にインスパイアされた構成です:
- **XML** — ウィジェットの構造を定義
- **CSS** — スタイル（色、サイズ、レイアウト）を定義
- **C++** — イベントハンドラを登録

## 基本構成

### XML ファイル (menu.xml)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Panel id="root" class="main-panel">
    <TextWidget id="title" class="title-text">Game Menu</TextWidget>
    <Button id="btnStart" class="menu-button">Start Game</Button>
    <Button id="btnOptions" class="menu-button">Options</Button>
    <Button id="btnQuit" class="menu-button">Quit</Button>
</Panel>
```

### CSS ファイル (menu.css)

```css
#root {
    width: 400;
    height: 500;
    padding: 20;
    flex-direction: column;
    align-items: center;
    gap: 15;
    background-color: rgba(20, 20, 40, 0.9);
    corner-radius: 10;
}

.title-text {
    font-size: 32;
    color: rgba(255, 255, 200, 1.0);
    margin-bottom: 30;
}

.menu-button {
    width: 300;
    height: 50;
    background-color: rgba(60, 80, 120, 1.0);
    color: rgba(255, 255, 255, 1.0);
    font-size: 20;
    corner-radius: 8;
    justify-content: center;
    align-items: center;
}

.menu-button:hover {
    background-color: rgba(80, 110, 160, 1.0);
}

.menu-button:pressed {
    background-color: rgba(40, 60, 100, 1.0);
}
```

### C++ コード

```cpp
#include "GUI/UIContext.h"
#include "GUI/UIRenderer.h"
#include "GUI/StyleSheet.h"
#include "GUI/GUILoader.h"

GX::UIContext uiContext;
GX::UIRenderer uiRenderer;
GX::StyleSheet styleSheet;
GX::GUILoader loader;

// 初期化
uiRenderer.Initialize(device, cmdList, spriteBatch, textRenderer, fontManager);
uiContext.SetDesignResolution(1280, 960);

// フォント登録
int fontHandle = fontManager.LoadFont("Arial", 20);
loader.RegisterFont("default", fontHandle);

// イベント登録
loader.RegisterEvent("btnStart", [](GX::Widget&) {
    StartGame();
});
loader.RegisterEvent("btnQuit", [](GX::Widget&) {
    PostQuitMessage(0);
});

// XML + CSS 読み込み
auto root = loader.LoadFromFile("Assets/ui/menu.xml");
styleSheet.LoadFromFile("Assets/ui/menu.css");

// レイアウト更新
uiContext.SetRoot(root);
styleSheet.ApplyToTree(*root);
uiContext.ComputeLayout(1280, 960);

// フレームループ内
uiContext.ProcessInputEvents(inputManager);
uiContext.ComputeLayout(1280, 960);
uiRenderer.Begin();
uiRenderer.DrawWidgetTree(*root);
uiRenderer.End();
```

## 利用可能なウィジェット

| ウィジェット | XML タグ | 説明 |
|---|---|---|
| Panel | `<Panel>` | コンテナ、レイアウト制御 |
| TextWidget | `<TextWidget>` | テキスト表示 |
| Button | `<Button>` | クリック可能ボタン |
| CheckBox | `<CheckBox>` | チェックボックス |
| Slider | `<Slider>` | 数値スライダー |
| ProgressBar | `<ProgressBar>` | 進行状況バー |
| Image | `<Image>` | 画像表示 |
| TextInput | `<TextInput>` | テキスト入力 |
| DropDown | `<DropDown>` | ドロップダウン選択 |
| RadioButton | `<RadioButton>` | ラジオボタン |
| ListView | `<ListView>` | リスト表示 |
| ScrollView | `<ScrollView>` | スクロール可能領域 |
| TabView | `<TabView>` | タブ切替 |
| Dialog | `<Dialog>` | モーダルダイアログ |
| Canvas | `<Canvas>` | カスタム描画 |
| Spacer | `<Spacer>` | 余白用スペーサー |

## レイアウト（Flexbox）

GXLib の GUI は CSS Flexbox ライクなレイアウトをサポートします。

```css
#container {
    flex-direction: row;        /* row / column */
    justify-content: center;    /* flex-start / center / flex-end / space-between */
    align-items: center;        /* flex-start / center / flex-end / stretch */
    gap: 10;                    /* 子要素間のギャップ */
    flex-grow: 1;               /* 余剰スペースの割当比率 */
}
```

## スタイルプロパティ

| プロパティ | 型 | 説明 |
|---|---|---|
| width / height | float | サイズ (px) |
| min-width / max-width | float | 最小/最大サイズ |
| padding | float | 内側余白 (一括) |
| padding-left/right/top/bottom | float | 内側余白 (個別) |
| margin | float | 外側余白 (一括) |
| margin-left/right/top/bottom | float | 外側余白 (個別) |
| background-color | rgba() | 背景色 |
| color | rgba() | テキスト色 |
| border-color | rgba() | ボーダー色 |
| border-width | float | ボーダー幅 |
| corner-radius | float | 角丸半径 |
| font-size | float | フォントサイズ |
| opacity | float | 不透明度 (0-1) |
| overflow | hidden/visible/scroll | オーバーフロー制御 |
| position | relative/absolute | 配置方法 |

## 擬似クラス

```css
Button:hover   { background-color: rgba(100, 100, 200, 1.0); }
Button:pressed { background-color: rgba(50, 50, 150, 1.0); }
Button:disabled { opacity: 0.5; }
TextInput:focused { border-color: rgba(100, 150, 255, 1.0); }
```

## GUI スケーリング

デザイン解像度を設定すると、実際のウィンドウサイズに自動スケーリングされます。

```cpp
uiContext.SetDesignResolution(1280, 960);
// 1920x1080 のウィンドウでも 1280x960 基準でレイアウトされる
```

## サンプル

`Samples/GUIMenuDemo/` に完全な GUI メニューのサンプルがあります。

```bash
cmake --build build --config Debug --target GUIMenuDemo
```
