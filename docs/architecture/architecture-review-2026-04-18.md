# Architecture Review — 2026-04-18

> **Mode**: `/architecture-review` (full, fresh session)
> **Engine**: Custom — GXLib (Phase 5, DX12, Windows, C++20, pinned 2026-04-15)
> **Previous review**: [architecture-review-2026-04-17.md](architecture-review-2026-04-17.md) (CONCERNS, patched)
> **New ADRs since previous review**: ADR-0018 (AI), ADR-0019 (Scene), ADR-0020 (Movie) — first independent audit
> **Scope**: GDDs 0 (ADR-only project per ADR-0001); ADRs 20 (all Accepted); TR registry v7 (55 TRs)

---

## Verdict: **CONCERNS**

**7 REAL ISSUES + 8 MINOR CONCERNS detected.** Coverage is 100% and no dependency cycles exist, but three of the four newest ADRs (0018/0019/0020) either self-contradict or describe code that already violates their own stated invariants. Pattern observed in every prior fresh-session review holds: self-review misses ≥3 real issues per ADR batch. This review confirms the pattern — do not skip fresh-session validation.

Blocking promotion to a PASS verdict:
- **E1/E2**: Code violates ADRs' own forbidden patterns (AI Foundation-only, Scene Core-isolation)
- **E3**: Non-atomic scene save = data loss risk
- **E4**: Process-wide Media Foundation refcount collision when Movie + Recorder coexist
- **R1**: PIE state-machine dual-ownership between ADR-0015 and ADR-0019
- **R2**: ADR-0018 threading contract self-contradicts across §Constraints and §9
- **R3**: ADR-0020 bypasses ADR-0007 AssetDatabase without declaring the exception

---

## Traceability Summary

| Status | Count | % |
|--------|-------|---|
| ✅ Covered | 55 | 100% |
| ⚠️ Partial | 0 | 0% |
| ❌ Gap | 0 | 0% |
| **Total (registry v7)** | **55** | **100%** |

Registered-but-deferred items (forward-looking, not gaps): 12 entries in
`architecture-traceability.md`, of which 2 are now completed (ADR-0019 closes
`TR-defer-scene-architecture`; ADR-0020 closes `TR-defer-movie-pipeline`) and 1
should be promoted to active follow-up (`TR-defer-game-shipping-preset`).

**New TR IDs registered this review**: none. All 55 existing entries confirmed
sourced from Accepted ADRs with one-to-one ADR section references.

---

## Full Traceability Matrix

