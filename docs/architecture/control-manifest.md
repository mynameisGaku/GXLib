# Control Manifest

> **Engine**: GXLib (self-hosted, Phase 5, 2026-03-01)
> **Last Updated**: 2026-04-17
> **Manifest Version**: 2026-04-17
> **ADRs Covered**: ADR-0001 through ADR-0020 (all 20 Accepted)
> **Status**: Active — regenerate with `/create-control-manifest` when ADRs change

This manifest is a programmer's quick-reference extracted from all Accepted ADRs,
technical preferences, and engine reference docs. For the reasoning behind each
rule, see the referenced ADR.

---

## Foundation Layer Rules

*Applies to: engine init, event architecture, asset pipeline, job system, save/load*

### Required Patterns

- **All asset access goes through AssetDatabase** — no direct `fopen`/`CreateFile` in subsystem code. Use `AssetDatabase::FindAsset` or the typed provider chain. — source: ADR-0007
- **Hot reload flows FileWatcher → AssetReloader → per-type rebinder** — consumers subscribe via generation-counted handles; reload must not invalidate held pointers silently. — source: ADR-0007
- **Job System is the single worker pool** — subsystems submit jobs, never own private thread pools. One singleton `JobSystem` per process. — source: ADR-0006
- **Jobs must not re-enter the JobSystem while holding a lock** — the scheduler is lock-free on the hot path; cross-worker uses short spinlocks. — source: ADR-0006
- **`SubmitMainThread` for main-thread-final work** — job results that touch main-thread state (UI, scene mutation) must funnel through `SubmitMainThread`. — source: ADR-0006
- **EventBus is the single cross-system pub/sub spine** — no sibling buses. `gx::EventBus::Instance()`. — source: ADR-0016
- **EventBus handlers must carry a HandlerCategory** — `Idempotent` (deterministic state mutation only) or `SideEffect` (audio/VFX/network). Default is `SideEffect`. — source: ADR-0016
- **EventBus Fire/Subscribe/Unsubscribe are main-thread-only** — worker threads use `QueueFromWorker<T>` (new API, mutex-guarded). — source: ADR-0016
- **Documentation is ADR-only** — no GDDs. Requirements are sourced from ADRs and registered in `tr-registry.yaml`. — source: ADR-0001
- **Compat functions return int (0=success, -1=failure)** — no exceptions across the Compat boundary. Handle-returning functions return ≥0 on success, -1 on failure. — source: ADR-0003

### Forbidden Approaches

- **Never `Fire<T>` from a worker thread** (`eventbus_fire_from_worker_thread`) — causes data race on HashMap. Use `QueueFromWorker<T>` or `SubmitMainThread`. — source: ADR-0016
- **Never create a second EventBus** (`eventbus_second_instance`) — fragments analytics, replay-suppression, and test setup. — source: ADR-0016
- **Never spawn raw `std::thread`** — use JobSystem submit instead. Oversubscription degrades all worker performance. — source: ADR-0006
- **Never `fopen`/`CreateFile` directly in subsystem code** — route through AssetDatabase or IFileProvider. — source: ADR-0007
- **Never use DXLib global-namespace symbols** — all Compat lives in `namespace gx`. GXLib-exclusive functions use `GX_` prefix. — source: ADR-0003
- **Never match DXLib symbols verbatim in global namespace** — prevents linker collisions with actual DXLib. — source: ADR-0003

### Performance Guardrails

- **JobSystem overhead**: ≤ 0.2 ms/frame scheduling. Worker sleep/wake ≤ 50 μs. — source: ADR-0006
- **AssetDatabase `Get<T>` resolve**: ≤ 100 ns steady state (hashmap lookup + generation check). — source: ADR-0007
- **EventBus `Fire<T>`**: ≤ 0.01 ms per call (note: handler-vector copy allocates per call — accepted trade-off). — source: ADR-0016

---

## Core Layer Rules

*Applies to: ECS, physics, networking, scene management, input*

### Required Patterns

