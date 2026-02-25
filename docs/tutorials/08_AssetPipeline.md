# 08 - アセットパイプライン (gxconv / gxpak / VFS)

gxconv でモデルを変換し、gxpak でバンドル化し、VFS でランタイム読み込みする
一連のワークフローを解説します。

## このチュートリアルで学ぶこと

- GXMD / GXAN バイナリ形式の概要
- gxconv によるモデル変換手順
- gxpak によるアセットバンドル作成
- VFS (仮想ファイルシステム) と PakFileProvider
- bone_matcher によるボーンマッチング（アニメーションリターゲット）

## 前提知識

- [04_Rendering3D.md](04_Rendering3D.md) の内容（3D モデルの読み込みと描画）
- コマンドプロンプトの基本操作

## なぜ独自フォーマットが必要か

glTF や FBX は汎用的で便利ですが、ゲームのランタイムには無駄が多い形式です。

| 項目 | glTF / FBX | GXMD / GXAN |
|------|-----------|-------------|
| 解析速度 | JSON/XML パース + バイナリ変換が必要 | ヘッダ + 固定オフセット読み込みで即座にロード |
| 頂点形式 | 各ツールで異なる | GXLib の頂点構造体とバイナリ互換 |
| マテリアル | PBR パラメータの変換が必要 | ShaderModelParams (256B) をそのままGPUに送れる |
| ファイルサイズ | テキスト JSON を含む | バイナリのみ。GXPAK で LZ4 圧縮も可能 |

## GXMD 形式 (.gxmd)

GXMD はメッシュ・マテリアル・スケルトン・アニメーションを単一バイナリにまとめた
3D モデル形式です。

### ファイル構造

```
[FileHeader 128B]           ← マジック、バージョン、各チャンクへのオフセット
[StringTable]               ← メッシュ名、マテリアル名、ボーン名等の UTF-8 文字列
[MeshChunk x meshCount]     ← メッシュごとの頂点数・インデックス数・AABB
[MaterialChunk x matCount]  ← マテリアルごとの ShaderModel + パラメータ 256B
[VertexData]                ← 頂点データ (Standard 48B or Skinned 80B)
[IndexData]                 ← インデックスデータ (16bit or 32bit)
[BoneData]                  ← ボーン階層 + 逆バインド行列
[AnimationData]             ← キーフレームアニメーション
```

### 頂点形式

| 型 | サイズ | 内容 | GXLib 対応 |
|----|--------|------|-----------|
| VertexStandard | 48B | position + normal + uv + tangent | Vertex3D_PBR |
| VertexSkinned | 80B | Standard + joints(4) + weights(4) | Vertex3D_Skinned |

> 頂点構造体は GXLib のランタイム頂点型とバイナリ互換です。
> memcpy でそのまま GPU バッファにコピーできます。

## GXAN 形式 (.gxan)

GXAN はスタンドアロンのアニメーション形式です。
スケルトンに依存せず、**ボーン名ベース**でチャネルを定義するため、
異なるモデル間でアニメーションを共有できます。

```
[GxanHeader 64B]                ← マジック、チャネル数、再生時間
[StringTable]                   ← ボーン名の文字列テーブル
[GxanChannelDesc x channelCount] ← 各チャネルのボーン名・ターゲット・キー数
[KeyData]                       ← VectorKey (T/S用) / QuatKey (R用)
```

## gxconv: モデル変換ツール

gxconv は OBJ / FBX / glTF を GXMD / GXAN 形式に変換する CLI ツールです。

### 基本的な使い方

```bash
# glTF → GXMD 変換（出力ファイル名は自動で .gxmd になる）
gxconv character.gltf character.gxmd

# FBX → GXMD 変換
gxconv character.fbx character.gxmd

# OBJ → GXMD 変換
gxconv scene.obj scene.gxmd
```

### オプション

```bash
# ファイル情報の表示（変換せずに内容を確認）
gxconv character.gltf --info

# シェーダーモデルを指定（デフォルトは standard = PBR）
gxconv character.gltf character.gxmd --shader-model toon

# Toon アウトラインの幅を指定
gxconv character.gltf character.gxmd --shader-model toon --toon-outline 0.002

# アニメーションを除外（メッシュのみ）
gxconv character.fbx character.gxmd --no-anim

# アニメーションのみを GXAN として出力
gxconv character.fbx walk.gxan --anim-only

# 16bit インデックスを強制（頂点数 65535 以下の場合にファイルサイズ削減）
gxconv scene.obj scene.gxmd --index16
```