| TR-ID | ADR | Requirement (short) | Status |
|-------|-----|---------------------|--------|
| TR-doc-001 | 0001 | Template skills operate without GDDs | ✅ |
| TR-doc-002 | 0001 | ADR → tr-registry → stories traceability preserved | ✅ |
| TR-doc-003 | 0001 | Documentation scheme scales to future subsystem additions | ✅ |
| TR-rnd-001 | 0002 | 60 fps at 1080p on mid-range hardware with Phase 5 features | ✅ |
| TR-rnd-002 | 0002 | HDR10, VRS Tier 1/2, Mesh Shaders, Sampler Feedback, DirectStorage, DXR 1.1 | ✅ |
| TR-rnd-003 | 0002, 0008 | Deferred + Forward+ hybrid rendering | ✅ |
| TR-rnd-004 | 0002 | Hardware-caps-gated graceful fallback | ✅ |
| TR-rnd-005 | 0002 | DX12/DXGI types hidden behind opaque handles in public headers | ✅ |
| TR-rnd-006 | 0008 | FrameGraph scheduler + transient aliasing + pass culling | ✅ |
| TR-rnd-007 | 0008 | Canonical PostFX order + insertion points | ✅ |
| TR-rnd-008 | 0008 | HDR workflow (linear float16 throughout, output transform) | ✅ |
| TR-api-001 | 0003 | DXLib-to-GXLib port: minimal changes | ✅ |
| TR-api-002 | 0003 | Compat int return codes (0/-1); no exceptions across boundary | ✅ |
| TR-api-003 | 0003 | Procedural + class-style APIs coexist | ✅ |
| TR-api-004 | 0003 | GXLib-exclusive features reachable without breaking DXLib calls | ✅ |
| TR-ecs-001 | 0004 | 100k entities at 60 fps, component access ≤ 2 ms/frame | ✅ |
| TR-ecs-002 | 0004 | Runtime component composition | ✅ |
| TR-ecs-003 | 0004 | `World::Query<Cs...>()` filtered iteration | ✅ |
| TR-ecs-004 | 0004 | EntityBridge (Transform + Lifetime/Tag mirrored per frame) | ✅ |
| TR-ecs-005 | 0004 | Lua spawn/destroy/query via `gxlib.world` | ✅ |
| TR-scr-001 | 0005 | Lua hot reload with generation-counted callbacks | ✅ |
| TR-scr-002 | 0005 | Sandboxed untrusted mod code | ✅ |
| TR-scr-003 | 0005 | Lua→C++ overhead ≤ 1 μs; ECS query ≤ 20 μs | ✅ |
| TR-scr-004 | 0005 | Deterministic call cost (no JIT, no unpredictable GC) | ✅ |
| TR-scr-005 | 0005 | Explicit `sol::usertype<T>` registration only | ✅ |
| TR-job-001 | 0006 | Submit O(1), near-linear scaling, graph deps, priority tiers | ✅ |
| TR-job-002 | 0006 | Main-thread-only public API; `ProcessMainThreadJobs` | ✅ |
| TR-job-003 | 0006 | Worker count = `hardware_concurrency() - 1`; fixed pool | ✅ |
| TR-ast-001 | 0007 | Stable AssetId, single-cache ownership, dedup | ✅ |
| TR-ast-002 | 0007 | Async loads via JobSystem; DirectStorage + fallback | ✅ |
| TR-ast-003 | 0007 | Hot reload via FileWatcher → AssetReloader → rebind | ✅ |
| TR-ast-004 | 0007 | Dependency tracking (A→B reload notifies A) | ✅ |
| TR-ast-005 | 0007 | AssetRemapper for mod override | ✅ |
| TR-ast-006 | 0007 | ≥3-frame deferred GPU release quarantine | ✅ |
| TR-phy-001 | 0009 | 2D + 3D separate worlds, archetype ECS, in-house stack | ✅ |
| TR-phy-002 | 0009 | GJK/EPA narrow-phase + AABB-tree broadphase + SI solver | ✅ |
| TR-phy-003 | 0009 | Cloth, ragdoll, character controller | ✅ |
| TR-phy-004 | 0009 | 1000 3D bodies at 60 fps within 3 ms | ✅ |
| TR-phy-005 | 0009 | Concurrent broadphase reads (versioned snapshot); rollback determinism | ✅ |
| TR-aud-001 | 0010 | XAudio2 backend, bus hierarchy, DSP chain | ✅ |
| TR-aud-002 | 0010 | 3D spatial + occlusion via physics raycast | ✅ |
| TR-aud-003 | 0010 | OGG streaming, 64-voice budget + virtualisation, reverb zones | ✅ |
| TR-aud-004 | 0010 | Audio snapshots (named mixer-state blends) | ✅ |
| TR-inp-001 | 0011 | InputManager + per-device classes + ActionMapping | ✅ |
| TR-inp-002 | 0011 | XInput (4 pads, vibration, deadzones) + IMM32 IME | ✅ |
| TR-inp-003 | 0011 | InputCapture for rebind UI; DXLib Compat wrappers | ✅ |
| TR-gui-001 | 0012 | Retained widget tree + Flexbox + 3-phase event dispatch | ✅ |
| TR-gui-002 | 0012 | XML/CSS + hot reload + data binding + UITween | ✅ |
| TR-gui-003 | 0012 | ImGui coexistence (ImGui → UIContext → game priority) | ✅ |
| TR-net-001 | 0013 | Reliable UDP + ReplicatedProperty + InterestManagement | ✅ |
| TR-net-002 | 0013 | NetworkPrediction + Rollback (GGPO-style, 8-frame window) | ✅ |
| TR-net-003 | 0013 | NATTraversal + Matchmaking + HTTPClient + CloudSave | ✅ |
| TR-anim-001 | 0014 | Skeleton + Animator SM + BlendTree + IK suite | ✅ |
| TR-anim-002 | 0014 | Motion matching, root motion, spring bones, GPU skinning | ✅ |
| TR-anim-003 | 0014 | AnimationEventDispatcher → EventBus bridge; atomic ragdoll handoff | ✅ |
| TR-edit-001 | 0015 | PIE state machine (Stopped/Playing/Paused) + snapshot/restore | ✅ |
| TR-edit-002 | 0015 | Undo/Redo (ICommand + ValueCommand<T> + UndoSystem) | ✅ |
| TR-edit-003 | 0015 | Reflection (GX_REFLECT_* + TypeRegistry + JsonSerializer) | ✅ |
| TR-edit-004 | 0015 | NodeGraph runtime (Lua-peer) | ✅ |
| TR-edit-005 | 0015 | GX_EDITOR=OFF shipping exclusion | ✅ |
| TR-edit-006, 007 | 0015 | Reflection macro ODR + editor focus non-steal during PIE | ✅ |
| TR-bus-001..006 | 0016 | Type-safe pub/sub + HandlerCategory + ReplayMode + QueueFromWorker + AnimationBridge + dispatch order | ✅ |
| TR-ai-001 | 0018 | BehaviorTree + Blackboard + Running-resumption | ✅ |
| TR-ai-002 | 0018 | GOAPPlanner (backward A*, WorldState, preconditions/effects) | ✅ |
| TR-ai-003 | 0018 | NavMesh (2D grid A*) | ✅ |
| TR-ai-004 | 0018 | NavMesh3D (voxel A* + off-mesh links) | ✅ |
| TR-ai-005 | 0018 | PolyNavMesh (funnel smoothing) | ✅ |
| TR-ai-006 | 0018 | RVO (half-responsibility, XZ projection) | ✅ |
| TR-ai-007 | 0018 | AI module Foundation-only; synchronous; caller threading | ⚠️ *see E1* |
| TR-scn-001 | 0019 | Entity/Component, O(1) lookup by type enum, hierarchy, deferred destroy | ✅ |
| TR-scn-002 | 0019 | SceneManager (stack + fade + factory) | ✅ |
| TR-scn-003 | 0019 | ScenePersistence (.gxscene / .gxscbin) | ⚠️ *see E3* |
| TR-scn-004 | 0019 | Prefab + PrefabVariantSystem | ✅ |
| TR-scn-005 | 0019 | SceneSnapshot + SimulationManager (PIE backend) | ⚠️ *see R1* |
| TR-scn-006 | 0019 | SceneStreamer (distance + hysteresis + callbacks) | ✅ |
| TR-scn-007 | 0019 | SceneRenderer in Graphics/3D/, not Core/Scene/ | ⚠️ *see E2* |
| TR-scn-008 | 0019 | EntityBridge (OOP↔ECS, ClearMappings on unload) | ✅ |
| TR-mov-001 | 0020 | MoviePlayer (MF decode → RGB32 texture handle) | ⚠️ *see R3, E4* |
| TR-mov-002 | 0020 | VideoRecorder (swap-chain capture → MP4) | ⚠️ *see E4* |

