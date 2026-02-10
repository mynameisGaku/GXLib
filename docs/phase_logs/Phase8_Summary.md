# Phase 8 Summary: Math / Physics / Collision

## Overview
GXLib独自の数学ライブラリ、2D/3D衝突判定、空間分割構造、2D/3Dの物理エンジンを実装。
数学型 (Vector2/3/4, Matrix4x4, Quaternion, Color) は DirectXMath の XMFLOAT 系をゼロオーバーヘッドで継承し、
既存の DirectXMath コードとの暗黙的な相互変換を維持しつつ、演算子オーバーロードや便利メソッドを追加。
衝突判定は SAT (分離軸定理) や Moller-Trumbore レイ-三角形交差などのアルゴリズムを実装。
空間分割は Quadtree/Octree/BVH のテンプレートヘッダーオンリーライブラリとして提供。
2D物理はカスタムインパルスベースエンジン、3D物理は Jolt Physics v5.3.0 を PIMPL パターンでラップし統合。

## Completion Condition
> Vector/Matrix/Quaternion数学ライブラリ、2D/3Dコリジョン、空間分割、2Dカスタム物理、3D Jolt物理が動作 → **達成**

---

## Sub-phases

| Sub-phase | Content |
|-----------|---------|
| 8a | Math types (Vector2/3/4, Matrix4x4, Quaternion, Color, MathUtil, Random) |
| 8b | Collision2D / Collision3D (AABB, Circle, Sphere, OBB, SAT, Moller-Trumbore, Raycast) |
| 8c | Spatial structures (Quadtree, Octree, BVH with SAH split) |
| 8d | Physics2D (RigidBody2D, PhysicsWorld2D — custom impulse-based engine) |
| 8e | Physics3D (PhysicsWorld3D — Jolt Physics v5.3.0 PIMPL wrapper) |

## New Files

| File | Description |
|------|-------------|
| `GXLib/Math/MathUtil.h` | 定数 (PI, TAU, EPSILON)、Clamp, Lerp, SmoothStep, 角度変換などのユーティリティ関数群 |
| `GXLib/Math/Vector2.h` | 2Dベクトル (XMFLOAT2継承)、演算子, Length, Dot, Cross, Lerp, Min/Max |
| `GXLib/Math/Vector3.h` | 3Dベクトル (XMFLOAT3継承)、Cross, Reflect, Transform, TransformNormal |
| `GXLib/Math/Vector4.h` | 4Dベクトル (XMFLOAT4継承)、Vector3+W コンストラクタ |
| `GXLib/Math/Matrix4x4.h` | 4x4行列 (XMFLOAT4X4継承)、ToXMMATRIX/FromXMMATRIX、Translation/Scaling/Rotation/LookAt/Perspective |
| `GXLib/Math/Quaternion.h` | クォータニオン (XMFLOAT4継承)、Slerp, FromAxisAngle, FromEuler, ToEuler, RotateVector |
| `GXLib/Math/Color.h` | RGBA色 (float4)、uint32_t/uint8_t コンストラクタ, HSV変換, プリセット色 |
| `GXLib/Math/Random.h/cpp` | Mersenne Twister 乱数、Int/Float範囲指定, PointInCircle/Sphere, Direction2D/3D, Global() |
| `GXLib/Math/Collision/Collision2D.h` | 2D形状 (AABB2D, Circle, Line2D, Polygon2D)、交差判定, Raycast, Sweep |
| `GXLib/Math/Collision/Collision3D.h` | 3D形状 (AABB3D, Sphere, Ray, Plane, Frustum, OBB, Triangle)、SAT, Moller-Trumbore, Raycast |
| `GXLib/Math/Collision/Quadtree.h` | テンプレート四分木 (AABB/Circle クエリ, GetPotentialPairs) |
| `GXLib/Math/Collision/Octree.h` | テンプレート八分木 (AABB/Sphere/Frustum クエリ, GetPotentialPairs) |
| `GXLib/Math/Collision/BVH.h` | テンプレートBVH (SAH分割, AABB/Rayクエリ, 最近傍Raycast) |
| `GXLib/Physics/RigidBody2D.h` | 2D剛体 (位置, 速度, 質量, コライダー, BodyType, トリガー, レイヤー) |
| `GXLib/Physics/PhysicsWorld2D.h/cpp` | 2D物理ワールド (重力, ブロードフェーズ, インパルス応答, 摩擦, レイキャスト, コールバック) |
| `GXLib/Physics/PhysicsWorld3D.h/cpp` | 3D物理ワールド (Jolt Physics PIMPL, ボディ管理, シェイプ作成, レイキャスト) |
| `GXLib/Physics/PhysicsShape.h` | 物理シェイプハンドル (Jolt内部シェイプ参照のvoid*ラッパー) |