- **ECS opaque handles only** — `EntityHandle` = idx32+gen32. No raw pointers in ECS components. — source: ADR-0004
- **ECS query iteration is deterministic** — entity-id order within archetype. Safe for rollback. — source: ADR-0004
- **EntityBridge syncs OOP↔ECS** — `ImportEntity`/`ExportEntity`/`SyncSceneToWorld`/`SyncWorldToScene`. Call `ClearMappings()` on scene unload. — source: ADR-0004, ADR-0019
- **Physics uses fixed timestep** — 60 Hz default. No `variable_timestep_in_physics_solver`. Interpolate for rendering. — source: ADR-0009
- **Physics island solve is deterministic** — barrier-then-serial-merge in island-ID order. Reduction order is independent of Job completion order. — source: ADR-0009 §15
- **Rollback re-simulation re-runs full per-frame schedule** — Input → Animation → Physics → replication apply. Not just physics. — source: ADR-0013 §13
- **Rollback `SetReplayMode(true)` suppresses SideEffect handlers** — EventBus skips `SideEffect` handlers; `Idempotent` handlers still run. `Queue<T>` during replay is a no-op. — source: ADR-0016 §4
- **All networking calls are main-thread** — `NetworkManager` calls from main; serialisation/snapshot Jobs via JobSystem. — source: ADR-0013
- **Replicated state uses ECS EntityHandle + AssetId** — no raw pointers in replication payloads. — source: ADR-0013
- **SceneManager is stack-based** — `ChangeScene` clears stack; `Push/Pop` preserves lower. Fade transitions are timer-driven. — source: ADR-0019
- **Scene owns entities via unique_ptr** — use `DestroyEntity` (deferred), never raw `delete`. — source: ADR-0019
- **One component per ComponentType per Entity** — O(1) lookup via enum-indexed array. — source: ADR-0019
- **Input state is main-thread-only** — `InputManager::Update()` once per frame from main. Cross-thread reads are forbidden. — source: ADR-0011

### Forbidden Approaches

- **Never use `time(NULL)` in gameplay logic** — nondeterministic in rollback. Use replicated frame counter. — source: ADR-0013
- **Never iterate `std::unordered_*` / `gx::HashMap` in gameplay logic** — nondeterministic order. Use `gx::VectorMap` or archetype iteration. — source: ADR-0013
- **Never use `std::rand` / `std::random_device` in gameplay** — use `gx::Random` with replicated seed. — source: ADR-0013
- **Never enable fast-math in physics/network/animation TUs** (`nondeterministic_reduction_in_rollback_physics_stage`) — breaks rollback determinism. — source: ADR-0009, ADR-0013
- **Never store raw physics body pointers in ECS** (`raw_physics_body_pointer_in_ecs`) — use opaque handles. — source: ADR-0009
- **Never call `Undo/Redo` during rollback replay** (`undo_during_rollback_replay`) — corrupts gameplay state. — source: ADR-0015
- **Never delete an Entity pointer directly** (`scene_entity_raw_delete`) — use `Scene::DestroyEntity`. — source: ADR-0019
- **Never query NavMesh during build** (`navmesh_query_during_build`) — must synchronise build completion first. — source: ADR-0018
- **Never iterate Blackboard in rollback** (`blackboard_iteration_in_rollback`) — HashMap order is nondeterministic. Use key-by-key `Get()`. — source: ADR-0018
- **Never read input from a worker thread** (`cross_thread_input_read`) — InputManager has no synchronisation. — source: ADR-0011
- **Never leave stale EntityBridge mappings** (`entity_bridge_stale_mapping`) — `ClearMappings()` on scene destroy. — source: ADR-0019
- **Never add two Custom-type components** (`two_custom_components`) — ComponentType::Custom collision. — source: ADR-0019

### Performance Guardrails

- **Physics**: ≤ 3 ms/frame for 1000 bodies at 60 Hz (8 velocity + 3 position iterations). — source: ADR-0009
- **Networking replication**: ≤ 0.5 ms/frame for 1000 entities at 30 Hz. — source: ADR-0013
- **Rollback worst case**: ≤ 8 × physics cost per spike (24 ms, amortised much less). — source: ADR-0013
- **Input Update**: ≤ 0.05 ms/frame. — source: ADR-0011
- **Scene Entity lookup**: O(1) for component access; O(N) for `FindEntity` by name. — source: ADR-0019

---

## Feature Layer Rules

*Applies to: AI, scripting, editor tooling*

### Required Patterns

- **AI module is synchronous, zero engine dependencies** — links only GXLib_Foundation. Callers bridge to ECS/JobSystem. — source: ADR-0018
- **BehaviorTree ticks per-frame via `BT::Tick(dt)`** — Running nodes resume at suspended child index. — source: ADR-0018
- **Lua scripts run sandboxed** — no `os.execute`, `io.open` arbitrary paths, `package.loadlib`, `dofile`, `loadfile`. `gxlib.io` path-restricted to script asset root. — source: ADR-0005
- **Lua hot reload uses generation-counted callbacks** — C++-held Lua references must not be invalidated. — source: ADR-0005
- **C++ types are Lua-visible only via explicit `sol::usertype<T>`** — `GraphicsDevice`, archetype internals, `detail::` types are never bound. — source: ADR-0005
- **NodeGraph peers Lua** — same gameplay-authority rules apply; neither bypasses native systems. — source: ADR-0015
- **Reflection macros live in `.cpp` files only** — `GX_REFLECT_BEGIN` in headers violates ODR. — source: ADR-0015
- **GX_EDITOR=OFF excludes all editor code** — no `#include "Editor/..."` from runtime code. CI must build both configurations. — source: ADR-0015

