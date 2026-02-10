# GXLib Phase 1: 2D描画エンジン — 作業レポート

## 概要

Phase 0（Hello Triangle）で構築したDirectX 12基盤の上に、
DXLib互換の**2D描画エンジン**を実装した。
SpriteBatch（テクスチャ付きスプライト描画）とPrimitiveBatch（基本図形描画）を中心に、
テクスチャ管理・ソフトウェアイメージ・カメラ・アニメーション・レンダーターゲットを完成させた。

---

## 技術仕様（Phase 0からの追加分）

| 項目 | 内容 |
|------|------|
| テクスチャ読み込み | stb_image.h（バンドル済み、ヘッダーオンリー） |
| 対応画像形式 | PNG, JPG, BMP, TGA 等（stb_image準拠） |
| スプライトバッチ容量 | 最大 4,096 スプライト/バッチ |
| プリミティブバッチ容量 | 最大 4,096×3 三角形頂点 + 4,096×2 線分頂点 |
| テクスチャ管理上限 | 256 テクスチャ |
| ブレンドモード | Alpha / Add / Sub / Mul / Screen / None（6種） |
| 座標系 | 左上原点、Y軸下向き（DXLib互換） |

---

## 作成ファイル一覧（Phase 1で追加: 24ファイル）

### Graphics/Resource（12ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `DynamicBuffer` | DynamicBuffer.h/cpp | フレーム書き換え用UPLOADバッファ（ダブルバッファリング対応） |
| `Texture` | Texture.h/cpp | GPU テクスチャ（stb_image読み込み → UPLOADステージング → DEFAULTヒープ → SRV作成） |
| `TextureManager` | TextureManager.h/cpp | ハンドルベースのテクスチャ管理（パスキャッシュ、フリーリスト、UV矩形） |
| `SoftImage` | SoftImage.h/cpp | CPUメモリ上のピクセル操作（DXLib LoadSoftImage/DrawPixelSoftImage互換） |
| `RenderTarget` | RenderTarget.h/cpp | オフスクリーンレンダーターゲット（RTV + SRV、DXLib MakeScreen互換） |
| `DepthBuffer` | DepthBuffer.h/cpp | 深度バッファ基盤（Phase 1では未使用、将来の3D対応用） |

### Graphics/Rendering（10ファイル）

| クラス | ファイル | 役割 |
|--------|----------|------|
| `SpriteBatch` | SpriteBatch.h/cpp | 2Dスプライトバッチ描画（DrawGraph系5種 + ブレンド6種 + 描画色） |
| `PrimitiveBatch` | PrimitiveBatch.h/cpp | 基本図形描画（線分、矩形、円、三角形、楕円、1ピクセル） |
| `Camera2D` | Camera2D.h/cpp | 2Dカメラ（位置・ズーム・回転 → ビュープロジェクション行列） |
| `SpriteSheet` | SpriteSheet.h/cpp | 画像分割読み込み（DXLib LoadDivGraph互換） |
| `Animation2D` | Animation2D.h/cpp | フレームベースアニメーション（ループ、速度、タイマー管理） |

### Shaders（2ファイル）

| ファイル | 役割 |
|----------|------|
| `Shaders/Sprite.hlsl` | スプライト描画用（正射影変換 + テクスチャサンプリング × 頂点カラー） |
| `Shaders/Primitive.hlsl` | プリミティブ描画用（正射影変換 + 頂点カラーのみ） |

### ThirdParty（1ファイル）

| ファイル | 役割 |
|----------|------|
| `GXLib/ThirdParty/stb_image.h` | ヘッダーオンリー画像ローダー（STB_IMAGE_IMPLEMENTATION はTexture.cppで定義） |

### Sandbox（更新: 1ファイル）

| ファイル | 役割 |
|----------|------|
| `Sandbox/main.cpp` | Phase 1テストアプリ（SpriteBatch + PrimitiveBatch + SoftImageによる動的テクスチャ） |

---

## ディレクトリ構成（Phase 1完了時点）

