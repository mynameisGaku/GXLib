# GXLib Phase 41-45 総合実装指令書

## Context

Phase 0-40 完了済み。エンジンは完全な 2D/3D 描画パイプライン、DXR レイトレーシング（反射＋GI）、
15+ ポストエフェクト、GUI（17 ウィジェット + CSS + XML）、物理(2D + Jolt 3D)、
オーディオ(XAudio2 + 3D空間音響)、シーングラフ/ECS、パーティクル(CPU 2D/3D + GPU Compute)、
ナビメッシュ(Grid A*)、LOD、デカール、IBL、IK(FABRIK + FootIK + LookAtIK)、
GPU インスタンシング、アクションマッピング、Lua スクリプティング(sol2)、2D タイルマップ(TMX)、
アニメーション(BlendStack/BlendTree/AnimatorStateMachine/RootMotion/AnimationEvent)、
マルチスレッドレンダリング(ParallelCommandRecorder)、Async Compute、間接描画、
GXModelViewer（ImGui Docking + 19 パネル）、アセットパイプライン（gxformat/gxconv/gxloader/gxpak）、
25 サンプルプロジェクト、273 ユニットテストを持つ成熟状態。

本指令書は **テスト拡充・不足サンプル追加・エディタ強化・パフォーマンス最適化・先進レンダリング** の
5 フェーズを網羅し、任意の Claude インスタンスが独立して各 Phase を実装できることを目的とする。

---

## 全体ロードマップ

| Phase | 名称 | 概要 | 依存 | 推定規模 |
|-------|------|------|------|----------|
| **41** | テスト拡充 | GPU不要モジュールの単体テスト大幅追加（273→450+テスト目標） | なし | 大 |
| **42** | 不足サンプル追加 | 未デモ機能の専用サンプル 8〜12 本新規作成 | なし | 大 |
| **43** | GXModelViewer シーンエディタ強化 | Undo/Redo、エンティティ生成、コンポーネント拡張、シミュレーション | Phase 41 推奨 | 大 |
| **44** | パフォーマンス最適化 | フラスタムカリング改善、リソースバリア最適化、描画ソート | なし | 中 |
| **45** | 先進レンダリング | Volumetric Clouds、Hi-Z Occlusion Culling、Virtual Texture Streaming | Phase 44 推奨 | 大 |

Phase 41/42 は並列着手可能。Phase 43 は Phase 41 完了後に着手推奨（テストで安定性確保後にエディタ変更）。
Phase 44/45 は独立して着手可能だが、Phase 44 → 45 の順序を推奨。

---

## 共通規約

### ビルド手順
```bash
cmake -B build -S .
cmake --build build --config Debug
```

### 新規ファイル追加時
- `GXLib/` 配下の `.cpp` は `GLOB_RECURSE` で自動収集されるが、新しいサブディレクトリを作った場合は `GXLib/CMakeLists.txt` に `file(GLOB_RECURSE ...)` を追加
- テストファイルは `Tests/CMakeLists.txt` に手動追加
- サンプルは `CMakeLists.txt` のルートに `gxlib_add_sample(NAME ...)` で追加
- **追加後は必ず `cmake -B build -S .` を再実行**

### コーディング規約
- C++20、名前空間 `GX`
- PCH: `pch.h`（`<sstream>` `<unordered_set>` は含まれない — 代替を使用）
- CD3DX12 ヘルパー不使用 — raw D3D12 構造体
- `Color{1, 1, 1, 1}` は曖昧 → `Color{1.0f, 1.0f, 1.0f, 1.0f}` を使用
- `std::min/max` は `(std::min)(a, b)` パターン（Windows.h マクロ回避）
- HLSL: `line` は予約語 → `gridLine` 等に変更
- `saturate()` は HLSL 専用 → C++ では `std::max(0.0f, std::min(1.0f, x))`

### テスト実行
```bash
cmake --build build --config Debug --target GXLib_Tests
cd build && ctest -C Debug --output-on-failure
```

### MEMORY.md 更新
各 Phase 完了後、`C:\Users\g0190\.claude\projects\C--Users-g0190-Desktop-GXLib\memory\MEMORY.md` の
"Completed Phases" セクションに Phase 概要を追記すること。

---

## Phase 41: テスト拡充

### 目的
現在 273 テスト / 20 テストファイルのカバレッジを大幅に拡張する。
GPU 不要で単体テスト可能な 30+ モジュールに対してテストを追加し、**450+ テスト / 35+ テストファイル** を目標とする。

### 現状分析

#### テスト済みモジュール (20ファイル / 273テスト)
| ファイル | テスト数 | カバー範囲 |
|----------|---------|-----------|
| test_Vector.cpp | 29 | Vector2/3/4 |
| test_Collision3D.cpp | 24 | AABB, Sphere, Ray, Frustum 等 |
| test_Collision2D.cpp | 19 | Circle, Rect, Line 等 |
| test_MathUtil.cpp | 19 | Lerp, Clamp, SmoothStep, Random |
| test_Spline.cpp | 18 | Linear/CatmullRom/CubicBezier |
| test_Entity.cpp | 20 | Entity, Scene, Components |
| test_StyleSheet.cpp | 14 | CSS パース、セレクタ、ルール |
| test_NavMesh.cpp | 14 | A*探索、障害物、NavAgent |
| test_Spatial.cpp | 13 | Quadtree/Octree/BVH |
| test_Animation.cpp | 12 | AnimationClip サンプリング、TRS 分解 |
| test_Matrix.cpp | 11 | 変換、逆行列、行列式 |
| test_Quaternion.cpp | 10 | Slerp, Euler, AxisAngle |
| test_Transform3D.cpp | 10 | Position/Rotation/Scale/行列 |
| test_Color.cpp | 10 | 色フォーマット、HSV、Lerp |
| test_Allocator.cpp | 10 | PoolAllocator, FrameAllocator |
| test_FileSystem.cpp | 10 | VFS, マウント、優先度 |
| test_SceneSerializer.cpp | 8 | JSON ラウンドトリップ |
| test_LODGroup.cpp | 8 | LOD選択、スクリーン占有率 |
| test_ActionMapping.cpp | 8 | バインディング、アクション、状態 |
| test_Crypto.cpp | 6 | AES-256, SHA-256, ランダム |

#### 未テストだがGPU不要でテスト可能なモジュール (優先度順)

**Priority 1 — 高価値・低労力（各 8-15 テスト想定）:**

| モジュール | ヘッダ | テスト可能な内容 |
|-----------|--------|-----------------|
| Camera2D | Graphics/Rendering/Camera2D.h | ビューポート変換、ズーム、スクロール、位置計算 |
| Camera3D | Graphics/3D/Camera3D.h | Perspective/Ortho設定、LookAt、Pitch/Yaw、ビュー行列 |
| Transform2D | Math/Transform2D.h | 位置、回転、スケール、行列合成 |
| AnimatorStateMachine | Graphics/3D/AnimatorStateMachine.h | 状態遷移、トリガー、パラメータ、条件評価 |
| BlendStack | Graphics/3D/BlendStack.h | レイヤー追加/削除、Override/Additive、ウェイト |
| BlendTree | Graphics/3D/BlendTree.h | 1D/2Dブレンド、パラメータ評価 |
| Tilemap | Graphics/Rendering/Tilemap.h | グリッド作成、タイル設定、レイヤー管理、座標変換 |
| XMLParser | GUI/XMLParser.h | XML パース、属性取得、子要素走査 |

**Priority 2 — 中価値・中労力（各 6-12 テスト想定）:**

| モジュール | ヘッダ | テスト可能な内容 |
|-----------|--------|-----------------|
| Timer | Core/Timer.h | フレーム時間、デルタ、FPS計算 |
| Logger | Core/Logger.h | メッセージフォーマット、レベルフィルタ |
| Material | Graphics/3D/Material.h | パラメータ設定、ShaderModel、デフォルト値 |
| Light | Graphics/3D/Light.h | CreateDirectional/Point/Spot、パラメータ検証 |
| Fog | Graphics/3D/Fog.h | Linear/Exponential パラメータ |
| IKSolver | Graphics/3D/IKSolver.h | FABRIK 反復、チェーン評価 |
| LookAtIK | Graphics/3D/LookAtIK.h | ターゲット追従、制約角度 |
| LayerStack | Graphics/Layer/LayerStack.h | レイヤー順序、追加/削除/検索 |
| Random | Math/Random.h | 分布、シード、方向ベクトル生成 |
| ParticleEmitter2D | Graphics/Rendering/ParticleEmitter2D.h | エミッション設定、パラメータ検証 |

**Priority 3 — 統合テスト（各 5-10 テスト想定）:**