### Forbidden Approaches

- **Never put `GX_REFLECT_*` macros in a header** (`reflection_macro_in_header`) — duplicate static initialisers across TUs. — source: ADR-0015
- **Never `#include "Editor/..."` from runtime modules** (`editor_included_from_runtime`) — breaks GX_EDITOR=OFF builds. — source: ADR-0015
- **Never use RVO with Y-dependent avoidance** (`rvo_with_y_dependent_avoidance`) — RVO operates XZ only; filter agents by floor for multi-storey. — source: ADR-0018
- **Never assume `MoviePlayer` plays audio** (`movie_audio_assumption`) — video only; use AudioManager separately. — source: ADR-0020
- **Never call `MFStartup`/`MFShutdown` globally** (`mf_global_init`) — each MoviePlayer manages its own MF session. — source: ADR-0020

### Performance Guardrails

- **Lua budget**: ≤ 1 ms/frame total across all scripts. Trivial call ≤ 1 μs. ECS query ≤ 20 μs. — source: ADR-0005
- **NavMesh FindPath**: ≤ 0.1 ms for 200×200 grid. NavMesh3D ≤ 0.5 ms for 100³. — source: ADR-0018
- **GOAP MakePlan**: ≤ 0.1 ms for 20 actions; MaxIterations=1000 caps search. — source: ADR-0018

---

## Presentation Layer Rules

*Applies to: rendering, audio, UI, VFX, animations*

### Required Patterns

- **DX12 Feature Level 12_1 minimum** — 12_2 where available. Caps-gated feature fallback. — source: ADR-0002
- **FrameGraph drives the render pipeline** — all passes declared; automatic resource transitions. Parallel command-list recording on workers. — source: ADR-0008
- **Deferred for opaque PBR; Forward+ for transparent/stylised** — same clustered light list for both paths. — source: ADR-0008
- **PostFX chain runs on HDR float16** — canonical order; opt-in per camera via `PostFXMask`. — source: ADR-0008
- **Custom shader models use RegisterCustomShaderModel(6-254)** — IDs 0-5 are built-in; 255 reserved. — source: ADR-0008
- **XAudio2 is the sole audio backend** — bus-based mixer with 4 buses. DSP insert chain max 4 effects per bus. — source: ADR-0010
- **Audio mix runs on XAudio2-owned thread** — no heap allocation on audio callback path. Atomic params for cross-thread communication. — source: ADR-0010
- **UI: retained-mode widget tree for game UI; ImGui for editor** — input ownership: ImGui → UIContext → game code. — source: ADR-0012
- **UI renders at native resolution** — not scaled by DynamicResolution. — source: ADR-0012
- **Animation evaluation is deterministic** — fixed dt, no fast-math, deterministic IK seed. Binding for rollback. — source: ADR-0014
- **Animation runs concurrent with physics broadphase** — no shared writers. Pose mirror to ECS on main thread after parallel section. — source: ADR-0014
- **GPU skinning via ComputeSkinning** — runs as FrameGraph compute pass before GBuffer. — source: ADR-0014
- **Ragdoll handoff is atomic at frame boundary** — `Animator::EnableRagdoll`/`DisableRagdoll`. — source: ADR-0014

### Forbidden Approaches

- **Never leak DX12/DXGI types into public headers** — forward-declared opaque handles only. — source: ADR-0002
- **Never scale UI with DynamicResolution** (`dynamic_resolution_on_ui_targets`) — UI stays at native res. — source: ADR-0008
- **Never use wall-clock dt in animation** (`wall_clock_dt_in_animation_tick`) — fixed dt only for determinism. — source: ADR-0014
- **Never put Skeleton as ECS component** (`skeleton_as_ecs_component`) — Skeleton is an Asset, not POD. Use `AssetHandle<Skeleton>`. — source: ADR-0014
- **Never steal ImGui focus from a Widget during PIE play** (`direct_imgui_focus_steal`) — respect ADR-0012 §9 focus guard. — source: ADR-0012
- **Never mutate widgets off main thread** (`widget_mutation_off_main_thread`) — UIContext has no synchronisation. — source: ADR-0012
- **Never run private input-processing thread** (`private_input_thread`) — InputManager is the single input consumer. — source: ADR-0011
- **Never treat touch as first-class input** (`touch_as_first_class_input`) — stub only; not supported. — source: ADR-0011
- **Never allocate on the animation hot path** — no `new`/`delete` inside `Animator::Update`. Pre-allocate pose buffers. — source: ADR-0014