```
GXLib/
├── CMakeLists.txt
├── Phase0_Summary.md
├── Phase1_Summary.md
├── GXLib/
│   ├── CMakeLists.txt
│   ├── pch.h
│   ├── pch.cpp
│   ├── Core/
│   │   ├── Application.h / .cpp
│   │   ├── Logger.h / .cpp
│   │   ├── Timer.h / .cpp
│   │   └── Window.h / .cpp
│   ├── Graphics/
│   │   ├── Device/
│   │   │   ├── GraphicsDevice.h / .cpp
│   │   │   ├── Fence.h / .cpp
│   │   │   ├── CommandQueue.h / .cpp
│   │   │   ├── CommandList.h / .cpp
│   │   │   ├── DescriptorHeap.h / .cpp
│   │   │   └── SwapChain.h / .cpp
│   │   ├── Pipeline/
│   │   │   ├── Shader.h / .cpp
│   │   │   ├── RootSignature.h / .cpp
│   │   │   └── PipelineState.h / .cpp
│   │   ├── Resource/                    ← Phase 1で拡張
│   │   │   ├── Buffer.h / .cpp          (Phase 0)
│   │   │   ├── DynamicBuffer.h / .cpp   (NEW)
│   │   │   ├── Texture.h / .cpp         (NEW)
│   │   │   ├── TextureManager.h / .cpp  (NEW)
│   │   │   ├── SoftImage.h / .cpp       (NEW)
│   │   │   ├── RenderTarget.h / .cpp    (NEW)
│   │   │   └── DepthBuffer.h / .cpp     (NEW)
│   │   └── Rendering/                   ← Phase 1で新設
│   │       ├── SpriteBatch.h / .cpp     (NEW)
│   │       ├── PrimitiveBatch.h / .cpp  (NEW)
│   │       ├── Camera2D.h / .cpp        (NEW)
│   │       ├── SpriteSheet.h / .cpp     (NEW)
│   │       └── Animation2D.h / .cpp     (NEW)
│   └── ThirdParty/
│       └── stb_image.h                  (NEW)
├── Sandbox/
│   ├── CMakeLists.txt
│   └── main.cpp                         (UPDATED)
└── Shaders/
    ├── HelloTriangle.hlsl               (Phase 0)
    ├── Sprite.hlsl                      (NEW)
    └── Primitive.hlsl                   (NEW)
```

---

## 描画パイプライン

### スプライト描画フロー

```
1. SpriteBatch.Begin(cmdList, frameIndex)
   → 定数バッファに正射影行列を書き込み
   → ディスクリプタヒープをバインド

2. DrawGraph / DrawRotaGraph / DrawExtendGraph 等
   → 4頂点（Position, UV, Color）をバッチ内バッファに蓄積
   → テクスチャハンドルやブレンドモードが変わるとFlush()

3. SpriteBatch.End()
   → 残りの頂点をFlush()
   → Flush():
      RootSignature設定 → PSO設定（ブレンドモード別）
      → 頂点バッファ（DynamicBuffer）書き込み
      → インデックスバッファ（事前生成、0-1-2, 2-3-0 パターン）
      → SRVテーブルにテクスチャをバインド
      → DrawIndexedInstanced(spriteCount × 6, 1)
```

### プリミティブ描画フロー

```
1. PrimitiveBatch.Begin(cmdList, frameIndex)
   → 定数バッファに正射影行列を書き込み

2. DrawBox / DrawCircle / DrawTriangle 等
   → 塗りつぶし: 三角形頂点バッファに蓄積
   → アウトライン: 線分頂点バッファに蓄積
   → 円/楕円: 扇形に三角形分割（segments引数で精度指定）

3. PrimitiveBatch.End()
   → FlushTriangles():
      三角形用PSO + 頂点バッファ → Draw(triVertexCount)
   → FlushLines():
      線分用PSO（TOPOLOGY_LINELIST）+ 頂点バッファ → Draw(lineVertexCount)
```

### シェーダー座標変換（共通）

```hlsl
// 正射影行列: スクリーン座標(px) → クリップ座標(-1〜+1)
// DirectXMathはrow-major、HLSL cbufferはcolumn-majorなので
// mul(matrix, vector)形式で暗黙転置により正しく変換される
output.pos = mul(projectionMatrix, float4(input.pos, 0.0f, 1.0f));
```

---

## 主要クラスのAPI

### SpriteBatch（DXLib DrawGraph系互換）

