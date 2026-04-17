# ADR-0019: Scene Architecture (Entity/Component, SceneManager, Persistence, Prefabs, Streaming)

## Status
Accepted

## Date
2026-04-17

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Core / Scene Graph / Persistence |
| **Knowledge Risk** | LOW — OOP entity/component model, stack-based scene manager, JSON/binary serialisation, prefab instantiation with variant overrides, distance-based scene streaming, and snapshot/restore for play-in-editor are well-documented patterns within LLM training data |
| **References Consulted** | `GXLib/Core/Scene/{Scene,Entity,Component,Components,SceneManager,ScenePersistence,SceneSerializer,SceneSnapshot,SimulationManager,SceneStreamer,Prefab,PrefabVariant,TransitionEffect}.{h,cpp}` (source of truth), `GXLib/Graphics/3D/SceneRenderer.{h,cpp}` (rendering separation), `GXLib/ECS/EntityBridge.{h,cpp}` (OOP↔ECS bridge), `Tests/test_Entity.cpp`, `Tests/test_SceneManager.cpp`, `Tests/test_SceneStreamer.cpp`, `Tests/test_ScenePersistence.cpp`, `Tests/test_SceneSerializer.cpp`, CHANGELOG Phases 0-5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | ScenePersistence text/binary round-trip preserves all entity state (transform, hierarchy, active, components) for 50+ entities; SceneManager fade-alpha interpolation is frame-rate-independent; SceneStreamer hysteresis prevents load/unload thrashing; PrefabVariant nested overrides apply in correct inheritance order; EntityBridge bidirectional sync preserves identity across OOP↔ECS; SceneSnapshot Capture+Restore returns entity state to pre-capture values; Component lookup O(1) via fixed-size array indexed by ComponentType |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0004 (ECS — EntityBridge syncs OOP Entity ↔ ECS World), ADR-0007 (Asset Database — scene files, prefab JSON, streaming volumes flow through AssetDB), ADR-0015 (Editor — SimulationManager/SceneSnapshot are the PIE backend; SceneHierarchyPanel navigates Entity tree) |
| **Enables** | Future ADRs on level-of-detail scene authoring, additive scene composition workflows, collaborative scene editing, deterministic scene replay |
| **Blocks** | None (code already exists since Phases 0-5; retroactive) |
| **Ordering Note** | ADR-0004 (ECS) and ADR-0015 (Editor) both reference Scene concepts; this ADR makes those references concrete. |

## Context

### Problem Statement

GXLib's Scene subsystem spans Phases 0 through 5: the Entity/Component model (Phase 0), Prefab system (Phase 1), SceneManager transitions (Phase 2), persistence (Phase 3→5), Play-in-Editor (Phase 4), and streaming (Phase 5). Together these form the largest subsystem by file count — 13+ classes in `Core/Scene/` plus a separate `SceneRenderer` in `Graphics/3D/`. No ADR has codified the layering, the ownership model, the serialisation contracts, or the relationship between OOP Scene entities and ECS World entities (ADR-0004). This ADR closes that gap.

### Constraints

- Scene owns entities; rendering is separated. `SceneRenderer` lives in `Graphics/3D/`, not `Core/Scene/`. Scene has no `#include` of any Graphics header.
- One component of each `ComponentType` per Entity. The `ComponentType` enum has 12 entries; `Custom` is shared by `NameComponent` and `TagComponent` (known collision).
- `EntityBridge` is a process-global static mapping. `ClearMappings()` must be called on scene unload.
- Prefab serialisation uses JSON strings. PrefabVariant IDs start at `0x100000` to avoid collision with base Prefab IDs (which start at 1).
- SceneStreamer calls `Scene::MergeFrom` to integrate loaded volumes — this moves entities from the loaded scene into the main scene (destructive on source).
- `SceneSnapshot` captures transform, active, name, and a few component fields. It does NOT capture the full component state (same shallow-snapshot limitation documented in ADR-0015 §4).