Entries marked ⚠️ are covered (an ADR addresses them) but the Phase 4/5 audit
uncovered a real issue with the coverage. See the detailed findings below.

---

## Phase 4 — Cross-ADR Conflicts

### 🔴 R1 — ADR-0015 PIE vs ADR-0019 SceneSnapshot: dual ownership of PIE lifecycle

- **Type**: State management / data ownership
- **ADR-0015 §3-4** claims `PlayInEditor` owns the PIE state machine (`Stopped/Playing/Paused`) and `PlayInEditor::TakeSnapshot(const Scene&) → PIESnapshot`.
- **ADR-0019 §7** claims `SimulationManager` owns the identical state machine (`Stopped → Playing → Paused`) with `Play/Pause/Resume/Stop/Step`, and `SceneSnapshot::Capture/Restore` is the actual capture mechanism: "SimulationManager + SceneSnapshot are the PIE backend."
- **Impact**: Readers following ADR-0015 will look for `PlayInEditor::EnterPlayMode`; readers following ADR-0019 will look for `SimulationManager::Play`. Unclear whether ADR-0015 wraps ADR-0019, duplicates it, or supersedes it.
- **Resolution options**:
  1. **Layering clarification** — patch both ADRs: `PlayInEditor` orchestrates editor UI + lifecycle and delegates state-machine + snapshot to `SimulationManager` + `SceneSnapshot`. `PIESnapshot` becomes a thin struct wrapping `SceneSnapshot`.
  2. **Deprecation** — audit `GXLib/Editor/PlayInEditor.*` and `GXLib/Core/Scene/SimulationManager.*`; supersede the non-load-bearing ADR section.