```cpp
// 初期化
bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue,
                uint32_t screenWidth, uint32_t screenHeight);

// 描画サイクル
void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);
void DrawGraph(float x, float y, int handle, bool transFlag = true);
void DrawRotaGraph(float cx, float cy, float extRate, float angle,
                   int handle, bool transFlag = true);
void DrawExtendGraph(float x1, float y1, float x2, float y2,
                     int handle, bool transFlag = true);
void DrawModiGraph(float x1, float y1, float x2, float y2,
                   float x3, float y3, float x4, float y4,
                   int handle, bool transFlag = true);
void DrawRectGraph(float x, float y, int srcX, int srcY, int w, int h,
                   int handle, bool transFlag = true);
void End();

// 描画設定
void SetBlendMode(BlendMode mode);     // Alpha, Add, Sub, Mul, Screen, None
void SetDrawColor(float r, float g, float b, float a = 1.0f);
void SetScreenSize(uint32_t width, uint32_t height);
void SetProjectionMatrix(const XMMATRIX& matrix);  // Camera2D用
void ResetProjectionMatrix();

// テクスチャ管理
TextureManager& GetTextureManager();
```

### PrimitiveBatch（DXLib DrawLine/DrawBox系互換）

```cpp
// 初期化
bool Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight);

// 描画サイクル
void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex);
void DrawLine(float x1, float y1, float x2, float y2, uint32_t color, int thickness = 1);
void DrawBox(float x1, float y1, float x2, float y2, uint32_t color, bool fillFlag);
void DrawCircle(float cx, float cy, float r, uint32_t color, bool fillFlag, int segments = 32);
void DrawTriangle(float x1, float y1, float x2, float y2,
                  float x3, float y3, uint32_t color, bool fillFlag);
void DrawOval(float cx, float cy, float rx, float ry, uint32_t color,
              bool fillFlag, int segments = 32);
void DrawPixel(float x, float y, uint32_t color);
void End();

// 設定
void SetScreenSize(uint32_t width, uint32_t height);
void SetProjectionMatrix(const XMMATRIX& matrix);
void ResetProjectionMatrix();
```

### TextureManager（DXLib LoadGraph互換）

```cpp
bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
int  LoadTexture(const std::wstring& filePath);            // → ハンドル（キャッシュ付き）
int  CreateTextureFromMemory(const void* pixels, uint32_t w, uint32_t h);
Texture* GetTexture(int handle);
void ReleaseTexture(int handle);
int  CreateRegionHandles(int baseHandle, int allNum,       // スプライトシート分割用
                         int xNum, int yNum, int xSize, int ySize);
```

### SoftImage（DXLib MakeSoftImage/DrawPixelSoftImage互換）

```cpp
bool Create(uint32_t width, uint32_t height);
bool LoadFromFile(const std::wstring& filePath);
uint32_t GetPixel(int x, int y) const;       // 0xAARRGGBB
void DrawPixel(int x, int y, uint32_t color); // 0xAARRGGBB
void Clear(uint32_t color = 0x00000000);
int  CreateTexture(TextureManager& textureManager);  // GPUにアップロード
```

### Camera2D

```cpp
void SetPosition(float x, float y);
void SetZoom(float scale);         // 1.0 = 等倍
void SetRotation(float angle);     // ラジアン
XMMATRIX GetViewProjectionMatrix(uint32_t screenWidth, uint32_t screenHeight) const;
```

### SpriteSheet（DXLib LoadDivGraph互換）

```cpp
static bool LoadDivGraph(TextureManager& textureManager,
                         const std::wstring& filePath,
                         int allNum, int xNum, int yNum,
                         int xSize, int ySize,
                         int* handleArray);
```

### Animation2D

```cpp
void AddFrames(const int* handles, int count, float frameDuration);
void Update(float deltaTime);
int  GetCurrentHandle() const;
void SetLoop(bool loop);
void SetSpeed(float speed);
void Reset();
bool IsFinished() const;
```

---

## 設計パターン

