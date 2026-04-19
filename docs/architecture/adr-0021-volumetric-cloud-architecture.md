# ADR-0021: Volumetric Cloud Architecture (Multi-Layer + Weather-Map Driven + Gameplay-Integrated)

## Status
Accepted

## Review history
- 2026-04-19 Proposed (initial authoring)
- 2026-04-19 Same-session independent review by technical-director + technical-artist agents caught 20+ real issues across 9 TD categories + 9 TA categories (Verdict: CONCERNS on both)
- 2026-04-19 Revised: ~18 targeted patches applied covering:
  - R1: `IXAudio2Texture*` phantom API → `int GetGPUTextureHandle()` (GXLib-style handle)
  - R2/Issue 9: SkyAtmosphere LUT hidden scope → **Phase D.0 sub-phase** explicit
  - R3: `CloudType` naming collision → new enum renamed to `CloudClass`, deprecation shim documented
  - R4: Phase C structural dependency → **Phase D.0 density compute module extraction** precursor
  - R5/Issue 18: HIGH tier 3.5 ms 矛盾 → MED-tier-only 3 ms binding, HIGH/ULTRA best-effort
  - R6: 4 missing forbidden patterns added (CPU noise regen, sync editor readback, shadow res over-limit, CPU/GPU density duplicate)
  - R7: Every forbidden pattern got a **Detection** mechanism line (grep rule / runtime assert / file-presence check)
  - R8: Motion vector contract clarified (camera delta + wind advection, **not** GBuffer motion vectors)
  - R9/§3a: EventBus handler category table (Lightning / WeatherChange / CloudStateChange)
  - TA Issue 4: "1/16 checkerboard" upgrade described as **temporal pipeline 本格的書き換え**, not tuning
  - TA Issue 7: Multi-layer 合成モデル = 単一統合 ray march、additive composite は new forbidden pattern
  - TA Issue 10-11: CloudTemporalPass = cloud-only history, CloudCompositePass が scene blend 担当 (分離)
  - TA Issue 15: Phase B に FileWatcher hot-reload 追加
  - TA Issue 16: Per-layer noise overrides (diffusivity / detailFadeDistance / densityMul)
  - Cross-ADR G1-G5: §9 "Cross-ADR Integration Commitments" 新設、§7 "Layer 1 / Layer 2 Readiness" 新設、ADR-0008 amendment 方式を footnote → in-place edit に修正
  - Validation criteria: FLIP/SSIM ツール化、perceptual ≤10% 定量化、batch query 1000 points、keyframe interpolation correctness、quality tier switching stall、disable path、hurricane → heavy_storm rename 等 11 件追加
- 2026-04-19 Accepted (same-session). Phase B 実装が実質的な load test — Phase B 完時点で blocking issue が surfaced した場合は amendment で対応 (project 慣習)

## Date
2026-04-19

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Graphics / Rendering / Gameplay |
| **Knowledge Risk** | LOW — Nubis lineage (2015-2023) + Frostbite atmosphere (Hillaire 2016/2020) + RDR2 (SIGGRAPH 2019) are all published. Target is well within LLM training data. AC8 Cloudly specifics are proprietary — ADR target caps at AC7 / Nubis Evolved parity with an optional Phase F for Nubis Cubed class |
| **References Consulted** | `GXLib/Graphics/3D/VolumetricClouds.{h,cpp}` (284 + 1034 lines), `Shaders/VolumetricClouds.hlsl` (570 lines), `Shaders/VolumetricCloudsTemporal.hlsl` (140 lines), `GXLib/Graphics/PostEffect/PostEffectPipeline.{h,cpp}`, `docs/engine-reference/gxlib/volumetric-clouds-analysis-2026-04-19.md` (本 ADR の基礎資料、詳細ギャップ分析 + 技術 benchmark + 参照書誌 10 件) |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | WeatherMap 駆動で既存視覚を保てる (Phase B 回帰); Multi-layer ray march が 1080p で cloud pass ≤3 ms 達成 (Phase D); `IsInCloud` / `DensityAlongRay` gameplay API が real-time frame budget で応答 (Phase C); cloud → terrain shadow が視覚的に確認可能 (Phase D); Lightning + precipitation が AC7 mission 4/5 級ドラマ性を再現 (Phase E); AC7 screenshot 並置テストで静止画差分 10% 以下 (Phase D 完了時) |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0002 (DX12 backend — GraphicsDevice + ShaderRegistry が前提), ADR-0007 (Asset Database — WeatherMap + cloud keyframe data asset flow), ADR-0008 (Rendering Pipeline — FrameGraph 新 pass 登録 + PostFX chain 統合), ADR-0017 (Two-Layer Accessibility — cloud API は L2 advanced) |
| **Enables** | Flight-combat / flight-sim ゲームタイトル、AC系 gameplay (icing / missile-lock degradation / radar / laser attenuation)、気象駆動 gameplay、cinematic storm sequence |
| **Blocks** | None (新機能、既存コードを段階的置き換え) |
| **Ordering Note** | ADR-0008 (Rendering) の FrameGraph に cloud-related pass を複数追加するため、ADR-0008 §Decision の pass list への addendum 参照が必要。ADR-0008 は既に Accepted のため、本 ADR § Migration Plan 内で addendum footnote を置く |

## Context

### Problem Statement

GXLib は現状、Nubis 1 (SIGGRAPH 2015) 系統の volumetric clouds を実装済み — 128³ Perlin-Worley + 32³ detail 3D texture、32-step ray march、6-step light cone、Wrenninge 式 multi-octave scattering、Beer-Powder、dual-lobe Henyey-Greenstein、bilateral upsample + temporal reprojection まで。**ビジュアル単品としては AC7 screenshot の 60-70% 相当。**

しかし ACE COMBAT 7 (2019) / ACE COMBAT 8 (2026 予定) 級のクオリティには以下が不足:

1. **Cloud identity が読めない** — 単層 + 純 procedural FBM 駆動のため、cumulus / stratus / cumulonimbus の視覚差別が薄い
2. **Multi-layer 構造がない** — AC8 Cloudly の核機能である「雲層 = altimeter」が実装不可能
3. **Weather map 非搭載** — regional 気象差異 + 気象推移が表現できない
4. **Gameplay query API 非搭載** — `IsInCloud(pos)` 系が無く、AC7 の "cloud = gameplay" 哲学 (icing, missile lock, radar, laser attenuation) を上位 game code が実装できない
5. **Terrain shadow 非搭載** — AC7 の象徴的「雲の影が海面を走る」が不可能
6. **Atmosphere LUT 部分結合** — Hillaire 2020 級の physical coupling 未達成、distant clouds aerial perspective が不自然
7. **Dynamics 不足** — 雷・precipitation・keyframe sequencer 不在、storm sequence 表現力欠如
8. **Artist tool 不在** — ImGui 設定のみで weather author 不可能、Nubis 2017 modeler 相当なし

