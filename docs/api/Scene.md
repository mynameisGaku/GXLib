# Scene API リファレンス

名前空間: `GX`。エンティティ・コンポーネントベースのシーン管理システム。

## Scene

エンティティのコンテナ。更新・描画・フラスタムカリングを管理する。

### エンティティ管理

| メソッド | 説明 |
|---|---|
| `Entity* CreateEntity(const std::string& name = "Entity")` | エンティティを作成して返す |
| `void DestroyEntity(Entity* entity)` | エンティティを破棄する (次フレームで実削除) |
| `Entity* FindEntity(const std::string& name)` | 名前でエンティティを検索する |
| `Entity* FindEntityByID(uint32_t id)` | ID でエンティティを検索する |
| `const auto& GetEntities()` | 全エンティティ一覧を取得する |
| `const auto& GetRootEntities()` | ルートエンティティ一覧を取得する |
| `uint32_t GetEntityCount()` | エンティティ数を取得する |

### 更新・描画

| メソッド | 説明 |
|---|---|
| `void Update(float deltaTime)` | 全エンティティを更新する (ScriptComponent の onUpdate を呼ぶ) |
| `void Render(Renderer3D& renderer)` | 全エンティティを描画する (カリングなし) |
| `void Render(Renderer3D& renderer, const Camera3D& camera)` | フラスタムカリング付き描画 |
| `RenderStats GetLastRenderStats()` | 描画統計情報を取得する |

### RenderStats

| フィールド | 説明 |
|---|---|
| `totalEntities` | 全エンティティ数 |
| `visibleEntities` | 可視エンティティ数 |
| `culledEntities` | カリングされた数 |
| `drawCalls` | ドローコール数 |
| `instancedBatches` | インスタンシングバッチ数 |
| `instancedEntities` | インスタンシング対象エンティティ数 |

### デバッグ描画

| フラグ | 説明 |
|---|---|
| `SceneDebug_BoundingSpheres` | バウンディング球を表示する |
| `SceneDebug_AABBs` | AABB を表示する |
| `SceneDebug_Frustum` | フラスタムを表示する |
| `SceneDebug_LODLevels` | LOD レベルを表示する |

## Entity

Transform3D を内蔵するゲームオブジェクト。親子階層とコンポーネントシステムをサポートする。

### 基本操作

| メソッド | 説明 |
|---|---|
| `const std::string& GetName()` / `SetName(name)` | 名前の取得/設定 |
| `uint32_t GetID()` | 一意の ID を取得する |
| `bool IsActive()` / `SetActive(bool)` | アクティブ状態 |
| `Transform3D& GetTransform()` | Transform3D を取得する (常に存在) |
| `XMMATRIX GetWorldMatrix()` | 親の変換を考慮したワールド行列 |

### 階層

| メソッド | 説明 |
|---|---|
| `void SetParent(Entity* parent)` | 親エンティティを設定する (nullptr でルートに戻る) |
| `Entity* GetParent()` | 親を取得する |
| `const auto& GetChildren()` | 子エンティティ一覧を取得する |

### コンポーネント

| メソッド | 説明 |
|---|---|
| `T* AddComponent<T>()` | コンポーネントを追加して返す |
| `T* GetComponent<T>()` | コンポーネントを取得する (なければ nullptr) |
| `bool HasComponent<T>()` | コンポーネントの有無を確認する |
| `void RemoveComponent<T>()` | コンポーネントを削除する |

### バウンディング

| メソッド | 説明 |
|---|---|
| `void SetBounds(const AABB3D& aabb)` | ローカル AABB を設定する |
| `Sphere GetWorldBoundingSphere()` | ワールド空間のバウンディング球を取得する |

## ビルトインコンポーネント

### MeshRendererComponent

静的メッシュの描画を担当するコンポーネント。

| フィールド | 型 | 説明 |
|---|---|---|
| `model` | `Model*` | 描画対象のモデル |
| `ownedModel` | `unique_ptr<Model>` | インポートしたモデルの所有権 |
| `materials` | `vector<Material>` | マテリアル配列 |
| `castShadow` | `bool` | 影を落とすか |
| `submeshVisibility` | `vector<bool>` | サブメッシュごとの表示 ON/OFF |
| `sourcePath` | `string` | インポート元ファイルパス |
| `useMaterialOverride` | `bool` | マテリアルオーバーライド有効化 |

### SkinnedMeshRendererComponent

スキニングアニメーション付きメッシュ。

| フィールド | 型 | 説明 |
|---|---|---|
| `model` | `Model*` | モデル |
| `animator` | `unique_ptr<Animator>` | アニメーター |
| `sourcePath` | `string` | インポート元ファイルパス |

### その他のコンポーネント

| コンポーネント | 説明 |
|---|---|
| `CameraComponent` | カメラ。`camera` (Camera3D), `isMain` (bool) |
| `LightComponent` | ライト。`lightData` (LightData) |
| `TerrainComponent` | 地形。`terrain` (Terrain*) |
| `LODComponent` | LOD グループ。`lodGroup` (LODGroup) |
| `AudioSourceComponent` | オーディオ。`soundHandle`, `playOnStart`, `loop` |
| `ParticleSystemComponent` | パーティクルシステム |
| `ScriptComponent` | ユーザーロジック。`onUpdate`, `onStart`, `onDestroy` コールバック |

## SceneSerializer

シーンの JSON 直列化。nlohmann/json 使用。

| 静的メソッド | 説明 |
|---|---|
| `bool SaveToJson(scene, filePath)` | シーンを JSON ファイルに保存する |
| `bool LoadFromJson(scene, filePath, modelLoader)` | JSON からシーンを読み込む |
| `std::string ToJsonString(scene)` | シーンを JSON 文字列に変換する |
| `bool FromJsonString(scene, json, modelLoader)` | JSON 文字列からシーンを復元する |

`ModelLoadCallback = std::function<Model*(const std::string& path)>` -- モデルパスから `Model*` を解決するコールバック。

### 使用例

```cpp
// シーン構築
Scene scene("MyScene");
Entity* player = scene.CreateEntity("Player");
auto* mesh = player->AddComponent<MeshRendererComponent>();
mesh->model = playerModel;

Entity* child = scene.CreateEntity("Weapon");
child->SetParent(player);
child->GetTransform().SetPosition(1.0f, 0.5f, 0.0f);

auto* light = scene.CreateEntity("Sun");
auto* lc = light->AddComponent<LightComponent>();
lc->lightData = Light::CreateDirectional({-0.5f, -1.0f, 0.5f}, {1,1,1}, 2.0f);

auto* npc = scene.CreateEntity("NPC");
auto* script = npc->AddComponent<ScriptComponent>();
script->onUpdate = [](float dt) { /* ロジック */ };

// 更新・描画
scene.Update(deltaTime);
scene.Render(renderer, camera);

// 保存・読み込み
SceneSerializer::SaveToJson(scene, "scene.json");

Scene loaded("Loaded");
SceneSerializer::LoadFromJson(loaded, "scene.json", [&](const std::string& path) {
    return modelCache[path];
});
```