### Requirements

- **Entity/Component model**: Entity with embedded Transform3D, typed component array (O(1) lookup by ComponentType enum), parent/child hierarchy, bounding info, active flag. Deferred destroy in Update.
- **SceneManager**: Stack-based scene transitions. `ChangeScene` (clear + push), `PushScene`, `PopScene`. Fade transitions with configurable duration. `SceneFactory` for deferred construction.
- **ScenePersistence**: Text (`.gxscene`) and binary (`.gxscbin`) save/load. Round-trip: entity name, ID, active, transform, hierarchy, components.
- **SceneSerializer**: JSON-specific serialiser with `ModelLoadCallback` for asset resolution during load.
- **Prefab**: JSON-blob capture/instantiate. `CaptureFromEntity` + `Instantiate(scene)`.
- **PrefabVariantSystem**: Registry with base prefabs and variant overrides (property-level, component-level, per-child-entity). Nested variants (variants of variants). Save/Load the whole registry.
- **SceneSnapshot + SimulationManager**: In-memory state capture for PIE. SimulationManager owns the Play/Pause/Resume/Stop/Step lifecycle.
- **SceneStreamer**: Distance-based streaming with hysteresis (loadRadius < unloadRadius). `ForceLoad`/`ForceUnload` for editor/cutscene. Callbacks on state transitions.
- **TransitionEffect**: CPU-side per-pixel mask (8 built-in types: Fade, Wipe, WipeVertical, CircleOpen/Close, Dissolve, SlideLeft/Right).
- **SceneRenderer**: Separate module in Graphics/3D/. Frustum culling, auto-instancing (threshold 4), animation update, render stats.
- **EntityBridge**: Bidirectional sync between OOP Entity and ECS World. Static process-global ID mapping.

## Decision

**GXLib's Scene subsystem is an OOP entity/component graph (`Core/Scene/`) with explicit rendering separation (`Graphics/3D/SceneRenderer`). Entities are owned by Scene, one component per ComponentType via O(1) enum-indexed lookup. SceneManager provides stack-based transitions with fade. Persistence supports text and binary formats. Prefabs are JSON-blob based with a variant override system for inheritance. SceneSnapshot + SimulationManager provide PIE and checkpoint functionality. SceneStreamer handles distance-based additive loading. EntityBridge syncs the OOP graph with the ECS World (ADR-0004) via a process-global static mapping.**

Concrete rules:

1. **Layering.**
   ```
   Presentation    SceneRenderer (Graphics/3D/ — frustum cull, instancing, animation)
   Authoring       PrefabVariantSystem, TransitionEffectManager
   Lifecycle       SceneManager (stack + fade), SceneStreamer (distance + hysteresis)
   Persistence     ScenePersistence (text/binary), SceneSerializer (JSON + asset callback)
   Checkpoint      SceneSnapshot, SimulationManager (PIE backend)
   Core            Scene (entity container + update), Entity (component host + hierarchy)
   Foundation      Component (base), ComponentType enum, Transform3D (embedded)
   Bridge          EntityBridge (OOP ↔ ECS static mapping)
   ```

2. **Entity ownership.**
   - Scene owns all Entity instances via `unique_ptr`. `CreateEntity` allocates; `DestroyEntity` defers to next `Update` flush.
   - Entity IDs are auto-incremented `uint32_t`, unique within a Scene instance. Cross-scene ID uniqueness is NOT guaranteed.
   - Parent/child hierarchy is maintained via raw Entity pointers. `SetParent(nullptr)` promotes to root.

3. **Component model.**
   - `ComponentType` enum (12 types + `_Count`). One component per type per entity.
   - O(1) lookup via `m_componentLookup[ComponentType::_Count]` int array. Swap-with-last removal.
   - `Custom` type is shared by `NameComponent` and `TagComponent` — known limitation; only one Custom component per entity.
   - All components implement `Clone()` for Prefab/Snapshot use.

