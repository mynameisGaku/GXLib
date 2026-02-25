# 07 - Scene/Entity システム

Entity と Scene を使った 3D シーングラフの構築方法を解説します。

## このチュートリアルで学ぶこと

- Entity（エンティティ）と Scene（シーン）の概念
- Component（コンポーネント）の追加と取得
- Transform3D による位置・回転・スケールの操作
- 親子階層によるワールド行列の合成
- ScriptComponent によるカスタムロジック
- SceneSerializer での JSON 保存・読み込み

## 前提知識

- [04_Rendering3D.md](04_Rendering3D.md) の内容（3D 描画の基本）
- [06_GXEasy2DGame.md](06_GXEasy2DGame.md) の内容（GXEasy::App の使い方）

## 概要

GXLib の Scene/Entity システムは Unity のゲームオブジェクトに近い設計です。

| 概念 | 説明 | Unity での対応 |
|------|------|---------------|
| **Scene** | エンティティのコンテナ。更新と描画を管理する | Scene |
| **Entity** | ゲーム内のオブジェクト。Transform3D を内蔵する | GameObject |
| **Component** | エンティティに機能を追加する部品 | MonoBehaviour 等 |

## Scene とエンティティの作成

```cpp
#include "Core/Scene/Scene.h"

// シーンを作成
auto scene = std::make_unique<GX::Scene>("MyScene");

// エンティティを作成（Scene が所有権を持つ）
GX::Entity* player = scene->CreateEntity("Player");
GX::Entity* enemy  = scene->CreateEntity("Enemy");

// 名前で検索
GX::Entity* found = scene->FindEntity("Player");

// エンティティ数を取得
uint32_t count = scene->GetEntityCount();  // 2

// エンティティを削除
scene->DestroyEntity(enemy);
```

> **Entity のライフサイクル**
>
> `CreateEntity()` で生成されたエンティティは Scene が所有します。
> `DestroyEntity()` を呼ぶとフレーム末に削除されます。
> Scene の破棄時に全エンティティが自動的に解放されるため、
> 手動で `delete` する必要はありません。

## Transform3D の操作

すべてのエンティティは Transform3D を内蔵しています。
位置・回転・スケールを設定して、3D 空間での配置を制御します。

```cpp
GX::Entity* box = scene->CreateEntity("Box");

// 位置の設定
box->GetTransform().SetPosition(3.0f, 1.0f, 0.0f);

// 回転の設定（ラジアン: X, Y, Z）
box->GetTransform().SetRotation(0.0f, 0.5f, 0.0f);

// スケールの設定
box->GetTransform().SetScale(2.0f, 1.0f, 1.0f);  // X方向に2倍

// 均等スケール
box->GetTransform().SetScale(1.5f);  // 全方向 1.5 倍

// 現在の値を取得
auto pos = box->GetTransform().GetPosition();  // XMFLOAT3
auto rot = box->GetTransform().GetRotation();  // XMFLOAT3 (Euler)
auto scl = box->GetTransform().GetScale();     // XMFLOAT3
```

## Component の追加

コンポーネントはエンティティに機能を追加する部品です。
テンプレートメソッドで追加・取得・削除を行います。

```cpp
#include "Core/Scene/Components.h"

GX::Entity* obj = scene->CreateEntity("MyObject");

// MeshRenderer コンポーネントを追加
auto* meshComp = obj->AddComponent<GX::MeshRendererComponent>();
meshComp->model = myModel;      // Model* を設定
meshComp->castShadow = true;    // 影を落とす

// コンポーネントの取得（存在しない場合は nullptr）
auto* mesh = obj->GetComponent<GX::MeshRendererComponent>();
if (mesh)
{
    mesh->castShadow = false;
}

// コンポーネントの存在確認
if (obj->HasComponent<GX::MeshRendererComponent>())
{
    // MeshRenderer が付いている
}

// コンポーネントの削除
obj->RemoveComponent<GX::MeshRendererComponent>();
```

### ビルトインコンポーネント一覧

| コンポーネント | 説明 | 用途 |
|---------------|------|------|
| MeshRendererComponent | 静的メッシュの描画 | 建物、小物、地形 |
| SkinnedMeshRendererComponent | スケルタルアニメーション付きメッシュ | キャラクター |
| CameraComponent | カメラ | プレイヤー視点、監視カメラ |
| LightComponent | ライト | 照明、松明 |
| ScriptComponent | カスタムロジック | 移動、回転、ゲームルール |
| AudioSourceComponent | 音源 | 効果音、環境音 |
| TerrainComponent | 地形 | フィールド |
| LODComponent | LOD グループ | 遠距離の描画最適化 |
| ParticleSystemComponent | パーティクル | エフェクト |

## ScriptComponent でカスタムロジック

ScriptComponent はラムダ式でカスタムロジックを記述できるコンポーネントです。
`onUpdate` が毎フレーム呼ばれ、`onStart` は初回の `Scene::Update()` で 1 回だけ呼ばれます。

```cpp
auto* entity = scene->CreateEntity("RotatingCube");
entity->GetTransform().SetPosition(0, 1.0f, 0);

auto* script = entity->AddComponent<GX::ScriptComponent>();

// 初回に1回だけ実行
script->onStart = []()
{
    // 初期化処理
};

// 毎フレーム実行（dt = 経過時間）
script->onUpdate = [entity](float dt)
{
    auto rot = entity->GetTransform().GetRotation();
    entity->GetTransform().SetRotation(
        rot.x,
        rot.y + dt * 1.0f,  // Y軸に毎秒1ラジアン回転
        rot.z
    );
};
```