### 🔴 R2 — ADR-0018 AI threading contract self-contradicts

- **Type**: Threading model integrity
- **ADR-0018 §Constraints**: "All AI calls are synchronous. Thread-safety is the caller's responsibility."
- **ADR-0018 §9** (first paragraph): "All AI API calls are main-thread-only by convention. No internal synchronisation."
- **ADR-0018 §9** (second paragraph): "Callers who want parallel AI ticks submit them as independent Jobs via ADR-0006. Since each BehaviorTree/NavAgent owns private state … this is safe as long as no two Jobs share the same instance."
- **Impact**: "Main-thread-only by convention" directly disallows what the same section then permits ("parallel AI ticks via Jobs"). Implementers following the strict reading cannot use ADR-0006 Job System parallelism for AI; implementers following the permissive reading will see no synchronisation on shared structures and introduce races.
- **Resolution options**:
  1. Rewrite §9: *per-instance non-reentrant — each AI object is owned by a single thread at a time. 100 BehaviorTrees across 100 Jobs is safe iff each Job owns its own Tree.* Drop the "main-thread-only by convention" sentence.
  2. Harden to truly main-thread-only and remove the parallel-Jobs clause.
- **Recommendation**: option 1 — it matches actual usage patterns and the zero-sync code.

### 🔴 R3 — ADR-0020 MoviePlayer bypasses ADR-0007 AssetDatabase

- **Type**: Integration contract violation
- **Control Manifest Foundation Layer § "Required Patterns" line 21**: "All asset access goes through AssetDatabase — no direct `fopen`/`CreateFile` in subsystem code. Source files may NOT `#include` or hard-code filesystem paths."
- **ADR-0020 §1**: `MoviePlayer::Open(filePath, device, texManager)` takes a raw filePath and opens it via Media Foundation (internal `fopen` equivalent outside `IFileProvider`).
- **ADR-0020 Depends On**: lists only ADR-0001 and ADR-0002. **ADR-0007 is not listed**, so the exception is undeclared.
- **Impact**: MoviePlayer is exempt from the AssetDatabase provider chain — cannot hot-reload video, cannot be mod-remapped (`AssetRemapper`), cannot be packed into `.gxa` / `.pak` archives. Hard-coded paths violate the Control Manifest invariant.
- **Resolution options**:
  1. **Accept the exception** — patch ADR-0020 Depends On to include ADR-0007 and add a §"Known Exception" section: "MoviePlayer bypasses AssetDatabase because Media Foundation requires a seekable `IMFByteStream`. Future extension: wrap `IFileProvider`-backed stream as `IMFByteStream`."
  2. **Route through AssetDatabase** — implement `IMFByteStream` adapter over `IFileProvider`; caller passes `AssetId`. Larger refactor but preserves invariant.
