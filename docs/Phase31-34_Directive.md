# GXLib Phase 31-34 総合実装指令書

## Context

Phase 0-30 完了済み。エンジンは完全な 2D/3D 描画パイプライン、DXR レイトレーシング、
ポストエフェクト、GUI、物理、オーディオ、シーングラフ、ECS、パーティクル（CPU/GPU）、
ナビメッシュ、LOD、デカール、IBL、IK、GPU インスタンシング、アクションマッピング、
GXModelViewer（ImGui Docking + 19 パネル）、アセットパイプライン（gxformat/gxconv/gxloader/gxpak）
を持つ成熟状態。

本指令書は **安定化・統合・最適化・新機能** の 4 フェーズを網羅し、
任意の Claude インスタンスが独立して各 Phase を実装できることを目的とする。

---

## 全体ロードマップ

| Phase | 名称 | 概要 | 依存 |
|-------|------|------|------|
| **31** | 安定化 & 品質パス | 全サンプルのバグ修正・パターン統一・エッジケース対応 | なし |
| **32** | GXModelViewer ↔ Scene統合 | Viewer の SceneGraph を GX::Scene ベースに移行 | Phase 31 |
| **33** | レンダリング最適化 | フラスタムカリング・自動バッチング・LOD統合 | Phase 31 |
| **34** | 新エンジン機能 | トレイルレンダラー・スプライン・テキストレイアウト・プロファイラ階層化等 | Phase 31 |

Phase 31 は全ての前提。Phase 32/33/34 は Phase 31 完了後に並列着手可能。

---

## Phase 31: 安定化 & 品質パス

### 目的
全 17+ サンプルの実行確認、既知バグ修正、描画パターン統一。
エンジン基盤の信頼性を確保してから機能追加に進む。

### 31a: 既知バグ修正

#### Bug 1: Audio3DShowcase — WAV ファイルリーク
- **ファイル**: `Samples/Audio3DShowcase/main.cpp`
- **問題**: `Start()` で `_tone_a4.wav`, `_tone_cs5.wav`, `_tone_e5.wav` を作業ディレクトリに生成するが、
  終了時に削除しない
- **修正方針**: GXEasy::App に `Release()` オーバーライドがあれば追加。なければ `~App()` デストラクタか
  atexit で `std::filesystem::remove()` を呼ぶ。
  ```cpp
  void Release() override
  {
      // Release はないかもしれない → デストラクタか WM_DESTROY で
      std::filesystem::remove("_tone_a4.wav");
      std::filesystem::remove("_tone_cs5.wav");
      std::filesystem::remove("_tone_e5.wav");
  }
  ```
- **確認事項**: GXEasy::App に Release/Shutdown 仮想関数があるか確認。なければ追加。
  `#include <filesystem>` は pch.h にないので `<cstdio>` の `std::remove()` を使うか、
  `<filesystem>` をサンプル内でインクルード。

#### Bug 2: PostEffectShowcase — DepthBuffer 遷移パターン不統一
- **ファイル**: `Samples/PostEffectShowcase/main.cpp`
- **問題**: Walkthrough3D, SceneShowcase 等は `depthBuffer.TransitionTo(SRV)` → `Resolve()` →
  `depthBuffer.TransitionTo(DEPTH_WRITE)` のパターンだが、PostEffectShowcase は遷移なしで Resolve() を呼ぶ。
- **修正方針**: 全 3D サンプルで以下のパターンに統一:
  ```cpp
  // Renderer3D::End() の後
  ctx.postEffect.EndScene();

  auto& depthBuffer = ctx.renderer3D.GetDepthBuffer();
  depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  ctx.postEffect.Resolve(ctx.swapChain.GetCurrentRTVHandle(),
                         depthBuffer, ctx.camera, m_lastDt);
  depthBuffer.TransitionTo(cmd, D3D12_RESOURCE_STATE_DEPTH_WRITE);
  ```
- **対象サンプル**: PostEffectShowcase, IBLShowcase, InstanceShowcase
  （これらが DepthBuffer 遷移を省略していないか全て確認）

#### Bug 3: CSM バリア遷移ガード（修正済み — 確認のみ）
- **ファイル**: `GXLib/Graphics/3D/Renderer3D.cpp` (lines 906-950)
- **状態**: 前セッションで修正済み。CSM 4 カスケード + Spot + Point の全シャドウマップに対して
  SRV バリア遷移ガードを追加済み。
- **確認事項**: `BeginShadowPass`/`EndShadowPass` を呼ばないサンプル（SceneShowcase 等）で
  D3D12 検証レイヤーエラーが出ないことを確認。

#### Bug 4: SSAO 深度バッファ復元削除（修正済み — 確認のみ）
- **ファイル**: `GXLib/Graphics/PostEffect/SSAO.cpp`
- **状態**: Execute() 末尾の `depthBuffer.TransitionTo(DEPTH_WRITE)` を削除済み。
  深度は SRV のまま後続エフェクトに渡す。
- **確認事項**: SSAO 有効時に DoF, MotionBlur, TAA が正常動作すること。

### 31b: パターン統一

#### 統一 1: `(std::max)()` / `(std::min)()` パターン
- **問題**: Windows.h の min/max マクロと STL の std::min/std::max が衝突する
- **対象**: 全サンプル `.cpp` ファイル
- **確認コマンド**:
  ```bash
  grep -rn "std::min\|std::max" Samples/ --include="*.cpp" | grep -v "(std::max)\|(std::min)"
  ```
- **修正**: `std::max(...)` → `(std::max)(...)` に統一
- **注意**: GXLib 本体は pch.h に `NOMINMAX` があるため問題ないが、サンプルは
  `<Windows.h>` を直接インクルードする場合があり衝突する

#### 統一 2: FormatT テンプレート共通化
- **問題**: SceneShowcase, NavmeshShowcase, GPUParticleShowcase 等が同一の `FormatT` テンプレートを
  各ファイルに重複定義している
- **修正方針**: `GXEasy.h` に `FormatT` を定義して全サンプルから重複を削除
  ```cpp
  // GXEasy.h に追加
  template <class... Args>
  TString FormatT(const TChar* fmt, Args... args)
  {
  #ifdef UNICODE
      return std::vformat(fmt, std::make_wformat_args(args...));
  #else
      return std::vformat(fmt, std::make_format_args(args...));
  #endif
  }
  ```