| テスト対象 | 内容 |
|-----------|------|
| Scene + Components 統合 | 全コンポーネントタイプの追加/取得/削除ラウンドトリップ |
| AnimationClip + Events | イベントコールバック発火、ループ境界ハンドリング |
| AnimatorStateMachine + BlendStack | 状態遷移 → ブレンドレイヤー変化の整合性 |
| NavMesh + NavAgent 統合 | 複雑地形でのパス追従、障害物回避 |
| FileSystem + Archive | VFS マウント → Archive 読み込みラウンドトリップ |

---

### 41a: Camera2D テスト

**ファイル:** `Tests/test_Camera2D.cpp`

```cpp
// テスト項目 (10テスト想定):
// Camera2D_DefaultPosition         - 初期位置が (0,0)
// Camera2D_SetPosition             - SetPosition() で位置変更
// Camera2D_SetZoom                 - SetZoom() でズーム倍率変更
// Camera2D_ZoomClamp               - ズーム範囲のクランプ (最小/最大)
// Camera2D_GetPositionXY           - GetPositionX()/GetPositionY() 個別取得
// Camera2D_ViewportTransform       - ワールド→スクリーン座標変換
// Camera2D_InverseTransform        - スクリーン→ワールド座標変換
// Camera2D_VisibleRegion           - 可視領域の計算（ズーム考慮）
// Camera2D_SmoothFollow            - 追従対象への補間移動
// Camera2D_Shake                   - カメラシェイク（オフセット適用）
```

**実装手順:**
1. `Camera2D.h` を読み、公開 API を確認
2. GPU依存のメソッド（描画系）は除外し、座標変換・状態管理のみテスト
3. Google Test (`TEST()`) で各項目を実装
4. `Tests/CMakeLists.txt` に `test_Camera2D.cpp` を追加

---

### 41b: Camera3D テスト

**ファイル:** `Tests/test_Camera3D.cpp`

```cpp
// テスト項目 (12テスト想定):
// Camera3D_DefaultValues           - 初期状態の検証
// Camera3D_SetPerspective          - FOV/Aspect/Near/Far 設定
// Camera3D_SetOrthographic         - 正射影設定
// Camera3D_SetPosition             - 位置設定と取得
// Camera3D_Rotate                  - Pitch/Yaw 回転
// Camera3D_LookAt                  - LookAt で pitch/yaw 自動計算
// Camera3D_GetViewMatrix           - ビュー行列の正当性
// Camera3D_GetProjectionMatrix     - 射影行列の正当性
// Camera3D_GetViewProjection       - VP行列の合成
// Camera3D_Forward                 - 前方ベクトルの計算
// Camera3D_Right                   - 右方ベクトルの計算
// Camera3D_Up                      - 上方ベクトルの計算
```

**注意点:**
- Camera3D は DirectXMath の `XMFLOAT3`, `XMFLOAT4X4` を使用
- テストでは `EXPECT_NEAR` で浮動小数点比較（許容誤差 1e-4f）
- `Camera3D.h` の `#include` が GPU 型を含まないことを確認。含む場合はテスト対象を限定

---

### 41c: Transform2D テスト

**ファイル:** `Tests/test_Transform2D.cpp`

```cpp
// テスト項目 (8テスト想定):
// Transform2D_DefaultValues        - 位置(0,0), 回転0, スケール(1,1)
// Transform2D_SetPosition          - 位置設定/取得
// Transform2D_SetRotation          - 回転設定/取得（ラジアン）
// Transform2D_SetScale             - スケール設定/取得
// Transform2D_GetMatrix            - ワールド行列の計算
// Transform2D_ComposeTransforms    - 親子変換の合成
// Transform2D_RotateAround         - 任意点回りの回転
// Transform2D_Forward              - 前方ベクトル（回転角度ベース）
```

---

### 41d: AnimatorStateMachine テスト

**ファイル:** `Tests/test_AnimatorStateMachine.cpp`

**前提:** AnimatorStateMachine.h を読み、以下の API を確認:
- `AddState(name)`, `AddTransition(from, to, conditions)`, `SetTrigger(name)`, `SetFloat(name, value)`
- `Update(dt)`, `GetCurrentState()`, `GetTransitions()`

```cpp
// テスト項目 (15テスト想定):
// ASM_AddState                     - 状態追加と存在確認
// ASM_AddTransition                - 遷移追加（条件付き）
// ASM_SetInitialState              - 初期状態の設定
// ASM_GetCurrentState              - 現在状態の取得
// ASM_TriggerTransition            - トリガーによる状態遷移
// ASM_FloatCondition               - float パラメータ条件での遷移
// ASM_BoolCondition                - bool パラメータ条件での遷移
// ASM_TransitionDuration           - 遷移時間の検証（ブレンド中の状態）
// ASM_NoTransitionWithoutCondition - 条件未達時に遷移しないことを確認
// ASM_MultipleTransitions          - 同一状態から複数遷移先への優先度
// ASM_SelfTransition               - 自己遷移の動作
// ASM_TransitionChain              - A→B→C の連鎖遷移
// ASM_GetTransitions               - GetTransitions() の正当性
// ASM_ResetTrigger                 - トリガーが1回の遷移後にリセットされること
// ASM_InvalidState                 - 存在しない状態への遷移を試みた場合のエラーハンドリング
```

**重要:**
- AnimatorStateMachine は Skeleton/AnimationClip に依存する可能性あり
- テスト用にダミーの Skeleton（2-3 ボーン）と AnimationClip（2-3 キーフレーム）を作成するヘルパー関数を用意
- GPU リソース不要で状態遷移ロジックのみテスト可能かを `AnimatorStateMachine.h` で確認

---

### 41e: BlendStack / BlendTree テスト

**ファイル:** `Tests/test_BlendSystem.cpp`

```cpp
// BlendStack テスト (8テスト想定):
// BlendStack_AddLayer              - レイヤー追加（最大8）
// BlendStack_RemoveLayer           - レイヤー削除
// BlendStack_OverrideMode          - Override モードでベースクリップを上書き
// BlendStack_AdditiveMode          - Additive モードでベースに加算
// BlendStack_WeightBlending        - ウェイト 0.0-1.0 での補間
// BlendStack_MaxLayers             - 8レイヤー上限の検証
// BlendStack_LayerOrder            - レイヤー優先度（後勝ち）
// BlendStack_ClearAll              - 全レイヤークリア

// BlendTree テスト (8テスト想定):
// BlendTree_1DBlend                - 1Dパラメータでの2クリップブレンド
// BlendTree_1DThresholds           - 閾値境界でのブレンド比率
// BlendTree_1DExtrapolation        - パラメータが範囲外の場合
// BlendTree_2DBlend                - 2Dパラメータ (X,Y) でのブレンド
// BlendTree_2DNearestSample        - 2D最近傍サンプルの重み
// BlendTree_SetParameter           - パラメータ変更とブレンド結果の更新
// BlendTree_EmptyTree              - クリップ未設定時の動作
// BlendTree_SingleClip             - 1クリップのみ時のパススルー
```

**注意:**
- BlendStack は `AnimBlendMode`（NOT `BlendMode`）を使用 — SpriteBatch.h の `BlendMode` と名前衝突回避
- テスト用ダミークリップを共有ヘルパーで生成

---

### 41f: Tilemap テスト

**ファイル:** `Tests/test_Tilemap.cpp`

```cpp
// テスト項目 (10テスト想定):
// Tilemap_Create                   - Create(width, height, tileW, tileH) 正常動作
// Tilemap_AddLayer                 - レイヤー追加と取得
// Tilemap_GetTile                  - タイルID取得（座標指定）
// Tilemap_SetTile                  - タイルID設定
// Tilemap_OutOfBounds              - 範囲外座標でのアクセス（安全性）
// Tilemap_MultipleLayer            - 複数レイヤーの独立性
// Tilemap_WorldToTile              - ワールド座標→タイル座標変換
// Tilemap_TileToWorld              - タイル座標→ワールド座標変換
// Tilemap_GetDimensions            - 幅・高さ・タイルサイズの取得
// Tilemap_CollisionLayer           - コリジョンレイヤーの判定
```

---

### 41g: XMLParser テスト

**ファイル:** `Tests/test_XMLParser.cpp`

```cpp
// テスト項目 (10テスト想定):
// XMLParser_ParseEmpty             - 空文字列のパース
// XMLParser_ParseSingleElement     - <tag/> 単一要素
// XMLParser_ParseAttributes        - <tag attr="value"/> 属性取得
// XMLParser_ParseNested            - <parent><child/></parent> 入れ子
// XMLParser_ParseText              - <tag>テキスト</tag> テキスト内容
// XMLParser_ParseMultipleChildren  - 複数子要素
// XMLParser_GetAttribute           - 存在/非存在属性の取得
// XMLParser_ParseWithNamespace     - 名前空間付き要素
// XMLParser_InvalidXML             - 不正XMLのエラーハンドリング
// XMLParser_ParseGUILayout         - GUI レイアウト XML の実践的パース
```

---

### 41h: IKSolver / LookAtIK テスト

**ファイル:** `Tests/test_IK.cpp`