- **Recommendation**: option 1 for now, schedule option 2 before the first shipping title that requires modded cutscenes.

### ⚠️ Minor concerns from Phase 4

1. **ADR-0020 §2 VideoRecorder vs ADR-0008 Rendering** — capture-point ambiguity (pre-Present FrameGraph pass vs post-Present readback); capture thread not stated.
2. **ADR-0020 §4 MoviePlayer + ADR-0013 rollback** — MoviePlayer uses wall-clock timing; if ever active inside a rollback section, it desyncs. No forbidden pattern currently records this.
3. **ADR-0019 missing dependency on ADR-0016** — Scene is a prime EventBus producer (scene-loaded, entity-created, streaming-volume-loaded) but has zero EventBus mention.
4. **ADR-0018 missing EventBus guidance** — AI Actions running as Jobs must use `EventBus::QueueFromWorker<T>`, not `Fire<T>`, per ADR-0016 main-thread contract. ADR-0018 forbidden_patterns does not record this.
5. **ADR-0004 / ADR-0019 EntityBridge API split** — `Attach(Node*, Entity)` (ADR-0004) vs `ImportEntity/ExportEntity/SyncSceneToWorld` (ADR-0019) with no cross-reference. Likely the same class, different methods.
6. **ADR-0019 / ADR-0008 SceneRenderer not explicitly a FrameGraph pass** — layer isolation asserted but topology not cross-referenced.
7. **ADR-0004 §Decision prose** — implies AI/particles/projectiles are canonical ECS consumers; ADR-0018 §Alternative 3 explicitly rejects ECS integration at the library level. Wording tension only.
8. **EntityBridge stale-sync risk** — no debug-mode assertion when OOP transform is mutated without a subsequent `SyncSceneToWorld` call.

### Dependency graph

✅ **No cycles detected.** Recommended topological build order (extended from prior review):

```
1. ADR-0001 (docs)
2. ADR-0002 (DX12)
3. ADR-0003 (Compat) | ADR-0006 (Job) | ADR-0018 (AI)        ← all leaf-deps
4. ADR-0004 (ECS)
5. ADR-0007 (AssetDB)
6. ADR-0008 (Rendering) | ADR-0009 (Physics) | ADR-0011 (Input)
7. ADR-0010 (Audio) | ADR-0014 (Animation)
8. ADR-0005 (Lua) | ADR-0012 (GUI) | ADR-0013 (Networking) | ADR-0017 (Pillar)
9. ADR-0015 (Editor) | ADR-0019 (Scene) | ADR-0020 (Movie)
10. ADR-0016 (EventBus) — codifies cross-system contract (highest layer)
```

**No unresolved forward declarations.** ADR-0013 §13's forward-reference to the
EventBus ADR is resolved by ADR-0016 (Accepted).

### Performance Budget Check

Main-thread serial budget across all ADRs:

| Item | Owning ADR | ms |
|------|-----------|-----|
| Input sampling | 0011 | ≤ 0.05 |
| Audio main-thread API | 0010 | ≤ 0.3 |
| Physics command-buffer flush + ECS mirror | 0009 | ≤ 0.7 |
| Animation transform mirror to ECS | 0014 | ≤ 0.3 |
| FrameGraph compile + Present | 0008 | ≤ 0.6 |
| EventBus dispatch | 0016 | ≤ 0.5 |
| Light cluster build | 0008 | ≤ 0.3 |
| JobSystem scheduling overhead | 0006 | ≤ 0.2 |
| UI HUD update+render | 0012 | ≤ 0.5 |
| ImGui / Editor pass (GX_EDITOR=ON) | 0015 | ≤ 1.0 |
| Game logic / Lua tick headroom | 0005 | ~3.0 |
| **Main-thread subtotal** | | **~7.2 / 16.6 ms** |

