# Phase 37: テスト強化 — 完全実装指令書

## Context

Phase 36（バグ修正）完了済み。現在 **151 テスト** が 11 ファイルに存在し、全パス。
本 Phase では GPU を使用しない純粋ロジックテストを **10 ファイル新規追加** し、
カバレッジを約 **260+ テスト** まで拡大する。

**重要**: この指令書は、GXLib プロジェクトの事前知識がない Claude インスタンスでも
単独で Phase 37 を完遂できるよう、全テストケースのコード骨格まで記載している。

---

## 前提条件

- **プロジェクトルート**: `C:\Users\g0190\Desktop\GXLib`
- **テストディレクトリ**: `C:\Users\g0190\Desktop\GXLib\Tests\`
- **テストフレームワーク**: Google Test v1.15.2 (FetchContent)
- **ビルド**: `cmake -B build -S .` → `cmake --build build --config Debug`
- **テスト実行**: `ctest --test-dir build --build-config Debug --output-on-failure`
- **pch.h**: GXLib 本体と共有（Windows.h, D3D12, DirectXMath, STL 含む）
- **pch.h に無いヘッダ**: `<sstream>`, `<unordered_set>` — 使用禁止
- **名前空間**: `using namespace GX;`（全テストファイル共通）

---

## 既存テスト一覧（11 ファイル / 151 テスト）

| ファイル | テスト数 | 対象 |
|---------|---------|------|
| test_main.cpp | 0 | エントリーポイント（gtest_main 使用） |
| test_Vector.cpp | 29 | Vector2/3/4 |
| test_Matrix.cpp | 11 | Matrix4x4 |
| test_Quaternion.cpp | 10 | Quaternion |
| test_Color.cpp | 10 | Color |
| test_MathUtil.cpp | 19 | MathUtil + Random |
| test_Collision2D.cpp | 19 | 2D 衝突判定 |
| test_Collision3D.cpp | 24 | 3D 衝突判定 |
| test_Spatial.cpp | 13 | Quadtree/Octree/BVH |
| test_Crypto.cpp | 6 | AES-256-CBC, SHA-256 |
| test_Allocator.cpp | 10 | PoolAllocator, FrameAllocator |

---

## テストファイルの標準パターン

```cpp
/// @file test_Xxx.cpp
/// @brief Xxx 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Path/To/Header.h"

using namespace GX;

TEST(XxxTest, MethodName_Condition)
{
    // Arrange
    // Act
    // Assert
    EXPECT_NEAR(actual, expected, 1e-5f);  // 浮動小数点
    EXPECT_EQ(actual, expected);            // 整数/bool
    EXPECT_TRUE(condition);
    EXPECT_FALSE(condition);
}
```

**命名規則:**
- ファイル名: `test_<ComponentName>.cpp`
- テストスイート名: `<ComponentName>Test`
- テストケース名: `<Method>_<Condition>` （例: `Evaluate_AtStart`）
- 浮動小数点の許容誤差: `1e-5f`（特に指定がなければ）
- `std::min/std::max` は `(std::min)(...)` パターンで呼ぶこと（NOMINMAX 対応）

---

## 新規テストファイル一覧（10 ファイル）

| # | ファイル | テスト対象 | 予想テスト数 |
|---|---------|----------|------------|
| 1 | test_Spline.cpp | Spline (3 補間タイプ) | ~18 |
| 2 | test_Transform3D.cpp | Transform3D | ~10 |
| 3 | test_Entity.cpp | Entity + Scene | ~20 |
| 4 | test_SceneSerializer.cpp | SceneSerializer JSON | ~8 |
| 5 | test_ActionMapping.cpp | ActionMapping | ~8 |
| 6 | test_NavMesh.cpp | NavMesh + NavAgent | ~14 |
| 7 | test_LODGroup.cpp | LODGroup | ~8 |
| 8 | test_Animation.cpp | AnimationClip + Skeleton | ~12 |
| 9 | test_StyleSheet.cpp | StyleSheet CSS パース | ~14 |
| 10 | test_FileSystem.cpp | VFS FileSystem | ~10 |

**合計: ~122 新規テスト → 既存 151 + 新規 122 = ~273 テスト**

---

## テスト 1: test_Spline.cpp

### ヘッダ

```cpp
/// @file test_Spline.cpp
/// @brief Spline 単体テスト — Linear/CatmullRom/CubicBezier 補間、弧長パラメータ化

#include "pch.h"
#include <gtest/gtest.h>
#include "Math/Spline.h"
#include "Math/Vector3.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Math/Spline.h
実装:   GXLib/Math/Spline.cpp

enum class SplineType { Linear, CatmullRom, CubicBezier };

