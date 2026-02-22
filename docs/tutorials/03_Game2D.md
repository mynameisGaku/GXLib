# 03 - 2D Game

入力処理、サウンド、衝突判定を組み合わせて 2D ゲームを作る方法を解説します。

## このチュートリアルで学ぶこと

- キーボード・マウス・ゲームパッドによる入力処理
- 「押した瞬間」を検出するトリガー入力
- 効果音 (SE) と BGM の再生
- 2D 衝突判定（矩形、円）
- フレームレートに依存しない移動（deltaTime）

## 前提知識

- [02_Drawing2D.md](02_Drawing2D.md) の内容（描画の基本）
- スプライトの読み込みと描画ができること

## 入力処理

### キーボード

```cpp
// 特定キーが「押されている」かを判定（押し続けている間ずっと TRUE）
if (CheckHitKey(KEY_INPUT_SPACE))
{
    // スペースキー押下中（移動や連射に使う）
}

// 全キーの状態を一括取得（複数キーの同時判定に便利）
char keys[256];             // 256個のキーの状態を格納する配列
GetHitKeyStateAll(keys);    // keys[i] != 0 なら i番目のキーが押されている

if (keys[KEY_INPUT_LEFT])  playerX -= speed;   // 左キー → 左に移動
if (keys[KEY_INPUT_RIGHT]) playerX += speed;   // 右キー → 右に移動
if (keys[KEY_INPUT_UP])    playerY -= speed;   // 上キー → 上に移動 (Y軸は下が正なので減算)
if (keys[KEY_INPUT_DOWN])  playerY += speed;   // 下キー → 下に移動
```

> **CheckHitKey と GetHitKeyStateAll の使い分け**
>
> | 関数 | 用途 | 特徴 |
> |------|------|------|
> | `CheckHitKey(キーコード)` | 1つのキーを確認 | シンプルだが複数キーで何度も呼ぶとやや非効率 |
> | `GetHitKeyStateAll(配列)` | 全キーを一括取得 | 1回の呼び出しで256キー分の状態が取れる |
>
> どちらも「押されている間ずっと TRUE」です。
> 「押した瞬間だけ」検出したい場合はネイティブ API の `IsKeyTriggered` を使います（下記参照）。

### マウス

```cpp
// マウスカーソルの座標を取得（ウィンドウ内の座標）
int mx, my;
GetMousePoint(&mx, &my);    // &mx, &my: ポインタ経由で値を受け取る

// マウスボタンの判定（ビットフラグ: 複数ボタン同時判定可能）
int mouse = GetMouseInput();
if (mouse & MOUSE_INPUT_LEFT)  { /* 左ボタンが押されている */ }
if (mouse & MOUSE_INPUT_RIGHT) { /* 右ボタンが押されている */ }

// マウスホイールの回転量（正=上回転, 負=下回転, 0=動いていない）
int wheel = GetMouseWheelRotVol();
```

### ゲームパッド

```cpp
int pad = GetJoypadInputState(GX_INPUT_PAD1);  // パッド1の状態取得（ビットフラグ）

if (pad & PAD_INPUT_UP)    playerY -= speed;    // 方向パッド上
if (pad & PAD_INPUT_DOWN)  playerY += speed;    // 方向パッド下
if (pad & PAD_INPUT_LEFT)  playerX -= speed;    // 方向パッド左
if (pad & PAD_INPUT_RIGHT) playerX += speed;    // 方向パッド右
if (pad & PAD_INPUT_A)     Fire();              // Aボタン（Xboxコントローラーの場合）
```

### ネイティブ API（トリガー検出）

Compat API の `CheckHitKey` は「押されている間ずっと TRUE」を返します。
「押した瞬間だけ」を検出したい場合（ジャンプ、メニュー選択など）は、
ネイティブ API の `IsKeyTriggered` を使います。

```cpp
GX::InputManager input;
input.Initialize(window);

// --- フレームループ内 ---
input.Update();  // 毎フレーム呼ぶ（前フレームとの差分を計算）

// トリガー: 押された「瞬間」のみ TRUE（押し続けても繰り返さない）
if (input.GetKeyboard().IsKeyTriggered(VK_SPACE))
{
    // ジャンプ開始（1回だけ実行される）
}

// 押されている間ずっと TRUE（移動などに使う）
if (input.GetKeyboard().IsKeyDown(VK_LEFT))
{
    playerX -= speed * deltaTime;
}
```

> **CheckHitKey vs IsKeyTriggered の比較**
>
> | 状況 | CheckHitKey (Compat) | IsKeyTriggered (Native) |
> |------|---------------------|------------------------|
> | キーを押した瞬間 | TRUE | TRUE |
> | キーを押し続けている | TRUE | FALSE |
> | キーを離した | FALSE | FALSE |
>
> ジャンプやショットの発射は `IsKeyTriggered` が適切です。
> 移動のように押し続けたい場合は `CheckHitKey` や `IsKeyDown` を使います。

## サウンド

### 効果音（SE）

```cpp
// 音声ファイルをメモリに読み込む
int seShotHandle = LoadSoundMem("Assets/shot.wav");

// 再生
PlaySoundMem(
    seShotHandle,       // サウンドハンドル
    GX_PLAYTYPE_BACK    // 再生方式:
                        //   GX_PLAYTYPE_BACK   = バックグラウンド再生（処理を止めずに再生）
                        //   GX_PLAYTYPE_NORMAL  = 再生完了まで処理を停止（通常使わない）
);

// 音量変更 (0=無音, 255=最大)
ChangeVolumeSoundMem(200, seShotHandle);

// 使い終わったら解放
DeleteSoundMem(seShotHandle);
```