```cpp
// IKSolver テスト (8テスト想定):
// IKSolver_Create                  - ソルバー生成
// IKSolver_TwoBoneReach            - 2ボーンチェーンで到達可能なターゲット
// IKSolver_TwoBoneUnreachable      - 到達不能ターゲット（最大伸展）
// IKSolver_PoleVector              - ポールベクトルで肘/膝の方向制御
// IKSolver_MultiBoneChain          - 3ボーン以上のチェーン
// IKSolver_ConvergenceIterations   - 反復回数による収束精度
// IKSolver_ZeroLength              - ボーン長0のエッジケース
// IKSolver_TargetAtRoot            - ターゲットがルート位置の場合

// LookAtIK テスト (6テスト想定):
// LookAtIK_LookForward             - 正面ターゲットへの回転
// LookAtIK_LookBehind              - 背面ターゲット（制約角度内）
// LookAtIK_ClampAngle              - 最大回転角度のクランプ
// LookAtIK_Weight                  - ウェイト0-1の補間
// LookAtIK_UpVector                - アップベクトルの影響
// LookAtIK_SmoothTracking          - スムース追従（補間速度）
```

**注意:**
- IKSolver が Skeleton 依存かを確認。依存する場合はダミー Skeleton を生成
- LookAtIK は Transform3D ベースの回転計算がメイン — GPU不要

---

### 41i: その他の単体テスト

**ファイル:** `Tests/test_Material.cpp`
```cpp
// Material テスト (6テスト想定):
// Material_DefaultValues           - デフォルトのアルベド、ラフネス、メタリック
// Material_SetShaderModel          - ShaderModel 変更
// Material_ShaderParams            - ShaderModelParams のバイトレイアウト確認
// Material_ToonParams              - Toon UTS2 パラメータの設定と取得
// Material_Clone                   - マテリアルのコピー
// Material_OverrideFlag            - useMaterialOverride フラグ
```

**ファイル:** `Tests/test_Light.cpp`
```cpp
// Light テスト (8テスト想定):
// Light_CreateDirectional          - 方向ライト作成
// Light_CreatePoint                - ポイントライト作成
// Light_CreateSpot                 - スポットライト作成
// Light_DirectionalDirection       - 方向の正規化確認
// Light_PointRange                 - 範囲パラメータ
// Light_SpotAngle                  - 内角/外角パラメータ
// Light_Intensity                  - 強度設定
// Light_Color                      - 色設定
```

**ファイル:** `Tests/test_LayerStack.cpp`
```cpp
// LayerStack テスト (8テスト想定):
// LayerStack_AddLayer              - レイヤー追加
// LayerStack_RemoveLayer           - レイヤー削除
// LayerStack_GetByIndex            - インデックスで取得
// LayerStack_GetByName             - 名前で取得
// LayerStack_LayerOrder            - 描画順序の保証
// LayerStack_Count                 - レイヤー数
// LayerStack_Clear                 - 全クリア
// LayerStack_DuplicateName         - 重複名の処理
```

---

### 41j: Tests/CMakeLists.txt 更新

以下のファイルを追加:
```cmake
# Phase 41 新規テストファイル
set(TEST_SOURCES
    # ... 既存のテスト ...
    Tests/test_Camera2D.cpp
    Tests/test_Camera3D.cpp
    Tests/test_Transform2D.cpp
    Tests/test_AnimatorStateMachine.cpp
    Tests/test_BlendSystem.cpp
    Tests/test_Tilemap.cpp
    Tests/test_XMLParser.cpp
    Tests/test_IK.cpp
    Tests/test_Material.cpp
    Tests/test_Light.cpp
    Tests/test_LayerStack.cpp
)
```

### 41k: テスト共通ヘルパー

**ファイル:** `Tests/TestHelpers.h`

テスト全体で共有するヘルパー関数:
```cpp
#pragma once
#include <DirectXMath.h>
#include <cmath>

namespace TestHelper {

// 浮動小数点比較マクロ
constexpr float k_Epsilon = 1e-4f;

// ダミー AnimationClip 生成（3キーフレーム、線形補間）
// ダミー Skeleton 生成（2-3ボーン）
// XMFLOAT3 比較ヘルパー
inline bool NearEqual(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, float eps = k_Epsilon)
{
    return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps && std::abs(a.z - b.z) < eps;
}

} // namespace TestHelper
```

### 完了基準
- [ ] 全新規テストが `ctest -C Debug` でパス
- [ ] 既存 273 テストがリグレッションなし
- [ ] テスト総数 450 以上
- [ ] Tests/CMakeLists.txt 更新済み

---

## Phase 42: 不足サンプル追加

### 目的
エンジン機能のうちデモが存在しない 17 の主要機能に対し、専用サンプルプロジェクトを作成する。
ユーザーが各機能の使い方を学べる実践的なコード例を提供する。

### 現状分析

#### サンプルが存在する機能 (25サンプル)
EasyHello, Shooting2D, Platformer2D, Walkthrough3D, GUIMenuDemo, PostEffectShowcase,
DXRShowcase, Particle2DShowcase, ParticleShowcase, IBLShowcase, InstanceShowcase,
IKShowcase, Audio3DShowcase, ActionMappingShowcase, GPUParticleShowcase,
NavmeshShowcase, SceneShowcase, TrailShowcase, SplineShowcase, WireframeShowcase,
TextLayoutShowcase, ProfilerShowcase, MultiThreadShowcase, LuaShowcase, TilemapShowcase

#### サンプルが存在しない主要機能
| 機能 | 重要度 | 実装済みAPI |
|------|--------|-----------|
| AnimatorStateMachine + BlendStack/Tree | Tier 1 | AnimatorStateMachine, BlendStack, BlendTree, AnimationEvent, RootMotion |
| Physics2D | Tier 1 | PhysicsWorld2D, RigidBody2D |
| Physics3D (Jolt) | Tier 1 | PhysicsWorld3D, RigidBody3D, PhysicsShape, MeshCollider |
| RenderLayer / MaskScreen | Tier 2 | RenderLayer, LayerStack, LayerCompositor, MaskScreen |
| AudioBus / AudioMixer | Tier 2 | AudioMixer, AudioBus |
| Decal | Tier 2 | Decal (ボックス投影 Deferred) |
| Terrain | Tier 2 | Terrain (ハイトフィールド) |
| LODGroup | Tier 2 | LODGroup (スクリーン占有率) |
| ShaderHotReload | Tier 3 | ShaderLibrary, ShaderHotReload |
| Network (TCP/UDP/WebSocket) | Tier 3 | TCPSocket, UDPSocket, HTTPClient, WebSocket |
| MoviePlayer | Tier 3 | MoviePlayer (Media Foundation) |
| FileSystem / Archive | Tier 3 | FileSystem, Archive, ArchiveFileProvider, PakFileProvider |
| PointShadowMap | Tier 3 | PointShadowMap |
| Skeletal Animation (.gxmd + .gxan) | Tier 3 | GxmdModelLoader, AnimationClip, Animator |
| Camera2D (dedicated) | Tier 3 | Camera2D (ズーム、追従) |
| Collision Debug Viz | Tier 3 | Collision2D/3D + PrimitiveBatch3D |

### 必須実装サンプル（Tier 1-2: 8本）

---

#### 42a: AnimationBlendingShowcase

**目的:** AnimatorStateMachine + BlendStack + BlendTree + AnimationEvent + RootMotion の統合デモ

**ファイル:** `Samples/AnimationBlendingShowcase/main.cpp`

**仕様:**
- .gxmd スキンドモデルを読み込み（アセットが無い場合はプロシージャル生成の簡易スキンドメッシュ）
- AnimatorStateMachine で Idle → Walk → Run → Jump の 4 状態を定義
- BlendStack で上半身/下半身の 2 レイヤーブレンド
- BlendTree 1D で Walk/Run のスピードパラメータブレンド
- AnimationEvent でフットステップタイミングにエフェクト発火
- RootMotion で移動デルタを Transform に適用
- HUD に現在の状態、ブレンドウェイト、RootMotion デルタを表示

**コントロール:**
- WASD: 移動パラメータ変更（BlendTree パラメータ）
- Space: ジャンプ（トリガー発火）
- 1-4: 状態直接切り替え
- Tab: RootMotion ON/OFF 切り替え
- ESC: 終了

**実装手順:**
1. `Samples/AnimationBlendingShowcase/main.cpp` 作成
2. `CMakeLists.txt` に `gxlib_add_sample(NAME AnimationBlendingShowcase)` 追加
3. GXEasy::App を継承、3D シーン + HUD 構成
4. ダミーアニメーションクリップを手動生成（アセットが無い場合）:
   - Idle: T-pose 微動（呼吸モーション）
   - Walk: 2秒ループ、ルートモーション XZ オフセット付き
   - Run: 1秒ループ、より大きいルートモーション
   - Jump: 0.5秒ワンショット
