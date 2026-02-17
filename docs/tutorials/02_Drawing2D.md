# 02 - 2D Drawing

スプライト、図形、テキストの描画方法を解説します。

## このチュートリアルで学ぶこと

- 画像（スプライト）の読み込みと描画
- スプライトシートによるアニメーション素材の管理
- 基本図形（線、四角形、円など）の描画
- ブレンドモード（半透明、加算合成）
- テキストの描画とフォント管理
- ネイティブ API による Camera2D / Animation2D

## 前提知識

- [01_GettingStarted.md](01_GettingStarted.md) の内容（ビルドと Hello Window）
- 2D 座標系の理解（左上が原点、右が X+、下が Y+）

→ 座標系について詳しくは [00_Prerequisites.md](00_Prerequisites.md) を参照

## スプライト描画

### 画像の読み込みと描画

```cpp
// 画像ファイルをGPUメモリに読み込む
// 戻り値の texHandle は画像を識別する番号（ハンドル）
int texHandle = LoadGraph("Assets/player.png");

// 画像をそのまま描画
DrawGraph(
    100, 200,       // 描画位置の左上座標 (x=100, y=200)
    texHandle,      // 描画する画像のハンドル
    TRUE            // TRUE: 透過処理あり（PNG の透明部分を透過する）
);

// 拡大・回転して描画
DrawRotaGraph(
    640, 480,       // 描画中心座標 (x=640, y=480)（※左上ではなく中心）
    2.0,            // 拡大率 (1.0 = 等倍, 2.0 = 2倍拡大, 0.5 = 半分)
    0.5,            // 回転角度（ラジアン単位, 約28.6度。π ≒ 3.14 で180度）
    texHandle,      // 画像ハンドル
    TRUE            // 透過処理あり
);

// 指定範囲に伸縮して描画（引き伸ばし）
DrawExtendGraph(
    0, 0,           // 描画先の左上座標
    320, 240,       // 描画先の右下座標 → 320x240 の範囲に伸縮
    texHandle,      // 画像ハンドル
    TRUE            // 透過処理あり
);

// 画像の一部だけ切り出して描画（スプライトシート用）
DrawRectGraph(
    100, 100,       // 描画位置の左上座標
    0, 0,           // 画像内の切り出し開始位置 (左上からのオフセット)
    32, 32,         // 切り出すサイズ (幅32px × 高さ32px)
    texHandle,      // 画像ハンドル
    TRUE,           // 透過処理あり
    FALSE           // FALSE: 左右反転なし（TRUE にすると鏡像になる）
);

// 使い終わったら解放（GPUメモリを返却）
DeleteGraph(texHandle);
```

> **DrawGraph と DrawRotaGraph の座標の違いに注意**
>
> `DrawGraph` は **左上座標** を指定しますが、
> `DrawRotaGraph` は **中心座標** を指定します。
> 回転の中心を画像の中央にするためこのような設計になっています。

### スプライトシート（分割読み込み）

1枚の大きな画像に複数のキャラクターポーズが並んだ「スプライトシート」を
一度に読み込み、フレーム番号で簡単に描画できます。

```cpp
int sprites[16];  // 16コマ分のハンドルを格納する配列

LoadDivGraph(
    "Assets/character.png",  // スプライトシート画像ファイル
    16,     // 総コマ数 (= 横分割数 × 縦分割数)
    4, 4,   // 横に4コマ, 縦に4コマの格子状に分割
    32, 32, // 1コマのサイズ (幅32px × 高さ32px)
    sprites // ハンドルの格納先配列
);

// フレーム番号を指定して描画（0〜15）
// アニメーションは frameIndex を時間経過で変えるだけ
DrawGraph(x, y, sprites[frameIndex], TRUE);
```

### ネイティブ API（SpriteBatch）

ネイティブ API では `SpriteBatch` クラスを使います。
より細かい制御（ソース矩形、スケール、回転、色の個別指定）が可能です。

