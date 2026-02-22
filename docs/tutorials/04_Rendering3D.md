# 04 - 3D Rendering

PBR (Physically Based Rendering, 物理ベースレンダリング)、カメラ、ライティング、3D モデルの使い方を解説します。

## このチュートリアルで学ぶこと

- 簡易 API で 3D モデルを表示する方法
- カメラの設定（位置、注視点、視野角）
- ライティング（太陽光、点光源、スポットライト）
- PBR マテリアル（金属度・粗さによるリアルな質感表現）
- プリミティブメッシュの生成と描画
- glTF / GXMD モデルの読み込みとスケルタルアニメーション
- シェーダーモデル（PBR, Toon, Phong 等）の切り替え
- ポストエフェクト（画面全体の映像加工）
- DXR レイトレーシング反射 / RTGI

## 前提知識

- [02_Drawing2D.md](02_Drawing2D.md) の内容（描画の基本）
- 3D 空間の座標系の概念（X=右、Y=上、Z=奥）

## 簡易 API での 3D 描画

```cpp
// --- カメラ設定 ---
SetCameraPositionAndTarget(
    VGet(0.0f, 5.0f, -10.0f),   // カメラの位置 (X=0, Y=5(上), Z=-10(手前))
    VGet(0.0f, 0.0f, 0.0f)      // 注視点: カメラが向く先 (原点)
);
SetCameraNearFar(
    0.1f,       // ニアクリップ: これより近い物体は描画しない（0 にしないこと）
    1000.0f     // ファークリップ: これより遠い物体は描画しない
);

// --- モデル読み込みと設定 ---
int model = LoadModel("Assets/models/character.gltf");  // glTF形式のモデルを読み込み
SetModelPosition(model, VGet(0.0f, 0.0f, 0.0f));       // モデルの位置 (原点に配置)
SetModelScale(model, VGet(1.0f, 1.0f, 1.0f));          // スケール (等倍)
SetModelRotation(model, VGet(0.0f, 0.5f, 0.0f));       // 回転 (Y軸に0.5ラジアン≒28.6度)

// --- フレームループ内 ---
ClearDrawScreen();
DrawModel(model);   // モデル描画
ScreenFlip();

// --- 終了時 ---
DeleteModel(model);
```

> **glTF とは？**
>
> glTF (GL Transmission Format) は 3D モデルの標準フォーマットです。
> メッシュ、マテリアル、テクスチャ、アニメーションをひとつのファイルにまとめられます。
> Blender 等の 3D ソフトから直接エクスポートでき、GXLib はこの形式に対応しています。
> (DXLib で使われていた .x, .mv1 形式には対応していません)

## ネイティブ API

### カメラ

```cpp
GX::Camera3D camera;
camera.SetPosition(0.0f, 5.0f, -10.0f);    // カメラ位置 (ワールド座標)
camera.SetTarget(0.0f, 0.0f, 0.0f);        // 注視点（カメラが向く先）
camera.SetFov(GX::MathUtil::PI / 4.0f);    // FOV (Field of View, 視野角): 45度
                                            // 人間の視野は~120度だが、ゲームでは45~90度が一般的
                                            // 小さい値=望遠（ズームイン）, 大きい値=広角
camera.SetNearFar(0.1f, 1000.0f);          // 描画範囲: 0.1m〜1000m
camera.SetAspectRatio(1280.0f / 960.0f);   // アスペクト比: 画面の横幅÷高さ
camera.Update();                            // 行列を再計算（設定変更後に必ず呼ぶ）
```

> **なぜニアクリップを 0 にしてはいけないのか？**
>
> ニアクリップを 0 にすると、深度バッファ（奥行き判定）の精度が極端に低下し、
> 遠くの物体が前後関係を正しく表示できなくなります（Z-fighting という現象）。
> 通常は 0.1〜1.0 に設定してください。

### ライティング

GXLib は3種類のライトをサポートしています。