5. AnimatorStateMachine 設定:
   ```cpp
   auto& sm = animator.GetStateMachine();
   sm.AddState("Idle");
   sm.AddState("Walk");
   sm.AddState("Run");
   sm.AddState("Jump");
   sm.AddTransition("Idle", "Walk", /* speed > 0.1 */);
   sm.AddTransition("Walk", "Run", /* speed > 0.7 */);
   sm.AddTransition("Walk", "Idle", /* speed < 0.1 */);
   sm.AddTransition("Run", "Walk", /* speed < 0.7 */);
   sm.AddTransition("*", "Jump", /* trigger: "jump" */);
   sm.AddTransition("Jump", "Idle", /* exitTime */);
   ```
6. BlendStack 設定:
   ```cpp
   blendStack.AddLayer("Base", AnimBlendMode::Override, 1.0f);
   blendStack.AddLayer("UpperBody", AnimBlendMode::Additive, 0.5f);
   ```
7. AnimationEvent 設定:
   ```cpp
   walkClip.AddEvent(0.25f, "footstep_left");
   walkClip.AddEvent(0.75f, "footstep_right");
   animator.SetEventCallback([](const std::string& name) {
       // エフェクト発火 or サウンド再生
   });
   ```

**注意:**
- `.gxmd`/`.gxan` アセットが利用可能であればそれを使用
- 利用不可の場合、手動でキーフレームを設定した AnimationClip を生成
- 3Dシーンのため Renderer3D + PostEffectPipeline 必須
- HDR パイプライン必須（R16G16B16A16_FLOAT）

---

#### 42b: Physics2DShowcase

**目的:** PhysicsWorld2D + RigidBody2D の物理シミュレーションデモ

**ファイル:** `Samples/Physics2DShowcase/main.cpp`

**仕様:**
- 画面下部に地面（静的ボディ）
- クリックで動的ボックスを生成（重力で落下）
- 複数ボックスが衝突して積み重なる
- 摩擦と反発係数のリアルタイムスライダー
- PrimitiveBatch で物理ボディの可視化

**コントロール:**
- LMB: ボックス生成
- RMB: 円形ボディ生成
- R: リセット（全ボディ削除）
- ↑/↓: 重力の強さ調整
- 1-3: 反発係数切り替え (0.0 / 0.5 / 0.9)
- ESC: 終了

**実装手順:**
1. `PhysicsWorld2D` と `RigidBody2D` の API を確認
2. 静的地面 + 壁をボディとして配置
3. マウスクリック位置に動的ボディを追加
4. `Update()` で `PhysicsWorld2D::Step(dt)` を呼び出し
5. `Draw()` で各ボディの位置/回転を取得し `PrimitiveBatch` で描画
6. HUD にボディ数、FPS、物理パラメータ表示

---

#### 42c: Physics3DShowcase

**目的:** PhysicsWorld3D (Jolt) の 3D 物理シミュレーションデモ

**ファイル:** `Samples/Physics3DShowcase/main.cpp`

**仕様:**
- 地面（静的メッシュ）
- 球・ボックス・カプセルの動的ボディを生成
- ドミノ倒しのプリセットシーン
- ラグドール風の連結ボディ（constraint）
- PrimitiveBatch3D でコリジョン形状のワイヤフレーム表示

**コントロール:**
- LMB: 球を射出（カメラ前方）
- 1: ドミノプリセット
- 2: ボックスタワープリセット
- 3: ラグドールプリセット
- R: リセット
- F: カメラ追従モード切り替え
- ESC: 終了

**実装手順:**
1. `PhysicsWorld3D.h` と `PhysicsShape.h` の API を確認
2. Jolt 初期化 → PhysicsWorld3D::Initialize()
3. 地面: 静的 PhysicsShape::Box (100x1x100)
4. ドミノ: 20個の薄いボックスを等間隔配置
5. `Update()` で `PhysicsWorld3D::Step(dt)` → 各ボディの Transform 取得
6. `Draw()` で Renderer3D + PrimitiveBatch3D（ワイヤフレーム）

**注意:**
- PhysicsWorld3D は Jolt Physics 依存 — ビルド時にリンク済み
- `PhysicsWorld3D.cpp` を読み、Step/CreateBody/GetTransform の API を確認

---

#### 42d: RenderLayerShowcase

**目的:** RenderLayer + LayerStack + LayerCompositor + MaskScreen のデモ

**ファイル:** `Samples/RenderLayerShowcase/main.cpp`

**仕様:**
- 3つのレンダーレイヤー: 背景(Sky)、メインシーン(3D)、UI(2D)
- MaskScreen で円形マスクを適用してメインシーンをマスキング
- LayerCompositor で合成順序を制御
- レイヤーの表示/非表示をトグル

**コントロール:**
- 1/2/3: 各レイヤーの表示/非表示トグル
- M: マスクの ON/OFF
- ↑/↓: マスク半径の調整
- WASD: マスク位置の移動
- ESC: 終了

**実装手順:**
1. `RenderLayer.h`, `LayerStack.h`, `LayerCompositor.h`, `MaskScreen.h` の API 確認
2. LayerStack に 3 レイヤーを追加（描画順序指定）
3. 各レイヤーに描画内容を設定:
   - Layer 0 (Sky): Skybox or グラデーション背景
   - Layer 1 (Scene): 3D オブジェクト
   - Layer 2 (UI): 2D テキスト/図形
4. MaskScreen で Layer 1 にマスク適用
5. LayerCompositor で最終合成

---

#### 42e: DecalShowcase

**目的:** Decal システム（ボックス投影 Deferred Decal）のデモ

**ファイル:** `Samples/DecalShowcase/main.cpp`

**仕様:**
- 3D シーン（地面 + 壁 + キューブ複数）
- クリック位置にデカール（弾痕/ペイント）を配置
- デカールが曲面にも正しく投影されることを示す
- 複数デカールの重ね描画
- デカールのフェードアウト（生存時間）

**コントロール:**
- LMB: デカール配置（カメラレイキャスト）
- 1/2/3: デカールテクスチャ切り替え（弾痕/血痕/ペイント）
- R: 全デカール削除
- ESC: 終了

**注意:**
- Decal は front-face culling（裏面描画）、depth write OFF
- 深度バッファからワールド位置を再構築
- テクスチャが必要 — 手動でプロシージャル生成 or SoftImage で白丸テクスチャを動的生成

---

#### 42f: TerrainShowcase

**目的:** Terrain ハイトフィールドレンダリングのデモ

**ファイル:** `Samples/TerrainShowcase/main.cpp`

**仕様:**
- プロシージャルハイトマップ生成（Perlin ノイズ風 sin/cos 合成）
- Terrain の描画（ライティング + テクスチャ）
- Camera3D でフライスルー
- 高度に応じた色分け（水/草/岩/雪）
- LODGroup でカメラ距離に応じたメッシュ解像度切り替え

**コントロール:**
- WASD: カメラ移動
- マウス: カメラ回転
- ↑/↓: 地形の起伏度調整
- L: LOD 可視化（ワイヤフレームで LOD レベル色分け）
- ESC: 終了

---

#### 42g: LODShowcase

**目的:** LODGroup のスクリーン占有率ベース LOD 切り替えデモ

**ファイル:** `Samples/LODShowcase/main.cpp`

**仕様:**
- 同一オブジェクトの 3 LOD レベル（High/Mid/Low ポリゴン）
- オブジェクトを遠近に配置（格子状 or 一列）
- カメラの距離で LOD が自動切り替わるのを可視化
- LOD レベルを色で表示（赤=High, 黄=Mid, 青=Low）
- HUD にポリゴン数、LOD レベル分布を表示

**コントロール:**
- WASD + マウス: カメラ移動
- H: ヒステリシスバンドの ON/OFF
- 1-3: LOD 強制切り替え
- Tab: LOD 可視化色の ON/OFF
- ESC: 終了

**実装手順:**
1. `LODGroup.h` の API 確認: `AddLOD(mesh, screenPercentage)`, `SelectLOD(camera, transform)`
2. 3段階のメッシュを MeshGenerator で生成:
   - LOD 0: CreateSphere(1.0, 32, 32) — 高ポリ
   - LOD 1: CreateSphere(1.0, 16, 16) — 中ポリ
   - LOD 2: CreateSphere(1.0, 8, 8) — 低ポリ
3. LODGroup に登録:
   ```cpp
   lodGroup.AddLOD(meshHigh, 0.3f);   // 30%以上で高ポリ
   lodGroup.AddLOD(meshMid,  0.1f);   // 10%以上で中ポリ
   lodGroup.AddLOD(meshLow,  0.0f);   // それ以下で低ポリ
   ```
4. 格子状にオブジェクトを配置、Update で LOD 選択、Draw で描画

---

#### 42h: AudioMixerShowcase

**目的:** AudioBus + AudioMixer の多トラックミキシングデモ

**ファイル:** `Samples/AudioMixerShowcase/main.cpp`

