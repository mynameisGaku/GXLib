# GUIMenuDemo

XML/CSS で UI を構築する GUI メニューデモです。

## 操作
- クリック: UI 操作
- F5: UI 再読み込み（XML/CSS）
- F2: レイアウトデバッグ表示
- ESC: 戻る/終了

## 主要ファイル
- `Assets/ui/guimenu_demo.xml`
- `Assets/ui/guimenu_demo.css`

## ビルド/実行
- ルートで `cmake -B build -S .`
- `cmake --build build --config Debug --target GUIMenuDemo`
- `build/` 配下に生成される `GUIMenuDemo.exe` を実行