4. **SceneManager.**
   - Stack-based: `ChangeScene` clears entire stack and pushes; `PushScene` preserves lower; `PopScene` restores previous.
   - `SceneFactory = std::function<unique_ptr<Scene>()>` — deferred construction.
   - Fade transitions: `SceneTransitionDesc { fadeOutDuration, fadeInDuration }`. `GetTransitionAlpha()` interpolates 0→1 (fade-out) then 1→0 (fade-in).
   - `Update(dt)` drives transition timer and calls `Scene::Update` on current top.

5. **Persistence.**
   - `ScenePersistence`: dual-format static utility. Text (`.gxscene`) for human readability; binary (`.gxscbin`) for compact storage.
   - Persists: scene name, entity (name, ID, active, transform position/rotation/scale, parent name, components).
   - `SceneSerializer`: JSON-specific, operates on existing `Scene&` (in-place load). `ModelLoadCallback` resolves asset paths.
   - Neither format persists runtime state (particle emitters, audio voices, network sessions).

6. **Prefab system.**
   - `Prefab`: JSON blob. `CaptureFromEntity` serialises; `Instantiate(scene)` deserialises into a new entity.
   - `PrefabVariantSystem`: registry of base prefabs (IDs 1+) and variants (IDs 0x100000+). Property-level and component-level overrides. Nested variants with inheritance chain. `ApplyOverrides` produces merged data.
   - Variant overrides are additive; `RevertOverride` restores base value; `RevertAllOverrides` resets to base.

7. **Checkpoint (SceneSnapshot + SimulationManager).**
   - `SceneSnapshot::Capture(scene)` stores entity state (ID, name, active, transform, parent, limited component data).
   - `SceneSnapshot::Restore(scene)` writes back. Same shallow-snapshot limitation as ADR-0015 §4.
   - `SimulationManager`: state machine (`Stopped → Playing → Paused → Playing → Stopped`). Owns a `SceneSnapshot`. `Play` captures; `Stop` restores; `Step` advances one fixed-dt frame while Paused.

8. **SceneStreamer.**
   - `AddVolume(StreamingVolume)` registers a scene path + load/unload radii + position.
   - `Update(playerX, playerY, playerZ)` checks distances. Hysteresis: load at `loadRadius`, unload only past `unloadRadius` (> loadRadius).
   - Loaded scenes integrated via `Scene::MergeFrom` (moves entities, destructive on source).
   - `ForceLoad`/`ForceUnload` bypass hysteresis for editor and cutscene use.
   - Callbacks: `SetOnSceneLoaded`, `SetOnSceneUnloaded`.

9. **SceneRenderer (separate module).**
   - `Render(scene, renderer)` performs frustum culling, auto-instancing (threshold 4 identical meshes), and submits draw calls.
   - `UpdateAnimations(scene, dt)` ticks all `Animator` components before render.
   - `SceneRenderStats` tracks visible/culled entities, draw calls, instanced batches.
   - Lives in `Graphics/3D/`, NOT `Core/Scene/`. Scene has no graphics dependency.

10. **EntityBridge (OOP ↔ ECS).**
    - `ImportEntity(world, entity)` creates an ECS entity with BridgePosition/Rotation/Scale/Name components.
    - `ExportEntity(world, ecsId, entity)` writes back from ECS to OOP.
    - `SyncSceneToWorld` / `SyncWorldToScene` batch-sync all mapped entities.
    - Static process-global maps (`s_entityMap`, `s_reverseMap`). `ClearMappings()` required on scene unload.
    - `BridgeName[64]` — max 63 chars. Known constraint.

11. **TransitionEffect.**
    - 8 built-in types: Fade, Wipe, WipeVertical, CircleOpen, CircleClose, Dissolve, SlideLeft, SlideRight.
    - `ITransitionEffect::GetMaskValue(progress, screenX, screenY)` returns 0-1 mask.
    - `TransitionEffectManager` owns the active effect and progress. Integrates with SceneManager fade.

