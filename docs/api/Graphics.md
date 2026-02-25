# Graphics API リファレンス

名前空間: `GX`。DirectX 12 ベースの 3D レンダリングシステム。

## Renderer3D

PBR / Toon / Phong 等のシェーダーモデル、CSM シャドウ、フォグ、スカイボックスに対応する 3D レンダラー。

### 初期化

| メソッド | 説明 |
|---|---|
| `bool Initialize(device, cmdQueue, width, height)` | 3D レンダラーを初期化する |
| `void OnResize(uint32_t width, uint32_t height)` | 画面サイズ変更を処理する |

### フレーム描画

| メソッド | 説明 |
|---|---|
| `void Begin(cmdList, frameIndex, camera, time)` | メインパスのフレームを開始する |
| `void SetLights(lights, count, ambient)` | ライト配列を設定する (最大16灯) |
| `void SetMaterial(const Material& mat)` | マテリアルを設定する |
| `void DrawMesh(mesh, transform)` | GPUMesh を描画する |
| `void DrawModel(model, transform)` | Model を描画する (マテリアル自動バインド) |
| `void DrawSkinnedModel(model, transform, animator)` | スキニングモデルを描画する |
| `void DrawModelInstanced(model, transforms, count)` | GPU インスタンシング描画 |
| `void DrawTerrain(terrain, transform)` | 地形を描画する |
| `void End()` | フレーム描画を終了する |

### マテリアルオーバーライド

| メソッド | 説明 |
|---|---|
| `void SetMaterialOverride(const Material* mat)` | 全サブメッシュにマテリアルを強制適用する |
| `void ClearMaterialOverride()` | マテリアルオーバーライドを解除する |
| `void SetWireframeMode(bool enabled)` | ワイヤフレーム表示モード |

### シャドウ

| メソッド | 説明 |
|---|---|
| `void UpdateShadow(const Camera3D& camera)` | シャドウマップを更新する |
| `void SetShadowEnabled(bool enabled)` | シャドウの有効/無効 |
| `void BeginShadowPass(cmdList, frameIndex, cascadeIndex)` | CSM シャドウパス開始 |
| `void EndShadowPass(cascadeIndex)` | CSM シャドウパス終了 |

### サブシステム

| メソッド | 説明 |
|---|---|
| `Skybox& GetSkybox()` | スカイボックス |
| `PrimitiveBatch3D& GetPrimitiveBatch3D()` | 3D プリミティブ描画 |
| `DepthBuffer& GetDepthBuffer()` | 深度バッファ |
| `TextureManager& GetTextureManager()` | テクスチャ管理 |
| `MaterialManager& GetMaterialManager()` | マテリアル管理 |
| `IBL& GetIBL()` | イメージベースドライティング |

## Camera3D

3D カメラ。Free / FPS / TPS モード対応。

### 射影設定

| メソッド | 説明 |
|---|---|
| `void SetPerspective(fovY, aspect, nearZ, farZ)` | 透視投影を設定する |
| `void SetOrthographic(width, height, nearZ, farZ)` | 正射影を設定する |

### 位置・方向

| メソッド | 説明 |
|---|---|
| `void SetPosition(x, y, z)` / `SetPosition(pos)` | カメラ位置を設定する |
| `void LookAt(const XMFLOAT3& target)` | ターゲットを注視する (pitch/yaw 自動設定) |
| `void Rotate(deltaPitch, deltaYaw)` | カメラを回転する |
| `void MoveForward(distance)` / `MoveRight(d)` / `MoveUp(d)` | カメラを移動する |
| `void SetMode(CameraMode mode)` | Free / FPS / TPS モード切替 |
| `void SetTPSDistance(float d)` / `SetTPSOffset(offset)` | TPS パラメータ |

### 行列取得

| メソッド | 説明 |
|---|---|
| `XMMATRIX GetViewMatrix()` | ビュー行列 |
| `XMMATRIX GetProjectionMatrix()` | 射影行列 |
| `XMMATRIX GetViewProjectionMatrix()` | ビュー射影行列 |
| `const XMFLOAT3& GetPosition()` | カメラ位置 |
| `XMFLOAT3 GetForward()` / `GetRight()` / `GetUp()` | 方向ベクトル |

## Material

PBR マテリアルデータ。シェーダーモデル: `Standard` / `Unlit` / `Toon` / `Phong` / `Subsurface` / `ClearCoat`。