**仕様:**
- 3つのオーディオバス: BGM, SFX, Voice
- 各バスの音量を個別制御
- マスターボリューム
- バスごとのミュート/ソロ
- サイン波生成でデモサウンド（外部ファイル不要）
- HUD にバスレベルメーター表示

**コントロール:**
- 1/2/3: 各バスのミュート切り替え
- ↑/↓: 選択バスの音量調整
- Tab: バス選択切り替え
- M: マスターミュート
- ESC: 終了

---

### 42i-42l: 追加サンプル（Tier 3, 余力があれば実装）

#### 42i: ShaderHotReloadShowcase
- シェーダーファイルを動的に変更 → 自動再コンパイル → リアルタイム反映
- `ShaderLibrary` + `ShaderHotReload` + `FileWatcher` の連携

#### 42j: PointShadowShowcase
- ポイントライトの全方位シャドウマップ
- キューブマップ6面レンダリングの可視化

#### 42k: SkeletalAnimShowcase
- `.gxmd` + `.gxan` を読み込みスキンドメッシュアニメーション再生
- Animator の Play/Pause/Speed 制御

#### 42l: CollisionDebugShowcase
- Collision2D/3D のデバッグ可視化
- Quadtree/Octree/BVH のノード表示
- レイキャストの可視化

### CMakeLists.txt 更新

```cmake
# Phase 42 新規サンプル (Tier 1-2)
gxlib_add_sample(NAME AnimationBlendingShowcase)
gxlib_add_sample(NAME Physics2DShowcase)
gxlib_add_sample(NAME Physics3DShowcase)
gxlib_add_sample(NAME RenderLayerShowcase)
gxlib_add_sample(NAME DecalShowcase)
gxlib_add_sample(NAME TerrainShowcase)
gxlib_add_sample(NAME LODShowcase)
gxlib_add_sample(NAME AudioMixerShowcase)

# Phase 42 追加サンプル (Tier 3, optional)
# gxlib_add_sample(NAME ShaderHotReloadShowcase)
# gxlib_add_sample(NAME PointShadowShowcase)
# gxlib_add_sample(NAME SkeletalAnimShowcase)
# gxlib_add_sample(NAME CollisionDebugShowcase)
```

### 完了基準
- [ ] Tier 1-2 の 8 サンプルがビルド・実行可能
- [ ] 各サンプルにファイル先頭コメント（目的、コントロール説明）
- [ ] CMakeLists.txt 更新済み
- [ ] 既存サンプルのビルドにリグレッションなし

---

## Phase 43: GXModelViewer シーンエディタ強化

### 目的
GXModelViewer を「モデルビューア」から「シーンエディタ」に進化させる。
インポートのみのエンティティ管理から、作成・編集・シミュレーションが可能な統合ツールへ。

### 現状分析

**現在できること (Phase 32完了時点):**
- モデルインポート (FBX/glTF/OBJ/GXMD) + ドラッグ&ドロップ
- エンティティ選択/削除/リネーム
- Transform 編集 (ImGuizmo: 移動/回転/スケール)
- マテリアルオーバーライド + シェーダーモデルパラメータ編集
- アニメーション再生 (Timeline, AnimatorStateMachine 可視化)
- ライティング (16灯, Directional/Point/Spot)
- ポストエフェクト (13種パラメータ制御)
- シーン保存/読み込み (JSON)
- 19 パネル体制

**現在できないこと (=今回のスコープ):**
- 空エンティティの作成
- プリミティブ形状の追加
- Undo/Redo
- ドラッグ&ドロップでの親子関係変更
- エンティティ複製
- マルチセレクト
- コンポーネントの動的追加/削除
- 物理シミュレーション プレビュー
- テクスチャのマテリアルへの割り当て UI

### ソースファイル構成

```
GXModelViewer/
├── main.cpp                        // WinMain
├── GXModelViewerApp.h/cpp          // メインアプリ (1934行)
├── ModelExporter.h/cpp             // GXMD/GXAN エクスポート
├── InfiniteGrid.h/cpp              // グリッド描画
├── Panels/ (19パネル)
│   ├── SceneHierarchyPanel.h/cpp
│   ├── PropertyPanel.h/cpp
│   ├── LightingPanel.h/cpp
│   ├── PostEffectPanel.h/cpp
│   ├── TimelinePanel.h/cpp
│   ├── AnimatorPanel.h/cpp
│   ├── BlendTreeEditor.h/cpp
│   ├── SkyboxPanel.h/cpp
│   ├── TerrainPanel.h/cpp
│   ├── PerformancePanel.h/cpp
│   ├── LogPanel.h/cpp
│   ├── ModelInfoPanel.h/cpp
│   ├── SkeletonPanel.h/cpp
│   ├── TextureBrowser.h/cpp
│   ├── AssetBrowserPanel.h/cpp
│   ├── IBLPanel.h/cpp
│   ├── ParticlePanel.h/cpp
│   ├── IKPanel.h/cpp
│   └── AudioPanel.h/cpp
└── Scene/
    ├── EditorMetadata.h
    ├── SceneGraph.h/cpp            // フラットエンティティ管理
    └── SceneSerializer.h/cpp       // JSON保存/読み込み
```

---

### 43a: Undo/Redo システム

**目的:** コマンドパターンによる操作の取り消し/やり直し

**新規ファイル:**
- `GXModelViewer/UndoSystem.h`
- `GXModelViewer/UndoSystem.cpp`

**設計:**
```cpp
// 抽象コマンドインターフェース
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

// Undoスタック管理
class UndoSystem {
public:
    void Execute(std::unique_ptr<ICommand> cmd);  // 実行+スタックにプッシュ
    void Undo();                                   // 最後のコマンドを取り消し
    void Redo();                                   // 最後のUndoをやり直し
    bool CanUndo() const;
    bool CanRedo() const;
    const std::string& GetUndoDescription() const;
    const std::string& GetRedoDescription() const;
    void Clear();

private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;
    static constexpr int k_MaxUndoLevels = 100;
};
```

**具体コマンド（優先度順）:**

```cpp
// 1. Transform 変更
class TransformCommand : public ICommand {
    int m_entityIndex;
    Transform3D m_oldTransform, m_newTransform;
    // Execute: entity.transform = m_newTransform
    // Undo: entity.transform = m_oldTransform
};

// 2. エンティティ追加/削除
class AddEntityCommand : public ICommand { /* ... */ };
class RemoveEntityCommand : public ICommand { /* ... */ };

// 3. マテリアル変更
class MaterialChangeCommand : public ICommand { /* ... */ };

// 4. 親子関係変更
class ReparentCommand : public ICommand { /* ... */ };

// 5. リネーム
class RenameCommand : public ICommand { /* ... */ };
```

**統合ポイント:**
- ImGuizmo のドラッグ終了時に `TransformCommand` を発行
- SceneHierarchyPanel の操作に対応する Command を発行
- PropertyPanel の変更に対応する Command を発行
- **ショートカット:** Ctrl+Z = Undo, Ctrl+Y = Redo

**注意:**
- GXModelViewerApp のメンバーに `UndoSystem m_undoSystem` を追加
- ImGuizmo の `ImGuizmo::IsUsing()` が `false` になった時点で Transform の差分をコマンド化
- Undo 時に GPU リソースの整合性を保証（モデル削除の Undo ではモデルを再読み込み）

---

### 43b: エンティティ生成 & プリミティブ追加

**目的:** インポートなしで空エンティティやプリミティブ形状を追加

**変更ファイル:**
- `GXModelViewer/Panels/SceneHierarchyPanel.cpp` — メニュー追加
- `GXModelViewer/GXModelViewerApp.cpp` — 生成ロジック

**追加メニュー項目（SceneHierarchyPanel のコンテキストメニュー）:**
```
右クリック → 追加
  ├── 空のエンティティ
  ├── ──────────────
  ├── キューブ
  ├── スフィア
  ├── プレーン
  ├── カプセル
  ├── シリンダー
  └── コーン
```

**プリミティブ生成:**
```cpp
void GXModelViewerApp::CreatePrimitive(const std::string& name, PrimitiveType type)
{
    int idx = m_sceneGraph.AddEntity(name);
    auto* entity = m_sceneGraph.GetEntity(idx);

    GX::MeshData meshData;
    switch (type) {
        case PrimitiveType::Cube:    meshData = GX::MeshGenerator::CreateBox(1, 1, 1); break;
        case PrimitiveType::Sphere:  meshData = GX::MeshGenerator::CreateSphere(0.5f, 32, 32); break;
        case PrimitiveType::Plane:   meshData = GX::MeshGenerator::CreatePlane(10, 10, 1, 1); break;
        case PrimitiveType::Capsule: meshData = GX::MeshGenerator::CreateCapsule(0.5f, 1.0f, 32); break;
        // ...
    }

    entity->ownedModel = std::make_unique<GX::Model>();
    entity->ownedModel->CreateFromMeshData(m_renderer, meshData);
    entity->model = entity->ownedModel.get();

    m_undoSystem.Execute(std::make_unique<AddEntityCommand>(/* ... */));
}
```