- **注意**: `Args... args` は **by-value** 必須（`Args&&` + `std::forward` は P2905R2/MSVC14.44 で
  `make_format_args` が lvalue 要求するため不可）。MEMORY.md の FormatT 項を参照。

#### 統一 3: LookAtCamera ヘルパー共通化
- **問題**: SceneShowcase, NavmeshShowcase 等で同一の `LookAtCamera` 関数が重複
- **修正方針**: `GXEasy.h` に移動、または `Camera3D` に `LookAt(target)` メソッドを追加

#### 統一 4: WASD カメラ操作の共通化
- **問題**: 複数の 3D サンプルが WASD+QE+マウスルックの同一コードを持つ
- **修正方針**: `GXEasy.h` にヘルパークラス `FPSCameraController` を追加
  ```cpp
  struct FPSCameraController
  {
      float moveSpeed = 5.0f;
      float sprintMultiplier = 3.0f;
      float mouseSensitivity = 0.003f;

      void Update(GX::Camera3D& camera, const GX::Keyboard& kb,
                  const GX::Mouse& mouse, float dt);
  };
  ```

### 31c: エッジケース対応

#### Edge 1: ParticleSystem2D コメント修正
- **ファイル**: `GXLib/Graphics/Rendering/ParticleSystem2D.h` line 63
- **問題**: コメントが `1x1白テクスチャ` のままだが実装は `16x16`
- **修正**: コメントを `16x16白テクスチャ` に修正

#### Edge 2: Entity::GetComponent の一時インスタンス生成
- **ファイル**: `GXLib/Core/Scene/Entity.h` lines 55-67
- **問題**: `GetComponent<T>()` が呼ばれるたびに `T temp;` で一時インスタンスを生成して
  `GetType()` を呼ぶ。パフォーマンス上の問題（特にホットパスで呼ぶ場合）
- **修正方針**: `constexpr` または `static` で ComponentType を取得する方式に変更
  ```cpp
  template<typename T>
  T* GetComponent() const
  {
      constexpr ComponentType type = T::k_Type; // T に static constexpr k_Type を追加
      int idx = static_cast<int>(type);
      if (idx < static_cast<int>(ComponentType::_Count) && m_componentLookup[idx] >= 0)
          return static_cast<T*>(m_components[m_componentLookup[idx]].get());
      return nullptr;
  }
  ```
  各コンポーネントに `static constexpr ComponentType k_Type = ComponentType::XXX;` を追加。
  仮想関数 `GetType()` は残す（ランタイム用途のため）。

#### Edge 3: Scene::DestroyEntity の遅延削除安全性
- **ファイル**: `GXLib/Core/Scene/Scene.cpp`
- **確認事項**: `DestroyEntity()` で `m_pendingDestroy` に追加した後、同フレーム内の
  `Render()` でそのエンティティが描画される可能性。`IsActive()` が false に設定されるタイミングを確認。
  必要なら `DestroyEntity()` 内で即座に `entity->SetActive(false)` を呼ぶ。

#### Edge 4: GXEasy::App ライフサイクル確認
- **ファイル**: `GXLib/Compat/GXEasy.h` (または相当)
- **確認事項**: `Release()` 仮想関数の存在確認。なければ追加:
  ```cpp
  virtual void Release() {} // リソース解放用、メインループ終了後に呼ばれる
  ```

### 31d: ビルド検証

全修正後に以下を実行:
```bash
cmake -B build -S .
cmake --build build --config Debug 2>&1
```

エラーゼロを確認。可能であれば各サンプル `.exe` を起動して D3D12 検証レイヤーエラーがないことを確認。

---

## Phase 32: GXModelViewer ↔ Engine Scene 統合

### 目的
GXModelViewer の独自 `SceneGraph`/`SceneEntity` を GX::Scene/GX::Entity ベースに移行し、
エンジン側のシーン管理と統一する。これにより：
- Viewer で作成したシーンをエンジンのシリアライザで保存/読み込み可能
- エンジン側のコンポーネントシステムを Viewer から直接操作可能
- コードの重複を排除

### 32a: 現状分析

#### GXModelViewer SceneGraph（現在の実装）
```
場所: GXModelViewer/Scene/SceneGraph.h
構造: SceneEntity（直接フィールド方式）
  - name, transform, model*, ownedModel, materialOverride
  - animator, selectedClipIndex
  - submeshVisibility, showBones, showWireframe
  - parentIndex (int), visible, _pendingRemoval
管理: SceneGraph クラス
  - vector<SceneEntity> + freeIndices + pendingRemovals
  - selectedEntity, selectedBone
```

#### GX::Scene（エンジン側）
```
場所: GXLib/Core/Scene/Scene.h, Entity.h, Components.h
構造: Entity（コンポーネントベース）
  - m_id, m_name, m_active, m_transform
  - m_parent (Entity*), m_children (vector<Entity*>)
  - m_components (vector<unique_ptr<Component>>)
  - m_componentLookup[] (O(1) 検索)
管理: Scene クラス
  - vector<unique_ptr<Entity>> + rootEntities + pendingDestroy
  - FindEntity, FindEntityByID, FindComponentsOfType<T>
```

#### 差分マッピング

| SceneEntity フィールド | GX::Entity 対応 |
|----------------------|----------------|
| name | Entity::GetName() |
| transform | Entity::GetTransform() |
| model* | MeshRendererComponent::model |
| ownedModel | MeshRendererComponent に所有権移動 |
| materialOverride | MeshRendererComponent::materials[0] |
| useMaterialOverride | （MeshRendererComponent に bool 追加） |
| animator | SkinnedMeshRendererComponent::animator |
| selectedClipIndex | エディタ固有状態 → EditorMetadata |
| submeshVisibility | MeshRendererComponent に追加 |
| showBones | エディタ固有状態 → EditorMetadata |
| showWireframe | エディタ固有状態 → EditorMetadata |
| parentIndex | Entity::SetParent() (ポインタベース) |
| visible | Entity::SetActive() |
| sourcePath | MeshRendererComponent に追加 |
| _pendingRemoval | Scene::DestroyEntity() 内部 |

### 32b: 移行アーキテクチャ

