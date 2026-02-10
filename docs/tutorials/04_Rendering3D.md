# 04 - 3D Rendering

PBR (物理ベースレンダリング)、カメラ、ライティング、3D モデルの使い方を解説します。

## 簡易 API での 3D 描画

```cpp
// カメラ設定
SetCameraPositionAndTarget(
    VGet(0.0f, 5.0f, -10.0f),   // カメラ位置
    VGet(0.0f, 0.0f, 0.0f));    // 注視点
SetCameraNearFar(0.1f, 1000.0f);

// モデル読み込み（glTF形式）
int model = LoadModel("Assets/models/character.gltf");
SetModelPosition(model, VGet(0.0f, 0.0f, 0.0f));
SetModelScale(model, VGet(1.0f, 1.0f, 1.0f));
SetModelRotation(model, VGet(0.0f, 0.5f, 0.0f));

// フレームループ
ClearDrawScreen();
DrawModel(model);
ScreenFlip();

// 解放
DeleteModel(model);
```

## ネイティブ API

### カメラ

```cpp
GX::Camera3D camera;
camera.SetPosition(0.0f, 5.0f, -10.0f);
camera.SetTarget(0.0f, 0.0f, 0.0f);
camera.SetFov(GX::MathUtil::PI / 4.0f);    // 45度
camera.SetNearFar(0.1f, 1000.0f);
camera.SetAspectRatio(1280.0f / 960.0f);
camera.Update();
```

### ライティング

```cpp
GX::Light light;
// ディレクショナルライト（太陽光）
light.type = GX::LightType::Directional;
light.direction = GX::Vector3(0.3f, -1.0f, 0.5f);
light.color = GX::Vector3(1.0f, 0.95f, 0.9f);
light.intensity = 3.0f;

// ポイントライト
GX::Light pointLight;
pointLight.type = GX::LightType::Point;
pointLight.position = GX::Vector3(5.0f, 3.0f, 0.0f);
pointLight.color = GX::Vector3(1.0f, 0.5f, 0.2f);
pointLight.intensity = 10.0f;
pointLight.radius = 20.0f;

// スポットライト
GX::Light spotLight;
spotLight.type = GX::LightType::Spot;
spotLight.position = GX::Vector3(0.0f, 10.0f, 0.0f);
spotLight.direction = GX::Vector3(0.0f, -1.0f, 0.0f);
spotLight.intensity = 15.0f;
spotLight.innerAngle = 0.3f;
spotLight.outerAngle = 0.5f;
```

### マテリアル（PBR）

```cpp
GX::Material material;
material.albedo = GX::Vector3(0.8f, 0.2f, 0.1f);  // ベース色
material.metallic = 0.0f;      // 金属度（0=非金属, 1=金属）
material.roughness = 0.5f;     // 粗さ（0=鏡面, 1=粗い）
material.emissive = GX::Vector3(0.0f, 0.0f, 0.0f); // 発光色
```

### メッシュ生成

```cpp
// プリミティブメッシュの生成
GX::MeshData cube = GX::MeshData::CreateCube(1.0f);
GX::MeshData sphere = GX::MeshData::CreateSphere(0.5f, 32, 16);
GX::MeshData cylinder = GX::MeshData::CreateCylinder(0.5f, 2.0f, 16);
GX::MeshData plane = GX::MeshData::CreatePlane(10.0f, 10.0f);
```

### 3D 描画ループ

```cpp
GX::Renderer3D renderer;
renderer.Initialize(device, cmdList);

// フレーム描画
renderer.BeginScene(camera);

// ライト設定
renderer.SetDirectionalLight(light);
renderer.AddPointLight(pointLight);

// メッシュ描画
GX::Transform3D transform;
transform.SetPosition(0.0f, 0.0f, 0.0f);
transform.SetRotation(0.0f, angle, 0.0f);
transform.SetScale(1.0f, 1.0f, 1.0f);

renderer.DrawMesh(cube, material, transform.GetWorldMatrix());

renderer.EndScene();
```

### glTF モデル

```cpp
#include "Graphics/3D/Model.h"

// glTF モデルの読み込み
auto model = GX::ModelLoader::LoadFromFile("Assets/models/character.gltf",
                                            device, texManager);

// 描画
renderer.DrawModel(*model, worldMatrix);

// スケルタルアニメーション
if (model->HasAnimations())
{
    model->UpdateAnimation(deltaTime);
}
```

## ポストエフェクト

GXLib には HDR パイプラインとポストエフェクトが組み込まれています。

```cpp
GX::PostEffectPipeline postFX;
postFX.Initialize(device, cmdList, width, height);

// エフェクト設定
postFX.SetBloomEnabled(true);
postFX.SetBloomThreshold(1.0f);
postFX.SetBloomIntensity(0.3f);

postFX.SetFXAAEnabled(true);
postFX.SetTonemapMode(GX::TonemapMode::ACES);
postFX.SetVignetteEnabled(true);

// フレーム描画
postFX.Begin(cmdList);         // HDR RT へ描画開始
// ... 3D描画 ...
postFX.Resolve(cmdList, deltaTime, depthBuffer, camera);  // ポストエフェクト適用
```

利用可能なポストエフェクト:
- **Bloom** — 高輝度部分の光のにじみ
- **SSAO** — 環境遮蔽
- **SSR** — スクリーン空間反射
- **DoF** — 被写界深度
- **MotionBlur** — モーションブラー
- **TAA** — テンポラルアンチエイリアシング
- **VolumetricLight** — ゴッドレイ
- **OutlineEffect** — アウトライン
- **AutoExposure** — 自動露出調整
- **ColorGrading** — カラーグレーディング

## シャドウ

CSM（カスケードシャドウマップ）、スポットシャドウ、ポイントシャドウが自動的に有効です。

## Skybox

```cpp
GX::Skybox skybox;
skybox.Initialize(device, cmdList, "Assets/skybox.hdr");

// 描画ループ内
skybox.Draw(cmdList, camera);
```

## 次のステップ

- [05_GUI.md](05_GUI.md) — XML+CSS による GUI 構築