class Spline {
    void AddPoint(const Vector3& point);
    void InsertPoint(int index, const Vector3& point);
    void RemovePoint(int index);
    void SetPoint(int index, const Vector3& point);
    const Vector3& GetPoint(int index) const;
    int GetPointCount() const;
    void Clear();
    void SetType(SplineType type);
    SplineType GetType() const;
    void SetClosed(bool closed);
    bool IsClosed() const;
    Vector3 Evaluate(float t) const;        // t ∈ [0, 1]
    Vector3 EvaluateTangent(float t) const;
    float GetTotalLength(int subdivisions = 64) const;
    Vector3 EvaluateByDistance(float distance, int subdivisions = 64) const;
    float FindClosestParameter(const Vector3& point, int subdivisions = 64) const;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `PointManagement_AddAndGet` | AddPoint 3 点 → GetPointCount()==3, GetPoint(i) が正しい |
| 2 | `PointManagement_InsertAndRemove` | InsertPoint(1, ...) → GetPointCount() 増加、RemovePoint(1) → 元に戻る |
| 3 | `PointManagement_Clear` | Clear() → GetPointCount()==0 |
| 4 | `SetType_Changes` | SetType(Linear) → GetType()==Linear |
| 5 | `SetClosed_Changes` | SetClosed(true) → IsClosed()==true |
| 6 | `Linear_Evaluate_Start` | 2 点 Linear、t=0 → 始点 |
| 7 | `Linear_Evaluate_End` | 2 点 Linear、t=1 → 終点 |
| 8 | `Linear_Evaluate_Mid` | 2 点 (0,0,0)→(10,0,0)、t=0.5 → (5,0,0) |
| 9 | `CatmullRom_Endpoints` | 4 点 CatmullRom、t=0 ≈ 始点、t=1 ≈ 終点 |
| 10 | `CatmullRom_Smooth` | 中間点が Linear とは異なる（曲線であること） |
| 11 | `CubicBezier_Endpoints` | 4 点 CubicBezier、t=0 = p0、t=1 = p3 |
| 12 | `GetTotalLength_TwoPoints` | (0,0,0)→(10,0,0) の長さ ≈ 10.0 |
| 13 | `GetTotalLength_Empty` | 点なし → 長さ 0 |
| 14 | `EvaluateByDistance_Zero` | distance=0 → 始点 |
| 15 | `EvaluateByDistance_Full` | distance=totalLength → 終点付近 |
| 16 | `EvaluateByDistance_Monotonic` | distance 増加 → x 座標も増加（直線上） |
| 17 | `FindClosestParameter` | 制御点上の点 → t ≈ 期待値 |
| 18 | `Closed_EvaluateWraps` | SetClosed(true)、t=0 と t=1 がほぼ同じ位置 |

### 実装上の注意

- `GetPoint()` は境界チェックなし（テストではインデックス範囲内のみアクセス）
- CubicBezier は 4 の倍数の制御点が必要（4 点 = 1 セグメント）
- GetTotalLength の subdivisions=64 はデフォルトのまま使用
- 浮動小数点比較は EXPECT_NEAR(a, b, **0.1f**) 程度のゆるい許容誤差でOK
  （弧長パラメータ化は近似のため）

---

## テスト 2: test_Transform3D.cpp

### ヘッダ

```cpp
/// @file test_Transform3D.cpp
/// @brief Transform3D 単体テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Transform3D.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Graphics/3D/Transform3D.h

class Transform3D {
    void SetPosition(float x, float y, float z);
    void SetPosition(const XMFLOAT3& pos);
    void SetRotation(float pitch, float yaw, float roll);  // radians
    void SetRotation(const XMFLOAT3& rot);
    void SetScale(float x, float y, float z);
    void SetScale(float uniform);
    void SetScale(const XMFLOAT3& s);
    const XMFLOAT3& GetPosition() const;
    const XMFLOAT3& GetRotation() const;
    const XMFLOAT3& GetScale() const;
    XMMATRIX GetWorldMatrix() const;
    XMMATRIX GetWorldInverseTranspose() const;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `DefaultState` | デフォルト: pos=(0,0,0), rot=(0,0,0), scale=(1,1,1) |
| 2 | `SetPosition_Float3` | SetPosition(1,2,3) → GetPosition()==(1,2,3) |
| 3 | `SetPosition_XMFLOAT3` | SetPosition(XMFLOAT3{4,5,6}) → GetPosition() |
| 4 | `SetRotation_Radians` | SetRotation(0.1f, 0.2f, 0.3f) → GetRotation() |
| 5 | `SetScale_Uniform` | SetScale(2.0f) → GetScale()==(2,2,2) |
| 6 | `SetScale_NonUniform` | SetScale(1,2,3) → GetScale()==(1,2,3) |
| 7 | `WorldMatrix_IdentityTransform` | デフォルト Transform → 単位行列 |
| 8 | `WorldMatrix_TranslationOnly` | pos=(5,0,0) → 行列の _41=5 |
| 9 | `WorldMatrix_ScaleOnly` | scale=(2,2,2) → 行列の対角=2 |
| 10 | `WorldInverseTranspose_Exists` | 結果が有効な行列であること（NaN なし） |

### 実装上の注意

- `GetWorldMatrix()` は XMMATRIX を返すので、XMStoreFloat4x4 で XMFLOAT4X4 に変換して検証
- 行列の要素アクセスは `._11`, `._41` 等（DirectXMath の行ベクトル規約）
- 回転はオイラー角（ラジアン）で設定、内部で SRT 順（Scale → Rotate → Translate）

---

## テスト 3: test_Entity.cpp

### ヘッダ

```cpp
/// @file test_Entity.cpp
/// @brief Entity / Scene 単体テスト — コンポーネント管理、親子階層、シーン操作

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Entity.h"
#include "Core/Scene/Scene.h"
#include "Core/Scene/Components.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Core/Scene/Entity.h, Scene.h, Components.h

enum class ComponentType : uint32_t {
    Transform, MeshRenderer, SkinnedMeshRenderer, Camera,
    Light, ParticleSystem, AudioSource, Terrain,
    RigidBody, LOD, Script, Custom, _Count
};

class Entity {
    Entity(const std::string& name = "Entity");
    const std::string& GetName() const;
    void SetName(const std::string& name);
    void SetParent(Entity* parent);
    Entity* GetParent() const;
    const std::vector<Entity*>& GetChildren() const;
    Transform3D& GetTransform();
    template<typename T> T* AddComponent();
    template<typename T> T* GetComponent() const;
    template<typename T> bool HasComponent() const;
    template<typename T> void RemoveComponent();
    bool IsActive() const;
    void SetActive(bool active);
    uint32_t GetID() const;
    void SetID(uint32_t id);
};

class Scene {
    Scene(const std::string& name = "Untitled");
    Entity* CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity* entity);
    Entity* FindEntity(const std::string& name) const;
    Entity* FindEntityByID(uint32_t id) const;
    const std::vector<std::unique_ptr<Entity>>& GetEntities() const;
    const std::vector<Entity*>& GetRootEntities() const;
    void Update(float deltaTime);
    const std::string& GetName() const;
    void SetName(const std::string& name);
    uint32_t GetEntityCount() const;
};