## 親子階層

エンティティは親子関係を持つことができます。
子エンティティの Transform は親の Transform に追従します。

```cpp
// ロボットアームの例
auto* base = scene->CreateEntity("Base");
base->GetTransform().SetPosition(0, 0.5f, 0);

auto* arm = scene->CreateEntity("Arm");
arm->SetParent(base);                          // base の子にする
arm->GetTransform().SetPosition(0, 1.5f, 0);  // 親からの相対位置

auto* hand = scene->CreateEntity("Hand");
hand->SetParent(arm);                          // arm の子にする
hand->GetTransform().SetPosition(0, 1.0f, 0); // arm からの相対位置
```

親を回転させると、子も一緒に回転します。

```cpp
// ベースを回転させると、arm と hand も追従する
auto* baseScript = base->AddComponent<GX::ScriptComponent>();
baseScript->onUpdate = [base](float dt)
{
    auto rot = base->GetTransform().GetRotation();
    base->GetTransform().SetRotation(rot.x, rot.y + dt * 0.5f, rot.z);
};
```

### ワールド行列の取得

親子関係を考慮したワールド行列（最終的な変換行列）を取得するには、
`GetWorldMatrix()` を使います。

```cpp
// ワールド行列: 親の変換を再帰的に合成した最終行列
XMMATRIX worldMat = hand->GetWorldMatrix();

// 描画時に使用
renderer.DrawMesh(mesh, worldMat);
```

> `GetTransform()` はローカル変換（親からの相対）を返します。
> `GetWorldMatrix()` はワールド変換（全ての親を合成した絶対）を返します。

## Scene の更新と描画

```cpp
// 毎フレームの更新（ScriptComponent の onUpdate が呼ばれる）
scene->Update(deltaTime);

// 描画方法1: カリングなし（全エンティティ描画）
scene->Render(renderer);

// 描画方法2: フラスタムカリングあり（カメラの視界外を除外）
scene->Render(renderer, camera);

// 描画統計を確認
auto stats = scene->GetLastRenderStats();
// stats.totalEntities   — 全エンティティ数
// stats.visibleEntities — 描画されたエンティティ数
// stats.culledEntities  — カリングで除外されたエンティティ数
// stats.drawCalls       — 描画コール数
```

## SceneSerializer で保存・読み込み

シーンの状態を JSON ファイルに保存し、後から復元できます。

```cpp
#include "Core/Scene/SceneSerializer.h"

// --- 保存 ---
GX::SceneSerializer::SaveToJson(*scene, "Saves/level01.json");

// --- 読み込み ---
auto loadedScene = std::make_unique<GX::Scene>();

// ModelLoadCallback: JSONに記録されたモデルパスからModel*を解決する関数
GX::SceneSerializer::LoadFromJson(*loadedScene, "Saves/level01.json",
    [&](const std::string& path) -> GX::Model*
    {
        // パスに応じてモデルをロードして返す
        return modelMap[path];
    }
);
```

### JSON の構造例

```json
{
    "name": "MyScene",
    "entities": [
        {
            "id": 1,
            "name": "Player",
            "active": true,
            "transform": {
                "position": [0.0, 1.0, 0.0],
                "rotation": [0.0, 0.0, 0.0],
                "scale": [1.0, 1.0, 1.0]
            },
            "components": [
                {
                    "type": "MeshRenderer",
                    "sourcePath": "Assets/models/player.gxmd",
                    "castShadow": true
                }
            ],
            "children": [2, 3]
        }
    ]
}
```

> **ModelLoadCallback が必要な理由**
>
> Model は GPU リソースを含むため、JSON にそのまま保存できません。
> 代わりにファイルパスだけを保存し、読み込み時にコールバックで
> 実際の Model* を解決します。

## 完全なサンプル

`Samples/SceneShowcase/` にシーングラフの完全なサンプルがあります。
ロボットアームの親子階層と、独立して回転するキューブ群を表示します。

```bash
cmake --build build --config Debug --target SceneShowcase
```

## よくある問題

### エンティティが描画されない

- `MeshRendererComponent` を追加して `model` を設定しているか確認
- `scene->Render(renderer)` を呼んでいるか確認
- エンティティの `IsActive()` が `true` か確認

### 親子関係が反映されない

- `SetParent()` を呼んだ後に `scene->Update()` を実行しているか確認
- ローカル座標が意図通りか確認（`SetPosition` は親からの相対位置）

### ScriptComponent の onUpdate が呼ばれない

- `scene->Update(dt)` を毎フレーム呼んでいるか確認
- `ScriptComponent` の `SetEnabled(false)` になっていないか確認

### JSON の読み込みでモデルが表示されない

- `ModelLoadCallback` を正しく実装しているか確認
- コールバック内でモデルを読み込んで返しているか確認

## 次のステップ

- [08_AssetPipeline.md](08_AssetPipeline.md) -- gxconv/gxpak でアセットを管理する
- [04_Rendering3D.md](04_Rendering3D.md) -- PBR マテリアルとポストエフェクトの詳細
- [05_GUI.md](05_GUI.md) -- XML+CSS による GUI 構築
