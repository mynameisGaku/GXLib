# Phase 6c Summary: XML Parser + GUILoader

## Overview
XMLファイルからウィジェットツリーを宣言的に定義する仕組みを実装。
C++のウィジェット構築コード (~40行) を XML + CSS + C++ イベントバインディング (~15行) に置き換え。

## Completion Condition
> XMLとCSSでメニュー・HUDが構築でき、C++でイベント処理可能 → **XML部分 達成**

---

## New Files

| File | Description |
|------|-------------|
| `GXLib/GUI/XMLParser.h` | XMLNode struct + XMLDocument class (DOM) |
| `GXLib/GUI/XMLParser.cpp` | 再帰降下XMLパーサー (BOM, comments, `<?xml?>`, entities, self-closing) |
| `GXLib/GUI/GUILoader.h` | GUILoader class (font/event registration, BuildFromFile/BuildFromDocument) |
| `GXLib/GUI/GUILoader.cpp` | BuildWidget (tag→widget mapping, font/event resolution, inline style) + Utf8ToWstring |
| `Assets/ui/menu.xml` | 宣言的メニューUI (root panel, menu panel, title text, 3 buttons) |

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/GUI/StyleSheet.h` | `ApplyProperty()` と `NormalizePropertyName()` を private → public static に移動 |
| `Sandbox/main.cpp` | GUILoader使用に書き換え (~40行 → ~15行), タイトル "Phase6c [XML Parser]" |

## Architecture

```
menu.xml → XMLDocument → XMLNode tree
                              ↓
GUILoader.BuildWidget() → Widget tree
  ├─ tag → widget type (Panel/Text/Button, unknown → Panel fallback)
  ├─ id/class/enabled/visible → widget properties
  ├─ font → ResolveFontHandle (registered name → int handle)
  ├─ onClick → m_eventMap lookup → widget.onClick
  ├─ text attr or text content → Utf8ToWstring → SetText
  ├─ other attrs → StyleSheet::ApplyProperty (inline style)
  └─ children → recursive BuildWidget
```

### XML Parser Details

**再帰降下パーサー** (トークナイザ不要、文字単位走査):
- UTF-8 BOM (0xEF 0xBB 0xBF) スキップ
- `<?xml ... ?>` 宣言スキップ
- `<!-- ... -->` コメントスキップ
- `<Tag attr="val">...</Tag>` 通常要素
- `<Tag attr="val" />` 自己閉じ
- `<Text>content</Text>` テキストコンテント
- 属性クォート: `"..."` と `'...'` 両対応
- XML エンティティ: `&amp;` `&lt;` `&gt;` `&quot;` `&apos;`
- タグ不一致: `GX_LOG_ERROR` + 位置ヒント

### GUILoader Details

**C++ Usage:**
```cpp
GX::GUI::GUILoader loader;
loader.SetRenderer(&g_uiRenderer);
loader.RegisterFont("default", g_guiFontHandle);
loader.RegisterFont("large", g_guiFontLarge);
loader.RegisterEvent("onStartGame", []() { GX_LOG_INFO("Start!"); });
loader.RegisterEvent("onOpenOptions", []() { GX_LOG_INFO("Options!"); });
loader.RegisterEvent("onExit", []() { PostQuitMessage(0); });

auto root = loader.BuildFromFile("Assets/ui/menu.xml");
```

## Key Design Decisions
- **自前XMLパーサー**: 外部依存なし、軽量 (pugixml/RapidXML不要)
- **StyleSheet public static**: ApplyProperty/NormalizePropertyName をGUILoaderから利用可能に
- **`<unordered_set>` 不使用**: pch.hに含まれないため、inline比較で代替
- **テキスト内容**: text属性 > テキストコンテント (属性優先)
- **未知タグ → Panel fallback**: エラーではなく警告 + フォールバック

## Error Handling

| Situation | Behavior |
|-----------|----------|
| XML file not found | `GX_LOG_ERROR`, return nullptr |
| Invalid XML (unclosed tag etc.) | `GX_LOG_ERROR` + position hint, return nullptr |
| Unknown tag name | `GX_LOG_WARN`, Panel fallback |
| Unknown font name | `GX_LOG_WARN`, handle=-1 |
| Unregistered event name | `GX_LOG_WARN`, onClick not set |
| Unknown entity | Passed through as-is |

## Verification
- Build: OK (Debug)
- 見た目: Phase 6b と同一のメニュー表示 (U キーでトグル)
- イベント: ボタンクリックで LOG 出力 + Exit で終了
- ホバー/プレス: CSS :hover/:pressed がXML構築ツリーでも動作
- コード削減: main.cpp GUI構築 ~40行 → ~15行