ADR-0018/0019/0020 introduce no new main-thread budget:
- AI: BT tick ~0.01 ms/instance; worker-parallel per caller's Job submission.
- Scene: `SceneManager::Update` O(1); `SceneStreamer::Update` typically < 0.1 ms.
- Movie: Media Foundation decode is on its own thread; main-thread cost is negligible (`Update` timer tick).

**~9 ms headroom remains.** ✅

---

## Phase 5 — Engine Compatibility + Specialist Consultation

### 🔴 E1 — ADR-0018 AI: code violates the ADR's own "Foundation-only dependency" claim

- **ADR-0018 §Constraints**: "AI module links only `GXLib_Foundation` (Math + Container + Core utilities). No ECS, Physics, or JobSystem dependency at compile time."
- **Actual code**:
  - `GXLib/AI/NavMesh.cpp` line 7-8: `#include "Graphics/3D/Terrain.h"` + `#include "Graphics/3D/PrimitiveBatch3D.h"`
  - `GXLib/AI/NavMesh3D.cpp` line 7: `#include "Graphics/3D/PrimitiveBatch3D.h"`
- **Why the header is clean but the .cpp is not**: headers use forward declarations (`class PrimitiveBatch3D; class Terrain;`) so consumers don't transitively pull Graphics, but the `.cpp` files always link Graphics for `DebugDraw` + `BuildFromTerrain`. A game using only AI still pulls the Graphics chain.
- **Suggested patch**: move the Graphics-dependent `DebugDraw` methods into a separate `GXLib/AI/Debug/NavMeshDebug.cpp` (or a `Graphics/AIDebug/` layer) explicitly declared to depend on Graphics. `BuildFromTerrain` could accept raw height-field data instead of `const Terrain&`. Core `NavMesh.cpp`/`NavMesh3D.cpp` remain Foundation-only.
- **ADR update required**: either amend ADR-0018 §Constraints to admit "debug visualisation requires Graphics" as an opt-in sibling module, or hold the line and fix the code.

### 🔴 E2 — ADR-0019 Scene: code violates `scene_renderer_in_core` forbidden pattern

- **ADR-0019 §Forbidden Patterns**: `scene_renderer_in_core — no #include "Graphics/..." from Core/Scene/ files.`
- **Actual code**: `GXLib/Core/Scene/SceneSerializer.cpp` line 6: `#include "Graphics/3D/GraphicsComponents.h"`
- **Impact**: the Core/Scene layer's Graphics-module isolation is already broken. Any TU linking `Core/Scene` now forces a Graphics compile-time dependency — the exact coupling the architecture is designed to prevent.
- **Suggested patch**: remove the include; move Graphics-component serialisation to a sibling `GXLib/Graphics/3D/SceneGraphicsSerializer.cpp` and register via a `ModelSaveCallback` symmetric to the existing `ModelLoadCallback`. Keep `SceneSerializer.cpp` Foundation+Core-only.

### 🔴 E3 — ADR-0019 Scene: `ScenePersistence::SaveToFile` is non-atomic

- **Actual code**: `ScenePersistence.cpp` lines 625-639 open `std::ofstream(path, ...)` directly on the destination. Both text and binary branches write directly. No temp-file-then-rename pattern.
- **Impact**: crash / power loss / exception during save leaves the scene file half-written. For an editor-authored scene, this is a data-loss risk on the only copy.
- **Suggested patch**: write to `path + ".tmp"`, `flush + close`, then `MoveFileExW(tmp, final, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`. Two-line change in each branch.
- **ADR update required**: add to §Persistence an explicit atomicity rule: "SaveToFile must be atomic — temp-rename pattern; partial writes must never be observable on the destination path."

### 🔴 E4 — ADR-0020 Movie: `MFStartup`/`MFShutdown` are per-instance, but the MF platform refcount is process-wide

- **Actual code**:
  - `MoviePlayer::Open` calls `MFStartup(MF_VERSION)`; `MoviePlayer::Close` calls `MFShutdown()`.
  - `VideoRecorder::Initialize` calls `MFStartup(MF_VERSION)`; destructor calls `MFShutdown()`.