// Components（全て Component 基底クラスを継承、static constexpr k_Type 持ち）
struct MeshRendererComponent;      // k_Type = ComponentType::MeshRenderer
struct SkinnedMeshRendererComponent; // k_Type = ComponentType::SkinnedMeshRenderer
struct CameraComponent;            // k_Type = ComponentType::Camera
struct LightComponent;             // k_Type = ComponentType::Light
struct AudioSourceComponent;       // k_Type = ComponentType::AudioSource
struct ScriptComponent;            // k_Type = ComponentType::Script
struct LODComponent;               // k_Type = ComponentType::LOD
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `Scene_CreateEntity_NotNull` | CreateEntity → non-null |
| 2 | `Scene_CreateEntity_Name` | CreateEntity("Player") → GetName()=="Player" |
| 3 | `Scene_CreateEntity_UniqueID` | 2 つの Entity の GetID() が異なる |
| 4 | `Scene_GetEntityCount` | 3 個作成 → GetEntityCount()==3 |
| 5 | `Scene_FindEntityByName` | FindEntity("Player") → 正しい Entity* |
| 6 | `Scene_FindEntityByName_NotFound` | FindEntity("NoExist") → nullptr |
| 7 | `Scene_FindEntityByID` | FindEntityByID(id) → 正しい Entity* |
| 8 | `Scene_DestroyEntity` | DestroyEntity + Update → FindEntity==nullptr |
| 9 | `Scene_RootEntities` | 親なし Entity が GetRootEntities に含まれる |
| 10 | `Scene_SetName` | SetName("MyScene") → GetName()=="MyScene" |
| 11 | `Scene_Update_NoCrash` | Update(0.016f) がクラッシュしない |
| 12 | `Entity_DefaultTransform` | GetTransform().GetPosition() == (0,0,0) |
| 13 | `Entity_AddComponent_Camera` | AddComponent<CameraComponent>() → non-null |
| 14 | `Entity_GetComponent_Camera` | AddComponent 後に GetComponent → 同じポインタ |
| 15 | `Entity_GetComponent_NotAdded` | GetComponent<LightComponent>() → nullptr |
| 16 | `Entity_HasComponent` | AddComponent 後 HasComponent==true |
| 17 | `Entity_RemoveComponent` | RemoveComponent 後 HasComponent==false |
| 18 | `Entity_SetParent` | e2.SetParent(&e1) → e2.GetParent()==&e1, e1.GetChildren() に e2 |
| 19 | `Entity_RemoveParent` | SetParent(nullptr) → GetParent()==nullptr |
| 20 | `Entity_ActiveState` | SetActive(false) → IsActive()==false |

### 実装上の注意

- **Entity は Scene 経由で作成**（Scene が unique_ptr で所有）
- DestroyEntity は遅延削除 → Update() 呼び出し後にチェック
- `MeshRendererComponent` の model は nullptr のままテスト
- `ScriptComponent` は onUpdate/onStart/onDestroy に std::function を設定可能
  → コールバックの呼び出し確認テストも可能だが、基本テストでは省略
- Entity のコンストラクタは直接使用せず、Scene::CreateEntity() 経由が安全

---

## テスト 4: test_SceneSerializer.cpp

### ヘッダ

