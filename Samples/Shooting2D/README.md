# Shooting2D

シンプルな 2D シューティングゲームのサンプル。弾の発射、敵の生成、当たり判定の流れを確認できます。

## このサンプルで学べること

- キーボード入力でプレイヤーを移動させる方法
- 弾オブジェクトの生成・移動・画面外削除
- 敵の自動出現パターン
- AABB (矩形) / Circle (円) による 2D 衝突判定
- スコア管理とテキスト表示
- 効果音 (SE) の再生タイミング

## コードのポイント

- **弾管理**: `std::vector` で弾のリストを管理し、画面外に出た弾を削除
- **当たり判定**: `Collision2D::TestCirclevsCircle()` で弾と敵の衝突を検出
- **deltaTime**: フレームレートに依存しない一定速度の移動

## 操作

- 矢印キー: 移動
- Space: ショット
- Enter: ゲームオーバー後にリスタート
- ESC: 終了

## ビルド/実行

```bash
cmake -B build -S .    # ルートで実行
cmake --build build --config Debug --target Shooting2D
```

`build/` 配下に生成される `Shooting2D.exe` を実行してください。

## 関連チュートリアル

- [03_Game2D](../../docs/tutorials/03_Game2D.md) — 入力・サウンド・衝突判定
- [02_Drawing2D](../../docs/tutorials/02_Drawing2D.md) — スプライト描画の基本
