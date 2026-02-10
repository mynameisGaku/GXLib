# Phase 6a+6b Summary: GUI Core Foundation + StyleSheet Parser

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