```cpp
// --- ディレクショナルライト（太陽光のような平行光源）---
GX::Light light;
light.type = GX::LightType::Directional;
light.direction = GX::Vector3(0.3f, -1.0f, 0.5f);     // 光の向き (斜め上から照射)
light.color = GX::Vector3(1.0f, 0.95f, 0.9f);          // 光の色 (やや暖色の白)
light.intensity = 3.0f;                                 // 強度 (HDR なので 1.0 以上も OK)

// --- ポイントライト（電球のような全方向光源）---
GX::Light pointLight;
pointLight.type = GX::LightType::Point;
pointLight.position = GX::Vector3(5.0f, 3.0f, 0.0f);   // ライトの位置
pointLight.color = GX::Vector3(1.0f, 0.5f, 0.2f);      // 暖色のオレンジ
pointLight.intensity = 10.0f;                            // 強度
pointLight.radius = 20.0f;                               // 光が届く範囲 (20m)

// --- スポットライト（懐中電灯のような方向付き光源）---
GX::Light spotLight;
spotLight.type = GX::LightType::Spot;
spotLight.position = GX::Vector3(0.0f, 10.0f, 0.0f);   // 位置 (上方)
spotLight.direction = GX::Vector3(0.0f, -1.0f, 0.0f);  // 向き (真下)
spotLight.intensity = 15.0f;                             // 強度
spotLight.innerAngle = 0.3f;    // 内側の角度 (ラジアン): この範囲は最大の明るさ
spotLight.outerAngle = 0.5f;    // 外側の角度: ここから外は徐々に暗くなり、外側で完全に暗くなる
```

### マテリアル（PBR）

PBR マテリアルは 4 つのパラメータで物体の質感を表現します。

```cpp
GX::Material material;
material.albedo = GX::Vector3(0.8f, 0.2f, 0.1f);  // ベース色 (赤っぽい色)
                                                     // 光の影響を除いた「素の色」
material.metallic = 0.0f;      // 金属度: 0.0=非金属(プラスチック,木,石), 1.0=金属(鉄,金,銀)
                                // 中間値は通常使わない（現実の素材は0か1のどちらか）
material.roughness = 0.5f;     // 粗さ: 0.0=完全な鏡面（映り込みがくっきり）
                                //        1.0=ざらざら（反射がぼやける）
material.emissive = GX::Vector3(0.0f, 0.0f, 0.0f); // 発光色 (0,0,0=発光なし)
                                                     // ネオンや溶岩の自己発光に使う
```

> **PBR の考え方**
>
> 従来の描画方式では「反射の強さ」「光沢度」などのパラメータを手動調整していましたが、
> PBR では物理法則に基づいて「金属度」と「粗さ」だけで自然な質感を表現できます。
> Albedo（基本色）を決めて、Metallic と Roughness を調整するだけでリアルな見た目になります。

### メッシュ生成

プログラムから基本的な3Dメッシュ（形状データ）を生成できます。

```cpp
GX::MeshData cube = GX::MeshData::CreateCube(1.0f);
// 立方体: 一辺 1.0m

GX::MeshData sphere = GX::MeshData::CreateSphere(0.5f, 32, 16);
// 球: 半径 0.5m, 横方向32分割, 縦方向16分割
// 分割数が多いほど滑らかだが頂点数が増える

GX::MeshData cylinder = GX::MeshData::CreateCylinder(0.5f, 2.0f, 16);
// 円柱: 半径 0.5m, 高さ 2.0m, 周方向16分割

GX::MeshData plane = GX::MeshData::CreatePlane(10.0f, 10.0f);
// 平面: 幅 10.0m × 奥行き 10.0m（地面や壁に使える）
```

### 3D 描画ループ

```cpp
GX::Renderer3D renderer;
renderer.Initialize(device, cmdList);

// --- フレーム描画 ---
renderer.BeginScene(camera);        // シーン描画の開始（カメラ設定を適用）

renderer.SetDirectionalLight(light);     // 太陽光を設定
renderer.AddPointLight(pointLight);      // 点光源を追加（複数追加可能）

// メッシュの位置・回転・スケールを設定
GX::Transform3D transform;
transform.SetPosition(0.0f, 0.0f, 0.0f);   // 位置: 原点
transform.SetRotation(0.0f, angle, 0.0f);   // Y軸回転 (angle は毎フレーム増加させる等)
transform.SetScale(1.0f, 1.0f, 1.0f);      // スケール: 等倍

renderer.DrawMesh(
    cube,                           // 描画するメッシュデータ
    material,                       // 適用するマテリアル
    transform.GetWorldMatrix()      // ワールド変換行列（位置・回転・スケールをまとめた行列）
);

renderer.EndScene();                // シーン描画の終了
```

### glTF / GXMD モデル