> **なぜ `GX_PLAYTYPE_BACK` を使うのか？**
>
> `GX_PLAYTYPE_NORMAL` は音の再生が完了するまでプログラムが止まります。
> ゲーム中に使うと画面がフリーズしてしまうため、
> ほぼ全ての場合で `GX_PLAYTYPE_BACK`（バックグラウンド再生）を使います。

### BGM

```cpp
// ファイルパスを指定してストリーミング再生
PlayMusic(
    "Assets/bgm.wav",      // 音声ファイルパス
    GX_PLAYTYPE_LOOP        // GX_PLAYTYPE_LOOP: ループ再生（曲が終わると最初から繰り返す）
);

// 再生中かチェック (1=再生中, 0=停止中)
if (CheckMusic()) { /* BGM 再生中 */ }

// 停止
StopMusic();
```

> **対応音声フォーマット**: 現在は **WAV 形式のみ** 対応しています。
> MP3 や OGG を使いたい場合は、事前に WAV に変換してください。

## 衝突判定

### AABB 衝突判定（矩形 vs 矩形）

AABB (Axis-Aligned Bounding Box) は、座標軸に平行な長方形による当たり判定です。
計算が軽量なため、多くのゲームで基本的な当たり判定に使われます。

```cpp
#include "Math/Collision/Collision2D.h"
using namespace GX;

// 矩形の定義: AABB2D(左上座標, 右下座標)
AABB2D playerRect(
    {playerX, playerY},                     // 左上 (min)
    {playerX + 32, playerY + 32}            // 右下 (max) — 32x32 ピクセルの矩形
);
AABB2D enemyRect(
    {enemyX, enemyY},                       // 左上
    {enemyX + 32, enemyY + 32}              // 右下
);

// 2つの矩形が重なっているか判定
if (Collision2D::TestAABBvsAABB(playerRect, enemyRect))
{
    // 衝突している！ — ダメージ処理など
}
```

### 円 vs 円

円の当たり判定は、キャラクターや弾のように「丸い」ものに適しています。

```cpp
// 円の定義: Circle(中心座標, 半径)
Circle bullet(
    {bulletX, bulletY},     // 弾の中心座標
    4.0f                    // 弾の半径 (4ピクセル)
);
Circle enemy(
    {enemyX + 16, enemyY + 16},    // 敵の中心（32x32画像の中心 = +16, +16）
    16.0f                           // 敵の当たり判定半径 (16ピクセル)
);

if (Collision2D::TestCirclevsCircle(bullet, enemy))
{
    // ヒット！ — 敵を倒す処理
}

// 衝突の詳細情報が必要な場合（跳ね返りなど）
auto hit = Collision2D::IntersectCirclevsCircle(bullet, enemy);
if (hit)
{
    // hit.point  — 衝突した位置 (Vector2)
    // hit.normal — 衝突面の法線（跳ね返り方向の計算に使う）
    // hit.depth  — 重なり深さ（めり込み補正に使う）
}
```

## サンプル：シューティングゲーム

`Samples/Shooting2D/` に完全なシューティングゲームのサンプルがあります。

主な構成要素:
- プレイヤー移動（キーボード入力）
- 弾の発射と移動
- 敵の出現と衝突判定
- スコア表示
- 効果音

```bash
# ビルドと実行
cmake --build build --config Debug --target Shooting2D
```

## フレームレート制御

フレームレートに依存しない移動を実現するため、**deltaTime**（フレーム間の経過時間）を使います。

```cpp
// 前フレームの時刻を記録
int prevTime = GetNowCount();   // 現在時刻をミリ秒で取得

while (ProcessMessage() == 0)
{
    int nowTime = GetNowCount();
    float deltaTime = (nowTime - prevTime) / 1000.0f;
    // ミリ秒を秒に変換 (例: 16ms → 0.016秒)
    prevTime = nowTime;

    // deltaTime を速度に掛けて移動量を計算
    // speed=300 なら「1秒間に300ピクセル移動」という意味になる
    playerX += speed * deltaTime;
    // 60FPS: 300 × 0.016 = 4.8px/フレーム
    // 30FPS: 300 × 0.033 = 9.9px/フレーム → どちらも1秒で300px移動
}
```

> **なぜ deltaTime を使うのか？**
>
> deltaTime を使わないと、移動速度が FPS（フレームレート）に依存します:
> - 60FPS のPCでは `speed × 60回/秒` で高速移動
> - 30FPS のPCでは `speed × 30回/秒` で低速移動
>
> deltaTime を掛けることで「1秒あたりの移動量」を指定でき、
> どのPCでも同じ速度で動作します。

## よくある問題

### キー入力が効かない

- `ProcessMessage()` を毎フレーム呼んでいるか確認（ウィンドウメッセージ処理に必要）
- ネイティブ API の場合、`input.Update()` を毎フレーム呼んでいるか確認
- ウィンドウにフォーカスが当たっているか確認（別ウィンドウがアクティブだと入力を受け取りません）

### 音が鳴らない

- ファイルパスが正しいか確認
- WAV 形式か確認（MP3, OGG は未対応）
- `LoadSoundMem` の戻り値が -1 でないか確認（-1 = 読み込み失敗）

### 衝突判定がうまくいかない

- AABB の座標が正しいか確認（左上 < 右下 になっているか）
- 円の中心座標がスプライトの中心と合っているか確認
- 画像サイズと当たり判定サイズが一致しているか確認

### 動きがカクカクする / PC ごとに速度が違う

- deltaTime を使っているか確認（上記「フレームレート制御」参照）
- `deltaTime` に異常に大きな値が入っていないか確認（初回フレームは 0 に clamp するとよい）

## 次のステップ

- [04_Rendering3D.md](04_Rendering3D.md) — 3D描画とPBRレンダリング
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認