12. **Forbidden patterns.**
    - `scene_entity_raw_delete` — never `delete` an Entity pointer; use `Scene::DestroyEntity` (deferred).
    - `scene_renderer_in_core` — no `#include "Graphics/..."` from `Core/Scene/` files.
    - `entity_bridge_stale_mapping` — `EntityBridge::ClearMappings()` must be called when a Scene is destroyed; stale mappings cause dangling ECS entity references.
    - `two_custom_components` — Entity supports only one `ComponentType::Custom`; adding both NameComponent and TagComponent as Custom components will collide.

## Alternatives Considered

### Alternative 1: Pure ECS scene (no OOP Entity)
- **Pros**: One data model; no bridge synchronisation.
- **Cons**: ECS is data-oriented and optimised for batch iteration, not for hand-placed authored content with hierarchy, inspector editing, and per-entity scripting. Beginners coming from DXLib find Entity/Component familiar.
- **Rejection Reason**: ADR-0004 already established the dual model (OOP for authoring, ECS for high-volume). Removing OOP would break the beginner surface.

### Alternative 2: Scene inherits from World (unified container)
- **Pros**: One container; no bridge.
- **Cons**: OOP Entity and ECS Entity have different memory layouts, ownership models, and iteration patterns. Forcing them into one container creates an impedance mismatch worse than the bridge.
- **Rejection Reason**: The bridge is the minimal coupling point. It syncs only Transform + Name + a few tags — not the full component set.

### Alternative 3: Single serialisation format (JSON only)
- **Pros**: Simpler codebase; one format to maintain.
- **Cons**: JSON is verbose for large scenes (50+ entities ≈ 100 KB text vs 20 KB binary). Binary is needed for fast load in shipped games.
- **Rejection Reason**: Dual format already exists and is tested. Text for authoring, binary for shipping.

## Consequences

### Positive
- Clean separation: Scene owns no rendering code. Graphics module can be swapped without touching Scene.
- Stack-based SceneManager with fade is simple and covers 90% of game scene-transition patterns.
- Dual persistence formats serve both authoring (text, human-readable) and shipping (binary, compact).
- PrefabVariant system enables content-pipeline workflows (base enemy → variant per level).
- SceneStreamer with hysteresis prevents load/unload thrashing in open-world games.
- SimulationManager + SceneSnapshot reuse across PIE and game-checkpoint use cases.

### Negative
- One component per ComponentType limits entity composition (no two MeshRenderers on one entity).
- `Custom` type collision between NameComponent and TagComponent is a known limitation.
- EntityBridge's process-global static maps preclude multi-World isolation.
- SceneSnapshot is shallow — same limitation as ADR-0015 §4 PIE snapshot.
- SceneStreamer uses `MergeFrom` (destructive move) — no incremental unmerge; unloading a volume requires tracking which entities came from it.
- Entity IDs are per-Scene auto-increment, not globally unique — cross-scene references require custom mapping.

### Risks
- **EntityBridge stale mappings** after scene unload crash or corrupt ECS state. *Mitigation*: `entity_bridge_stale_mapping` forbidden pattern; `ClearMappings()` in scene teardown.
- **PrefabVariant registry ID collision** if base and variant ID ranges overlap. *Mitigation*: variant IDs start at 0x100000; documented; assert on overlap.
- **SceneStreamer memory pressure** when many volumes are loaded simultaneously. *Mitigation*: volume budgeting is the game's responsibility; SceneStreamer logs loaded volume count.
- **Serialisation breaking change** if ComponentType enum is reordered. *Mitigation*: persistence uses component type names (strings), not enum values, for forward compatibility.
- **Custom component collision** silently overwrites the first Custom component with the second. *Mitigation*: `two_custom_components` forbidden pattern; future ADR may extend ComponentType enum or switch to string-keyed components.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Retroactive codification of Phases 0-5 Scene subsystem. Registers TR-scn-001 through TR-scn-008. |