| パターン | 適用箇所 | 説明 |
|----------|----------|------|
| バッチレンダリング | SpriteBatch, PrimitiveBatch | 頂点データをCPU側に蓄積し、End()で一括描画。DrawCall数を最小化 |
| ダブルバッファリング | DynamicBuffer | GPUが前フレームのバッファを読み中に、CPUが次フレームに書き込み |
| ハンドルベース管理 | TextureManager | `int`ハンドルで間接参照。DXLibと同じAPI体験 |
| フリーリスト | TextureManager, DescriptorHeap | 解放されたハンドル/インデックスを再利用 |
| パスキャッシュ | TextureManager | 同一パスの二重読み込みを防止（`unordered_map`） |
| UV矩形ハンドル | TextureRegion | 1枚のテクスチャに対し複数のUV矩形ハンドルを発行（スプライトシート対応） |
| PSO配列 | SpriteBatch | ブレンドモード6種をPSO配列で保持し、モード変更時にPSO切替 |
| 三角形/線分分離 | PrimitiveBatch | TOPOLOGY_TRIANGLELIST用とTOPOLOGY_LINELIST用で別バッファ・別PSO |

---

## GPUリソースのアップロードフロー

```
=== テクスチャアップロード ===
stb_image (CPU) → ピクセルデータ
                → UPLOADヒープにステージングバッファ作成
                → DEFAULTヒープ（GPU専用）にテクスチャリソース作成
                → コマンドリスト: CopyTextureRegion(staging → texture)
                → フェンス待ち: GPU側コピー完了を保証
                → SRV作成 → シェーダーからアクセス可能に

=== 動的頂点データ ===
CPU (DrawGraph等) → DynamicBuffer[frameIndex].Map() で直接書き込み
                  → Unmap()
                  → IASetVertexBuffers で GPU にバインド
                  ※ UPLOADヒープなのでコピー不要（CPU/GPU共有メモリ）
```

---

## テストアプリ（Sandbox/main.cpp）

Phase 1の全機能を検証するデモ：

### プリミティブ描画テスト
| 描画内容 | API | 色/設定 |
|----------|-----|---------|
| 塗りつぶし矩形 | `DrawBox(50,50, 200,150, ..., true)` | 赤 (0xFFFF4444) |
| アウトライン矩形 | `DrawBox(220,50, 370,150, ..., false)` | 緑 (0xFF44FF44) |
| 塗りつぶし円 | `DrawCircle(480,100, 50, ..., true)` | 青 (0xFF4444FF) |
| アウトライン円 | `DrawCircle(600,100, 50, ..., false)` | 黄 (0xFFFFFF44) |
| 塗りつぶし三角形 | `DrawTriangle(700,50, 750,150, 650,150, ..., true)` | マゼンタ (0xFFFF44FF) |
| 水平線 | `DrawLine(50,180, 750,180, ...)` | 白 (0xFFFFFFFF) |
| 塗りつぶし楕円 | `DrawOval(400,250, 100,50, ..., true)` | シアン (0xFF44FFFF) |
| アニメーション線 | `DrawLine(400,300, sinf(t)*300+400, 350, ...)` | オレンジ (0xFFFF8800) |

### スプライト描画テスト
| 描画内容 | API | 備考 |
|----------|-----|------|
| 通常描画 | `DrawGraph(50, 400, handle)` | 64x64チェッカーパターン（SoftImageで動的生成） |
| 拡大描画 | `DrawExtendGraph(150,400, 310,528, handle)` | 160x128に引き伸ばし |
| 回転描画 | `DrawRotaGraph(450,464, 2.0, totalTime, handle)` | 2倍拡大＋時間回転 |
| 加算ブレンド | `SetBlendMode(Add) → DrawGraph(600,400, handle)` | 発光効果 |
| 色付き描画 | `SetDrawColor(1,0.5,0.5,0.8) → DrawGraph(700,400, handle)` | 赤みがかった半透明 |

### テストテクスチャ生成
```
SoftImageで64x64のチェッカーパターンを動的生成：
- 8x8ピクセル単位の格子
- 白 (0xFFFFFFFF) と 青 (0xFF4488CC) の交互
- CreateTexture()でGPUにアップロード
```

---

## 修正した不具合

### プロジェクション行列の二重転置問題

**症状**: 矩形・円・スプライトが一切表示されず、線分だけが画面中央付近に極小サイズで斜めに表示される。

**原因**: `SpriteBatch.cpp`と`PrimitiveBatch.cpp`で定数バッファ書き込み前に`XMMatrixTranspose()`を呼んでいた。