プロジェクトが flight-combat 系タイトル (特に AC 系インスパイア) を視野に入れる場合、**clouds は単なる視覚要素ではなく第一級 gameplay 要素**として取り扱う必要がある。Kono 監督 (AC8 director) の公開発言:

> "improved visuals without improved functionality には興味ない"

この思想を GXLib に継承するのが本 ADR の目的。

### Constraints

- Target fps 60 @ 1080p、**MED quality tier で cloud pass ≤ 3 ms budget** (Phase D 以降 binding)。HIGH / ULTRA tier は higher-end hardware opt-in over-budget として扱い、binding commitment に含まない (quality tier 表参照)
- GXLib は DX12 Windows 専用 (ADR-0002)、他 RHI 対応は Out of Scope
- 既存 Nubis 1 系コード (~2100 行) を **throwaway にしない** — 段階的 refactor で data model + pipeline 分離
- **Gameplay API を Phase C で提供** — 実ゲーム側が早期に cloud 依存設計を開始できること
- AC8 Cloudly 内部仕様は非公開 — 確定事実 (multi-layer altimeter、contrails、canopy reflection、functional signals) のみ target 化し、speculative な voxel/SDF 実装は Phase F optional に位置付け
- Artist tool (weather editor) は **core の一部** として扱う — bolt-on にしない (Nubis 2017 の教訓)
- Middleware 購入 (trueSKY 等) は **明示的 NO** — license + proprietary lock-in + GXLib アーキテクチャとの integration コスト過大
- UE5 VolumetricCloud 移植は **明示的 NO** — RHI 差異 + license + アーキテクチャ不一致
- 本 ADR は AC7 **実用同等** (Phase E 完で 95% 到達) を binding target、AC8 Cloudly **級** (Phase F 完で 70-80% 推定) を aspirational target

### Requirements

本 ADR が architecture として保証するもの:

- **Data 層**: `WeatherMap` 2D texture (512² RGB 以上、R=coverage, G=precipitation, B=cloud type) + `CloudLayer` struct (altitude band + height-density curve) + `CloudField` 集約クラス
- **Query 層**: `gx::CloudQuery::IsInCloud(Vec3)`, `DensityAlongRay(Vec3, Vec3, float)`, `GetCloudTypeAt(Vec3)` — async GPU readback 許容 (1-frame delay OK)
- **Rendering 層**: Multi-layer ray march (default 3 layer: 低高度 stratus / 中高度 cumulus / 高高度 cirrus)、Hillaire 2020 atmosphere LUT 完全結合、curl noise advection、1/16 checkerboard temporal reconstruction、cloud-to-terrain shadow pass (low-res transmittance map cascaded)
- **Dynamics 層**: Lightning (local bright emission + 3D attenuation)、precipitation particle emission (cloud density → GPU particle)、keyframe sequencer (clear → storm blend)
- **Authoring 層**: Weather map JSON format + 組み込み preset library + (Phase F) in-engine paint tool
- **Performance 層**: ≤3 ms cloud pass @ 1080p 60fps (Phase D 以降 binding)、graceful fallback preset (LOW/MED/HIGH quality tier)
- **Integration 層**: ADR-0007 AssetDatabase 経由での weather map + keyframe 読込、ADR-0008 FrameGraph への新 pass 登録、ADR-0016 EventBus 経由での lightning / weather-change event

## Decision

**GXLib の volumetric clouds を、現状の Nubis 1 系 single-layer procedural 実装から、Nubis 2 Evolved 系 multi-layer weather-map-driven + gameplay-API-integrated 実装へ段階的に進化させる。最終到達点は AC7 trueSKY 実用同等 (視覚 95% / gameplay 85%) を binding target、AC8 Cloudly 級 (視覚 + gameplay 70-80%) を optional Phase F の aspirational target として位置付ける。Middleware (trueSKY / UE5 VolumetricCloud) 採用は明示的に不採用、自前実装堅持。**

具体的アーキテクチャ:

### 1. データモデル (Phase B — foundation)

```cpp
namespace gx {

// 2D weather map; drives regional coverage + type + precipitation
class WeatherMap {
public:
    // R=coverage (0-1), G=precipitation (0-1), B=cloud type (0=stratus, 1=cumulonimbus)
    bool Initialize(uint32_t width = 512, uint32_t height = 512);
    void SetRegion(int x, int y, const WeatherCell& cell);
    WeatherCell Sample(float worldX, float worldZ) const;
    bool LoadFromJSON(const gx::String& path);       // ADR-0007 asset flow
    bool SaveToJSON(const gx::String& path) const;   // atomic tmp+rename per ADR-0019 §5 pattern (see also ADR-0007 §7 hot-reload contract)
    int  GetGPUTextureHandle() const;                // GXLib-style int handle (same convention as MoviePlayer / TextureManager); resolved via TextureManager to SRV for shader sampling. See ADR-0002 forbidden pattern `dx12_type_in_public_api`.
private:
    // 実装詳細
};

// 高度帯別 cloud layer
// NOTE: new enum `CloudClass` — NOT `CloudType` (which exists as a float
// scalar in current VolumetricClouds.h SetCloudType/GetCloudType API for
// the 0/0.5/1 gradient lerp). Renamed to avoid silent API break on
// Phase B consumers (example 11/15/16). Old float API remains as
// deprecated shim that forwards to `CloudField::GetActiveLayer()
// .dominantClass` where applicable.
enum class CloudClass {
    Stratus, Stratocumulus, Cumulus, Cumulonimbus, Cirrus, Cirrocumulus, Altocumulus
};

struct CloudLayer {
    float altitudeBottom;         // 世界座標 m
    float altitudeTop;
    CloudClass dominantClass;     // Phase D shader consumes this to pick HeightDensityCurve
    HeightDensityCurve curve;     // 高度 → density multiplier per class
    float animationSpeed;         // wind multiplier
    // Per-layer noise overrides (Phase B gap fix from TA review Issue 16)
    float diffusivity;            // 0.5-1.0、per-layer detail softening
    float detailFadeDistance;     // m、per-layer detail fade start
    float densityMul;             // per-layer overall density multiplier
};