```cpp
#include "Graphics/3D/Model.h"

// glTF モデルの読み込み
auto model = GX::ModelLoader::LoadFromFile(
    "Assets/models/character.gltf", // モデルファイルパス (glTF または GXMD)
    device,                         // GraphicsDevice (GPU デバイス)
    texManager                      // TextureManager (テクスチャ管理)
);
// 戻り値は std::unique_ptr<Model>（自動でメモリ解放される）

// 描画
renderer.DrawModel(*model, worldMatrix);

// スケルタルアニメーション（骨格によるアニメーション）
if (model->HasAnimations())
{
    model->UpdateAnimation(deltaTime);  // deltaTime でアニメーションを進行
}
```

## ポストエフェクト

ポストエフェクトは、3D シーンの描画が完了した後に画面全体に適用する映像加工です。
映画のような映像表現を実現します。

```cpp
GX::PostEffectPipeline postFX;
postFX.Initialize(
    device, cmdList,
    width, height           // 画面解像度 (例: 1280, 960)
);

// 個別エフェクトの設定
postFX.SetBloomEnabled(true);           // Bloom (光のにじみ) を有効化
postFX.SetBloomThreshold(1.0f);         // 輝度がこの値を超えた部分がにじむ
postFX.SetBloomIntensity(0.3f);         // にじみの強さ (0.0=なし, 1.0=強い)

postFX.SetFXAAEnabled(true);            // FXAA (高速近似アンチエイリアシング) を有効化
                                        // → ジャギー（斜め線のギザギザ）を滑らかにする

postFX.SetTonemapMode(GX::TonemapMode::ACES);
// トーンマッピング: HDR → LDR 変換方式
// ACES = 映画業界標準の色変換（自然で鮮やかな仕上がり）

postFX.SetVignetteEnabled(true);        // ビネット: 画面の四隅を暗くする効果

// --- フレーム描画 ---
postFX.Begin(cmdList);              // HDR レンダーターゲットへの描画を開始
// ... ここに3D描画処理 ...
postFX.Resolve(
    cmdList,                        // コマンドリスト
    deltaTime,                      // フレーム間隔 (TAA, AutoExposure 等が使用)
    depthBuffer,                    // 深度バッファ (SSAO, DoF が奥行き情報を必要とする)
    camera                          // カメラ (SSAO, SSR がカメラ行列を必要とする)
);
// Resolve() で全ポストエフェクトが順番に適用され、最終画像がバックバッファに出力される
```

### 利用可能なポストエフェクト一覧

| エフェクト | 正式名称 | 効果 |
|-----------|---------|------|
| **Bloom** | ブルーム | 明るい部分から光がにじみ出す（太陽や電球の光輪） |
| **SSAO** | Screen Space Ambient Occlusion (環境遮蔽) | 隅や溝に自然な影を追加して立体感を出す |
| **SSR** | Screen Space Reflections (スクリーン空間反射) | 水面や光沢のある床に映り込みを表示 |
| **DoF** | Depth of Field (被写界深度) | ピント範囲外をぼかして一眼カメラ風に |
| **MotionBlur** | モーションブラー | カメラや物体の動きに残像を付ける |
| **TAA** | Temporal Anti-Aliasing (テンポラルAA) | 複数フレームでジャギーを滑らかに（FXAA より高品質） |
| **VolumetricLight** | ボリュームライト (ゴッドレイ) | 光の筋を表示（森の木漏れ日風） |
| **OutlineEffect** | アウトライン | 物体の輪郭線を描画（トゥーンシェーディング風） |
| **AutoExposure** | 自動露出調整 | 暗い/明るいシーンで自動的に明るさ調整 |
| **ColorGrading** | カラーグレーディング | 映像全体の色調を調整（映画風の雰囲気に） |

## シャドウ

CSM (Cascaded Shadow Maps, カスケードシャドウマップ) が自動的に有効です。
カメラから近い範囲は高解像度、遠い範囲は低解像度で影を描画し、
広い範囲を効率的にカバーします。

スポットライトとポイントライトの影も自動で処理されます。

## Skybox

```cpp
GX::Skybox skybox;
skybox.Initialize(
    device, cmdList,
    "Assets/skybox.hdr"            // HDR 環境マップ画像 (空の画像)
);

// 描画ループ内（3D シーンの最初に描画）
skybox.Draw(cmdList, camera);
```

## よくある問題

### モデルが表示されない

- glTF 形式 (.gltf / .glb) または GXMD 形式 (.gxmd) か確認（.fbx, .obj, .x, .mv1 は LoadModel では非対応。gxconv で変換可能）
- ファイルパスが正しいか確認
- カメラの位置がモデルと同じ場所になっていないか確認（離れた位置から見る必要がある）
- ニアクリップ / ファークリップの範囲にモデルが入っているか確認