### 対応シェーダーモデル

| 名前 | 説明 |
|------|------|
| standard | PBR（デフォルト） |
| unlit | ライティングなし |
| toon | トゥーンシェーディング (UTS2) |
| phong | Phong シェーディング |
| subsurface | サブサーフェス散乱 |
| clearcoat | クリアコート |

### 変換例: Blender からの完全な手順

```bash
# 1. Blender から glTF でエクスポート
#    Blender > File > Export > glTF 2.0 (.glb/.gltf)
#    設定: Format=glTF Binary (.glb), Mesh + Armature + Animation にチェック

# 2. GXMD に変換（メッシュ + スケルトン + アニメーション）
gxconv character.glb character.gxmd

# 3. 追加アニメーションを GXAN として別途出力
gxconv run_anim.glb run.gxan --anim-only

# 4. 変換結果を確認
gxconv character.gxmd --info
```

## gxpak: アセットバンドルツール

gxpak は複数のアセットを単一の .gxpak ファイルにまとめるツールです。
LZ4 圧縮に対応しており、配布サイズの削減と読み込み高速化を実現します。

### パック（バンドル作成）

```bash
# ディレクトリ内の全ファイルをバンドル化
gxpak pack -o game_assets.gxpak -d Assets/

# LZ4 圧縮を有効にしてバンドル
gxpak pack -o game_assets.gxpak -d Assets/ --compress
```

ディレクトリ構造の例:

```
Assets/
  models/
    character.gxmd
    enemy.gxmd
  animations/
    walk.gxan
    run.gxan
  textures/
    character_albedo.png
    character_normal.png
```

### 一覧表示

```bash
gxpak list -i game_assets.gxpak
```

出力例:

```
GXPAK: game_assets.gxpak
  Version: 1, Entries: 6

  [0] Model    models/character.gxmd  (124800 -> 89600 bytes, 71.8%)
  [1] Model    models/enemy.gxmd  (98304 bytes)
  [2] Anim     animations/walk.gxan  (8192 -> 5120 bytes, 62.5%)
  [3] Anim     animations/run.gxan  (6144 -> 4096 bytes, 66.7%)
  [4] Tex      textures/character_albedo.png  (262144 bytes)
  [5] Tex      textures/character_normal.png  (131072 bytes)
```

### 展開（アンパック）

```bash
gxpak unpack -i game_assets.gxpak -d output/
```

## VFS と PakFileProvider

GXLib の VFS（仮想ファイルシステム）は、ファイルの読み込み元を抽象化する仕組みです。
PakFileProvider を登録すると、GXPAK バンドル内のファイルを通常のファイルパスで
読み込めるようになります。

### VFS の仕組み

```
アプリケーション
    |
    v
FileSystem (VFS)
    |
    +-- PhysicalFileProvider (優先度: 0)   ← ディスク上のファイル
    +-- PakFileProvider      (優先度: 100) ← .gxpak 内のファイル
```

PakFileProvider は優先度 100 で登録されるため、
同じパスのファイルがディスクとバンドルの両方にある場合、
バンドル側が優先されます。

### 使い方

```cpp
#include "IO/FileSystem.h"
#include "IO/PakFileProvider.h"

// FileSystem を初期化
GX::FileSystem fs;

// GXPAK をマウント
auto pakProvider = std::make_shared<GX::PakFileProvider>();
if (pakProvider->Open("game_assets.gxpak"))
{
    fs.Mount(pakProvider);  // VFS に登録
}

// 通常のファイルパスで読み込み（VFS が自動的にバンドル内を検索）
auto data = fs.ReadFile("models/character.gxmd");
if (data.IsValid())
{
    // data.Data() — バイトデータへのポインタ
    // data.Size() — バイト数
    // LZ4 圧縮エントリは自動的に伸長される
}

// ファイルの存在確認
if (fs.Exists("textures/character_albedo.png"))
{
    // バンドル内にファイルが存在する
}
```

### gxloader でのメモリ読み込み

gxloader にはメモリバッファから直接ロードする関数があり、
VFS と組み合わせて使えます。