- **Impact**: MF platform refcount is process-global. If `MoviePlayer` and `VideoRecorder` coexist (a common case: recording a play session that includes a cutscene) and `MoviePlayer::Close` is called first, `MFShutdown` decrements the refcount to 0 **while VideoRecorder still holds a live `IMFSinkWriter`.** Subsequent `WriteSample` calls operate against a torn-down platform → silent failures, access violations, or test-hard crashes.
- **ADR-0020 §Forbidden Patterns already names `mf_global_init`** but phrases the rule wrong: "Each instance manages its own MF session" — this **is** the bug. MF sessions are not per-instance; `MFStartup`/`MFShutdown` is apartment-level refcount.
- **Suggested patch**: introduce `GXLib/Core/MFPlatform` singleton with `Acquire()` / `Release()` refcounting internally. Both `MoviePlayer` and `VideoRecorder` call `MFPlatform::Acquire()` at init and `MFPlatform::Release()` at teardown. Only the last `Release` calls `MFShutdown`.
- **ADR update required**: rewrite `mf_global_init` forbidden pattern to: *"Do not call `MFStartup`/`MFShutdown` directly. All callers use `MFPlatform::Acquire/Release` to participate in the shared refcount. Raw direct calls break cross-module MF cohabitation."*

### ⚠️ Minor concerns from Phase 5

1. **ADR-0020 MoviePlayer per-frame `CreateCommittedResource`** — DX12 `CreateCommittedResource` is not free-threaded; `TextureManager::CreateTextureFromMemory` being called while previous frame's upload is in flight on the copy queue is a latent debug-layer warning. Verifiable only by reading `TextureManager` source. Flag for the next code review pass.
2. **ADR-0020 VideoRecorder::CaptureFrame** — assumes back buffer in `D3D12_RESOURCE_STATE_RENDER_TARGET`. Undocumented precondition; callers who've already transitioned to `PRESENT` will hit a debug-layer error. Add docstring + `§2` text.
3. **ADR-0018 GOAPPlanner.h** — includes raw `<string>/<vector>/<unordered_map>/<functional>` instead of `pch_common.h` pattern used by all other AI headers. Consistency gap, not a bug.
4. **ADR-0018 NavAgent** — `Initialize(NavMesh* navMesh)` binds only to 2D grid `NavMesh*`. The ADR's architecture diagram shows NavAgent working with all three variants (NavMesh, NavMesh3D, PolyNavMesh). Aspirational vs actual mismatch — downgrade the diagram or add templated/overloaded Initialize.

### Version consistency: ✅ No ADR references a stale engine version.
### Deprecated API references: ✅ None detected.
### Missing Engine Compatibility sections: ✅ All 20 ADRs have the section (confirmed by structural scan).

---

## Phase 5b — GDD Revision Flags

**N/A.** ADR-only project per ADR-0001. No GDDs to flag.

---

## Phase 6 — Architecture Document Coverage

`docs/architecture/architecture.md` exists as the master architecture document.
Consistency spot-checks:

- ✅ All 20 ADRs are referenced at least once
- ✅ `control-manifest.md` (date-stamped) reflects all 20 ADRs' forbidden_patterns
- ⚠️ The `architecture.md §7` frame-budget table should now include a line for the
  `GX_EDITOR=ON` ImGui pass (currently folded into "misc main-thread") — minor.
- ⚠️ `architecture.md §2 Tech Stack` lists Media Foundation without the clarifying
  "video-only in GXLib" note — easy add.

No orphaned architecture (architecture document without corresponding ADR) detected.

---

## Blocking Issues (must resolve before PASS)

