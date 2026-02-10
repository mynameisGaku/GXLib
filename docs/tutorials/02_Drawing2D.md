# 02 - 2D Drawing

スプライト、図形、テキストの描画方法を解説します。

## スプライト描画

### 画像の読み込みと描画

```cpp
// 画像読み込み
int texHandle = LoadGraph("Assets/player.png");

// 基本描画
DrawGraph(100, 200, texHandle, TRUE);  // TRUE = 透過有効

// 拡大・回転描画
DrawRotaGraph(640, 480, 2.0, 0.5, texHandle, TRUE);

// 矩形伸縮描画
DrawExtendGraph(0, 0, 320, 240, texHandle, TRUE);

// 矩形切り出し描画（スプライトシート用）
DrawRectGraph(100, 100, 0, 0, 32, 32, texHandle, TRUE, FALSE);

// 解放
DeleteGraph(texHandle);
```

### スプライトシート（分割読み込み）

```cpp
int sprites[16];
LoadDivGraph("Assets/character.png",
    16,     // 総分割数
    4, 4,   // 横分割数, 縦分割数
    32, 32, // 1コマのサイズ
    sprites);

// フレーム番号で描画
DrawGraph(x, y, sprites[frameIndex], TRUE);
```

### ネイティブ API（SpriteBatch）

```cpp
GX::SpriteBatch spriteBatch;
spriteBatch.Initialize(device, cmdList);

// フレームループ内
spriteBatch.Begin(cmdList, frameIndex);
spriteBatch.Draw(texHandle, 100.0f, 200.0f);  // 基本描画
spriteBatch.Draw(texHandle, 100.0f, 200.0f,
    0.0f, 0.0f, 64.0f, 64.0f,  // ソース矩形
    2.0f, 2.0f,                  // スケール
    0.5f,                        // 回転角度（ラジアン）
    1.0f, 1.0f, 1.0f, 1.0f);   // カラー (RGBA)
spriteBatch.End();
```

## プリミティブ描画

### 基本図形

```cpp
// 直線
DrawLine(0, 0, 100, 100, GetColor(255, 0, 0), 2);

// 矩形（fillFlag: TRUE=塗りつぶし, FALSE=枠線）
DrawBox(50, 50, 200, 150, GetColor(0, 255, 0), TRUE);

// 円
DrawCircle(320, 240, 50, GetColor(0, 0, 255), TRUE);

// 三角形
DrawTriangle(100, 300, 200, 200, 300, 300, GetColor(255, 255, 0), TRUE);

// 楕円
DrawOval(400, 300, 80, 40, GetColor(255, 128, 0), TRUE);
```

### ブレンドモード

```cpp
// 半透明描画（アルファブレンド、不透明度128/255）
SetDrawBlendMode(GX_BLENDMODE_ALPHA, 128);
DrawBox(100, 100, 300, 200, GetColor(255, 0, 0), TRUE);
SetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);  // 戻す

// 加算合成（光のエフェクト向き）
SetDrawBlendMode(GX_BLENDMODE_ADD, 200);
DrawCircle(320, 240, 100, GetColor(255, 255, 200), TRUE);
SetDrawBlendMode(GX_BLENDMODE_NOBLEND, 0);
```

## テキスト描画

### デフォルトフォント

```cpp
// 基本テキスト
DrawString(10, 10, "Score: 12345", GetColor(255, 255, 255));

// 書式付きテキスト
DrawFormatString(10, 40, GetColor(255, 255, 0),
    "HP: %d / %d", currentHP, maxHP);

// 文字列幅の取得
int width = GetDrawStringWidth("Hello", 5);
```

### カスタムフォント

```cpp
// フォント作成（フォント名、サイズ、太さ）
int font = CreateFontToHandle("MS Gothic", 24, 3);

// フォント指定で描画
DrawStringToHandle(100, 100, "Title", GetColor(255, 255, 255), font);

// 解放
DeleteFontToHandle(font);
```

> **注意:** GXLib のフォントは ASCII 文字（32-126）のみ対応しています。
> 日本語テキストは描画できません。

## ネイティブ API の高度な機能

### Camera2D

```cpp
GX::Camera2D camera;
camera.SetPosition(100.0f, 200.0f);
camera.SetZoom(1.5f);
camera.SetRotation(0.1f);

// SpriteBatch に適用
spriteBatch.SetViewMatrix(camera.GetViewMatrix());
```

### SpriteSheet / Animation2D

```cpp
GX::SpriteSheet sheet;
sheet.Initialize(texHandle, 32, 32, 4, 4); // 32x32px, 4x4グリッド

GX::Animation2D anim;
anim.AddFrames(sheet, 0, 3, 0.1f); // フレーム0-3, 0.1秒/フレーム
anim.Update(deltaTime);
anim.Draw(spriteBatch, 100.0f, 200.0f);
```

## 次のステップ

- [03_Game2D.md](03_Game2D.md) — 入力処理とサウンドを追加してゲームを作る