**注意:**
- `MeshGenerator` の API を確認 — `CreateBox`, `CreateSphere`, `CreatePlane` が存在するか
- 存在しない場合は `MeshData` を直接構築するヘルパーを作成
- 生成されたプリミティブのデフォルトマテリアル: 灰色 PBR (albedo=0.5, roughness=0.5, metallic=0.0)

---

### 43c: エンティティ複製 & マルチセレクト

**目的:** エンティティの複製とマルチセレクトで効率的なシーン構築

**複製 (Ctrl+D):**
```cpp
void GXModelViewerApp::DuplicateEntity(int sourceIdx)
{
    auto* src = m_sceneGraph.GetEntity(sourceIdx);
    if (!src) return;

    int newIdx = m_sceneGraph.AddEntity(src->name + " (Copy)");
    auto* dst = m_sceneGraph.GetEntity(newIdx);

    dst->transform = src->transform;
    dst->transform.SetPosition(
        src->transform.GetPosition().x + 1.0f,
        src->transform.GetPosition().y,
        src->transform.GetPosition().z
    );
    dst->materialOverride = src->materialOverride;
    dst->useMaterialOverride = src->useMaterialOverride;

    if (src->ownedModel) {
        // モデルの共有参照（メモリ節約、同じメッシュを指す）
        dst->model = src->model;
    }

    m_undoSystem.Execute(std::make_unique<AddEntityCommand>(/* ... */));
}
```

**マルチセレクト:**
- `SceneGraph` に `std::vector<int> selectedEntities` を追加（単一 `selectedEntity` を置換）
- Ctrl+クリック: 選択に追加/解除
- Shift+クリック: 範囲選択
- PropertyPanel: マルチセレクト時は共通プロパティのみ表示
- ImGuizmo: マルチセレクト時は中心位置にギズモ表示、全選択エンティティに変換適用

---

### 43d: ドラッグ&ドロップ親子関係変更

**目的:** SceneHierarchyPanel でドラッグ&ドロップによる親子関係変更

**変更ファイル:** `GXModelViewer/Panels/SceneHierarchyPanel.cpp`

**実装:**
```cpp
// ImGui のドラッグ&ドロップ API を使用
if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
{
    ImGui::SetDragDropPayload("ENTITY_INDEX", &entityIndex, sizeof(int));
    ImGui::Text("Move: %s", entity->name.c_str());
    ImGui::EndDragDropSource();
}

if (ImGui::BeginDragDropTarget())
{
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_INDEX"))
    {
        int draggedIdx = *(const int*)payload->Data;
        // 循環参照チェック: draggedIdx が targetIdx の祖先でないことを確認
        if (!IsAncestor(draggedIdx, targetIdx))
        {
            m_undoSystem.Execute(std::make_unique<ReparentCommand>(
                draggedIdx, oldParent, targetIdx));
        }
    }
    ImGui::EndDragDropTarget();
}
```

**循環参照防止:**
```cpp
bool SceneGraph::IsAncestor(int candidateIdx, int entityIdx) const
{
    int current = entityIdx;
    while (current >= 0) {
        if (current == candidateIdx) return true;
        current = m_entities[current].parentIndex;
    }
    return false;
}
```

---

### 43e: テクスチャ割り当て UI

**目的:** PropertyPanel でマテリアルのテクスチャスロットにテクスチャを割り当て可能にする

**変更ファイル:** `GXModelViewer/Panels/PropertyPanel.cpp`

**UI:**
```
Material Override
├── Albedo Map:     [albedo.png] [Browse...] [Clear]
├── Normal Map:     [none]       [Browse...] [Clear]
├── Roughness Map:  [none]       [Browse...] [Clear]
├── Metallic Map:   [none]       [Browse...] [Clear]
└── Emissive Map:   [none]       [Browse...] [Clear]
```

**実装:**
- テクスチャスロットごとに `ImGuiFileDialog` でファイル選択
- `TextureManager::LoadTexture(path)` でテクスチャをロード
- マテリアルの対応するテクスチャハンドルを更新
- テクスチャブラウザからのドラッグ&ドロップも対応

---

### 43f: シミュレーションモード

**目的:** シーン全体のシミュレーション（物理+アニメーション+スクリプト）をプレビュー

**UI:**
```
[▶ Play] [⏸ Pause] [⏹ Stop]    Simulation Mode: ON
```

**設計:**
- Play: 全エンティティのスナップショットを保存 → PhysicsWorld3D 初期化 → シミュレーション開始
- Pause: シミュレーションを一時停止
- Stop: スナップショットから全エンティティを復元
- シミュレーション中は Transform 編集を無効化

**状態管理:**
```cpp
enum class SimulationState { Editing, Playing, Paused };

struct SceneSnapshot {
    std::vector<Transform3D> transforms;
    std::vector<std::string> names;
    // ... 必要なエンティティ状態
};

class SimulationManager {
    SimulationState m_state = SimulationState::Editing;
    SceneSnapshot m_snapshot;

    void Play(SceneGraph& scene);   // スナップショット保存+開始
    void Pause();
    void Stop(SceneGraph& scene);   // スナップショット復元
    void Update(float dt, SceneGraph& scene);  // 物理+アニメ更新
};
```

---

### 完了基準
- [ ] Ctrl+Z / Ctrl+Y で Undo/Redo 動作
- [ ] 空エンティティ & プリミティブ（6種）の追加
- [ ] Ctrl+D でエンティティ複製
- [ ] ドラッグ&ドロップで親子関係変更
- [ ] テクスチャ割り当て UI 動作
- [ ] Play/Pause/Stop でシミュレーションモード動作
- [ ] 既存の全 19 パネルにリグレッションなし

---

## Phase 44: パフォーマンス最適化

### 目的
レンダリングパイプラインの効率を向上させ、大規模シーンでのフレームレートを改善する。

### 現状分析

**既存の最適化:**
- GPU インスタンシング (k_InstancingThreshold=4, Scene 自動バッチング)
- ParallelCommandRecorder (マルチスレッドコマンド記録)
- IndirectCommandBuffer (GPU 間接描画)
- AsyncComputeQueue (非同期コンピュート)
- DynamicBuffer (リングバッファ方式)
- Entity BoundsInfo + フラスタムカリング (Scene/Entity)
- BVH/Octree/Quadtree (空間加速構造)
- GPUProfiler (タイムスタンプベース計測)

**最適化可能な領域:**

| 領域 | 現状 | 改善案 | 期待効果 |
|------|------|--------|---------|
| フラスタムカリング | Entity 単位の AABB カリング | Octree ベースの階層カリング | 大規模シーンで 2-3x |
| 描画ソート | 順序なし描画 | マテリアル→深度ソート | PSO 切り替え削減 50%+ |
| リソースバリア | 個別バリア発行 | BarrierBatch による一括発行 | API コール削減 |
| 定数バッファ | オブジェクトごとに CBV バインド | SSBO/StructuredBuffer | ルートシグネチャ効率化 |
| テクスチャバインド | 描画ごとにディスクリプタ設定 | Bindless テクスチャ | バインドコスト削減 |
| ポストエフェクト | 各エフェクトが個別パス | マルチパス融合 | RT 切り替え削減 |

---

### 44a: 描画ソート & PSO 切り替え最適化

**変更ファイル:** `GXLib/Graphics/3D/Renderer3D.h`, `Renderer3D.cpp`

**現状:**
- `DrawModel()` は呼ばれた順に描画
- サブメッシュごとにマテリアル→PSO の切り替えが発生
- 同じ PSO のオブジェクトが交互に描画されると切り替えコストが累積

**改善:**
```cpp
// 描画キュー構造体
struct DrawCommand {
    const Model* model;
    Transform3D transform;
    uint64_t sortKey;  // PSO ID << 48 | Material ID << 32 | Depth
};

// ソート基準: PSO → マテリアル → 前後 (不透明: front-to-back)
void Renderer3D::SortDrawCommands()
{
    std::sort(m_drawQueue.begin(), m_drawQueue.end(),
        [](const DrawCommand& a, const DrawCommand& b) {
            return a.sortKey < b.sortKey;
        });
}
```

**ソートキー構成:**
- Bits 63-48: PSO ハンドル (不透明/透明/シャドウ)
- Bits 47-32: マテリアルハンドル
- Bits 31-0: カメラからの深度 (uint32_t にエンコード)
  - 不透明: front-to-back (early-Z 活用)
  - 透明: back-to-front (正しいブレンド)

**実装手順:**
1. `Renderer3D` に `std::vector<DrawCommand> m_drawQueue` を追加
2. `DrawModel()` を描画キューへの追加に変更（実描画は後段）
3. `FlushDrawCommands()` でソート → 一括描画
4. `EndFrame()` から `FlushDrawCommands()` を呼び出し

---

### 44b: BarrierBatch 活用