```cpp
GX::SpriteBatch spriteBatch;
spriteBatch.Initialize(device, cmdList);

// --- フレームループ内 ---
spriteBatch.Begin(cmdList, frameIndex);
// Begin() と End() の間に描画命令を記録する

// シンプルな描画
spriteBatch.Draw(
    texHandle,          // テクスチャハンドル
    100.0f, 200.0f      // 描画位置 (x, y)
);

// フル指定の描画
spriteBatch.Draw(
    texHandle,                          // テクスチャハンドル
    100.0f, 200.0f,                     // 描画位置 (x, y)
    0.0f, 0.0f, 64.0f, 64.0f,          // ソース矩形: 画像内の切り出し範囲 (x, y, 幅, 高さ)
    2.0f, 2.0f,                         // スケール (横2倍, 縦2倍)
    0.5f,                               // 回転角度（ラジアン）
    1.0f, 1.0f, 1.0f, 1.0f             // カラー乗算 (R, G, B, A) 1.0=変化なし
);

spriteBatch.End();
// End() で実際に GPU 描画命令が発行される
```

## プリミティブ描画

### 基本図形

```cpp
// 直線
DrawLine(
    0, 0,                   // 始点 (x1, y1)
    100, 100,               // 終点 (x2, y2)
    GetColor(255, 0, 0),    // 色 (R=255, G=0, B=0 = 赤)
    2                       // 線の太さ（ピクセル）
);

// 矩形（四角形）
DrawBox(
    50, 50,                 // 左上 (x1, y1)
    200, 150,               // 右下 (x2, y2)
    GetColor(0, 255, 0),    // 色 (緑)
    TRUE                    // TRUE: 塗りつぶし, FALSE: 枠線のみ
);

// 円
DrawCircle(
    320, 240,               // 中心座標 (x, y)
    50,                     // 半径（ピクセル）
    GetColor(0, 0, 255),    // 色 (青)
    TRUE                    // TRUE: 塗りつぶし
);

// 三角形
DrawTriangle(
    100, 300,               // 頂点1 (x1, y1)
    200, 200,               // 頂点2 (x2, y2)
    300, 300,               // 頂点3 (x3, y3)
    GetColor(255, 255, 0),  // 色 (黄)
    TRUE                    // TRUE: 塗りつぶし
);

// 楕円
DrawOval(
    400, 300,               // 中心座標 (x, y)
    80, 40,                 // 横半径, 縦半径（横長の楕円）
    GetColor(255, 128, 0),  // 色 (オレンジ)
    TRUE                    // TRUE: 塗りつぶし
);
```

### ブレンドモード

ブレンドモードを使うと、半透明描画や光のエフェクトが実現できます。

```cpp
// --- アルファブレンド（半透明）---
SetDrawBlendMode(
    GX_BLENDMODE_ALPHA,     // アルファブレンド: 描画色と背景色を透明度で混合
    128                     // 不透明度 (0=完全に透明, 255=完全に不透明, 128=半透明)
);
DrawBox(100, 100, 300, 200, GetColor(255, 0, 0), TRUE);
SetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);  // ブレンドを元に戻す（忘れると後続の描画も半透明に！）

// --- 加算合成（光のエフェクト）---
// 色を「足し算」するので、重ねるほど明るくなります
SetDrawBlendMode(
    GX_BLENDMODE_ADD,       // 加算合成: 光源、炎、パーティクルに最適
    200                     // 強度 (0-255)
);
DrawCircle(320, 240, 100, GetColor(255, 255, 200), TRUE);
SetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);  // 必ず戻す
```

> **なぜブレンドモードを「戻す」必要があるのか？**
>
> `SetDrawBlendMode` はグローバルな状態を変更します。
> 戻さないと、以降の描画すべてにブレンドが適用されてしまいます。
> 半透明描画の直後に `GX_BLENDMODE_NOBLEND` を呼ぶのを習慣にしてください。

## テキスト描画

### デフォルトフォント

```cpp
// 基本テキスト描画
DrawString(
    10, 10,                         // 描画位置の左上 (x, y)
    "Score: 12345",                 // 表示テキスト
    GetColor(255, 255, 255)         // 文字色 (白)
);

// 書式付きテキスト（printf 形式で変数を埋め込める）
DrawFormatString(
    10, 40,                         // 描画位置
    GetColor(255, 255, 0),          // 文字色 (黄)
    "HP: %d / %d", currentHP, maxHP // %d に整数値が入る
);

// 文字列の描画幅を取得（テキスト中央揃えなどに使用）
int width = GetDrawStringWidth(
    "Hello",    // 測定するテキスト
    5           // 文字数
);
```