```cpp
#include <model_loader.h>
#include <anim_loader.h>

// VFS からバイトデータを取得
auto modelData = fs.ReadFile("models/character.gxmd");

// メモリから直接 GXMD をロード（ファイル I/O なし）
auto loadedModel = gxloader::LoadGxmdFromMemory(
    modelData.Data(), modelData.Size()
);

// 同様にアニメーションもメモリからロード
auto animData = fs.ReadFile("animations/walk.gxan");
auto loadedAnim = gxloader::LoadGxanFromMemory(
    animData.Data(), animData.Size()
);
```

## ボーンマッチング（リターゲット）

異なるツールで作られたモデルとアニメーションでは、
ボーン名の命名規則が異なることがあります。
bone_matcher はこの問題を 4 段階のフォールバック戦略で解決します。

### フォールバック戦略

| ステップ | 方法 | 例 |
|---------|------|-----|
| 1 | 完全一致 | `Hips` == `Hips` |
| 2 | 大文字小文字無視 | `hips` == `Hips` |
| 3 | プレフィックス除去 + 大文字小文字無視 | `mixamorig:Hips` -> `hips` == `Hips` |
| 4 | 数値サフィックス除去 + ステップ3 | `mixamorig:Hips.001` -> `hips` == `Hips` |

### 使い方

```cpp
#include <bone_matcher.h>
#include <anim_loader.h>
#include <model_loader.h>

// モデルのスケルトンからボーン名一覧を取得
std::vector<std::string> skeletonNames;
for (const auto& joint : loadedModel->joints)
{
    skeletonNames.push_back(joint.name);
}

// GXAN のチャネルをモデルのボーンに対応付ける
for (const auto& channel : loadedAnim->channels)
{
    int boneIndex = gxloader::MatchBoneName(
        channel.boneName,   // アニメーション側のボーン名
        skeletonNames       // モデル側のボーン名一覧
    );

    if (boneIndex >= 0)
    {
        // マッチ成功: channel のキーフレームを boneIndex に適用
    }
    else
    {
        // マッチ失敗: このボーンのアニメーションはスキップ
    }
}
```

> **NormalizeBoneName** を使うと、ボーン名を正規化して比較に使えます。
>
> ```cpp
> std::string normalized = gxloader::NormalizeBoneName("mixamorig:LeftUpperArm.001");
> // → "leftupperarm"
> ```

### よくあるプレフィックスパターン

| ツール | プレフィックス例 |
|--------|-----------------|
| Mixamo | `mixamorig:` |
| Blender | `Armature\|` |
| 数値サフィックス | `.001`, `.002` |

bone_matcher はこれらを自動的に除去してマッチングを試みます。

## 実践ワークフロー

ゲーム開発における典型的なアセットパイプラインの流れです。

```
[Blender/Maya]
    |  glTF / FBX エクスポート
    v
[gxconv]
    |  .gxmd / .gxan に変換
    v
[Assets/ フォルダ]
    |  開発中はファイルを直接読み込み
    v
[gxpak pack]
    |  リリース時に .gxpak にバンドル
    v
[配布パッケージ]
    game.exe + game_assets.gxpak
```

開発中は Assets フォルダから直接読み込み、
リリースビルドでは GXPAK をマウントして配布します。

## よくある問題

### gxconv で「Unknown format」エラー

- 入力ファイルの拡張子が `.obj`, `.fbx`, `.gltf`, `.glb` のいずれかであることを確認
- ファイルが壊れていないことを確認（Blender で開き直してみる）

### GXPAK 内のテクスチャが読み込めない

- バンドル内のパスが正しいか `gxpak list` で確認
- VFS に PakFileProvider をマウントしているか確認
- テクスチャの読み込みが VFS 経由になっているか確認

### ボーンマッチングが失敗する

- `gxconv --info` でモデルのボーン名一覧を確認
- アニメーション側のボーン名を確認（Mixamo はプレフィックスが付く）
- 4 段階全てで不一致の場合は、ボーン名を手動でリネームする必要があります

### GXMD のファイルサイズが大きい

- `--index16` オプションで 16bit インデックスを使用（頂点数 65535 以下の場合）
- GXPAK の `--compress` で LZ4 圧縮を適用

## 次のステップ

- [07_3DScene.md](07_3DScene.md) -- Scene/Entity でシーンを構築する
- [04_Rendering3D.md](04_Rendering3D.md) -- PBR/シェーダーモデル/ポストエフェクト
- [06_GXEasy2DGame.md](06_GXEasy2DGame.md) -- GXEasy::App の使い方