#### Step 1: MeshRendererComponent 拡張
```cpp
// GXLib/Core/Scene/Components.h — 既存を拡張
struct MeshRendererComponent : Component
{
    ComponentType GetType() const override { return ComponentType::MeshRenderer; }
    Model* model = nullptr;
    std::unique_ptr<Model> ownedModel;       // 追加: モデル所有権
    std::vector<Material> materials;
    bool castShadow = true;
    bool receiveShadow = true;
    std::vector<bool> submeshVisibility;     // 追加: サブメッシュ可視性
    std::string sourcePath;                  // 追加: インポート元パス
    bool useMaterialOverride = false;        // 追加: マテリアルオーバーライド有効化
    Material materialOverride;               // 追加: オーバーライドマテリアル
};
```

#### Step 2: エディタ固有メタデータコンポーネント
```cpp
// GXModelViewer/Scene/EditorMetadata.h — 新規
#pragma once
#include "Core/Scene/Component.h"

/// @brief エディタ専用メタデータ（エンジン側には存在しない）
struct EditorMetadata : GX::Component
{
    GX::ComponentType GetType() const override { return GX::ComponentType::Custom; }

    int  selectedClipIndex = -1;    // タイムラインで選択中のクリップ
    bool showBones = false;         // ボーン可視化
    bool showWireframe = false;     // ワイヤフレーム表示
};
```

#### Step 3: SceneGraph → Scene アダプタ

SceneGraph を一気に置換するのではなく、段階的に移行:

1. **Phase 32b-1**: `SceneGraph` クラス内部で `GX::Scene` を保持するアダプタ版を作成
   ```cpp
   class SceneGraph
   {
   public:
       // 既存 API は維持（パネルへの影響最小化）
       int AddEntity(const std::string& name);
       void RemoveEntity(int index);
       SceneEntity* GetEntity(int index);
       // ...

       // 新規: GX::Scene への直接アクセス
       GX::Scene& GetScene() { return m_scene; }

   private:
       GX::Scene m_scene;
       // SceneEntity ラッパー（一時的、段階的に消す）
       std::vector<SceneEntityProxy> m_proxies;
   };
   ```

2. **Phase 32b-2**: 各パネルを GX::Entity ベースに移行
   - SceneHierarchyPanel: `SceneEntity*` → `GX::Entity*`
   - PropertyPanel: `SceneEntity.transform` → `entity->GetTransform()`
   - TimelinePanel: `SceneEntity.animator` → `entity->GetComponent<SkinnedMeshRendererComponent>()->animator`
   - 等

3. **Phase 32b-3**: SceneEntityProxy を廃止、全面的に GX::Entity 使用

### 32c: パネル移行チェックリスト

| パネル | 依存する SceneEntity フィールド | 移行方法 |
|--------|-------------------------------|---------|
| SceneHierarchyPanel | name, parentIndex, visible | Entity::GetName, GetParent, IsActive |
| PropertyPanel | transform, materialOverride, model, showWireframe, showBones | Transform3D, MeshRendererComponent, EditorMetadata |
| TimelinePanel | animator, selectedClipIndex | SkinnedMeshRendererComponent, EditorMetadata |
| AnimatorPanel | animator | SkinnedMeshRendererComponent |
| BlendTreeEditor | animator | SkinnedMeshRendererComponent |
| ModelInfoPanel | model | MeshRendererComponent |
| SkeletonPanel | model, showBones | MeshRendererComponent, EditorMetadata |
| LightingPanel | （SceneEntityに依存しない） | 変更不要 |
| PostEffectPanel | （SceneEntityに依存しない） | 変更不要 |
| SkyboxPanel | （SceneEntityに依存しない） | 変更不要 |
| TerrainPanel | （SceneEntityに依存しない） | 変更不要 |
| PerformancePanel | （SceneEntityに依存しない） | 変更不要 |
| LogPanel | （SceneEntityに依存しない） | 変更不要 |
| TextureBrowser | （SceneEntityに依存しない） | 変更不要 |
| AssetBrowserPanel | sourcePath | MeshRendererComponent::sourcePath |
| IBLPanel | （SceneEntityに依存しない） | 変更不要 |
| ParticlePanel | （SceneEntityに依存しない） | 変更不要 |
| IKPanel | （SceneEntityに依存しない） | 変更不要 |
| AudioPanel | （SceneEntityに依存しない） | 変更不要 |

**移行が必要なパネル**: SceneHierarchy, Property, Timeline, Animator, BlendTreeEditor,
ModelInfo, Skeleton, AssetBrowser（8 パネル）。残り 11 パネルは変更不要。

### 32d: シリアライゼーション統合

現在 2 つの独立したシリアライザが存在:
- `GXModelViewer/Scene/SceneSerializer.h` — Viewer 独自形式
- `GXLib/Core/Scene/SceneSerializer.h` — エンジン形式

#### 統合方針
- **エンジン側のシリアライザを権威的バージョンとする**
- Viewer 側シリアライザを薄いラッパーに変更:
  ```cpp
  // GXModelViewer/Scene/SceneSerializer.h
  class ViewerSceneSerializer
  {
  public:
      // エンジン SceneSerializer を呼び出し + エディタ固有データを追加
      static bool Save(const GX::Scene& scene, const std::string& path);
      static bool Load(GX::Scene& scene, const std::string& path,
                       GX::SceneSerializer::ModelLoadCallback modelLoader);
  };
  ```
- エディタ固有データ（EditorMetadata）は JSON の "editor" セクションに別途保存

### 32e: 移行手順まとめ

1. MeshRendererComponent に `ownedModel`, `submeshVisibility`, `sourcePath`,
   `materialOverride`, `useMaterialOverride` フィールドを追加
2. EditorMetadata コンポーネントを作成
3. SceneGraph クラスを GX::Scene ラッパーに変更
4. 8 パネルを GX::Entity ベースに移行（1パネルずつ）
5. GXModelViewerApp の描画ループを GX::Entity ベースに変更
6. ViewerSceneSerializer を GX::SceneSerializer ベースに変更
7. 旧 SceneEntity/SceneGraph を完全削除
8. ビルド検証 + 動作確認

### 注意点
- **ImGui Image**: `ImTextureRef(static_cast<ImTextureID>(gpuHandle.ptr))` パターン継続
- **std::vector<bool>**: ImGui::Checkbox に直接渡せない。ローカル bool にコピー
- **Entity ID**: Viewer の int index → Entity::GetID() (uint32_t) への移行
- **selectedEntity**: SceneGraph の int メンバー → 別管理（GXModelViewerApp に移動）