```cpp
/// @file test_SceneSerializer.cpp
/// @brief SceneSerializer JSON ラウンドトリップテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Core/Scene/Scene.h"
#include "Core/Scene/SceneSerializer.h"
#include "Core/Scene/Components.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Core/Scene/SceneSerializer.h

class SceneSerializer {
    using ModelLoadCallback = std::function<Model*(const std::string& path)>;
    static bool SaveToJson(const Scene& scene, const std::string& filePath);
    static bool LoadFromJson(Scene& scene, const std::string& filePath,
                             ModelLoadCallback modelLoader = nullptr);
    static std::string ToJsonString(const Scene& scene);
    static bool FromJsonString(Scene& scene, const std::string& json,
                               ModelLoadCallback modelLoader = nullptr);
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `ToJsonString_EmptyScene` | 空シーン → 有効な JSON 文字列（`{` で始まる） |
| 2 | `RoundTrip_EmptyScene` | ToJsonString → FromJsonString → エンティティ数 0 |
| 3 | `RoundTrip_SingleEntity` | 名前 "Box" の Entity → 復元後に名前一致 |
| 4 | `RoundTrip_EntityTransform` | Position(1,2,3) 設定 → 復元後に Position 一致 |
| 5 | `RoundTrip_Hierarchy` | 親子関係 → 復元後に GetParent が一致 |
| 6 | `RoundTrip_CameraComponent` | CameraComponent 付き → 復元後に HasComponent |
| 7 | `RoundTrip_LightComponent` | LightComponent 付き → 復元後に HasComponent |
| 8 | `FromJsonString_InvalidJson` | 不正 JSON → false を返す |

### 実装上の注意

- ファイル I/O テスト (SaveToJson/LoadFromJson) は一時ファイルを使用するか、
  文字列ベースの ToJsonString/FromJsonString のみでテスト（推奨）
- ModelLoadCallback には nullptr を渡す（モデルの読み込みテストは不要）
- シリアライズ対象外のコンポーネント（LOD, Terrain 等）はテストしない
- JSON フォーマットは nlohmann/json（GXLib/ThirdParty/json.hpp）
- Transform の復元精度は EXPECT_NEAR(..., 1e-3f) で検証

---

## テスト 5: test_ActionMapping.cpp

### ヘッダ

```cpp
/// @file test_ActionMapping.cpp
/// @brief ActionMapping 構造テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Input/ActionMapping.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Input/ActionMapping.h

enum class InputBindingType { KeyboardKey, MouseButton, GamepadButton, GamepadAxis, MouseAxis };

struct InputBinding {
    InputBindingType type;
    int keyCode; float deadZone; float scale; int padIndex;
    static InputBinding Key(int vk);
    static InputBinding MouseBtn(int btn);
    static InputBinding PadBtn(int btn, int pad = 0);
    static InputBinding PadAxis(GamepadAxisId, float, float, int);
    static InputBinding KeyAxis(int vk, float s);
};

struct ActionState {
    bool pressed, triggered, released;
    float value;
};

class ActionMapping {
    void DefineAction(const std::string& name, const std::vector<InputBinding>& bindings);
    void RemoveAction(const std::string& name);
    const ActionState& GetAction(const std::string& name) const;
    bool IsActionPressed(const std::string& name) const;
    bool IsActionTriggered(const std::string& name) const;
    bool IsActionReleased(const std::string& name) const;
    float GetActionValue(const std::string& name) const;
    void Clear();
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `DefineAction_Exists` | DefineAction → GetAction が空でない ActionState を返す |
| 2 | `GetActionValue_Undefined` | 未定義アクション → 0.0f |
| 3 | `IsActionPressed_Undefined` | 未定義アクション → false |
| 4 | `RemoveAction_Removes` | RemoveAction → GetActionValue == 0.0f |
| 5 | `Clear_RemovesAll` | 3 つ定義 → Clear → 全て 0.0f |
| 6 | `KeyBinding_Factory` | InputBinding::Key(VK_SPACE) → type==KeyboardKey, keyCode==VK_SPACE |
| 7 | `KeyAxisBinding_Factory` | InputBinding::KeyAxis('W', 1.0f) → type==KeyboardKey, scale==1.0f |
| 8 | `PadBtnBinding_Factory` | InputBinding::PadBtn(0, 1) → type==GamepadButton, padIndex==1 |

### 実装上の注意

- `Update()` には Keyboard, Mouse, Gamepad の実インスタンスが必要なのでテスト不可
  → 構造テスト（定義/削除/初期値）に限定
- `s_emptyState` が GetAction() の未定義アクション用フォールバック
- InputBinding のファクトリメソッドは純粋な値構築なので完全にテスト可能

---

## テスト 6: test_NavMesh.cpp

### ヘッダ

```cpp
/// @file test_NavMesh.cpp
/// @brief NavMesh A* パス検索 + NavAgent テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "AI/NavMesh.h"
#include "AI/NavAgent.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/AI/NavMesh.h, NavAgent.h

class NavMesh {
    bool Build(float minX, float minZ, float maxX, float maxZ,
               float cellSize = 0.5f, float maxClimb = 0.9f, float maxSlope = 45.0f);
    void SetCellWalkable(int cellX, int cellZ, bool walkable);
    void SetCellCost(int cellX, int cellZ, float cost);
    bool FindPath(const XMFLOAT3& start, const XMFLOAT3& end,
                  std::vector<XMFLOAT3>& outPath) const;
    bool FindNearestWalkable(const XMFLOAT3& pos, XMFLOAT3& outPos) const;
    bool IsWalkable(const XMFLOAT3& pos) const;
    int GetGridWidth() const;
    int GetGridHeight() const;
    float GetCellSize() const;
    bool IsBuilt() const;
};

class NavAgent {
    void Initialize(NavMesh* mesh);
    void SetDestination(const XMFLOAT3& dest);
    void Update(float dt);
    void Stop();
    XMFLOAT3 GetPosition() const;
    void SetPosition(const XMFLOAT3& pos);
    float GetYaw() const;
    bool HasPath() const;
    bool HasReachedDestination() const;
    const std::vector<XMFLOAT3>& GetPath() const;
    int GetCurrentWaypointIndex() const;
    // Public members:
    float speed = 3.5f;
    float angularSpeed = 360.0f;
    float stoppingDistance = 0.15f;
    float height = 0.0f;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `Build_FlatGrid` | Build(0,0, 10,10, 0.5) → IsBuilt()==true |
| 2 | `Build_GridDimensions` | GetGridWidth()/GetGridHeight() が正しい |
| 3 | `IsWalkable_Default` | Build 直後 → 全セルが walkable |
| 4 | `SetCellWalkable_False` | SetCellWalkable(5,5,false) → IsWalkable({2.5,0,2.5})==false |
| 5 | `FindPath_StraightLine` | 障害物なし → パス発見、waypoints > 0 |
| 6 | `FindPath_AroundObstacle` | 中央に壁 → パスが壁を迂回 |
| 7 | `FindPath_NoPath` | 目的地を壁で完全包囲 → false |
| 8 | `FindPath_SamePoint` | start==end → パスが空 or 1 点 |
| 9 | `FindNearestWalkable` | 非 walkable 位置 → 近隣の walkable 位置を返す |
| 10 | `NavAgent_Initialize` | Initialize → HasPath()==false, HasReachedDestination()==false |
| 11 | `NavAgent_SetDestination` | SetDestination → HasPath()==true |
| 12 | `NavAgent_Update_Moves` | Update(1.0f) → GetPosition() が変化 |
| 13 | `NavAgent_HasReached` | 目的地近くに配置 → Update 後に HasReachedDestination()==true |
| 14 | `NavAgent_Stop` | Stop() → HasPath()==false |

### 実装上の注意

- Build の座標系: minX, minZ, maxX, maxZ（Y は高さ、パス検索では XZ 平面）
- セルインデックス: `cellX = (worldX - minX) / cellSize`
- NavAgent のテストでは Build 後の NavMesh を使用
- NavAgent::Update は deltaTime でスケールされる（dt=1.0f で speed=3.5 ユニット移動）
- NavAgent::HasReachedDestination は stoppingDistance(0.15f) 以内で true
- 「目的地近くに配置」テスト: SetPosition を目的地の stoppingDistance 以内に設定

---

## テスト 7: test_LODGroup.cpp

### ヘッダ

```cpp
/// @file test_LODGroup.cpp
/// @brief LODGroup LOD 選択ロジックテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/LODGroup.h"
#include "Graphics/3D/Camera3D.h"
#include "Graphics/3D/Transform3D.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Graphics/3D/LODGroup.h

struct LODLevel {
    Model* model = nullptr;
    float screenPercentage = 1.0f;
};

class LODGroup {
    void AddLevel(Model* model, float screenPercentage);
    int GetLevelCount() const;
    const LODLevel& GetLevel(int index) const;
    void Clear();
    Model* SelectLOD(const Camera3D& camera, const Transform3D& transform,
                     float boundingRadius) const;
    void SetCullDistance(float distance);
    float GetCullDistance() const;
    // static constexpr float k_Hysteresis = 0.05f;
    // mutable int m_lastSelectedLevel = 0;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `Empty_ReturnsNull` | LOD なし → SelectLOD==nullptr |
| 2 | `AddLevel_Count` | 3 レベル追加 → GetLevelCount()==3 |
| 3 | `AddLevel_SortedByScreenPct` | 順不同で追加 → GetLevel(0).screenPercentage が最大 |
| 4 | `Clear_RemovesAll` | Clear → GetLevelCount()==0 |
| 5 | `SelectLOD_Close` | カメラ近距離 → LOD 0 (高詳細) の Model* |
| 6 | `SelectLOD_Far` | カメラ遠距離 → 最低 LOD の Model* |
| 7 | `SelectLOD_CullDistance` | SetCullDistance(50), カメラ 100m → nullptr |
| 8 | `SelectLOD_NoCullDistance` | SetCullDistance(0) → カリングなし |

### 実装上の注意

- **Model* は nullptr で OK**（SelectLOD 内で Model を deref しない、ただ返すだけ）
- テスト用に区別できるダミーポインタを使用:
  ```cpp
  Model* dummyLOD0 = reinterpret_cast<Model*>(0x1);
  Model* dummyLOD1 = reinterpret_cast<Model*>(0x2);
  Model* dummyLOD2 = reinterpret_cast<Model*>(0x3);
  ```
- Camera3D は SetPerspective() と SetPosition() で初期化が必要
- SelectLOD 内部で `camera.GetPosition()` と `camera.GetFovY()` を使用
- Camera3D の初期化:
  ```cpp
  Camera3D camera;
  camera.SetPerspective(XM_PIDIV4, 16.0f/9.0f, 0.1f, 1000.0f);
  camera.SetPosition(0, 0, -10);  // カメラ位置
  ```
- Transform3D で対象オブジェクトの位置を設定
- boundingRadius は正の float（例: 1.0f）

---

## テスト 8: test_Animation.cpp

### ヘッダ

```cpp
/// @file test_Animation.cpp
/// @brief AnimationClip / Skeleton / TransformTRS テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "Graphics/3D/Animation.h"
#include "Graphics/3D/Skeleton.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/Graphics/3D/Animation.h (AnimationClip.h), Skeleton.h

struct TransformTRS {
    XMFLOAT3 translation = {0,0,0};
    XMFLOAT4 rotation = {0,0,0,1};  // quaternion
    XMFLOAT3 scale = {1,1,1};
};

TransformTRS IdentityTRS();
TransformTRS DecomposeTRS(const XMFLOAT4X4& mat);
XMFLOAT4X4 ComposeTRS(const TransformTRS& trs);

struct AnimationChannel {
    int jointIndex = -1;
    std::vector<Keyframe<XMFLOAT3>> translationKeys;
    std::vector<Keyframe<XMFLOAT4>> rotationKeys;
    std::vector<Keyframe<XMFLOAT3>> scaleKeys;
    InterpolationType interpolation = InterpolationType::Linear;
};

class AnimationClip {
    void SetName(const std::string& name);
    const std::string& GetName() const;
    void SetDuration(float duration);
    float GetDuration() const;
    void AddChannel(const AnimationChannel& channel);
    const std::vector<AnimationChannel>& GetChannels() const;
    void SampleTRS(float time, uint32_t jointCount, TransformTRS* outPose,
                   const TransformTRS* basePose = nullptr) const;
    void Sample(float time, uint32_t jointCount, XMFLOAT4X4* outLocalTransforms) const;
};

struct Joint {
    std::string name;
    int parentIndex = -1;
    XMFLOAT4X4 inverseBindMatrix;
    XMFLOAT4X4 localTransform;
};

class Skeleton {
    void AddJoint(const Joint& joint);
    const std::vector<Joint>& GetJoints() const;
    uint32_t GetJointCount() const;
    int FindJointIndex(const std::string& name) const;
    void ComputeGlobalTransforms(const XMFLOAT4X4* local, XMFLOAT4X4* global) const;
    void ComputeBoneMatrices(const XMFLOAT4X4* global, XMFLOAT4X4* bone) const;
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `IdentityTRS_Values` | IdentityTRS() → translation(0,0,0), rotation(0,0,0,1), scale(1,1,1) |
| 2 | `ComposeTRS_Identity` | ComposeTRS(IdentityTRS()) ≈ 単位行列 |
| 3 | `ComposeTRS_Translation` | translation(5,0,0) → 行列の _41=5 |
| 4 | `DecomposeTRS_RoundTrip` | ComposeTRS → DecomposeTRS → 元の TRS と一致 |
| 5 | `AnimationClip_NameAndDuration` | SetName/SetDuration → GetName/GetDuration |
| 6 | `AnimationClip_AddChannel` | AddChannel → GetChannels().size()==1 |
| 7 | `AnimationClip_SampleTRS_Static` | 1 キーフレーム → 任意の time で同じポーズ |
| 8 | `AnimationClip_SampleTRS_Interpolated` | 2 キー (t=0, t=1) → t=0.5 で中間値 |
| 9 | `Skeleton_AddJoint` | AddJoint 2 個 → GetJointCount()==2 |
| 10 | `Skeleton_FindJointIndex` | FindJointIndex("Root") → 0 |
| 11 | `Skeleton_FindJointIndex_NotFound` | FindJointIndex("NoExist") → -1 |
| 12 | `Skeleton_ComputeGlobalTransforms` | 2 ジョイント (parent→child) → child のグローバル行列が親×自身 |

### テストデータ作成ヘルパー

```cpp
// 2ジョイント Skeleton 作成ヘルパー
Skeleton CreateTestSkeleton()
{
    Skeleton skel;
    Joint root;
    root.name = "Root";
    root.parentIndex = -1;
    XMFLOAT4X4 identity;
    XMStoreFloat4x4(&identity, XMMatrixIdentity());
    root.inverseBindMatrix = identity;
    root.localTransform = identity;
    skel.AddJoint(root);

    Joint child;
    child.name = "Child";
    child.parentIndex = 0;
    child.inverseBindMatrix = identity;
    XMStoreFloat4x4(&child.localTransform, XMMatrixTranslation(0, 1, 0));
    skel.AddJoint(child);

    return skel;
}

// 単純なアニメーションクリップ作成ヘルパー
AnimationClip CreateTestClip(float duration = 1.0f)
{
    AnimationClip clip;
    clip.SetName("TestClip");
    clip.SetDuration(duration);

    AnimationChannel ch;
    ch.jointIndex = 0;
    ch.translationKeys.push_back({0.0f, {0, 0, 0}});
    ch.translationKeys.push_back({duration, {1, 0, 0}});
    ch.rotationKeys.push_back({0.0f, {0, 0, 0, 1}});
    ch.rotationKeys.push_back({duration, {0, 0, 0, 1}});
    ch.scaleKeys.push_back({0.0f, {1, 1, 1}});
    ch.scaleKeys.push_back({duration, {1, 1, 1}});
    clip.AddChannel(ch);

    return clip;
}
```

### 実装上の注意

- `Animation.h` は実際のファイル名が `AnimationClip.h` の可能性あり → 確認して正しいヘッダを include
  (`GXLib/Graphics/3D/Animation.h` または `GXLib/Graphics/3D/AnimationClip.h`)
- XMFLOAT4X4 の行列比較は各要素を EXPECT_NEAR で比較
- SampleTRS は basePose に nullptr を渡すと IdentityTRS がベースになる
- Joint の inverseBindMatrix は単位行列で OK（テスト用）

---

## テスト 9: test_StyleSheet.cpp

### ヘッダ

```cpp
/// @file test_StyleSheet.cpp
/// @brief StyleSheet CSS パースと StyleLength/StyleColor テスト

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/StyleSheet.h"
#include "GUI/Style.h"

using namespace GX;
```

### 対象 API

```
ヘッダ: GXLib/GUI/Style.h, GXLib/GUI/StyleSheet.h

struct StyleLength {
    float value; SizeUnit unit;
    static StyleLength Px(float v);
    static StyleLength Pct(float v);
    static StyleLength Auto();
    bool IsAuto() const;
    float Resolve(float parentSize) const;
};

struct StyleColor {
    float r, g, b, a;
    StyleColor(float r, float g, float b, float a = 1.0f);
    static StyleColor FromHex(const std::string& hex);
    bool IsTransparent() const;
};

struct StyleEdges {
    float top, right, bottom, left;
    StyleEdges(float all);
    StyleEdges(float v, float h);
    StyleEdges(float t, float r, float b, float l);
    float HorizontalTotal() const;
    float VerticalTotal() const;
};

struct StyleSelector {
    static StyleSelector Parse(const std::string& str);
    int Specificity() const;
};

class StyleSheet {
    bool LoadFromString(const std::string& source);
    size_t GetRuleCount() const;
    static std::string NormalizePropertyName(const std::string& name);
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `StyleLength_Px` | StyleLength::Px(100) → value==100, unit==Px |
| 2 | `StyleLength_Pct` | StyleLength::Pct(50) → unit==Percent |
| 3 | `StyleLength_Auto` | StyleLength::Auto() → IsAuto()==true |
| 4 | `StyleLength_Resolve_Px` | Px(100).Resolve(500) == 100 |
| 5 | `StyleLength_Resolve_Pct` | Pct(50).Resolve(200) == 100 |
| 6 | `StyleColor_FromHex_RGB` | FromHex("#FF0000") → r≈1.0, g≈0, b≈0 |
| 7 | `StyleColor_FromHex_RGBA` | FromHex("#FF000080") → a≈0.5 |
| 8 | `StyleColor_IsTransparent` | StyleColor(0,0,0,0).IsTransparent()==true |
| 9 | `StyleEdges_AllSame` | StyleEdges(10) → top==right==bottom==left==10 |
| 10 | `StyleEdges_Totals` | StyleEdges(5,10,15,20) → HorizontalTotal==30, VerticalTotal==20 |
| 11 | `StyleSelector_Parse_Type` | Parse("Button") → specificity > 0 |
| 12 | `StyleSelector_Parse_Class` | Parse(".highlight") → specificity == 10 |
| 13 | `StyleSelector_Parse_ID` | Parse("#main") → specificity == 100 |
| 14 | `StyleSheet_LoadFromString` | 有効な CSS → LoadFromString==true, GetRuleCount() > 0 |

### テスト用 CSS 文字列

```cpp
const char* testCSS = R"(
    Button {
        background-color: #333333;
        color: #FFFFFF;
        font-size: 16px;
        padding: 8px 12px;
    }
    .highlight {
        color: #FFCC00;
    }
    #main {
        width: 100%;
        height: 50px;
    }
)";
```

### 実装上の注意

- StyleColor::FromHex は `#RRGGBB` と `#RRGGBBAA` の 2 形式をサポート
- NormalizePropertyName: `background-color` → `backgroundColor`（camelCase 変換）
- StyleSelector::Parse は Widget* のマッチングに使うが、テストでは Specificity のみ検証
- LoadFromString は内部でトークナイザーを使用（依存なし）

---

## テスト 10: test_FileSystem.cpp

### ヘッダ

```cpp
/// @file test_FileSystem.cpp
/// @brief VFS FileSystem テスト（モック IFileProvider 使用）

#include "pch.h"
#include <gtest/gtest.h>
#include "IO/FileSystem.h"

using namespace GX;
```

### モック IFileProvider

```cpp
/// テスト用のインメモリファイルプロバイダ
class MockFileProvider : public IFileProvider
{
public:
    std::unordered_map<std::string, std::vector<uint8_t>> files;
    int m_priority = 0;

    MockFileProvider(int priority = 0) : m_priority(priority) {}

    void AddFile(const std::string& path, const std::string& content)
    {
        files[path] = std::vector<uint8_t>(content.begin(), content.end());
    }

    bool Exists(const std::string& path) const override
    {
        return files.count(path) > 0;
    }

    FileData Read(const std::string& path) const override
    {
        FileData fd;
        auto it = files.find(path);
        if (it != files.end()) fd.data = it->second;
        return fd;
    }

    bool Write(const std::string& path, const void* data, size_t size) override
    {
        files[path] = std::vector<uint8_t>(
            static_cast<const uint8_t*>(data),
            static_cast<const uint8_t*>(data) + size);
        return true;
    }

    int Priority() const override { return m_priority; }
};
```

### テストケース

| # | テスト名 | 内容 |
|---|---------|------|
| 1 | `FileData_Empty` | デフォルト FileData → IsValid()==false |
| 2 | `FileData_AsString` | データあり → AsString() が正しい文字列 |
| 3 | `Mount_Exists` | Mount + AddFile → Exists==true |
| 4 | `Mount_ReadFile` | Mount + AddFile → ReadFile().AsString() が正しい |
| 5 | `Mount_WriteFile` | WriteFile → ReadFile で書いた内容が読める |
| 6 | `Mount_NotFound` | マウントなし → Exists==false |
| 7 | `Mount_MountPoint` | "assets/" にマウント → "assets/test.txt" が見つかる |
| 8 | `Priority_HigherWins` | 2 プロバイダ (priority 0, 10) → 高優先の内容が返る |
| 9 | `Unmount_Removes` | Unmount → Exists==false |
| 10 | `Clear_RemovesAll` | Clear → 全ファイル Exists==false |

### 実装上の注意

- **FileSystem はシングルトン** → テスト間で状態が残る問題がある
  → 各テストの SetUp/TearDown で `FileSystem::Instance().Clear()` を呼ぶ
  → テストフィクスチャ `TEST_F` を使用:
  ```cpp
  class FileSystemTest : public ::testing::Test {
  protected:
      void SetUp() override { FileSystem::Instance().Clear(); }
      void TearDown() override { FileSystem::Instance().Clear(); }
  };
  ```
- MockFileProvider は `std::make_shared<MockFileProvider>()` で作成
- パス正規化: バックスラッシュ → スラッシュ、先頭スラッシュ除去
- `pch.h` に `<unordered_map>` があるので MockFileProvider で使用可能

---

## CMakeLists.txt 更新

`Tests/CMakeLists.txt` の `TEST_SOURCES` リストに新規ファイルを追加:

```cmake
set(TEST_SOURCES
    test_main.cpp
    test_Vector.cpp
    test_Matrix.cpp
    test_Quaternion.cpp
    test_Color.cpp
    test_MathUtil.cpp
    test_Collision2D.cpp
    test_Collision3D.cpp
    test_Spatial.cpp
    test_Crypto.cpp
    test_Allocator.cpp
    # Phase 37 新規
    test_Spline.cpp
    test_Transform3D.cpp
    test_Entity.cpp
    test_SceneSerializer.cpp
    test_ActionMapping.cpp
    test_NavMesh.cpp
    test_LODGroup.cpp
    test_Animation.cpp
    test_StyleSheet.cpp
    test_FileSystem.cpp
)
```

---

## 実行手順

### Step 1: CMakeLists.txt 更新
`Tests/CMakeLists.txt` の TEST_SOURCES に 10 ファイルを追加（上記参照）

### Step 2: テストファイル作成（推奨順序）
以下の順に作成し、各ファイル作成後にビルド検証:

1. `test_Transform3D.cpp` — 最も単純、依存なし
2. `test_Spline.cpp` — Vector3 のみ依存
3. `test_StyleSheet.cpp` — GUI スタイル、依存なし
4. `test_FileSystem.cpp` — VFS モック、依存なし
5. `test_ActionMapping.cpp` — Input 構造テスト
6. `test_Animation.cpp` — AnimationClip + Skeleton
7. `test_Entity.cpp` — Scene/Component 統合
8. `test_SceneSerializer.cpp` — JSON ラウンドトリップ
9. `test_NavMesh.cpp` — A* パス検索
10. `test_LODGroup.cpp` — Camera3D 依存

### Step 3: ビルド & テスト

```bash
cmake -B build -S .
cmake --build build --config Debug
ctest --test-dir build --build-config Debug --output-on-failure
```

### Step 4: 全テスト PASS 確認

目標: **260+ テスト全パス**（既存 151 + 新規 ~110-120）

---

## トラブルシューティング

### Q: `#include "Graphics/3D/Animation.h"` が見つからない
実際のファイル名を確認。`AnimationClip.h` の可能性がある。
```bash
ls GXLib/Graphics/3D/Anim*
```

### Q: `std::unordered_map` が使えない
`pch.h` に `<unordered_map>` は含まれている。`<map>` は含まれていないので注意。

### Q: FileSystem シングルトンのテスト間干渉
TEST_F フィクスチャの SetUp/TearDown で `FileSystem::Instance().Clear()` を必ず呼ぶ。

### Q: Camera3D の初期化
SetPerspective() を呼ばないと FOV が 0 になり SelectLOD が正常に動作しない。
```cpp
Camera3D camera;
camera.SetPerspective(XM_PIDIV4, 16.0f/9.0f, 0.1f, 1000.0f);
camera.SetPosition(0, 0, -10);
```

### Q: `(std::min)` / `(std::max)` パターン
Windows.h の min/max マクロと競合するため、括弧で囲む:
```cpp
int clamped = (std::min)(val, maxVal);
```

### Q: NavMesh の Build 座標系
Build(minX, minZ, maxX, maxZ, cellSize) — Y は高さ方向でパス検索には使わない。
XMFLOAT3 の y=0 でテスト。

### Q: Entity は Scene 経由で作成する
Entity を直接 new しない。Scene::CreateEntity() で作成すると ID が自動割り当てされる。

### Q: AnimationClip のヘッダ名
実際のヘッダファイル名を Glob で確認:
```
GXLib/Graphics/3D/Animation.h  (AnimationClip クラスを含む可能性)
```

### Q: コンパイルエラー「ComponentType undeclared」
`#include "Core/Scene/Component.h"` を追加するか、
`Components.h` が Component.h を include しているか確認。

---

## 完了条件

1. 10 個の新規テストファイルが `Tests/` に存在する
2. `Tests/CMakeLists.txt` に全ファイルが登録されている
3. `cmake --build build --config Debug` がエラーなし
4. `ctest` で **全テスト PASS**（0 failures）
5. 新規テスト数が **100 以上**