**変更ファイル:** `GXLib/Graphics/Device/BarrierBatch.h` (既存), 各使用箇所

**現状:**
- `BarrierBatch` クラスが存在するが、多くの箇所で個別バリアを発行
- `RenderTarget::TransitionTo()` は内部で `ResourceBarrier(1, &barrier)` を呼ぶ

**改善:**
- ポストエフェクトチェーンでの連続バリアを BarrierBatch で一括化
- Render → PostEffect 間のバリアを統合

```cpp
// Before (各エフェクトが個別にバリア発行):
hdrRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
bloomRT.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

// After (一括バリア):
BarrierBatch batch;
batch.Add(hdrRT.GetResource(), oldState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
batch.Add(bloomRT.GetResource(), oldState, D3D12_RESOURCE_STATE_RENDER_TARGET);
batch.Flush(cmdList);
```

**対象箇所:**
1. `PostEffectPipeline::Resolve()` — ピンポン RT の状態遷移
2. `Renderer3D::BeginFrame()` / `EndFrame()` — シャドウ→メイン→ポストのバリア
3. `RTReflections::Dispatch()` / `RTGI::Dispatch()` — UAV バリア

---

### 44c: Octree ベース階層フラスタムカリング

**変更ファイル:**
- `GXLib/Core/Scene/Scene.h`, `Scene.cpp`

**現状:**
- Entity ごとに `BoundsInfo` で AABB フラスタムカリング
- 全エンティティを線形走査

**改善:**
- シーン内の全エンティティを Octree に登録
- フラスタムクエリで可視エンティティのみを取得
- エンティティの移動時に Octree を更新

```cpp
class Scene {
    // 既存
    std::vector<Entity*> m_entities;

    // 追加
    GX::Octree<Entity*> m_spatialIndex;
    bool m_spatialIndexDirty = true;

    void RebuildSpatialIndex();
    void QueryVisibleEntities(const GX::Frustum& frustum,
                              std::vector<Entity*>& visible) const;
};
```

**更新タイミング:**
- エンティティ追加/削除時: `m_spatialIndexDirty = true`
- Transform 変更時: `m_spatialIndexDirty = true`
- `RenderInternal()` 冒頭で dirty ならリビルド

---

### 44d: GPU プロファイリング統合

**目的:** GPUProfiler の計測データをレンダリング最適化の意思決定に活用

**変更ファイル:** `GXLib/Graphics/Device/GPUProfiler.h`, `GPUProfiler.cpp`

**追加機能:**
```cpp
struct ProfilingStats {
    float shadowPassMs;
    float mainPassMs;
    float postEffectMs;
    float uiPassMs;
    float totalFrameMs;
    int drawCallCount;
    int triangleCount;
    int psoSwitchCount;
};

// Renderer3D に統計カウンタを追加
class Renderer3D {
    int m_drawCallCount = 0;
    int m_psoSwitchCount = 0;
    // ... BeginFrame() でリセット、DrawIndexedInstanced() ごとにインクリメント
};
```

---

### 完了基準
- [ ] 描画ソートにより PSO 切り替え回数 50%+ 削減
- [ ] BarrierBatch でリソースバリアの一括発行
- [ ] Octree フラスタムカリングで 1000+ エンティティシーンのフレーム時間改善
- [ ] GPU プロファイリング統計が ProfilerOverlay に表示
- [ ] 既存サンプル・テストにリグレッションなし

---

## Phase 45: 先進レンダリング

### 目的
レンダリングパイプラインに先進的な手法を追加し、視覚品質と効率を向上させる。

### 前提
Phase 44 の最適化基盤（描画ソート、BarrierBatch、空間加速構造）が完了していること。

---

### 45a: Hi-Z Occlusion Culling

**目的:** 深度バッファの階層的ミップマップを使い、遮蔽されたオブジェクトを GPU 側で事前にカリング

**新規ファイル:**
- `GXLib/Graphics/3D/HiZBuffer.h`
- `GXLib/Graphics/3D/HiZBuffer.cpp`
- `Shaders/HiZGenerate.hlsl` — Hi-Z ミップ生成 Compute Shader
- `Shaders/HiZCull.hlsl` — Hi-Z カリング Compute Shader

**設計:**

```
[深度バッファ] → [Hi-Z Generate CS] → [Hi-Z ミップチェーン]
                                          ↓
[バウンディングボックスバッファ] → [Hi-Z Cull CS] → [可視フラグバッファ]
                                                      ↓
                                             [IndirectCommandBuffer]
                                                      ↓
                                             [ExecuteIndirect描画]
```

**Hi-Z Generate (Compute Shader):**
```hlsl
// 深度バッファの各ミップレベルを生成
// ミップN+1 の各ピクセル = ミップN の 2x2 ピクセルの最大深度
[numthreads(8, 8, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
    float d00 = srcMip[tid.xy * 2 + uint2(0, 0)];
    float d01 = srcMip[tid.xy * 2 + uint2(0, 1)];
    float d10 = srcMip[tid.xy * 2 + uint2(1, 0)];
    float d11 = srcMip[tid.xy * 2 + uint2(1, 1)];
    dstMip[tid.xy] = max(max(d00, d01), max(d10, d11));
}
```

**Hi-Z Cull (Compute Shader):**
```hlsl
// 各オブジェクトのスクリーン空間 AABB を Hi-Z とテスト
// 結果を IndirectCommandBuffer のカウンタに反映
[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    AABB aabb = objectBounds[tid];
    float4 screenAABB = ProjectToScreen(aabb, viewProj);
    int mipLevel = CalculateMipLevel(screenAABB);
    float hiZDepth = SampleHiZ(screenAABB.center, mipLevel);

    if (screenAABB.nearZ <= hiZDepth) // 可視
    {
        uint idx;
        InterlockedAdd(visibleCount[0], 1, idx);
        visibleCommands[idx] = drawCommands[tid];
    }
}
```

**C++ 側:**
```cpp
class HiZBuffer {
public:
    bool Initialize(GraphicsDevice& device, uint32_t width, uint32_t height);
    void GenerateHiZ(CommandList& cmdList, DepthBuffer& depthBuffer);
    void CullObjects(CommandList& cmdList,
                     const Buffer& boundsBuf, uint32_t objectCount,
                     const IndirectCommandBuffer& indirectBuf);
    ID3D12Resource* GetHiZTexture() const;

private:
    Texture m_hiZTexture;  // R32_FLOAT with mip chain
    PipelineState m_generatePSO;
    PipelineState m_cullPSO;
    RootSignature m_rootSig;
    uint32_t m_mipLevels;
};
```

**統合ポイント:**
1. `Renderer3D::BeginFrame()` 後に前フレームの深度から Hi-Z 生成
2. Scene の全エンティティの AABB を GPU バッファにアップロード
3. Hi-Z Cull CS で可視オブジェクトを選別
4. `ExecuteIndirect()` で可視オブジェクトのみ描画

---

### 45b: Volumetric Clouds

**目的:** レイマーチベースのボリュメトリッククラウド

**新規ファイル:**
- `GXLib/Graphics/3D/VolumetricClouds.h`
- `GXLib/Graphics/3D/VolumetricClouds.cpp`
- `Shaders/VolumetricClouds.hlsl`

**設計:**
- レイマーチ（SV_VertexID フルスクリーン三角形 → PS でマーチ）
- ワーリーノイズ + パーリンノイズで雲密度
- ビールランバート法則で光散乱
- 時間経過で雲が流れる（風パラメータ）
- Temporal reprojection でノイズ削減

**パラメータ:**
```cpp
struct CloudParams {
    float cloudLayerBottom = 1500.0f;   // 雲底高度
    float cloudLayerTop = 3000.0f;      // 雲頂高度
    float coverage = 0.5f;              // 雲の被覆率 0-1
    float density = 0.05f;              // 密度乗数
    float windSpeed = 10.0f;            // 風速 (m/s)
    DirectX::XMFLOAT3 windDirection = {1, 0, 0}; // 風向き
    int marchSteps = 64;                // レイマーチステップ数
    int lightSteps = 6;                 // ライトマーチステップ数
    float silverLiningIntensity = 0.5f; // シルバーライニング強度
};
```

**ポストエフェクトパイプラインへの統合:**
- Skybox の後、シーン描画の前（または後）にクラウドパスを挿入
- 深度バッファを使ってシーンとの交差を計算（雲の手前にあるオブジェクトは遮蔽）

---

### 45c: Bindless テクスチャ

**目的:** ディスクリプタヒープ全体を一度にバインドし、テクスチャインデックスでアクセス

**変更ファイル:**
- `GXLib/Graphics/Device/DescriptorHeap.h`, `.cpp`
- `GXLib/Graphics/3D/Renderer3D.h`, `.cpp`
- `Shaders/PBR.hlsl` (修正)