---

## Phase 33: レンダリング最適化

### 目的
Scene::Render() に自動フラスタムカリング・LOD 選択・インスタンシングバッチングを統合し、
CPU 駆動描画のパフォーマンスを最大化する。

### 33a: フラスタムカリング統合

#### 現状
- `Collision3D` に `Frustum`, `TestFrustumVsSphere`, `TestFrustumVsAABB` が実装済み
- `Frustum::FromViewProjection(viewProj)` で VP 行列からフラスタム抽出可能
- しかし **Scene::Render() も Renderer3D もカリングを呼んでいない**

#### 実装方針

##### Step 1: Entity にバウンディング情報を追加
```cpp
// Entity.h に追加
struct BoundsInfo
{
    AABB3D localAABB;      // モデルのローカルAABB
    float  boundingSphereRadius = 0.0f; // バウンディング球半径
    bool   hasBounds = false;
};

class Entity
{
    // ...
    BoundsInfo m_bounds;
public:
    void SetBounds(const AABB3D& aabb);
    const BoundsInfo& GetBounds() const { return m_bounds; }

    /// ワールド空間のバウンディング球を取得（フラスタムテスト用）
    Sphere GetWorldBoundingSphere() const;
};
```

##### Step 2: Model からバウンディング情報を自動計算
```cpp
// MeshData.h に追加（または Model に）
AABB3D ComputeAABB(const Vertex3D_PBR* vertices, uint32_t count);
```

MeshRendererComponent 追加時に自動計算:
```cpp
auto* meshComp = entity->AddComponent<MeshRendererComponent>();
meshComp->model = model;
entity->SetBounds(model->ComputeAABB()); // 自動設定
```

##### Step 3: Scene::Render にカリングを統合
```cpp
void Scene::Render(Renderer3D& renderer, const Camera3D& camera)
{
    // VP行列からフラスタムを抽出
    XMMATRIX vp = camera.GetViewMatrix() * camera.GetProjectionMatrix();
    Frustum frustum = Frustum::FromViewProjection(vp);

    for (const auto& entity : m_entities)
    {
        if (!entity->IsActive()) continue;

        // フラスタムカリング
        if (entity->GetBounds().hasBounds)
        {
            Sphere worldSphere = entity->GetWorldBoundingSphere();
            if (!Collision3D::TestFrustumVsSphere(frustum, worldSphere))
                continue; // 画面外 → スキップ
        }

        // MeshRendererComponent の描画
        auto* mesh = entity->GetComponent<MeshRendererComponent>();
        if (mesh && mesh->model)
        {
            // マテリアル設定
            if (mesh->useMaterialOverride)
                renderer.SetMaterialOverride(&mesh->materialOverride);

            renderer.DrawModel(*mesh->model, entity->GetTransform());

            if (mesh->useMaterialOverride)
                renderer.ClearMaterialOverride();
        }

        // SkinnedMeshRendererComponent の描画
        auto* skinned = entity->GetComponent<SkinnedMeshRendererComponent>();
        if (skinned && skinned->model && skinned->animator)
        {
            renderer.DrawSkinnedModel(*skinned->model, entity->GetTransform(),
                                      *skinned->animator);
        }
    }
}
```

##### Step 4: Scene::Render の API 拡張
```cpp
// Scene.h
class Scene
{
public:
    // 既存（カリングなし — 後方互換）
    void Render(Renderer3D& renderer);

    // 新規（カメラ付き — フラスタムカリング有効）
    void Render(Renderer3D& renderer, const Camera3D& camera);

    // 新規（カリング統計取得）
    struct RenderStats
    {
        uint32_t totalEntities;
        uint32_t visibleEntities;
        uint32_t culledEntities;
        uint32_t drawCalls;
    };
    RenderStats GetLastRenderStats() const;
};
```

### 33b: LOD 統合

#### 現状
- `LODGroup` クラスが存在（`Graphics/3D/LODGroup.h`）
- `SelectLOD(camera, transform)` でカメラ距離ベースのモデル選択が可能
- ヒステリシスバンド 5%
- しかし **Scene::Render に統合されていない**

#### 実装方針

##### Step 1: LODComponent を追加
```cpp
// Components.h に追加
struct LODComponent : Component
{
    ComponentType GetType() const override { return ComponentType::Custom; }
    // ComponentType::Custom を使うか、LOD 専用の列挙値を追加
    LODGroup lodGroup;
};
```

##### Step 2: Scene::Render で LOD を使用
```cpp
// Scene::Render 内
auto* mesh = entity->GetComponent<MeshRendererComponent>();
auto* lod = entity->GetComponent<LODComponent>();

const Model* drawModel = nullptr;
if (lod)
{
    drawModel = lod->lodGroup.SelectLOD(camera, entity->GetTransform());
    if (!drawModel) continue; // カリング距離を超えた
}
else if (mesh && mesh->model)
{
    drawModel = mesh->model;
}

if (drawModel)
{
    renderer.DrawModel(*drawModel, entity->GetTransform());
}
```

### 33c: 自動インスタンシングバッチング

#### 現状
- `InstanceBuffer` クラスが存在
- `Renderer3D::DrawModelInstanced(model, transforms[], count)` が使用可能
- しかし **自動バッチングは一切ない**（ユーザーが手動で同じモデルの Transform を集めて呼ぶ必要）

#### 実装方針

Scene::Render 内で同一モデルのエンティティをグループ化し、自動的にインスタンシングを使用する。