// Cloud field = N layers + weather + atmosphere reference
class CloudField {
public:
    void AddLayer(const CloudLayer& layer);          // default で 3 layer 提供
    CloudLayer& GetLayer(int index);
    WeatherMap& GetWeatherMap();
    void SetAtmosphereLUT(IAtmosphereLUT* lut);     // Hillaire 2020 coupling
};

} // namespace gx
```

### 2. Rendering アーキテクチャ (Phase D — AC7 parity)

**Multi-layer 合成モデル** (TA review Issue 7 への応答): **単一統合 ray march**。複数 CloudLayer は独立 march ではなく、1 本の primary ray が全 altitude band を camera → sky の順に単調走査し、`transmittance` を累積する。各 sample で `GetLayerAt(worldY)` が layer を特定し、その layer の `HeightDensityCurve` + `dominantClass` + WeatherMap sample で density 計算。物理的に正しい transmittance chain を保証し、cirrus が thick stratus 越しに減衰する挙動を自然に得る。Per-layer 独立 march を additive composite する方式 (誤った simpler alternative) は forbidden。

**Temporal reconstruction 上の注意** (TA review Issue 4 + TD R8): 現行実装は **half-resolution (1/4 pixel)** + bilateral upsample。Phase D はこれを **true 1/16 checkerboard 4×4 spatiotemporal pattern** に置き換える = **temporal pipeline の breaking change** (history buffer 形式変更、jitter pattern 再設計、reconstruction kernel 新設)。軽い tuning ではない。Cloud reprojection は **camera delta + per-layer wind velocity advection** を使う、**GBuffer motion vectors は使わない** (cloud は volumetric で screen-space pixel owner を持たない)。

**Temporal history 内容の分離** (TA review Issue 10-11): CloudTemporalPass は **cloud-only history** (RGB = lightEnergy、A = transmittance) に対して clamp + blend を行う。scene+cloud blended 値の history への書き込みは forbidden。CloudCompositePass が最終的な scene HDR への blend を担当 (分離されたパスとして責任明確化)。

FrameGraph (ADR-0008) に以下 pass を追加:

```
[既存] SceneDepthPass → GBufferPass → ...
                              ↓
[NEW]  WeatherMapUpdatePass    (compute shader; wind advect + keyframe blend; 0.1 ms)
                              ↓
[NEW]  CloudShadowPass         (low-res cascaded transmittance map; 0.3 ms)
                              ↓
[既存] DeferredShadePass       (cloud shadow を sun direct light に乗算)
                              ↓
[NEW]  VolumetricCloudPass     (checkerboard 1/16 pixel ray march; 1.5 ms)
                              ↓
[NEW]  CloudTemporalPass       (reprojection + accumulate; 0.4 ms)
                              ↓
[NEW]  CloudCompositePass      (scene HDR に blend; 0.2 ms)
                              ↓
[既存] GodRays / Bloom / TAA / Tonemap
```

合計 cloud budget = ~2.5 ms at 1080p target (0.5 ms headroom)。

### 3. Gameplay Query API (Phase C — **Phase D 部分実装後** に完全 ship)

**構造依存の明示** (TD review R4): `IsInCloud(worldPos)` / `BatchSample` の実装には density 評価関数を **compute shader として抽出** する必要がある (ray march pixel shader と共有、CPU duplicate は forbidden per `cloud_density_duplicated_in_cpu_and_gpu`)。この compute shader module は Phase D の前提でもあるため、**Phase C 実装は Phase D の "density compute module" 抽出 sub-phase を含む**。

**Phase ordering 決定**: Phase B (data model) → Phase **D.0** (density compute module + SkyAtmosphere LUT) → Phase C (gameplay query API) → Phase **D.1-5** (残る rendering upgrade) → Phase E (dynamics) → Phase F (optional)。Phase C 単独では ship できない — D.0 の density compute 共有を前提とする。



```cpp
namespace gx {

class CloudQuery {
public:
    // 即時判定 (GPU readback 1-frame delayed でもゲーム側は許容設計)
    static bool       IsInCloud(const Vec3& worldPos);                  // threshold は SetInCloudThreshold() で調整可、default 0.15
    static float      DensityAlongRay(const Vec3& start, const Vec3& dir, float length);
    static CloudClass GetCloudClassAt(const Vec3& worldPos);            // 新 enum、float API (GetCloudTypeAt) とは別物
    static float      SunTransmittance(const Vec3& worldPos);            // 雲越し日射
    static void       SetInCloudThreshold(float threshold);              // IsInCloud 閾値、default 0.15 (薄霧は「内部」と判定しない)