## Performance Implications

- **CPU**: `Entity::GetComponent<T>` is O(1) (array index). `Scene::Update` is O(entities) for ScriptComponent callbacks. `SceneManager::Update` is O(1) for transition timer. `SceneStreamer::Update` is O(volumes) for distance checks.
- **Memory**: Entity ~200 bytes baseline (Transform + component lookup array + hierarchy pointers + bounds). Component varies: TransformComponent ~36 bytes, MeshRendererComponent ~80 bytes. SceneSnapshot ~100 bytes per entity captured.
- **Load Time**: `ScenePersistence::LoadFromFile` text ≈ 5 ms for 50 entities; binary ≈ 1 ms. `Prefab::Instantiate` ≈ 0.1 ms per entity. `SceneStreamer` loads are async (if caller routes through AssetDB + JobSystem).
- **Serialisation Size**: Text `.gxscene` ≈ 2 KB per entity (verbose). Binary `.gxscbin` ≈ 0.4 KB per entity (compact).

## Migration Plan

Not applicable — retroactive ADR. Going forward:

1. **String-keyed component system** (future ADR): replace `ComponentType` enum with string/hash keys to remove the one-per-type and Custom-collision limitations.
2. **World-scoped EntityBridge** (future): replace process-global static maps with per-World instances for multi-World isolation.
3. **Incremental scene unmerge** (future): track entity provenance in SceneStreamer to enable non-destructive unload.
4. **Deep SceneSnapshot** (future, shared with ADR-0015): full reflection-driven component serialisation for complete PIE/checkpoint fidelity.

## Validation Criteria

- **Existing tests pass**: test_Entity, test_SceneManager, test_SceneStreamer, test_ScenePersistence, test_SceneSerializer — all passing.
- **Persistence round-trip**: 50-entity scene → SaveToFile (text) → LoadFromFile → all entity names, IDs, transforms, hierarchy, active flags match.
- **Binary round-trip**: same as above with binary format; file size < text size.
- **SceneManager transitions**: ChangeScene clears stack (depth → 1); PushScene increments depth; PopScene decrements; fade alpha interpolates correctly over configured duration.
- **SceneStreamer hysteresis**: volume at distance < loadRadius transitions to Loaded; moving to distance between loadRadius and unloadRadius stays Loaded; moving past unloadRadius transitions to Unloaded.
- **Prefab instantiate**: CaptureFromEntity + Instantiate produces an entity with identical transform and component state.
- **PrefabVariant override**: base prefab with property A=1; variant overrides A=2; ApplyOverrides returns data with A=2; RevertOverride returns A=1.
- **EntityBridge sync**: ImportEntity then ExportEntity preserves position/rotation/scale within float epsilon.
- **Component O(1) lookup**: GetComponent<MeshRendererComponent> on an entity with 5 components completes in constant time (no linear scan).

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — EntityBridge syncs OOP Entity ↔ ECS World; dual-model architectural split)
- ADR-0007 (Asset Database — scene files, prefab JSON, streaming volumes flow through AssetDB)
- ADR-0008 (Rendering — SceneRenderer submits to Renderer3D; GBuffer pass consumes MeshRendererComponent)
- ADR-0015 (Editor — SimulationManager is the PIE backend; SceneHierarchyPanel, EntityPicker navigate Entity tree)
- ADR-0017 (Two-Layer Accessibility — Scene is L1.5: not in DXLib Compat, but usable without ECS)
- `GXLib/Core/Scene/*.{h,cpp}` (source of truth for Core layer)
- `GXLib/Graphics/3D/SceneRenderer.{h,cpp}` (rendering separation)
- `GXLib/ECS/EntityBridge.{h,cpp}` (OOP↔ECS bridge)
- CHANGELOG.md Phases 0-5