```cpp
void Scene::Render(Renderer3D& renderer, const Camera3D& camera)
{
    // --- Phase 1: カリング + 描画リスト構築 ---
    struct DrawEntry
    {
        const Model* model;
        Transform3D transform;
        const Material* materialOverride; // nullptr = デフォルト
        bool isSkinned;
        Animator* animator;
    };

    std::vector<DrawEntry> drawList;
    // ... カリング後の drawList 構築 ...

    // --- Phase 2: モデル別グループ化 ---
    // Model* をキーにしてグループ化
    std::unordered_map<const Model*, std::vector<size_t>> modelGroups;
    for (size_t i = 0; i < drawList.size(); ++i)
    {
        if (!drawList[i].isSkinned) // スキンドモデルはインスタンシング非対応
            modelGroups[drawList[i].model].push_back(i);
    }

    // --- Phase 3: 描画発行 ---
    for (auto& [model, indices] : modelGroups)
    {
        if (indices.size() >= k_InstancingThreshold) // 例: 4以上でインスタンシング
        {
            // Transform 配列を構築
            std::vector<Transform3D> transforms;
            transforms.reserve(indices.size());
            for (size_t idx : indices)
                transforms.push_back(drawList[idx].transform);

            renderer.DrawModelInstanced(*model, transforms.data(),
                                        static_cast<uint32_t>(transforms.size()));
        }
        else
        {
            // 個別描画
            for (size_t idx : indices)
            {
                auto& entry = drawList[idx];
                renderer.SetMaterial(/* ... */);
                renderer.DrawModel(*entry.model, entry.transform);
            }
        }
    }

    // スキンドモデルは個別描画
    for (auto& entry : drawList)
    {
        if (entry.isSkinned)
        {
            renderer.DrawSkinnedModel(*entry.model, entry.transform, *entry.animator);
        }
    }
}
```

**閾値**: `k_InstancingThreshold = 4`（4 以上の同一モデルでインスタンシング発動）

### 33d: パフォーマンス計測

#### ProfilerOverlay との連携
Phase 30 で実装済みの `ProfilerOverlay` に以下の情報を追加:
- カリング統計（total/visible/culled）
- インスタンシングバッチ数
- 個別描画コール数

```cpp
// ProfilerOverlay に追加
void SetSceneRenderStats(const Scene::RenderStats& stats);
```

### 33e: 注意事項

- **pch.h に `<unordered_map>` は未確認** — `std::unordered_map` を使う前に確認。
  なければ `<map>` を使うか、pch.h にないヘッダはサンプル内でインクルード
- **マテリアルオーバーライドとインスタンシングの併用**: 現在の `DrawModelInstanced` は
  マテリアルオーバーライドを考慮しない。同一マテリアルのインスタンスのみグループ化すること
- **スキンドモデル**: Animator のポーズが異なるためインスタンシング不可（ボーン行列が異なる）。
  ただし同一ポーズの場合（パレードアニメ等）は `DrawSkinnedModelInstanced` が使用可能
- **ソート**: 不透明メッシュは front-to-back、半透明は back-to-front でソートすると
  オーバードローを削減できるが、Phase 33 では未実装（将来的な拡張として記録）

---

## Phase 34: 新エンジン機能

### 目的
エンジンの機能的ギャップを埋め、ゲーム開発に必要な汎用機能を追加する。

### 34a: トレイル/リボンレンダラー

#### 概要
剣の軌跡、弾丸の尾、キャラクターの残像等に使う帯状メッシュの動的生成。

#### 新規ファイル
- `GXLib/Graphics/3D/TrailRenderer.h`
- `GXLib/Graphics/3D/TrailRenderer.cpp`
- `Shaders/Trail.hlsl`

#### クラス設計
```cpp
namespace GX
{

struct TrailPoint
{
    XMFLOAT3 position;
    XMFLOAT3 up;           // トレイルの「上」方向（幅の方向）
    float    width;
    Color    color;
    float    time;          // 追加時刻
};

class TrailRenderer
{
public:
    bool Initialize(ID3D12Device* device, uint32_t maxPoints = 256);

    /// @brief 新しいポイントを追加（毎フレーム呼ぶ）
    void AddPoint(const XMFLOAT3& position, const XMFLOAT3& up,
                  float width = 1.0f, const Color& color = {1,1,1,1});

    /// @brief 古いポイントを寿命で削除
    void Update(float deltaTime);

    /// @brief トレイルを描画
    void Draw(ID3D12GraphicsCommandList* cmdList,
              const Camera3D& camera, uint32_t frameIndex);

    /// @brief トレイルをクリア
    void Clear();

    // 設定
    float lifetime = 1.0f;     // ポイントの寿命（秒）
    int   textureHandle = -1;  // テクスチャ（-1=白）
    bool  fadeWithAge = true;   // 古いほど透明に
    BlendMode blendMode = BlendMode::Alpha;

private:
    std::deque<TrailPoint> m_points;
    Buffer m_vertexBuffer;     // 動的 VB (2 vertices per point)
    DynamicBuffer m_cb;
    ComPtr<ID3D12PipelineState> m_pso;
    ComPtr<ID3D12RootSignature> m_rs;
    uint32_t m_maxPoints = 256;
};

} // namespace GX
```

#### シェーダー: Trail.hlsl
```hlsl
// VS: position + width → ビルボード展開（カメラ右方向に展開）
// PS: UV.x = 0..1（幅方向）, UV.y = 0..1（先頭→末尾）→ テクスチャサンプル
// b0: ViewProjection
// t0: テクスチャ
// s0: リニアクランプ
```

#### 実装の注意点
- **std::deque**: pch.h に `<deque>` がない可能性 → `<vector>` + リングバッファで代替
- **HDR 対応**: PSO の RT フォーマットは `R16G16B16A16_FLOAT`
- **深度**: 深度テスト ON、深度書き込み OFF（半透明のため）
- **VB 更新**: DynamicBuffer パターンで毎フレーム頂点データをアップロード

### 34b: スプライン/パスシステム

#### 概要
カメラパス、NPC パトロールルート、ベジェ曲線等の汎用スプラインシステム。

#### 新規ファイル
- `GXLib/Math/Spline.h`
- `GXLib/Math/Spline.cpp`

#### クラス設計
```cpp
namespace GX
{

enum class SplineType { CatmullRom, CubicBezier, Linear };

class Spline
{
public:
    Spline(SplineType type = SplineType::CatmullRom);

    /// @brief 制御点を追加
    void AddPoint(const Vector3& point);
    void SetPoint(int index, const Vector3& point);
    void RemovePoint(int index);
    void Clear();

    /// @brief t (0..1) での位置を取得
    Vector3 Evaluate(float t) const;

    /// @brief t (0..1) での接線を取得
    Vector3 EvaluateTangent(float t) const;

    /// @brief スプラインの総距離を取得（離散近似）
    float GetTotalLength() const;

    /// @brief 距離ベースの位置取得（等速移動用）
    Vector3 EvaluateByDistance(float distance) const;

    /// @brief 制御点数
    int GetPointCount() const;
    const Vector3& GetPoint(int index) const;

    /// @brief デバッグ描画
    void DebugDraw(PrimitiveBatch3D& batch, int segments = 64,
                   uint32_t color = 0xFFFFFF00) const;

private:
    SplineType m_type;
    std::vector<Vector3> m_points;
    mutable float m_cachedLength = -1.0f;
};

/// @brief スプラインに沿ってカメラを自動移動するコントローラー
class SplineCameraController
{
public:
    void SetPath(const Spline& path);
    void SetLookAtPath(const Spline& lookAtPath); // 視点用の別スプライン

    void Play(float duration, bool loop = false);
    void Pause();
    void Stop();

    /// @brief 更新（Camera3D のポジション・ルックアットを自動設定）
    void Update(Camera3D& camera, float deltaTime);

    bool IsPlaying() const;
    float GetProgress() const; // 0..1

private:
    Spline m_path;
    Spline m_lookAtPath;
    float m_duration = 5.0f;
    float m_time = 0.0f;
    bool m_playing = false;
    bool m_loop = false;
};

} // namespace GX
```