### 主要フィールド

| フィールド | 型 | 説明 |
|---|---|---|
| `constants.albedoFactor` | `XMFLOAT4` | アルベド色 (RGBA) |
| `constants.metallicFactor` | `float` | 金属度 (0.0 - 1.0) |
| `constants.roughnessFactor` | `float` | 粗さ (0.0 - 1.0) |
| `constants.emissiveStrength` | `float` | 自発光の強度 |
| `albedoMapHandle` | `int` | アルベドテクスチャハンドル (-1=なし) |
| `normalMapHandle` | `int` | ノーマルマップハンドル (-1=なし) |
| `shaderModel` | `ShaderModel` | シェーダーモデル種別 |
| `shaderParams` | `ShaderModelParams` | シェーダーモデル固有パラメータ (256B) |

## Light / LightData

ライトの生成ファクトリとデータ構造。

| 静的メソッド | 説明 |
|---|---|
| `Light::CreateDirectional(direction, color, intensity)` | 平行光源を生成する |
| `Light::CreatePoint(position, range, color, intensity)` | 点光源を生成する |
| `Light::CreateSpot(position, direction, range, spotAngleDeg, color, intensity)` | スポットライトを生成する |

### LightData フィールド

| フィールド | 説明 |
|---|---|
| `position` | 位置 (Point/Spot) |
| `direction` | 方向 (Directional/Spot) |
| `color` / `intensity` | 色と強度 |
| `range` | 到達距離 (Point/Spot) |
| `spotAngle` | スポット角度 (cos 値) |
| `type` | `LightType` (Directional/Point/Spot) |

## PostEffectPipeline

HDR ポストエフェクトパイプライン。エフェクトチェーン:
`HDR -> [RTGI] -> [SSAO] -> [RT/SSR] -> [VolumetricLight] -> [Bloom] -> [DoF] -> [MotionBlur] -> [Outline] -> [TAA] -> [ColorGrading] -> [AutoExposure] -> [Tonemap] -> [FXAA] -> [Vignette] -> LDR`

### 基本操作

| メソッド | 説明 |
|---|---|
| `bool Initialize(device, width, height)` | パイプラインを初期化する |
| `void BeginScene(cmdList, frameIndex, dsvHandle, camera)` | HDR シーン描画を開始する |
| `void EndScene()` | シーン描画を終了する |
| `void Resolve(backBufferRTV, depthBuffer, camera, deltaTime)` | 全エフェクトを実行して出力する |

### エフェクト設定

| メソッド | 説明 |
|---|---|
| `void SetTonemapMode(TonemapMode mode)` | Reinhard / ACES / Uncharted2 |
| `void SetExposure(float v)` | 露出値 |
| `void SetFXAAEnabled(bool)` | FXAA の有効/無効 |
| `void SetVignetteEnabled(bool)` | ビネットの有効/無効 |
| `void SetColorGradingEnabled(bool)` | カラーグレーディングの有効/無効 |
| `bool LoadSettings(path)` / `bool SaveSettings(path)` | JSON 設定の保存/読み込み (F12) |

### サブエフェクトアクセス

| メソッド | 説明 |
|---|---|
| `SSAO& GetSSAO()` | SSAO パラメータ |
| `Bloom& GetBloom()` | ブルーム |
| `DepthOfField& GetDoF()` | 被写界深度 |
| `MotionBlur& GetMotionBlur()` | モーションブラー |
| `SSR& GetSSR()` | スクリーン空間反射 |
| `TAA& GetTAA()` | 時間的アンチエイリアシング |
| `AutoExposure& GetAutoExposure()` | 自動露出 |

### 使用例

```cpp
PostEffectPipeline postFX;
postFX.Initialize(device, 1920, 1080);
postFX.GetBloom().SetEnabled(true);
postFX.GetSSAO().SetEnabled(true);

// レンダリングループ
postFX.BeginScene(cmdList, frameIndex, dsvHandle, camera);
renderer.Begin(cmdList, frameIndex, camera, time);
renderer.SetLights(lights, lightCount, ambient);
renderer.DrawModel(model, transform);
renderer.End();
postFX.EndScene();
depthBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
postFX.Resolve(backBufferRTV, depthBuffer, camera, deltaTime);
depthBuffer.TransitionTo(cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
```
