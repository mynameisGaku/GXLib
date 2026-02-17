# Platformer2D

2D プラットフォーマーのサンプル。ジャンプ、簡易コリジョン、コイン収集の流れを確認できます。

## このサンプルで学べること

- 重力とジャンプの実装（速度 + 加速度）
- タイルベースの地形当たり判定
- アイテム（コイン）の収集とスコア管理
- スプライトシートによるキャラクターアニメーション
- カメラのプレイヤー追従

## コードのポイント

- **重力処理**: 毎フレーム `velocityY += gravity * deltaTime` で落下加速
- **地面判定**: AABB でプレイヤーの足元と地形ブロックの衝突を検出
- **ジャンプ**: 接地中にスペースキーで `velocityY` を負の値に設定
- **コイン収集**: プレイヤーとコインの矩形が重なったらスコア加算＆コイン消去

## 操作

- 矢印キー: 移動
- Space: ジャンプ
- ESC: 終了

## ビルド/実行

```bash
cmake -B build -S .    # ルートで実行
cmake --build build --config Debug --target Platformer2D
```

`build/` 配下に生成される `Platformer2D.exe` を実行してください。

## 関連チュートリアル

- [03_Game2D](../../docs/tutorials/03_Game2D.md) — 入力・サウンド・衝突判定
- [02_Drawing2D](../../docs/tutorials/02_Drawing2D.md) — スプライト描画の基本