#### 実装の注意点
- **Catmull-Rom**: `0.5 * ((2*P1) + (-P0 + P2) * t + (2*P0 - 5*P1 + 4*P2 - P3) * t^2 + (-P0 + 3*P1 - 3*P2 + P3) * t^3)`
- **距離ベース評価**: 事前に等間隔サンプリングして距離テーブルを構築、バイナリサーチで t を逆算
- **PrimitiveBatch3D::DrawLine** でデバッグ描画

### 34c: PrimitiveBatch3D ソリッドシェイプ追加

#### 概要
デバッグ可視化のためのソリッドプリミティブ（球、箱、コーン、カプセル）。

#### 修正ファイル
- `GXLib/Graphics/3D/PrimitiveBatch3D.h`
- `GXLib/Graphics/3D/PrimitiveBatch3D.cpp`

#### 追加 API
```cpp
class PrimitiveBatch3D
{
public:
    // 既存
    void DrawLine(const XMFLOAT3& from, const XMFLOAT3& to, uint32_t color);
    void DrawWireBox(const XMFLOAT3& center, const XMFLOAT3& halfExtents, uint32_t color);
    void DrawWireSphere(const XMFLOAT3& center, float radius, uint32_t color, int segments = 16);
    void DrawGrid(float size, int divisions, uint32_t color);

    // 新規（ソリッド）
    void DrawSolidBox(const XMFLOAT3& center, const XMFLOAT3& halfExtents,
                      uint32_t color, float alpha = 0.5f);
    void DrawSolidSphere(const XMFLOAT3& center, float radius,
                         uint32_t color, float alpha = 0.5f, int segments = 8);
    void DrawCone(const XMFLOAT3& tip, const XMFLOAT3& base, float radius,
                  uint32_t color, float alpha = 0.5f, int segments = 8);
    void DrawCapsule(const XMFLOAT3& p0, const XMFLOAT3& p1, float radius,
                     uint32_t color, float alpha = 0.5f, int segments = 8);
    void DrawFrustum(const XMMATRIX& invViewProj, uint32_t color);

    // ワイヤー追加
    void DrawWireCone(const XMFLOAT3& tip, const XMFLOAT3& base, float radius,
                      uint32_t color, int segments = 8);
    void DrawWireCapsule(const XMFLOAT3& p0, const XMFLOAT3& p1, float radius,
                         uint32_t color, int segments = 8);
};
```

#### 実装の注意点
- **ソリッド描画**: 三角形バッチを使用（ライン専用の既存バッチとは別のフラッシュが必要）
- **PSO**: 深度テスト ON、深度書き込み OFF、アルファブレンド ON
- **バッチサイズ**: ソリッド球 (8 segments) = 128 三角形 = 384 頂点。
  MAX 65536 頂点制限に注意

### 34d: TextRenderer テキストレイアウト強化

#### 概要
テキストの折り返し・アライメント・複数行サポートを追加。

#### 修正ファイル
- `GXLib/Graphics/Rendering/TextRenderer.h`
- `GXLib/Graphics/Rendering/TextRenderer.cpp`

#### 追加 API
```cpp
enum class TextAlign { Left, Center, Right };
enum class TextVAlign { Top, Middle, Bottom };

struct TextLayoutOptions
{
    float maxWidth = 0.0f;          // 0 = 折り返しなし
    float lineSpacing = 1.2f;       // 行間（lineHeight の倍率）
    TextAlign align = TextAlign::Left;
    TextVAlign valign = TextVAlign::Top;
    bool wordWrap = true;           // true=単語単位、false=文字単位
};

class TextRenderer
{
public:
    // 既存
    void DrawString(float x, float y, const wchar_t* text, uint32_t color, int fontHandle);
    int  GetStringWidth(const wchar_t* text, int fontHandle);

    // 新規
    void DrawStringLayout(float x, float y, const wchar_t* text,
                          uint32_t color, int fontHandle,
                          const TextLayoutOptions& options);

    int GetStringHeight(const wchar_t* text, int fontHandle,
                        const TextLayoutOptions& options);

    /// @brief テキストをバウンディングボックス内に描画
    void DrawStringInRect(float x, float y, float w, float h,
                          const wchar_t* text, uint32_t color, int fontHandle,
                          TextAlign align = TextAlign::Left,
                          TextVAlign valign = TextVAlign::Top);
};
```

#### 実装の注意点
- **折り返しアルゴリズム**: 単語区切り（スペース、句読点）でグリーディに行を構築
- **日本語対応**: 日本語には明確な単語区切りがないため、文字単位折り返しも必要
- **行間**: `FontManager::GetLineHeight(fontHandle) * lineSpacing`
- **右揃え/中央揃え**: 各行の描画開始 X を `maxWidth - lineWidth` / 2 でオフセット

### 34e: GPUProfiler 階層スコープ

#### 概要
フラットなスコープリストを階層ツリーに変更し、プロファイリング UI を改善。

#### 修正ファイル
- `GXLib/Graphics/Device/GPUProfiler.h`
- `GXLib/Graphics/Device/GPUProfiler.cpp`

#### 追加 API
```cpp
struct ScopeResult
{
    const char* name;
    float timeMs;
    int depth;              // 追加: ネスト深度 (0 = トップレベル)
    int parentIndex;        // 追加: 親スコープのインデックス (-1 = ルート)
    std::vector<int> children; // 追加: 子スコープのインデックス
};
```