| レイヤー | 格納形式 |
|---------|---------|
| DirectXMath (`XMMATRIX`) | row-major |
| HLSL `cbuffer`（デフォルト） | column-major |

Row-majorデータをcolumn-majorとして読み込む時点で**暗黙的に転置**される。
HLSL側で`mul(projectionMatrix, float4(pos, 0, 1))`を使う場合、この暗黙転置だけで正しい変換結果になる。
C++側で追加の`XMMatrixTranspose()`を行うと**二重転置**となり、`w`成分が座標依存の値になる。
perspective divideで全座標が極小の一点に潰れ、描画が消失していた。

**計算例**（頂点 (400, 300)、1280x720画面）:

| ケース | clip座標 | 結果 |
|-------|----------|------|
| 正しい（転置なし） | (-0.375, 0.167, 0, **1.0**) | 画面内に正常表示 |
| 二重転置 | (0.625, -0.833, 0, **-99**) | perspective divideで消失 |

**修正内容**（2ファイル、各1行）:

`PrimitiveBatch.cpp`:
```cpp
// 変更前
XMMATRIX transposed = XMMatrixTranspose(proj);
memcpy(cbData, &transposed, sizeof(XMMATRIX));

// 変更後
memcpy(cbData, &proj, sizeof(XMMATRIX));
```

`SpriteBatch.cpp`:
```cpp
// 変更前
XMMATRIX transposed = XMMatrixTranspose(proj);
memcpy(cbData, &transposed, sizeof(XMMATRIX));

// 変更後
memcpy(cbData, &proj, sizeof(XMMATRIX));
```

**教訓**: DirectXMath + HLSL column-major cbuffer + `mul(matrix, vector)`の組み合わせでは、C++側で`XMMatrixTranspose()`を呼ぶ必要はない。暗黙転置が自動的に正しい変換を行う。

---

## ビルド方法

```bash
# CMake構成
cmake -B build -G "Visual Studio 17 2022" -A x64

# ビルド（Debug）
cmake --build build --config Debug

# 実行
build\Sandbox\Debug\Sandbox.exe
```

---

## 実行結果

- ウィンドウが画面中央に1280x720で表示される
- ダークブルーの背景に以下が描画される：
  - 画面上部: 赤い塗りつぶし矩形、緑のアウトライン矩形、青い塗りつぶし円、黄色のアウトライン円、マゼンタの三角形
  - 中段: 白い水平線、シアンの楕円、オレンジのアニメーション線（sin波で往復）
  - 画面下部: チェッカーパターンスプライト5種（通常、拡大、回転アニメ、加算ブレンド、色付き半透明）
- タイトルバーにFPSが1秒ごとに更新表示される
- ESCキーで終了
- ウィンドウリサイズ時にSwapChain + SpriteBatch + PrimitiveBatchが自動リサイズ

---

## Phase 1 完了チェックリスト

- [x] DynamicBuffer（ダブルバッファリング対応UPLOADバッファ）
- [x] Texture（stb_imageによるファイル読み込み + GPU転送 + SRV作成）
- [x] TextureManager（intハンドル管理、パスキャッシュ、フリーリスト）
- [x] SoftImage（CPUピクセル操作 → GPU転送）
- [x] RenderTarget（オフスクリーン描画）
- [x] DepthBuffer（基盤のみ、将来用）
- [x] SpriteBatch（DrawGraph / DrawRotaGraph / DrawExtendGraph / DrawModiGraph / DrawRectGraph）
- [x] PrimitiveBatch（DrawLine / DrawBox / DrawCircle / DrawTriangle / DrawOval / DrawPixel）
- [x] ブレンドモード6種（Alpha / Add / Sub / Mul / Screen / None）
- [x] Camera2D（位置・ズーム・回転 → ビュープロジェクション行列）
- [x] SpriteSheet（LoadDivGraph互換の画像分割読み込み）
- [x] Animation2D（フレームベースアニメーション）
- [x] Sprite.hlsl / Primitive.hlsl（正射影変換シェーダー）
- [x] テストアプリ（プリミティブ + スプライト + ブレンドモード + 動的テクスチャ）
- [x] 二重転置バグ修正（XMMatrixTranspose削除）
- [x] 全クラスに初学者向け日本語コメント
- [x] Debugビルド成功（エラー・警告なし）