### 画面が真っ暗（ライトが効かない）

- ディレクショナルライトを設定しているか確認
- ライトの intensity が 0 になっていないか確認
- ポストエフェクトのトーンマッピングが有効か確認（HDR 値がそのまま表示されると白飛びする場合がある）

### ポストエフェクトが効かない

- `postFX.Begin()` と `postFX.Resolve()` で 3D 描画を挟んでいるか確認
- `Resolve()` に `depthBuffer` と `camera` を渡しているか確認（SSAO, DoF に必要）
- HDR レンダリングが有効か確認

## シェーダーモデル

GXLib は PBR 以外にも複数のシェーダーモデル（描画方式）をサポートしています。
Material の `shaderModel` を変更するだけで、PSO（描画設定）が自動的に切り替わります。

```cpp
// マテリアルのシェーダーモデルを指定
material.shaderModel = GX::ShaderModel::PBR;         // 物理ベースレンダリング（デフォルト）
material.shaderModel = GX::ShaderModel::Toon;        // トゥーンシェーディング（UTS2 ベース）
material.shaderModel = GX::ShaderModel::Phong;       // Phong シェーディング
material.shaderModel = GX::ShaderModel::Unlit;       // ライティングなし（UI 用等）
material.shaderModel = GX::ShaderModel::Subsurface;  // サブサーフェス散乱（肌、蝋等）
material.shaderModel = GX::ShaderModel::ClearCoat;   // クリアコート（車の塗装等）
```

> **ShaderRegistry について**
>
> ShaderRegistry は 6 種類のシェーダーモデル x static/skinned = 14 PSO を一括管理します。
> Material の shaderModel フィールドに応じて適切な PSO が自動選択されるため、
> ユーザーが PSO を手動で切り替える必要はありません。
> Toon シェーダーはアウトライン描画用の追加パスを自動で実行します。

### Toon シェーダー（UTS2 ベース）

Toon シェーダーは UTS2 (Unity Toon Shader 2.0) をベースにした、アニメ調の描画方式です。
ダブルシェード（3ゾーン遷移: ベース色 → 1st シェード → 2nd シェード）、リムライト、
ハイカラー、スムース法線ベースのアウトラインを備えています。

```cpp
material.shaderModel = GX::ShaderModel::Toon;

// Toon 固有パラメータは ShaderModelParams 経由で設定
auto& params = material.shaderParams;
params.shadeColor()     = {0.6f, 0.3f, 0.3f};   // 1st シェード色
params.shade2ndColor()  = {0.3f, 0.15f, 0.15f};  // 2nd シェード色
params.outlineWidth()   = 0.002f;                 // アウトライン幅
params.outlineColor()   = {0.1f, 0.05f, 0.05f};  // アウトライン色
```

## アセットパイプライン

GXLib には独自のバイナリモデル形式 (.gxmd / .gxan) と変換ツールがあります。

```bash
# FBX → GXMD/GXAN 変換
gxconv input.fbx -o output.gxmd

# 複数アセットを GXPAK にバンドル
gxpak pack bundle.gxpak Assets/

# GXPAK の内容一覧
gxpak list bundle.gxpak
```

ランタイムでは gxloader を使って高速に読み込み、PakFileProvider で VFS に統合できます。

## DXR レイトレーシング

DXR 対応 GPU では、ハードウェアレイトレーシングによる高品質な反射と
グローバルイルミネーションを利用できます。

- **RTReflections** — DXR レイトレーシング反射（Y キーでトグル、SSR と排他）
- **RTGI** — グローバルイルミネーション（G キーでトグル、半解像度 + テンポラル蓄積 + A-Trous フィルタ）

> **注意**: DXR は ID3D12Device5 対応 GPU が必要です。非対応 GPU では SSR にフォールバックしてください。

## アニメーションブレンド

Animator に加え、高度なアニメーション制御が利用できます。

- **BlendStack** — 最大 8 レイヤーの Override/Additive ブレンド
- **BlendTree** — 1D/2D パラメータによるアニメーション自動合成
- **AnimatorStateMachine** — トリガーと遷移条件によるステートマシン

## 次のステップ

- [05_GUI.md](05_GUI.md) — XML+CSS による GUI 構築
- [用語集 (Glossary)](../Glossary.md) — 専門用語の確認