    // バッチ query (missile batch / radar batch 用、単一 readback で多点)
    // Async GPU readback、max batch 1024 points/dispatch、複数 dispatch は ring buffer queue。
    // Results[i].stale == true は WeatherMap hot-reload が query in-flight 中に発生した signal。
    static void       BatchSample(
        const gx::Vector<Vec3>& points,
        gx::Vector<CloudSample>& results);
};

struct CloudSample {
    float density;           // 0-1
    CloudClass klass;        // 新 enum 名 (C++ keyword 回避のため `klass`)
    float sunTransmittance;
    bool  inside;            // density > threshold
    bool  stale;             // WeatherMap reload が query 途中で発生した場合 true
};

} // namespace gx
```

Async GPU readback ring buffer (3 frame、1024 points/dispatch × 3 = 3072 concurrent query slot、~48 KB buffer) 経由で frame 跨ぎの latency を吸収。

### 3a. EventBus Handler Category (ADR-0016 §3 契約)

| Event | Firing mode | Intended handler category |
|-------|-------------|---------------------------|
| `LightningStrikeEvent` | `Fire` (synchronous, same frame、AC7 avionics 30s disrupt 用) | `SideEffect` (audio, VFX flash)。ただしゲーム側で damage-flag 更新は `Idempotent` handler として別登録推奨 |
| `WeatherChangeEvent` | `Fire` (synchronous) | `SideEffect` 既定 (BGM swap, UI banner)。gameplay-state cache の invalidation は `Idempotent` |
| `CloudStateChangeEvent` | `Fire` | `SideEffect` |

Rollback-using game は ADR-0016 §4 replay-suppression に従い、`SideEffect` handler は replay 中 skip される設計。

### 4. Dynamics (Phase E)

- **Lightning**: `CloudField::TriggerLightning(Vec3 epicenter)` → 内部 emissive point light + 3D attenuated flash (shader 側 local multiplier)。ADR-0016 EventBus に `LightningStrikeEvent` fire、audio + screen flash + aircraft avionics hook が subscribe 可能。
- **Precipitation**: `CloudLayer::precipitationDensity` (WeatherMap G channel 経由) が 0.3 以上の領域で GPU particle emit (ADR-0008 GPU particles 既存系流用)。
- **Keyframe sequencer**: `CloudKeyframe` struct (全 WeatherMap + cloud layer state snapshot) の Vector を `CloudFieldAnimator` が補間。artist が JSON で author、runtime で play。

### 5. Authoring tools (Phase F)

- **JSON format** (Phase B で先行定義、Phase F で editor が書き出す形式と一致):
  ```json
  {
    "weatherMap": {
      "resolution": [512, 512],
      "cells": [{ "x": 0, "y": 0, "coverage": 0.4, "precipitation": 0, "cloudType": 0.2 }, ...]
    },
    "layers": [
      { "name": "low-stratus", "altitudeBottom": 800, "altitudeTop": 1500, "dominantType": "Stratus", "animationSpeed": 1.0 },
      { "name": "mid-cumulus", "altitudeBottom": 2000, "altitudeTop": 4000, "dominantType": "Cumulus", "animationSpeed": 1.0 },
      { "name": "high-cirrus", "altitudeBottom": 7000, "altitudeTop": 11000, "dominantType": "Cirrus", "animationSpeed": 0.3 }
    ],
    "keyframes": [
      { "time": 0.0, "weatherMapRef": "clear.json" },
      { "time": 120.0, "weatherMapRef": "storm_front.json" }
    ]
  }
  ```
- **In-engine paint tool**: ImGui + brush painting on weather map (Phase F)
- **Preset library**: `presets/clear.json`, `overcast.json`, `storm_front.json`, `scattered_cumulus.json`, `hurricane.json` (組み込み 5+ preset)

### 6. Forbidden patterns

各 pattern に **Detection** 行を付記 (TD review R7 への応答 — 検出方法が曖昧な forbidden pattern は aspirational に終わる)。

- `cloud_procedural_only_no_weathermap` — 新 cloud system は必ず WeatherMap 経由で configure する。従来の「shader 直 FBM」置き換えは Phase B で完了させ、以後は procedural-only は forbidden
  - **Detection**: CI grep rule — `Shaders/VolumetricClouds*.hlsl` に `sampleWeatherMap` 呼び出しが無ければ fail
- `cloud_single_layer_only` — Multi-layer CloudField が mandatory。single layer fallback は preset として表現 (すべて同 altitude band、coverage 変化のみ)
  - **Detection**: runtime assertion — `CloudField::GetLayerCount() >= 2` をリリースビルドで validate
- `cloud_blocking_gpu_readback` — Gameplay query は async 必須。main-thread から `Map()` で CPU-stall は forbidden
  - **Detection**: CI grep rule — `CloudQuery` 実装ファイルに `Map(` がある場合 fail
- `cloud_synchronous_readback_for_editor_query` (**新規、TD review R6**) — gameplay query だけでなく editor preview / screenshot capture path でも同期 readback は禁止 (別 code path で回避しがち)
  - **Detection**: `Editor/*.cpp` および `tools/*.cpp` での `ReadbackBuffer::Map` 直呼出しに CI gate
- `cloud_middleware_adoption` — trueSKY / UE5 VolumetricCloud / Simul / 3rd party cloud middleware の採用は禁止 (本 ADR Constraints による)
  - **Detection**: CI grep rule — `#include.*trueSKY`、`UE5.*VolumetricCloud`、`simul::` 等を禁止語リストで検索
- `cloud_per_pixel_terrain_shadow_raymarch` — Terrain に対して per-pixel cloud 再 raymarch は禁止 (高コスト)。cascaded low-res transmittance map からサンプル
  - **Detection**: `Shaders/Terrain*.hlsl` + `Shaders/DeferredShade*.hlsl` で `traceCloudDensity` 呼出し禁止
- `cloud_modifies_gbuffer_directly` — Cloud pass は HDR color を blend するのみ、GBuffer への直接書き込み禁止
  - **Detection**: FrameGraph validation — VolumetricCloudPass の WriteResources に GBuffer* が含まれる場合 fail
- `cloud_per_frame_cpu_noise_generation` (**新規、TD review R6**) — 3D noise texture (128³ / 32³) の CPU 生成は Initialize() 1 回のみ。per-frame、あるいは hot-reload 毎の regeneration は禁止 (~2 秒 stall が発生する)
  - **Detection**: `VolumetricClouds::GenerateNoise3D()` が `Initialize()` 以外から呼ばれる場合 CI gate、さらに runtime 側で `m_noiseGenerated` flag 再生成を禁止
- `cloud_shadow_resolution_exceeds_budget` (**新規、TD review R6**) — Cloud shadow map は `1024² × 2 cascade` が上限 (`SHADOW_RES_LIMIT` macro)。独断での拡張禁止
  - **Detection**: `static_assert(kCloudShadowResolution <= 1024, ...)` in code; runtime clamp + warn if overridden via config
- `cloud_density_duplicated_in_cpu_and_gpu` (**新規、TD review R6**) — gameplay query の CPU 実装 + shader の GPU 実装で density 関数を複製することは禁止。両方とも **shared compute shader module** (Phase D.0 extract 成果物) を呼び出すこと
  - **Detection**: コード review rule + file-presence check — `Shaders/CloudDensityCommon.hlsli` が存在し、両 path が `#include` していることを CI で確認
- `cloud_temporal_reprojection_uses_gbuffer_motion_vectors` (**新規、TA review Issue 4 + TD R8**) — Cloud temporal reprojection は camera delta + 雲 wind velocity のみ使う。GBuffer motion vectors の使用禁止 (cloud は volumetric で screen-space pixel owner を持たない)
  - **Detection**: `Shaders/VolumetricCloudsTemporal.hlsl` で `SampleLevel` の source として `MotionVectors` texture 使用禁止 — grep gate
- `cloud_layer_independent_additive_composite` (**新規、TA review Issue 7**) — 複数 CloudLayer を独立 march してから additive composite することは禁止。1 本の unified ray march が全 altitude band を走査、transmittance chain を正しく accumulate
  - **Detection**: shader 内で `RayMarch` 関数が複数回呼ばれる代わりに、`GetLayerAt(worldY)` で単一 march 内の layer 切替が実装されていることを code review で確認

### 7. Layer 1 / Layer 2 Readiness (ADR-0017 対応、TD review G4)

ADR-0017 Two-Layer Accessibility pillar 準拠。

**Layer 1 (beginner、DXLib-compat shape)**:
- `gx::SetCloudPreset(const char* name)` — Compat 関数、preset 名で雲を切替 ("clear" / "overcast" / "storm_front" / "scattered_cumulus" / "heavy_storm" ≈ 旧 hurricane)
- `gx::GetCloudField()` — CloudField reference (advanced user が L2 access する route)
- Default preset = **"clear"** (雲はうっすら見えるが gameplay 影響なし)
- Gameplay query API (`IsInCloud` 等) は L2 のみ — L1 user は cloud 連動 gameplay を実装しない前提

**Layer 2 (advanced、full SDK)**:
- `gx::CloudField`, `gx::CloudLayer`, `gx::WeatherMap`, `gx::CloudQuery::*` — すべて `sdk/include/GXLib/Graphics/3D/` 公開
- Extension point: custom `HeightDensityCurve` 実装、custom `CloudClass` 追加 (enum 拡張できないので string-keyed map も併設予定)
- Example: `examples/17-custom-cloud-preset/`、`examples/18-cloud-gameplay-integration/` (icing + missile lock demo)

### 8. Shipping-build invariants

- Phase D 以降、`cloud-quality-tier` preset (LOW / MED / HIGH / ULTRA) が shipping build で選択可能。
  - **LOW**: 2-layer (低 stratus + 中 cumulus、cirrus 省略 — `IsInCloud` 高々度では false 返却)、32³ detail、16-step march、1/16 checkerboard、~1.5 ms binding
  - **MED**: 3-layer、32³ detail、32-step march、1/16 checkerboard + clamp、**~2.5 ms binding (≤3 ms 保証)**
  - **HIGH**: 3-layer、32³ detail、64-step march、1/8 checkerboard + clamp + curl、~3.5 ms (best-effort、high-end hardware opt-in — 3 ms binding に含めない)
  - **ULTRA** (Phase F 完のみ): 3-layer + voxel accel、1/8 checkerboard、~3-4 ms (fluid-sim dynamics の cost で変動)
- Cloud pass は GPU 計測される、log の `cloud_cost_over_budget_warning` が MED tier で budget 超過時に emit
- Disable path: `CloudField::SetEnabled(false)` で全 5 pass を FrameGraph から skip、cloud 無しの shipping 可能 (flight 以外のジャンル向け)

### 9. Cross-ADR Integration Commitments (TD review G1-G5)

- **ADR-0007 Asset Database**: `WeatherMap` は `AssetDatabase::RegisterType<WeatherMap>` で登録。AssetId = `hash("weather/presets/<name>")`。Reload handler は (a) GPU texture 再 upload、(b) in-flight `BatchSample` query に `CloudSample{ stale=true }` 返却、(c) deferred-release per ADR-0007 §10 (3-frame quarantine)。
- **ADR-0008 Rendering Pipeline**: FrameGraph に 5 新 pass 追加は **ADR-0008 本体への amendment** として扱う。footnote ではなく in-place ADR edit + version bump。Phase D.0 着手時に ADR-0008 を update (本 ADR Migration Plan D.0 タスクに明記)。
- **ADR-0016 EventBus**: §3a 記載の handler category 契約を遵守。`LightningStrikeEvent` / `WeatherChangeEvent` / `CloudStateChangeEvent` の発火は **main thread のみ** (`Fire<T>`、ADR-0016 §4)。worker thread からは `QueueFromWorker<T>` 使用、これは storm-sim が worker で回る場合のみ該当。
- **ADR-0017 Two-Layer Accessibility**: §7 に L1/L2 Readiness 定義済。L1 `SetCloudPreset` が Compat 経由で expose、L2 は `gx::CloudField` 等の full SDK API。
- **Control Manifest**: ADR-0021 Accepted 時点で `docs/architecture/control-manifest.md` Presentation Layer に 11 個 (既存 6 + 新 5) の forbidden pattern を追加するタスクを起票する。

### 10. Architecture Diagram

```
  Weather data + Assets
         │
         │ WeatherMap.json / keyframe timeline  (ADR-0007 Asset DB 経由)
         ▼
  ┌─────────────────────────────────────┐
  │ Data layer                           │
  │  WeatherMap (2D RGB texture)         │
  │  CloudLayer[] (3 default)            │
  │  CloudField (aggregate)              │
  │  Keyframe sequencer                  │
  └─────────────────────────────────────┘
         │                         │
         │ CloudQuery API          │ CloudConstants (shader CB)
         ▼                         ▼
  ┌──────────────┐      ┌──────────────────────────────┐
  │ Gameplay     │      │ Rendering (ADR-0008 Frame-   │
  │  IsInCloud   │      │  Graph addendum)             │
  │  DensityRay  │      │  WeatherMapUpdatePass  0.1ms │
  │  GetType     │      │  CloudShadowPass       0.3ms │
  │  BatchSample │      │  VolumetricCloudPass   1.5ms │
  └──────────────┘      │  CloudTemporalPass     0.4ms │
         │              │  CloudCompositePass    0.2ms │
         │ Events       │   ───────────────────────    │
         ▼              │   Total budget: ≤3 ms         │
  ┌──────────────┐      └──────────────────────────────┘
  │ ADR-0016     │                   │
  │ EventBus     │                   │
  │              │◄──────────────────┘
  │ Lightning    │   (LightningStrikeEvent, WeatherChangeEvent)
  │ Weather ch.  │
  └──────────────┘
         │
         │ Game code subscribes: icing, missile lock,
         │ radar attenuation, canopy droplet spawn, etc.
         ▼
  Game-level gameplay
```

## Alternatives Considered

### Alternative 1: Keep current Nubis 1 single-layer (status quo)

- **Pros**: Zero engineering investment; 現状動作
- **Cons**: AC7 gap 永続。flight-combat title で gameplay 連動実装不可能。`cloud = gameplay` 思想実現不可能。Cloud identity 読めない問題残る
- **Rejection**: User 要件「AC7/AC8 クオリティ、完璧に」と incompatible

### Alternative 2: Simul trueSKY middleware 購入

- **Pros**: AC7 と identical rendering stack が即手に入る
- **Cons**: ライセンス費用大 (contacted pricing、$10K+/year 規模)、proprietary lock-in、GXLib DX12 native pipeline と UE plugin architecture の integration 層が要追加、trueSKY は pre-built UE4 plugin が中心で DX12 native integration は要別プロジェクト、license 違反リスク、source 非公開で debug 困難
- **Rejection**: GXLib は self-hosted SDK のポリシー (ADR-0002) + user が完璧な自前実装を求める。middleware 依存は thesis 違反

### Alternative 3: UE5 VolumetricCloud component の port

- **Pros**: Epic が maintain、documented、UE community support
- **Cons**: UE5 RHI (abstraction layer) + GXLib DX12 native pipeline が non-trivial に乖離。UE5 VolumetricCloud は FSkyAtmosphere component + material system + Volume Texture asset system に密結合、port は実質新規実装。license 問題 (UE5 EULA + GXLib 自前 SDK ポリシー)
- **Rejection**: 実質新規実装なら、Nubis 2/Evolved を直接実装する方が target に近い

### Alternative 4: Nubis Cubed (voxel-based) へ直接ジャンプ

- **Pros**: AC8 Cloudly に最も近いと思われる (Schneider 2023 の voxel 系譜)
- **Cons**: 既存 Nubis 1 系 ~2100 行 + 128³/32³ 3D texture infrastructure が **throwaway** になる。voxel pipeline + SDF 圧縮 + fluid sim-driven modeling は engineering cost が単独で 3-6 ヶ月。AC8 Cloudly 内部実装が voxel かは **推測**、SDF accelerated 3D noise という可能性も残る。確実な中間成果物なしに完成を待つ design
- **Rejection**: 段階的 ship ができない。Phase A-E で AC7 実用同等に到達し、Phase F で optional 追加の方が risk 管理上妥当

### Alternative 5: Minimal patch (multi-scattering 追加のみ等 1-2 箇所 polish)

- **Pros**: Low cost、現 implementation の延長
- **Cons**: Multi-scattering は既に実装済み。polish だけでは AC7 gap 埋まらない (gameplay API、multi-layer、weather map の fundamental 欠落は tweak では対応不能)
- **Rejection**: User 要件と質的に乖離

## Consequences

### Positive

- **AC7 実用同等** (Phase E 完で 95%+) を段階 ship で実現、各 Phase で游離した value 提供
- `cloud = gameplay` 哲学実装、flight-combat / flight-sim タイトルの差別化要素に
- Multi-layer altimeter が AC8 Cloudly の中核機能に対応、将来 Phase F で voxel 拡張可能な data model 土台
- Middleware 非依存で license + lock-in 問題なし、GXLib の self-hosted SDK thesis 継承
- 既存 Nubis 1 系コードは段階的 refactor で活用、throwaway 最小化
- WeatherMap + keyframe format が JSON で authored、ADR-0007 AssetDatabase + AssetRemapper 経由で mod サポートも自動で入る
- Gameplay query API を Phase C で早期提供、game code が cloud 依存設計を Phase D 完了前に開始可能
- ADR-0008 FrameGraph に綺麗に追加 (既存 pass 変更なし)、backward-compatible

### Negative

- 総工数 3-4 ヶ月 (engineer full-time 換算、user 1 人 + Claude 支援で実時間 6-9 ヶ月)。プロジェクト他領域の engineering 時間を圧迫
- Phase D 以降、cloud pass budget 3 ms が commitment 入り、frame budget 圧迫 (現 main thread 7.2 ms / 16.6 ms の headroom を cloud で消費)
- Artist tool (Phase F) は engineering cost 高く、tool が無いと Phase B-E の capability を活かす data が authoring 不可。Phase B で JSON preset library を組み込み対応必須
- AC8 Cloudly 級は非公開仕様の推測を含み、100% 到達は Project ACES が詳細公開するまで claim 不可能
- Performance 逆 lift 可能性 (multi-layer + shadow pass で budget 超過)、early profiling + tier fallback が必要
- 既存 VolumetricClouds.{cpp,h,hlsl} は refactor 対象、consumer (PostEffectPipeline、DebugOverlay) の修正に伝播

### Risks

- **R1 Scope 膨張**: AC 系は polish 要求が無限湧き。*Mitigation*: Phase 完で retrospective + 次 Phase 見積再確認 + explicit "done" line 定義
- **R2 Performance 乖離**: Multi-layer + shadow で 3ms 超過可能性。*Mitigation*: Phase D early profiling、quality tier preset (LOW/MED/HIGH) を Phase D から実装、LOW tier は 2-layer + 16-step で 1.5 ms 達成目標
- **R3 Artist tool 不在**: Phase F まで tool 無しだと capability 活用不可。*Mitigation*: Phase B で JSON preset library + 5+ 組み込みプリセット (clear/overcast/storm_front/scattered/hurricane)
- **R4 AC8 Cloudly 新情報**: 2026 release 前に Project ACES が talk 出す可能性。*Mitigation*: Phase F 着手前に情報確認、必要なら ADR amendment
- **R5 Nubis Cubed 系への prematureジャンプ誘惑**: 2023 talk 公開済で魅力的。*Mitigation*: Phase F optional の位置付けを厳守、Phase E 完 AC7 同等を binding target
- **R6 ADR-0008 との重複**: Rendering Pipeline ADR が cloud を扱わないが、FrameGraph pass 追加は ADR-0008 の範囲。*Mitigation*: ADR-0008 への **in-place amendment** (footnote ではなく ADR 本体 edit + version bump、TD M7)、Phase D.0 着手時のタスク
- **R7 Hardware variation**: RTX 3060 (mid) と RTX 4090 (high) で 2-3x perf 差、LOW tier で旧 Skylake iGPU 想定も必要。*Mitigation*: Phase D early profiling で 3-hardware tier 実測、quality tier に hardware 側自動選択を Phase G で追加
- **R8 SkyAtmosphere LUT 依存の hidden scope**: Phase D.0 として明示化済み (TD R2 / TA Issue 9)、ただし 2-3 週分の追加工数。*Mitigation*: Phase D.0 を独立 sub-phase として管理、Phase C が D.0 完了待ちであることを schedule 上で明確化

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | 新規 TRs を tr-registry.yaml に登録 (TR-cloud-001 ~ TR-cloud-020 想定)、詳細は Phase B 着手時に確定 |

## Performance Implications

### Budget (Phase D binding)

| Pass | Frame time @ 1080p | 備考 |
|------|---------------------|------|
| WeatherMapUpdatePass | 0.1 ms | Async compute、wind advect + keyframe blend |
| CloudShadowPass | 0.3 ms | Low-res cascaded transmittance map (1024² × 2 cascade) |
| VolumetricCloudPass | 1.5 ms | 1/16 checkerboard pixel、各 pixel 32-64 step ray march |
| CloudTemporalPass | 0.4 ms | 3×3 min/max clamp + bilateral reproject |
| CloudCompositePass | 0.2 ms | HDR blend + alpha out for god rays |
| **Total cloud budget** | **~2.5 ms** | 0.5 ms headroom for storm/lightning over-budget |

### Quality tier preset

| Tier | Layers | Resolution | Steps | Temporal | 想定 ms |
|------|--------|------------|-------|----------|---------|
| LOW | 2 | 1/16 | 16 | checkerboard | ~1.5 ms |
| MED | 3 | 1/16 | 32 | checkerboard + clamp | ~2.5 ms |
| HIGH | 3 | 1/8 | 64 | full checkerboard + clamp + curl | ~3.5 ms (超過許容) |
| ULTRA (Phase F) | 3+voxel | 1/8 | 64+voxel | + voxel accel | ~3.0 ms |

### Memory

- WeatherMap: 512² RGBA8 ≈ 1 MB、1024² RGBA8 ≈ 4 MB
- Cloud shadow map: 1024² × 2 cascade R8 ≈ 2 MB
- Base 3D noise 128³ RGBA8 ≈ 2 MB (既存維持)
- Detail 3D noise 32³ RGBA8 ≈ 128 KB (既存維持)
- Checkerboard history buffer: 1920×1080 × 2 (color + depth) × 2 frame ≈ 32 MB
- Voxel cloud field (Phase F optional): ~64 MB regional、全世界は sparse voxel tree

### Network

- N/A (single player gameplay で cloud 同期 unnecessary)
- 将来 multiplayer 対応時は `WeatherMap + keyframe timestamp` を sync (low bandwidth、数 KB/update)

## Migration Plan

### Phase A (今セッション完了): Foundation docs
1. `docs/engine-reference/gxlib/volumetric-clouds-analysis-2026-04-19.md` 執筆 ✅ (本 ADR と同時 commit)
2. 本 ADR 執筆 ✅
3. `tr-registry.yaml` に TR-cloud-001 ~ TR-cloud-020 先行登録 (詳細は Phase B 着手時に確定)
4. `architecture-traceability.md` に新 TR を登録
5. `/architecture-review` fresh-session で ADR-0021 を Proposed → Accepted 昇格

### Phase B (1-2 週間、1 sprint): Data model foundation
1. `WeatherMap` class 実装 + JSON load/save (ADR-0019 §5 atomic-save pattern 準拠)
2. `CloudLayer` struct + `HeightDensityCurve` 抽象 + `CloudClass` enum (名前衝突回避 per TD R3)
3. `CloudField` 集約クラス + 現 VolumetricClouds との結線 (既存視覚保持、但し data source が WeatherMap 経由に)
4. **5+ 組み込み preset JSON 作成**。WeatherMap は FBM を pre-bake した texture で開始 (procedural → baked texture 置き換えで視覚差を minimize、TA review Issue 19)
5. **WeatherMap hot-reload path** = `FileWatcher` (ADR-0007 §3) + `AssetReloader` 統合 (TA review Issue 15)。JSON edit → 即 runtime reflect が authoring 可能
6. `AssetDatabase::RegisterType<WeatherMap>` 登録 + reload handler
7. Unit test: WeatherMap 設定 → density 変化検証、JSON round-trip、hot-reload reflects within 1 frame
8. Sprint DoD: 既存 example 11/15/16 等で **perceptual diff ≤10% SSIM、artifact-class regression なし** (TD M12 / TA Issue 19 の実現可能な基準)、新規 example `17-cloud-weather-preset/` 追加で preset 切替デモ

### Phase D.0 (2-3 週間、Phase C の前提): SkyAtmosphere LUT upgrade + density compute module
**TD review R2 + R4 + TA Issue 9 で発覚した hidden scope を明示化。**

1. `SkyAtmosphere` を Hillaire 2020 LUT 方式に書き換え:
   - Transmittance LUT (256×64 R16G16B16A16F)
   - Multi-scattering LUT (32×32 R11G11B10F)
   - Sky-view LUT (192×108 R11G11B10F)
   - Aerial perspective LUT (32×32×32 voxel R11G11B10F)
2. ADR-0008 Rendering Pipeline への **in-place amendment**: FrameGraph pass list に SkyAtmosphere LUT update passes を追加、version bump
3. **Density compute module 抽出** (`Shaders/CloudDensityCommon.hlsli`): 現 pixel shader 内 density 関数を共有 HLSL header に切り出し、ray march pixel shader + Phase C gameplay compute shader 両方から `#include` される
4. Sprint DoD: SkyAtmosphere LUT sample 出力 + 既存 sky + Rayleigh 結果との visual 一致、density compute module の unit test (pixel shader と compute shader 結果が float epsilon 以内で一致)

### Phase C (1 週間、Phase D.0 完了後): Gameplay query API
**Phase D.0 density compute module 抽出が前提** (TD R4)。独立では ship できない。

1. `CloudQuery` namespace + 5 static methods (IsInCloud / DensityAlongRay / GetCloudClassAt / SunTransmittance / SetInCloudThreshold) + BatchSample
2. `CloudQueryCompute` dispatch (Phase D.0 で抽出した `CloudDensityCommon.hlsli` を使う compute shader)
3. Async GPU readback ring buffer (3 frame × 1024 points = 3072 slot、~48 KB)
4. `CloudSample::stale` flag propagation for hot-reload race
5. Example: `18-cloud-gameplay-integration/` — icing stub (cloud 中で timer increment、閾値で warning overlay) + missile lock degradation stub
6. ADR-0016 EventBus に `CloudStateChangeEvent` 追加 (handler category: SideEffect per §3a)
7. Sprint DoD: `BatchSample(100 points)` 1-frame 以内で完了、`BatchSample(1000 points)` 3-frame 以内、regression test for CPU/GPU density parity

### Phase D.1-5 (3-4 週間、2 sprint): Render pipeline AC7 parity
(Phase D.0 の SkyAtmosphere LUT + density compute module が前提)

1. **D.1** Multi-layer unified ray march (1 本の primary ray で全 altitude band 走査、`GetLayerAt(worldY)` で per-sample layer 切替、`transmittance chain` 正しく accumulate — §Decision 2 参照)
2. **D.2** Hillaire 2020 Atmosphere LUT 結合 (D.0 で LUT ready、本 Phase で cloud 側 sample 実装)
3. **D.3** Curl noise advection (3D curl noise texture 64³ 新規生成、morphing animation)
4. **D.4** True 1/16 checkerboard temporal reconstruction (bilateral → checkerboard 置き換え、**temporal pipeline 本格的書き換え** — history buffer 形式変更、new jitter spec、reconstruction kernel; **cloud-only history** で scene+cloud blended は forbidden per `cloud_temporal_history_contains_scene_color`)
5. **D.5** Cloud-to-terrain shadow pass (1024² × 2 cascade、world extent 2km² + 4km²、sun direction 追従、per-layer additive transmittance product で合成 per TA review Issue 8)
6. Performance profiling + quality tier preset (LOW/MED/HIGH、ULTRA は Phase F 専用)
7. Sprint DoD: 1080p 60fps で MED tier ≤3 ms binding 達成、AC7 screenshot 並置 perceptual diff ≤10% (FLIP/SSIM ツールで定量化、TD M1), terrain shadow golden image regression test in `production/qa/evidence/`

### Phase E (2-3 週間、1 sprint): Dynamics + storm
1. Lightning 実装 (`CloudField::TriggerLightning` + local emission + EventBus fire + 3D attenuation shader)
2. Precipitation GPU particle (cloud density → emit rate、rain/snow 切替は WeatherMap G channel + temperature)
3. Keyframe sequencer (`CloudFieldAnimator`、JSON timeline 解釈、linear blend)
4. Wind field 拡張 (storm で wind shift + turbulence)
5. Example: storm sequence demo (clear → cumulonimbus 推移 + lightning + rain)
6. Sprint DoD: AC7 mission 4 級 storm sequence を reference app で再現可能

### Phase F (optional、3-6 ヶ月、AC8 Cloudly 級 aspirational target)
**Phase F 着手前に AC8 Cloudly 公式 talk / GDC / CEDEC 情報を再確認** (TD M9)。Voxel / SDF / hybrid いずれが採用されたか判明していれば本 ADR を amendment する。公開情報無ければ以下の **Nubis Cubed (SIGGRAPH 2023) 準拠** path で進行:

1. **F1** Advanced density representation (voxel accelerator を first-choice、SDF or hybrid は AC8 情報次第で切替可能 — prescriptive ではなく flexible)
2. **F2** SDF cloud primitives (artist-placable cumulus cluster、region-focused voxel volume)
3. **F3** Fluid-sim-driven local animation (region-focused、global は wind field 維持)
4. **F4** In-engine weather map paint tool (ImGui brush + keyframe recorder、Phase B hot-reload の上位互換)
5. **F5** Contrail + canopy droplet + cockpit rain integration
6. **F6** 4D noise (time coherent、temporal 間 artifact 低減)
7. Sprint DoD: AC8 Cloudly 公式 talk が出た場合、それとの comparison を `docs/engine-reference/gxlib/volumetric-clouds-cloudly-comparison-<date>.md` に記録

### Phase G (production polish、Phase D 以降 継続)
1. Performance golden path test (各 quality tier で budget 監視)
2. Regression screenshot suite (各 preset で PSNR 測定)
3. Documentation: developer guide (how to configure clouds for a game)、artist guide (how to author weather maps)
4. Reference app (flight sample with cloud-driven gameplay)

### ADR-0008 への addendum (Phase D 着手時)

ADR-0008 Rendering Pipeline の § Decision FrameGraph pass list に次を追加するための amendment ADR (ADR-0021-addendum または ADR-0008 を update):
- `WeatherMapUpdatePass` (after SceneDepthPass)
- `CloudShadowPass` (before DeferredShadePass、サンライトに影響)
- `VolumetricCloudPass` (after SSR、before GodRays — 現状と同じ位置)
- `CloudTemporalPass` (immediately after VolumetricCloudPass)
- `CloudCompositePass` (replacing direct blend in VolumetricCloudPass)

## Validation Criteria

- **現状機能回帰なし**: 既存 example 11/15/16 で perceptual diff ≤10% SSIM, artifact-class regression なし (Phase B、TD M12 で softened from ≤5% PSNR)
- **WeatherMap load/save**: 5+ 組み込み preset all JSON round-trip (Phase B)
- **WeatherMap hot-reload**: JSON edit → 1 frame 内 reflection (Phase B)
- **Density compute parity**: pixel shader density output = compute shader density output within float epsilon (Phase D.0)
- **SkyAtmosphere LUT**: Hillaire 2020 sample output が既存 Rayleigh + sun color の統計的同等 (Phase D.0)
- **Gameplay query accuracy**: `IsInCloud` が shader sampling と一致 (Phase C、density parity test 通過前提)
- **GPU readback latency**: `BatchSample(100 points)` 1-frame 以内、`BatchSample(1000 points)` 3-frame 以内 (Phase C、TD M3)
- **Multi-layer visual**: 3 layer (低 stratus / 中 cumulus / 高 cirrus) が altitude で視覚的に区別可能、下から上へ飛行して段階的に visible layer 変化を観察可能 (Phase D)
- **Multi-layer transmittance chain**: thick stratus 越しに cirrus を見たとき、cirrus が dimmed (独立 additive composite であれば過度に bright に見える) — visual + SSIM で確認 (Phase D.1)
- **Performance budget**: 1080p MED tier で cloud pass ≤ 3 ms sustained over 1000 frame、LOW tier ≤1.5 ms、HIGH tier best-effort (Phase D)
- **Terrain shadow**: 地形上を雲の影が走る golden image in `production/qa/evidence/`、FLIP/SSIM 定量化 (Phase D.5、TD M1 / M2)
- **AC7 screenshot parity**: AC7 mission 1 日中海上 screenshot と並置で perceptual diff ≤10% (tool-based: FLIP or SSIM、Phase D 完)
- **Lightning**: storm preset でランダム lightning trigger、local flash + EventBus fire 確認 (Phase E)
- **Storm sequence**: clear → storm keyframe 推移 90 秒で滑らかに transition、keyframe interpolation correctness = density at t=60s equals blend(kf[0], kf[1], 0.5) ± epsilon (Phase E、TD M4)
- **Preset library completeness**: 5+ preset (clear/overcast/storm_front/scattered_cumulus/heavy_storm) が JSON load で再現 (Phase B、`hurricane` は物理正確でないため rename to `heavy_storm` per TD M6)
- **Quality tier switching**: runtime で LOW/MED/HIGH switch、history buffer 再 alloc の stall ≤50 ms (Phase D、TD Q6)
- **Disable path**: `CloudField::SetEnabled(false)` で 5 pass が FrameGraph から完全除外、cloud cost ≤0.01 ms (Phase D)
- **Forbidden pattern CI**: 11 patterns 全て detection mechanism 動作 (grep rule / runtime assert / file-presence check) — control-manifest.md 登録完了後 (Phase D.0 deliverable)

## Related Decisions

- ADR-0001 (Documentation strategy — ADR-only project)
- ADR-0002 (DX12 backend — GraphicsDevice + ShaderRegistry 前提)
- ADR-0007 (Asset Database — WeatherMap JSON + keyframe asset flow)
- ADR-0008 (Rendering Pipeline — FrameGraph 新 pass 追加、本 ADR Migration Plan に addendum 明記)
- ADR-0016 (EventBus — LightningStrikeEvent + WeatherChangeEvent を使用)
- ADR-0017 (Two-Layer Accessibility — cloud API は L2 advanced、L1 beginner には WeatherMap preset 選択のみ expose)
- ADR-0019 (Scene — WeatherMap が Scene/Entity のように serialize 対象、ScenePersistence §5 atomic invariant 準拠)
- `GXLib/Graphics/3D/VolumetricClouds.{h,cpp}` (current)
- `Shaders/VolumetricClouds.hlsl` (current)
- `Shaders/VolumetricCloudsTemporal.hlsl` (current)
- `docs/engine-reference/gxlib/volumetric-clouds-analysis-2026-04-19.md` (本 ADR 基礎資料)
- Nubis SIGGRAPH 2015 / 2017 / 2022 / 2023 PDFs (詳細は analysis doc Appendix A)
- Hillaire Frostbite 2016 / 2020 atmosphere papers
- AC7 CEDEC 2019 (Kanno) + AC8 2024-2026 public announcements
