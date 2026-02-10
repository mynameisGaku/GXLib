# 03 - 2D Game

入力処理、サウンド、衝突判定を組み合わせて 2D ゲームを作る方法を解説します。

## 入力処理

### キーボード

```cpp
// 特定キーが押されているか
if (CheckHitKey(KEY_INPUT_SPACE))
{
    // スペースキー押下中
}

// 全キーの状態を一括取得
char keys[256];
GetHitKeyStateAll(keys);
if (keys[KEY_INPUT_LEFT])  playerX -= speed;
if (keys[KEY_INPUT_RIGHT]) playerX += speed;
if (keys[KEY_INPUT_UP])    playerY -= speed;
if (keys[KEY_INPUT_DOWN])  playerY += speed;
```

### マウス

```cpp
// マウス座標
int mx, my;
GetMousePoint(&mx, &my);

// マウスボタン（ビットフラグ）
int mouse = GetMouseInput();
if (mouse & MOUSE_INPUT_LEFT)  { /* 左クリック中 */ }
if (mouse & MOUSE_INPUT_RIGHT) { /* 右クリック中 */ }

// マウスホイール（回転量）
int wheel = GetMouseWheelRotVol();
```

### ゲームパッド

```cpp
int pad = GetJoypadInputState(GX_INPUT_PAD1);
if (pad & PAD_INPUT_UP)    playerY -= speed;
if (pad & PAD_INPUT_DOWN)  playerY += speed;
if (pad & PAD_INPUT_LEFT)  playerX -= speed;
if (pad & PAD_INPUT_RIGHT) playerX += speed;
if (pad & PAD_INPUT_A)     Fire();
```

### ネイティブ API（トリガー検出）

簡易 API の `CheckHitKey` は「押されているか」のみ。
「押された瞬間」を検出するにはネイティブ API を使用します。

```cpp
GX::InputManager input;
input.Initialize(window);

// フレーム更新
input.Update();

// トリガー（押された瞬間のみ TRUE）
if (input.GetKeyboard().IsKeyTriggered(VK_SPACE))
{
    // ジャンプ開始
}
```

## サウンド

### 効果音（SE）

```cpp
// 読み込み
int seShotHandle = LoadSoundMem("Assets/shot.wav");

// 再生（NORMAL=最初から再生、BACK=バックグラウンド再生）
PlaySoundMem(seShotHandle, GX_PLAYTYPE_BACK);

// 音量変更（0〜255）
ChangeVolumeSoundMem(200, seShotHandle);

// 解放
DeleteSoundMem(seShotHandle);
```

### BGM

```cpp
// ファイルパスを指定してストリーミング再生
PlayMusic("Assets/bgm.wav", GX_PLAYTYPE_LOOP);

// 再生中チェック
if (CheckMusic()) { /* 再生中 */ }

// 停止
StopMusic();
```

## 衝突判定

### 2D 衝突判定（ネイティブ API）

```cpp
#include "Math/Collision/Collision2D.h"
using namespace GX;

// 矩形 vs 矩形
AABB2D playerRect({playerX, playerY}, {playerX + 32, playerY + 32});
AABB2D enemyRect({enemyX, enemyY}, {enemyX + 32, enemyY + 32});
if (Collision2D::TestAABBvsAABB(playerRect, enemyRect))
{
    // 衝突！
}

// 円 vs 円
Circle bullet({bulletX, bulletY}, 4.0f);
Circle enemy({enemyX + 16, enemyY + 16}, 16.0f);
if (Collision2D::TestCirclevsCircle(bullet, enemy))
{
    // ヒット！
}

// 詳細な衝突情報
auto hit = Collision2D::IntersectCirclevsCircle(bullet, enemy);
if (hit)
{
    // hit.point  — 衝突点
    // hit.normal — 衝突法線
    // hit.depth  — 重なり深さ
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

```cpp
// GetNowCount() でフレーム時間を計測
int prevTime = GetNowCount();

while (ProcessMessage() == 0)
{
    int nowTime = GetNowCount();
    float deltaTime = (nowTime - prevTime) / 1000.0f;
    prevTime = nowTime;

    // deltaTime を使って速度を調整
    playerX += speed * deltaTime;
}
```

## 次のステップ

- [04_Rendering3D.md](04_Rendering3D.md) — 3D描画とPBRレンダリング