| # | Fix type | Target | Effort |
|---|----------|--------|--------|
| R1 | ADR text edit | ADR-0015 + ADR-0019 layering clarification | Small |
| R2 | ADR text edit | ADR-0018 §9 rewrite | Small |
| R3 | ADR text edit + dependency declaration | ADR-0020 add ADR-0007 to Depends On + Known Exception section | Small |
| E1 | **Code change + ADR amendment** | Split `NavMeshDebug.cpp` out of `AI/`; keep core AI Foundation-only | Medium |
| E2 | **Code change** | Remove `#include "Graphics/3D/GraphicsComponents.h"` from `SceneSerializer.cpp`; add ModelSaveCallback pattern | Small-Medium |
| E3 | **Code change + ADR amendment** | Temp-rename atomic write in `ScenePersistence::SaveToFile`; add §Persistence atomicity rule | Small |
| E4 | **Code change + ADR amendment** | Introduce `MFPlatform` refcount wrapper; rewrite `mf_global_init` forbidden pattern | Medium |

Total: 3 ADR-only fixes + 4 code+ADR fixes. None break existing single-instance
smoke-tested usage. All are preconditions for confident production use across
the combinations the ADRs claim to support.

---

## Required ADRs

**None** — every charter-level subsystem has an Accepted ADR (AI, Scene, Movie
added since the 2026-04-17 review close the last retroactive gaps).

Future (deferred, not gaps): see `architecture-traceability.md` § Deferred /
Forward-Looking Items.

---

## Deferred-items maintenance

Apply the following changes to `architecture-traceability.md`:

| Change | Item | Reason |
|--------|------|--------|
| **Remove (completed)** | `TR-defer-scene-architecture` | ADR-0019 published 2026-04-17 closes this |
| **Remove (completed)** | `TR-defer-movie-pipeline` | ADR-0020 published 2026-04-17 closes this |
| **Promote to active follow-up** | `TR-defer-game-shipping-preset` | ADR-0015 §11 Migration Plan step 2 declares this as immediate work |

All remaining 10 deferred items are appropriate forward-looks, not hidden gaps.

---

## Pattern Observation (cumulative)

Self-review → independent review delta since this project began:

| Review | Self-review verdict | Independent verdict | Real issues caught |
|--------|---------------------|---------------------|---------------------|
| 2026-04-15 | N/A | CONCERNS | — |
| 2026-04-16 (run 1-3) | Self, CONCERNS | TD same-session caught 3 | 3 |
| 2026-04-17 (fresh) | Self, clean | CONCERNS caught 5 | 5 |
| 2026-04-18 (fresh, this) | — | CONCERNS caught 7 | 7 |

**Conclusion**: skipping fresh-session validation at any promotion point costs
~3-5 real architectural seams per ADR batch. Do not promote ADR-0018/0019/0020
patched versions without another fresh-session re-review after patching.

---

## Handoff

### Recommended next actions, in order

1. **Fix E3 (atomic scene save)** — smallest effort, largest data-loss reduction
2. **Fix E2 (SceneSerializer Graphics include)** — restores the Core/Scene layer boundary
3. **Fix E4 (MFPlatform refcount wrapper)** — enables Movie + Recorder cohabitation
4. **Fix E1 (split NavMeshDebug out of AI/)** — restores Foundation-only claim
5. **Patch ADR-0018/0019/0020 text for R1/R2/R3** (and relevant pieces of E1/E3/E4 ADR updates)
6. Re-run `/architecture-review` in a fresh session to confirm PASS before any
   story implementation blocks on these decisions

### Gate guidance

**Do NOT advance past Pre-Production → Production** until E2 + E3 + E4 are fixed
in code. E1 + R-series are ADR-text issues only — still blockers for architectural
correctness but do not break running code.

### Rerun trigger

Re-run `/architecture-review` after each patch batch lands; target PASS verdict
before kicking off the next sprint that implements a story touching Scene, AI,
or Movie subsystems.

---

**Review signed**: fresh session 2026-04-18, 3 independent agents (Explore for
coverage, technical-director for Phase 4, engine-programmer for Phase 5).
Report generated: `docs/architecture/architecture-review-2026-04-18.md`.