## Modified Files

| File | Changes |
|------|---------|
| `CMakeLists.txt` | Jolt Physics を FetchContent (v5.3.0), USE_STATIC_MSVC_RUNTIME_LIBRARY OFF |
| `GXLib/CMakeLists.txt` | Math/*.cpp, Physics/*.cpp を GLOB_RECURSE に追加; Jolt リンク |
| `GXLib/pch.h` | `<random>`, `<functional>` 追加 (Random用 mt19937, PhysicsWorld コールバック用) |
| `Sandbox/main.cpp` | 2D/3D物理デモ統合, Phase 8 ステータス表示 |

## Architecture

### Math Types (DirectXMath継承)
```
XMFLOAT2 ← Vector2  (演算子 +,-,*,/、Length, Dot, Cross2D, Lerp, Min/Max)
XMFLOAT3 ← Vector3  (Cross3D, Reflect, Transform, TransformNormal, 方向定数)
XMFLOAT4 ← Vector4  (Vector3+W ctor)
XMFLOAT4 ← Quaternion (Slerp, NLerp, FromAxisAngle/Euler, ToEuler, RotateVector)
XMFLOAT4X4 ← Matrix4x4 (ToXMMATRIX/FromXMMATRIX, Translation/Scaling/Rotation/LookAt/Perspective)

Color (独立struct, float r/g/b/a)
  ├── float/uint32_t/uint8_t ctors
  ├── HSV変換 (FromHSV/ToHSV)
  └── プリセット色 (White, Black, Red, Green, Blue, Yellow, Cyan, Magenta, Transparent)

MathUtil: PI, TAU, EPSILON, Clamp, Lerp, InverseLerp, Remap, SmoothStep, SmootherStep,
          DegreesToRadians/RadiansToDegrees, NormalizeAngle, IsPowerOfTwo, NextPowerOfTwo

Random (Mersenne Twister mt19937):
  ├── Int/Float (範囲指定)
  ├── Vector2InRange/Vector3InRange
  ├── PointInCircle/PointInSphere (rejection sampling, 均一分布)
  ├── Direction2D (angle-based) / Direction3D (Marsaglia method)
  └── Global() (static singleton)
```

### Collision Detection (2D)
```
形状: AABB2D, Circle, Line2D, Polygon2D (winding number)
判定: TestAABBvsAABB, TestCirclevsCircle, TestAABBvsCircle
      TestPointIn{AABB,Circle,Polygon}, TestLinevs{AABB,Circle,Line}
交差: IntersectAABBvsAABB → HitResult2D (point, normal, depth)
      IntersectCirclevsCircle, IntersectAABBvsCircle
Ray:  Raycast2D (AABB: parametric slab, Circle: quadratic)
Sweep: SweepCirclevsCircle (相対速度→レイキャスト)
```

### Collision Detection (3D)
```
形状: AABB3D, Sphere, Ray, Plane, Frustum (6-plane), OBB (center+halfExtents+axes), Triangle
判定: TestSphereVsSphere, TestAABBVsAABB, TestSphereVsAABB, TestOBBVsOBB (SAT 15軸)
      TestFrustumVs{Sphere,AABB,Point}
交差: IntersectSphereVsSphere, IntersectSphereVsAABB → HitResult3D
Ray:  RaycastSphere, RaycastAABB (slab method), RaycastPlane
      RaycastTriangle (Moller-Trumbore, u/v barycentric output)
      RaycastOBB (OBB→ローカル空間変換→AABB raycast)
Sweep: SweepSphereVsSphere
Closest: ClosestPointOn{AABB, Triangle, Line}
```

### Spatial Partitioning
```
Quadtree<T> (2D空間分割):
  ├── AABB2D bounds, maxDepth=8, maxObjects=8
  ├── 4分割 (NW, NE, SW, SE)
  ├── Query(AABB2D), Query(Circle)
  └── GetPotentialPairs (ancestor propagation)

Octree<T> (3D空間分割):
  ├── AABB3D bounds, maxDepth=8, maxObjects=8
  ├── 8分割 (bit-mask octant selection)
  ├── Query(AABB3D), Query(Sphere), Query(Frustum)
  └── GetPotentialPairs

BVH<T> (Bounding Volume Hierarchy):
  ├── Build: SAH (Surface Area Heuristic) 分割
  │   └── 3軸ソート → 全分割点のコスト評価 → 最小コスト選択
  ├── Query(AABB3D), Query(Ray)
  └── Raycast → 最近傍ヒット (early out by closestT)
```

### 2D Physics (Custom Engine)
```
PhysicsWorld2D
  ├── IntegrateBodies(dt)
  │   ├── 重力適用
  │   ├── 蓄積力/トルク適用 (F * invMass * dt)
  │   ├── 減衰 (linear/angular damping)
  │   └── 位置/回転の積分
  ├── BroadPhase → O(n^2) AABB overlap + レイヤーフィルタ
  ├── NarrowPhase → Circle/Circle, AABB/AABB, AABB/Circle 判定
  └── ResolveCollision
      ├── 位置補正 (Baumgarte-style, percent=0.8, slop=0.01)
      ├── インパルス応答 (反発係数 e = min(a,b))
      └── 摩擦 (Coulomb friction, mu = sqrt(a*b))

RigidBody2D: position, velocity, mass, restitution, friction, damping
             BodyType (Static/Dynamic/Kinematic), ColliderShape2D (Circle/AABB)
             isTrigger, userData, layer bitmask, ApplyForce/Impulse/Torque
```

### 3D Physics (Jolt Physics PIMPL Wrapper)
```
PhysicsWorld3D (PIMPL: struct Impl)
  ├── Initialize(maxBodies) → RegisterDefaultAllocator, Factory, RegisterTypes
  │   ├── TempAllocatorImpl (32MB)
  │   ├── JobSystemThreadPool (hardware_concurrency - 1 threads)
  │   └── PhysicsSystem::Init (maxBodies, maxBodyPairs, maxContactConstraints)
  ├── Shape creation: Box, Sphere, Capsule, Mesh → PhysicsShape (void* → JPH::ShapeRefC*)
  ├── Body management: AddBody(shape, settings) → PhysicsBodyID
  │   ├── MotionType → JPH::EMotionType mapping
  │   ├── CalculateInertia (mass override)
  │   └── Friction, restitution, damping settings
  ├── Body manipulation: Set/Get Position/Rotation/Velocity, ApplyForce/Impulse/Torque
  ├── Step(dt) → PhysicsSystem::Update
  ├── Raycast → NarrowPhaseQuery::CastRay → RaycastResult
  └── Callbacks: onContactAdded, onContactRemoved (ContactListenerImpl)

Jolt内部クラス (anonymous namespace):
  ├── BPLayerInterface (2-layer: NON_MOVING, MOVING)
  ├── ObjectVsBroadPhaseFilter (Static → Moving のみ衝突)
  ├── ObjectLayerPairFilter (Static-Static ペアをスキップ)
  └── ContactListenerImpl → PhysicsWorld3D callbacks

変換ヘルパー:
  ToJolt(Vector3→Vec3, Quaternion→Quat)
  FromJoltV(Vec3→Vector3), FromJoltR(RVec3→Vector3), FromJoltQ(Quat→Quaternion)
```

## Key Design Decisions
- **DirectXMath継承 (ゼロオーバーヘッド)**: Vector2/3/4, Matrix4x4, Quaternion を XMFLOAT 系から継承し、メモリレイアウト同一でキャスト不要の相互変換を実現
- **ヘッダーオンリー数学ライブラリ**: Random.cpp のみ .cpp (mt19937 statics)。他は全てインラインで最適化を促進
- **Color の複数コンストラクタ**: float, uint32_t, uint8_t の3種類を提供。ただし `Color(1, 0, 0)` のような整数リテラルで曖昧性が発生するため、float版は `1.0f` と明示する必要がある
- **SAH (Surface Area Heuristic) BVH分割**: 全3軸の全分割点を総当たり評価し、最小表面積コストの分割を選択。小規模オブジェクトセットで十分な品質
- **O(n^2) 2Dブロードフェーズ**: 小〜中規模ボディ数を想定し、シンプルなペアワイズAABB判定を採用。大規模時は Quadtree を組み合わせ可能
- **Jolt PIMPL パターン**: PhysicsWorld3D.cpp のみに Jolt ヘッダーを include し、公開ヘッダーからの依存を完全に排除。PhysicsShape は void* ハンドルで抽象化
- **Jolt FetchContent v5.3.0**: CMake FetchContent で自動取得。USE_STATIC_MSVC_RUNTIME_LIBRARY=OFF で GXLib の /MDd と互換性を確保
- **2D物理の画面座標系対応**: Y-down の画面座標系では重力Yを正の値に設定 (物理学の慣例と逆)
- **テンプレートヘッダーオンリー空間分割**: Quadtree/Octree/BVH を全てテンプレートクラスとしてヘッダーに実装。型パラメータで任意のオブジェクト型を格納可能
- **Moller-Trumbore レイ-三角形交差**: バリセントリック座標 (u, v) を出力し、テクスチャ座標補間やヒット判定に活用可能
- **OBB vs OBB (SAT 15軸)**: 3+3+9=15軸の分離軸テストで OBB 同士の正確な交差判定を実現
- **Random: rejection sampling**: PointInCircle/PointInSphere は棄却法で均一分布を保証、Direction3D は Marsaglia 法で球面上の均一方向を生成

## Issues Encountered
- **Jolt USE_STATIC_MSVC_RUNTIME_LIBRARY**: デフォルトONだと /MT でリンクされ、GXLib の /MDd と衝突。FetchContent 時に OFF を明示して解決。
- **Jolt RegisterDefaultAllocator() 必須**: v5.3.0 では Reallocate 関数ポインタが追加されており、個別関数設定ではなく RegisterDefaultAllocator() を使用する必要があった。
- **JPH::RVec3 型の曖昧性**: Jolt のダブルプレシジョンビルドでは RVec3 が DVec3 になる場合があり、FromJoltV/FromJoltR/FromJoltQ と変換ヘルパーを分離して対応。
- **CalculateInertia の mass アサーション**: mMassPropertiesOverride.mMass に質量を設定しないと mass>0 のアサーションが発生。OverrideMassProperties と mMass をセットで設定して解決。
- **TempAllocatorImpl サイズ不足**: maxBodies が大きい場合、デフォルトの 10MB では不足。32MB に増量して安定化。
- **maxBodyPairs/maxContactConstraints 上限**: maxBodies に比例して設定すると過大になるため、上限値 (65536/10240) でキャップ。
- **Color コンストラクタ曖昧性**: `Color(1, 0, 0)` が int→uint8_t と int→float の両方に一致。明示的な `1.0f` リテラル使用で回避。
- **GetNumBroadPhaseLayers 戻り値型**: Jolt が `JPH::uint` を期待するが、`uint` と書くと未定義。`JPH::uint` を明示して解決。

## Verification
- Build: OK
- Math types: Vector2/3/4, Matrix4x4, Quaternion, Color 全演算正常動作
- Collision2D: AABB/Circle/Line/Polygon 判定+交差+Raycast 正常
- Collision3D: Sphere/AABB/OBB/Frustum/Triangle 判定+Raycast (Moller-Trumbore) 正常
- Spatial: Quadtree/Octree/BVH のInsert/Query正常動作
- Physics2D: 重力落下、衝突応答 (インパルス+摩擦)、トリガーコールバック正常
- Physics3D: Jolt初期化OK、ボディ追加/シミュレーション/レイキャスト正常動作
