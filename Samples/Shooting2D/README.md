# Shooting2D

シンプルな 2D シューティングのサンプル。弾の発射、敵の生成、当たり判定の流れを確認できます。

## 操作
- 矢印キー: 移動
- Space: ショット
- Enter: ゲームオーバー後にリスタート
- ESC: 終了

## ビルド/実行
- ルートで `cmake -B build -S .`
- `cmake --build build --config Debug --target Shooting2D`
- `build/` 配下に生成される `Shooting2D.exe` を実行