**設計:**
```hlsl
// HLSL: Bindless テクスチャアクセス
Texture2D textures[] : register(t0, space1);  // Unbounded array
SamplerState linearSampler : register(s0);

// マテリアルの定数バッファにテクスチャインデックスを格納
cbuffer MaterialCB : register(b1)
{
    int albedoTexIndex;    // textures[albedoTexIndex].Sample(...)
    int normalTexIndex;
    int roughnessTexIndex;
    int metallicTexIndex;
    // ...
};
```

**C++ 側:**
```cpp
// TextureManager がグローバル SRV ヒープを管理
// ハンドル = ヒープ内のインデックス → シェーダーに渡す
class TextureManager {
    // 既存のハンドルがそのまま SRV インデックスになる
    int LoadTexture(const std::wstring& path);  // returns heap index
};
```

**注意:**
- D3D12 の `D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE` が必要
- SM 6.0+ が必要（既に dxcompiler を使用しているので問題なし）
- フォールバック: Bindless 非対応 GPU 向けに従来のバインド方式を残す

---

### 45d: Screen Space Contact Shadows

**目的:** 小さなオブジェクト（草、小石等）のセルフシャドウをスクリーンスペースで近似

**新規ファイル:**
- `GXLib/Graphics/PostEffect/ContactShadows.h`
- `GXLib/Graphics/PostEffect/ContactShadows.cpp`
- `Shaders/ContactShadows.hlsl`

**設計:**
- 各ピクセルからライト方向へスクリーンスペースでレイマーチ
- 深度バッファとの交差でシャドウ判定
- CSM が表現できない細部のシャドウを補完

**パラメータ:**
```cpp
struct ContactShadowParams {
    float maxDistance = 0.3f;   // マーチ最大距離 (ビュースペース)
    int stepCount = 16;         // マーチステップ数
    float thickness = 0.05f;   // 厚み閾値
    float intensity = 0.5f;    // シャドウ強度
};
```

**パイプライン統合:**
- SSAO の直後に配置（共に深度・法線を使用）
- 出力: シャドウマスクテクスチャ（R8_UNORM）
- ライティングパスでシャドウマスクを乗算

---

### 45e: Temporal Super Resolution (簡易版)

**目的:** TAA の拡張としてアップスケーリングを実現（内部解像度を下げてパフォーマンス向上）

**変更ファイル:**
- `GXLib/Graphics/PostEffect/TAA.h`, `TAA.cpp`

**設計:**
- 内部解像度を 75% に下げて描画
- TAA のテンポラル蓄積でアップスケール
- モーションベクトルによるリプロジェクション
- シャープニングフィルタで品質回復

**パラメータ追加:**
```cpp
float internalResolutionScale = 0.75f;  // 内部解像度スケール (0.5-1.0)
float sharpness = 0.5f;                 // シャープニング強度
```

---

### 完了基準
- [ ] Hi-Z Occlusion Culling で 1000+ オブジェクトシーンの描画コール 30%+ 削減
- [ ] Volumetric Clouds がリアルタイムで描画（30fps 以上維持）
- [ ] Bindless テクスチャでマテリアル切り替えコスト削減
- [ ] Contact Shadows で CSM 補完のセルフシャドウ
- [ ] 既存サンプル・テストにリグレッションなし

---

## 付録

### A. ファイル命名規則

| 種類 | パターン | 例 |
|------|---------|-----|
| ヘッダ | PascalCase.h | `HiZBuffer.h` |
| 実装 | PascalCase.cpp | `HiZBuffer.cpp` |
| シェーダー | PascalCase.hlsl | `VolumetricClouds.hlsl` |
| インクルードシェーダー | PascalCase.hlsli | `ShaderModelCommon.hlsli` |
| テスト | test_PascalCase.cpp | `test_Camera3D.cpp` |
| サンプル | SampleName/main.cpp | `Physics3DShowcase/main.cpp` |

### B. サンプルテンプレート

```cpp
/// @file Samples/XXXShowcase/main.cpp
/// @brief XXX のデモ。
///
/// 説明文。
///
/// Controls:
///   WASD       - 移動
///   ESC        - 終了
#include "GXEasy.h"
// #include 必要なヘッダ

class XXXShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: XXX";
        config.width = 1280;
        config.height = 720;
        config.bgR = 20; config.bgG = 20; config.bgB = 30;
        return config;
    }

    void Start() override { /* 初期化 */ }
    void Update(float dt) override { /* 更新 */ }
    void Draw() override { /* 描画 */ }

private:
    // メンバ変数
};

GX_EASY_APP(XXXShowcaseApp)
```

### C. 3D サンプルテンプレート（Renderer3D 使用時）

```cpp
/// @file Samples/XXX3DShowcase/main.cpp
#include "GXEasy.h"
#include "Compat/CompatContext.h"
// 追加 #include

class XXX3DShowcaseApp : public GXEasy::App
{
public:
    GXEasy::AppConfig GetConfig() const override
    {
        GXEasy::AppConfig config;
        config.title = L"GXLib Sample: XXX 3D";
        config.width = 1280;
        config.height = 720;
        config.bgR = 10; config.bgG = 10; config.bgB = 15;
        config.enable3D = true;  // 3D レンダラー有効化
        return config;
    }

    void Start() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& r = ctx.renderer3D;
        auto& c = ctx.camera3D;
        auto& p = ctx.postEffectPipeline;

        // ポストエフェクト設定
        p.SetTonemappingMode(GX::TonemappingMode::ACES);
        p.SetExposure(1.0f);
        p.GetBloom().SetEnabled(true);
        p.GetSSAO().SetEnabled(true);
        p.SetFXAAEnabled(true);

        // ライティング
        GX::LightData lights[1];
        lights[0] = GX::Light::CreateDirectional(
            { 0.3f, -1.0f, 0.5f }, { 1.0f, 0.98f, 0.95f }, 3.0f);
        r.SetLights(lights, 1, { 0.08f, 0.08f, 0.08f });

        // カメラ
        float aspect = static_cast<float>(ctx.swapChain.GetWidth())
                     / ctx.swapChain.GetHeight();
        c.SetPerspective(DirectX::XM_PIDIV4, aspect, 0.1f, 500.0f);
        c.SetPosition(0.0f, 5.0f, -10.0f);
        c.LookAt({ 0.0f, 0.0f, 0.0f });

        // メッシュ/モデル読み込み
        // ...
    }

    void Update(float dt) override
    {
        // 入力処理、ロジック更新
    }

    void Draw() override
    {
        auto& ctx = GX_Internal::CompatContext::Instance();
        auto& r = ctx.renderer3D;
        auto& c = ctx.camera3D;
        auto& p = ctx.postEffectPipeline;
        auto& db = ctx.depthBuffer;

        r.BeginFrame(ctx.cmdList, c);
        // r.DrawModel(...);
        r.EndFrame();

        db.TransitionTo(ctx.cmdList, D3D12_RESOURCE_STATE_DEPTH_READ);
        p.Resolve(ctx.cmdList, ctx.swapChain, db, c);
        db.TransitionTo(ctx.cmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // 2D HUD (SpriteBatch / TextRenderer)
    }

private:
    // メンバ変数
};

GX_EASY_APP(XXX3DShowcaseApp)
```

### D. テストテンプレート

```cpp
/// @file Tests/test_XXX.cpp
#include <gtest/gtest.h>
#include "対象ヘッダ.h"

// ヘルパー
namespace {
    constexpr float k_Eps = 1e-4f;
}

TEST(XXX, DefaultValues)
{
    GX::XXX obj;
    EXPECT_EQ(obj.GetSomething(), expectedValue);
}

TEST(XXX, SomeOperation)
{
    GX::XXX obj;
    obj.DoSomething(param);
    EXPECT_NEAR(obj.GetResult(), expected, k_Eps);
}
```

### E. Phase 間の依存関係図

```
Phase 41 (テスト拡充) ─────────────────┐
                                        ├──→ Phase 43 (エディタ強化)
Phase 42 (サンプル追加) ──── 独立 ───── │
                                        │
Phase 44 (パフォーマンス最適化) ────────┼──→ Phase 45 (先進レンダリング)
```

### F. 各 Phase の開始前チェックリスト

1. `git status` で未コミット変更がないことを確認
2. `cmake --build build --config Debug` でビルドが通ることを確認
3. `ctest -C Debug` で全テストがパスすることを確認
4. `MEMORY.md` で前 Phase の完了が記録されていることを確認
5. 本指令書の対象 Phase セクションを精読

### G. 各 Phase の完了後チェックリスト

1. 全ファイルがビルドできること
2. 既存テストがリグレッションなしでパス
3. 新規テストが全パス
4. 新規サンプルが実行可能
5. `MEMORY.md` に Phase 完了を記録
6. コミットメッセージに Phase 番号と概要を含める

---

*本指令書は Phase 36-40 指令書 (`docs/Phase36-40_Directive.md`) の後継であり、
GXLib エンジンの成熟フェーズ（品質・安定化・高度化）をカバーする。*