#### 実装の注意点
- **スタックベースのネスト追跡**: `BeginScope()` でスタックに push、`EndScope()` で pop
- **depth** と **parentIndex** を `ScopeResult` に付与
- **ProfilerOverlay** のツリー表示: インデントで階層を表現
  ```
  Frame: 16.67ms
  ├─ ShadowPass: 2.1ms
  │  ├─ Cascade0: 0.5ms
  │  ├─ Cascade1: 0.6ms
  │  ├─ Cascade2: 0.5ms
  │  └─ Cascade3: 0.5ms
  ├─ ScenePass: 8.2ms
  │  ├─ OpaqueModels: 6.1ms
  │  └─ Skybox: 2.1ms
  └─ PostEffects: 5.3ms
     ├─ SSAO: 1.2ms
     ├─ Bloom: 0.8ms
     └─ Tonemapping: 0.3ms
  ```

### 34f: ShaderLibrary インクルード依存追跡

#### 概要
`.hlsli` ファイルの変更で依存する `.hlsl` を自動リコンパイル。

#### 修正ファイル
- `GXLib/Graphics/Pipeline/ShaderLibrary.h`
- `GXLib/Graphics/Pipeline/ShaderLibrary.cpp`

#### 実装方針
```cpp
class ShaderLibrary
{
    // 既存
    std::unordered_map<ShaderKey, ComPtr<IDxcBlob>, ShaderKeyHasher> m_cache;

    // 追加: include 依存グラフ
    std::unordered_map<std::string, std::vector<std::string>> m_includeDependencies;
    // key = normalized path of .hlsli
    // value = list of .hlsl files that include it

    void TrackDependency(const std::string& hlslFile, const std::string& includeFile);

    // InvalidateFile を拡張
    void InvalidateFile(const std::string& filePath)
    {
        // 直接のシェーダーを無効化
        InvalidateDirect(filePath);

        // include 依存の逆引き: この .hlsli を含む全 .hlsl も無効化
        auto it = m_includeDependencies.find(Normalize(filePath));
        if (it != m_includeDependencies.end())
        {
            for (const auto& dependent : it->second)
            {
                InvalidateDirect(dependent);
            }
        }
    }
};
```

#### 依存情報の収集方法
- **方法 A**: DXC コンパイル時に `-Fd` フラグでデバッグ情報を出力し、`#include` を解析
- **方法 B**: シンプルにシェーダーソースを正規表現で `#include "..."` をスキャンし、
  コンパイル成功時に依存マップに登録
  ```cpp
  // 方法 B の実装（推奨 — DXC 依存なし）
  void ShaderLibrary::ScanIncludes(const std::string& hlslPath)
  {
      std::ifstream file(hlslPath);
      std::string line;
      while (std::getline(file, line))
      {
          // #include "Foo.hlsli" パターンをマッチ
          auto pos = line.find("#include");
          if (pos != std::string::npos)
          {
              auto q1 = line.find('"', pos);
              auto q2 = line.find('"', q1 + 1);
              if (q1 != std::string::npos && q2 != std::string::npos)
              {
                  std::string includeName = line.substr(q1 + 1, q2 - q1 - 1);
                  TrackDependency(hlslPath, includeName);
              }
          }
      }
  }
  ```

### 34g: Camera3D LookAt メソッド追加

#### 概要
`Camera3D::LookAt(target)` を追加して、複数サンプルの `LookAtCamera` ヘルパーを不要にする。

#### 修正ファイル
- `GXLib/Graphics/3D/Camera3D.h`
- `GXLib/Graphics/3D/Camera3D.cpp`

```cpp
void Camera3D::LookAt(const XMFLOAT3& target)
{
    auto pos = GetPosition();
    float dx = target.x - pos.x;
    float dy = target.y - pos.y;
    float dz = target.z - pos.z;
    float dist = std::sqrt(dx * dx + dz * dz);
    SetPitch(-std::atan2(dy, dist));
    SetYaw(std::atan2(dx, dz));
}
```

### 34h: Scene::Render 統計 & デバッグオーバーレイ

#### 概要
Phase 33c のカリング統計を ProfilerOverlay に表示し、デバッグビジュアルを追加。

#### ProfilerOverlay に追加
```cpp
// Detailed モードに追加:
// "Entities: 150/200 (50 culled, 12 instanced batches)"
// "DrawCalls: 38 (instanced: 12, individual: 26)"
```

#### デバッグ描画オプション
```cpp
// Scene に追加
void Scene::SetDebugDrawFlags(uint32_t flags);

enum SceneDebugFlags : uint32_t
{
    SceneDebug_None             = 0,
    SceneDebug_BoundingSpheres  = 1 << 0,
    SceneDebug_AABBs            = 1 << 1,
    SceneDebug_Frustum          = 1 << 2,
    SceneDebug_LODLevels        = 1 << 3,  // LODレベルを色で表示
};
```

---

## 共通ルール（全 Phase 共通）

### コーディング規約
- namespace `GX` 内に配置
- `#pragma once` + Doxygen `/// @file` / `/// @brief`
- `#include "pch.h"` を .cpp の先頭に
- DirectXMath は `using namespace DirectX;` を .cpp 内のみ
- XMFLOAT3/4/4X4 はメンバー格納用、XMVECTOR/XMMATRIX は演算用
- ComPtr で COM オブジェクト管理
- `std::unique_ptr` で所有権管理
- `constexpr` で定数定義
- エラーログは `GX::Logger::Error()`、情報ログは `GX::Logger::Info()`
- No CD3DX12 helpers — raw D3D12 structs を使用
- C++ に saturate() はない — `std::max(0.0f, std::min(1.0f, x))` を使用
- pch.h に `<sstream>`, `<unordered_set>` は **ない** — 代替を使用

### ビルド手順（各 Phase 完了時）
```bash
cmake -B build -S .
cmake --build build --config Debug 2>&1
# エラーゼロを確認
```

### 新規ディレクトリの CMake 対応
- `GXLib/Core/*.cpp`, `GXLib/Graphics/*.cpp` 等の既存 GLOB_RECURSE パターンでサブディレクトリは自動収集
- `GXLib/AI/*.cpp` は Phase 29 で追加済み
- 新しいサンプルは `gxlib_add_sample(NAME SampleName)` で追加