### Performance Guardrails

- **Rendering CPU**: command-list recording ≤ 1.5 ms (4 workers). Light cluster ≤ 0.3 ms. FrameGraph compile ≤ 0.1 ms. — source: ADR-0008
- **Rendering memory**: GBuffer at 1080p ≈ 40 MB; PostFX bounce ≈ 20 MB float16. — source: ADR-0008
- **Audio budget**: ≤ 1.5 ms/frame total. Mix + DSP ≤ 1.0 ms (XAudio2 thread). Occlusion ≤ 0.2 ms. — source: ADR-0010
- **Animation budget**: ≤ 1.0 ms wall for ≤ 20 Animators. LOD required beyond. — source: ADR-0014
- **UI budget**: ≤ 0.5 ms HUD; ≤ 2.0 ms complex menu (≤ 1000 widgets). — source: ADR-0012
- **ImGui (editor)**: ≤ 1.0 ms/frame for GXModelViewer panel set. GX_EDITOR=OFF eliminates. — source: ADR-0015

---

## Global Rules (All Layers)

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Namespace | `gx` | `gx::EventBus` |
| Classes | PascalCase | `Application`, `PlayerController` |
| Methods / Free Functions | PascalCase | `Initialize()`, `TakeDamage()` |
| Member Variables | `m_camelCase` | `m_window`, `m_running` |
| Local Variables / Struct Fields | camelCase | `title`, `width` |
| Signals/Events | PascalCase past tense callback | `OnHealthChanged` |
| Files | PascalCase matching class | `Application.h` / `.cpp` |
| Constants | UPPER_SNAKE_CASE | `MAX_HEALTH` |
| Doc Comments | Doxygen `///` on public APIs | Japanese acceptable |

### Performance Budgets

| Target | Value | Source |
|--------|-------|--------|
| Framerate | 60 fps | technical-preferences |
| Frame budget | 16.6 ms | technical-preferences |
| Main-thread serial | ≤ ~7.0 ms | ADR-0008 review |
| Worker parallel wall | ≤ ~3.0 ms | ADR-0009 review |
| Draw calls | DX12 indirect preferred | technical-preferences |
| Memory ceiling | TBD | technical-preferences |

### Approved Libraries / Addons

| Library | Purpose | Source |
|---------|---------|--------|
| DirectX 12 (Windows SDK) | Rendering backend | ADR-0002 |
| XAudio2 | Audio backend | ADR-0010 |
| Lua 5.4 + sol2 | Scripting | ADR-0005 |
| ImGui | Editor / debug UI | ADR-0012, ADR-0015 |
| Media Foundation | Video playback/recording | ADR-0020 |
| FBX / glTF loaders | Model import | technical-preferences |

### Two-Layer Accessibility Pillar (ADR-0017)

Every public API must satisfy both layers:

- **Layer 1 (Beginner)**: DXLib user can draw/play/input within minutes. No DX12/ECS/FrameGraph knowledge required. Sensible defaults. Discoverable errors (`GX_LOG_ERROR` on failure).
- **Layer 2 (Advanced)**: Core-modifiable extension points (custom shader, custom DSP, custom widget, custom asset type). Public, not `detail::` or `friend`-locked.
- **L1.1**: Every `gx::` Compat function has a working `examples/` sample.
- **L1.2**: `GX_Init` check must produce a human-readable error message.
- **L1.3**: Default `PostFXMask` is sane (all effects on).
- **L1.4**: Doxygen `///` on all public functions with examples.
- **Forbidden**: `internal_only_extension_point` — extension points must be public, not `detail::` / `*_internal.h` / `friend class`.
- **Forbidden**: `dx12_type_in_public_api` — Layer 1 surface must not leak DX12/DXGI types.
- **Forbidden**: `undocumented_compat_function` — every Compat function needs Doxygen.
- **Forbidden**: `layer2_breaks_layer1` — L2 modifications must not break L1 callers.

### DLL Boundary Limitations

Both `EventBus` (ADR-0016) and `TypeRegistry` (ADR-0015) use `std::type_index(typeid(T))`. On MSVC, `type_info` identity is per-module. **If GXLib is ever packaged as a DLL**, cross-module EventBus dispatch and Reflection lookup will silently fail. Migration to string-keyed or hash-keyed dispatch would be required. This is NOT a concern for the current static-library configuration.