### カスタムフォント

```cpp
// フォントを作成
int font = CreateFontToHandle(
    "MS Gothic",    // フォント名（OS にインストールされているフォント）
    24,             // フォントサイズ（ピクセル）
    3               // 太さ (0=通常, 大きいほど太い)
);

// 作成したフォントを指定して描画
DrawStringToHandle(
    100, 100,                       // 描画位置 (x, y)
    "Title",                        // 表示テキスト
    GetColor(255, 255, 255),        // 文字色
    font                            // フォントハンドル
);

// 使い終わったら解放
DeleteFontToHandle(font);
```

> **フォントの日本語対応について**
>
> **Compat API** (簡易API) のテキスト関数は ASCII 文字 (英数字・記号) のみ対応しています。
> **ネイティブ API** の `TextRenderer` は Unicode をフルサポートしており、
> 日本語テキストも描画可能です。日本語を使いたい場合はネイティブ API を利用してください。
>
> ```cpp
> // ネイティブ API で日本語テキスト描画
> textRenderer.DrawString(fontHandle, 10, 10, L"こんにちは世界！", 0xFFFFFFFF);
> ```

## ネイティブ API の高度な機能

### Camera2D

2D カメラを使うと、「カメラ」の位置・ズーム・回転を変えることで
ゲームワールド全体を動かすことができます。プレイヤーの追従や画面のシェイクに使います。

```cpp
GX::Camera2D camera;
camera.SetPosition(100.0f, 200.0f);    // カメラの注視位置（ワールド座標）
camera.SetZoom(1.5f);                  // ズーム倍率 (1.0=等倍, 1.5=1.5倍拡大)
camera.SetRotation(0.1f);              // 回転角度 (ラジアン)

// カメラのビュー行列を SpriteBatch に適用
// これにより、全ての描画がカメラ基準に変換されます
spriteBatch.SetViewMatrix(camera.GetViewMatrix());
```

### SpriteSheet / Animation2D

スプライトシートとアニメーションを組み合わせると、
パラパラ漫画のようなキャラクターアニメーションが簡単に実装できます。

```cpp
GX::SpriteSheet sheet;
sheet.Initialize(
    texHandle,      // スプライトシートの画像ハンドル
    32, 32,         // 1コマのサイズ (幅32px, 高さ32px)
    4, 4            // グリッド (横4コマ × 縦4コマ = 計16コマ)
);

GX::Animation2D anim;
anim.AddFrames(
    sheet,          // スプライトシート
    0, 3,           // 使用するフレーム範囲 (0番〜3番の4コマ)
    0.1f            // 1コマの表示時間 (0.1秒 = 秒間10コマ)
);

// フレームループ内:
anim.Update(deltaTime);                     // 経過時間でアニメーション進行
anim.Draw(spriteBatch, 100.0f, 200.0f);     // 現在のフレームを描画
```

> **deltaTime（デルタタイム）とは？**
>
> 前フレームから現在フレームまでの経過時間（秒）です。
> これをアニメーション速度や移動速度に掛けることで、
> フレームレート (FPS) に依存しない一定速度の動作を実現できます。
> 詳しくは [03_Game2D.md](03_Game2D.md) の「フレームレート制御」を参照してください。

## よくある問題

### 画像が表示されない

- ファイルパスが正しいか確認（`Assets/` からの相対パス）
- `LoadGraph` の戻り値が -1 でないか確認（-1 は読み込み失敗）
- 対応画像形式: PNG, JPG, BMP, TGA

### 描画が全体的に半透明 / 暗い

- `SetDrawBlendMode` を呼んだ後、`GX_BLENDMODE_NOBLEND` に戻し忘れていないか確認
- `SetDrawBright` で明るさを変更した後、元に戻し忘れていないか確認

### DrawRotaGraph で位置がずれる

- `DrawRotaGraph` は **中心座標** を指定します（DrawGraph は左上座標）
- 画像の中心を (x, y) に合わせたい場合はそのまま使えます
- 左上を合わせたい場合は `(x + 幅/2, y + 高さ/2)` を指定してください

## 次のステップ

- [03_Game2D.md](03_Game2D.md) — 入力処理とサウンドを追加してゲームを作る
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認