### テスト方法
- ビルド通過確認
- 新規サンプル .exe の起動確認（クラッシュしないこと）
- 既存サンプル（17 個）が壊れていないこと:
  - EasyHello, Shooting2D, Platformer2D, Walkthrough3D, GUIMenuDemo
  - PostEffectShowcase, DXRShowcase, Particle2DShowcase, ParticleShowcase
  - IBLShowcase, InstanceShowcase, IKShowcase, Audio3DShowcase
  - ActionMappingShowcase, GPUParticleShowcase, NavmeshShowcase, SceneShowcase
- GXModelViewer が起動すること

### MEMORY.md 更新
各 Phase 完了後に MEMORY.md の Completed Phases セクションと Common Issues を更新すること。

---

## 実装優先順序

```
Phase 31 (安定化) ← 全ての前提
    ↓
┌──────────────────┐
│ 並列着手可能:      │
│ Phase 32 (Viewer統合) │
│ Phase 33 (最適化)     │
│ Phase 34 (新機能)     │
└──────────────────┘
```

Phase 34 内部の推奨順序:
1. 34g (Camera3D::LookAt) — 最も軽量、すぐに効果
2. 34c (PrimitiveBatch3D ソリッド) — デバッグに即使用可能
3. 34d (TextRenderer レイアウト) — GUI 改善に直結
4. 34e (GPUProfiler 階層) — デバッグ品質向上
5. 34f (ShaderLibrary include) — 開発ワークフロー改善
6. 34b (Spline) — ゲームプレイ向け
7. 34a (TrailRenderer) — VFX 向け
8. 34h (Scene デバッグオーバーレイ) — Phase 33 完了後

---

## 検証チェックリスト

各 Phase 完了時に以下を確認:

- [ ] `cmake -B build -S . && cmake --build build --config Debug` エラーゼロ
- [ ] 新規サンプルが起動してクラッシュしない
- [ ] 既存 17 サンプルが壊れていない
- [ ] GXModelViewer が起動する
- [ ] gxconv / gxpak がビルドできる
- [ ] D3D12 デバッグレイヤーでエラーなし
- [ ] MEMORY.md が更新されている

---

## 既知の技術的制約

| 制約 | 影響 | 回避策 |
|------|------|--------|
| pch.h に `<sstream>` なし | std::stringstream 使用不可 | std::format/std::to_string 使用 |
| pch.h に `<unordered_set>` なし | unordered_set 使用不可 | vector + sort + unique 等 |
| pch.h に `<deque>` なし | std::deque 使用不可 | vector + リングバッファ |
| pch.h に `<filesystem>` なし | std::filesystem 使用不可 | Win32 API or `<cstdio>` std::remove |
| Windows.h min/max マクロ | std::min/max と衝突 | `(std::max)(...)` パターン |
| FormatT by-value 引数 | Args&& 使用不可 | P2905R2/MSVC14.44 制約 |
| ImTextureID = ImU64 | ポインタ直接渡し不可 | `static_cast<ImTextureID>(handle.ptr)` |
| std::vector<bool> proxy | ImGui::Checkbox 互換なし | ローカル bool にコピー |
| D3D12 Root SRV | Texture2D.Sample() 不可 | shader-visible ヒープ使用 |
| ID3D12GraphicsCommandList4 | DXR 関数に必要 | pch.h で ID3D12Device5 と共にキャスト |

---

## ファイル一覧（全 Phase）

### Phase 31: 修正対象
```
Samples/Audio3DShowcase/main.cpp          — WAV リーク修正
Samples/PostEffectShowcase/main.cpp       — DepthBuffer 遷移追加
Samples/**/main.cpp                       — std::min/max 統一
GXLib/Compat/GXEasy.h                     — FormatT, FPSCameraController
GXLib/Graphics/Rendering/ParticleSystem2D.h — コメント修正
GXLib/Core/Scene/Entity.h                 — GetComponent constexpr 最適化
```

### Phase 32: 修正/新規
```
GXLib/Core/Scene/Components.h             — MeshRendererComponent 拡張
GXModelViewer/Scene/EditorMetadata.h      — 新規
GXModelViewer/Scene/SceneGraph.h          — GX::Scene ラッパー化
GXModelViewer/Scene/SceneGraph.cpp        — 〃
GXModelViewer/Panels/SceneHierarchyPanel.*
GXModelViewer/Panels/PropertyPanel.*
GXModelViewer/Panels/TimelinePanel.*
GXModelViewer/Panels/AnimatorPanel.*
GXModelViewer/Panels/BlendTreeEditor.*
GXModelViewer/Panels/ModelInfoPanel.*
GXModelViewer/Panels/SkeletonPanel.*
GXModelViewer/Panels/AssetBrowserPanel.*
GXModelViewer/Scene/SceneSerializer.*     — GX::SceneSerializer ベースに
```

### Phase 33: 修正/新規
```
GXLib/Core/Scene/Entity.h                 — BoundsInfo 追加
GXLib/Core/Scene/Entity.cpp               — GetWorldBoundingSphere 実装
GXLib/Core/Scene/Scene.h                  — Render(renderer, camera) オーバーロード
GXLib/Core/Scene/Scene.cpp                — カリング+バッチング実装
GXLib/Core/ProfilerOverlay.*              — RenderStats 表示
```

### Phase 34: 新規/修正
```
GXLib/Graphics/3D/TrailRenderer.h         — 新規
GXLib/Graphics/3D/TrailRenderer.cpp       — 新規
Shaders/Trail.hlsl                        — 新規
GXLib/Math/Spline.h                       — 新規
GXLib/Math/Spline.cpp                     — 新規
GXLib/Graphics/3D/PrimitiveBatch3D.h      — ソリッドシェイプ追加
GXLib/Graphics/3D/PrimitiveBatch3D.cpp    — 〃
GXLib/Graphics/Rendering/TextRenderer.h   — レイアウト機能追加
GXLib/Graphics/Rendering/TextRenderer.cpp — 〃
GXLib/Graphics/Device/GPUProfiler.h       — 階層スコープ
GXLib/Graphics/Device/GPUProfiler.cpp     — 〃
GXLib/Graphics/Pipeline/ShaderLibrary.h   — include 依存追跡
GXLib/Graphics/Pipeline/ShaderLibrary.cpp — 〃
GXLib/Graphics/3D/Camera3D.h              — LookAt 追加
GXLib/Graphics/3D/Camera3D.cpp            — 〃
```
