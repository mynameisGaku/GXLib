# GXLib Master Architecture

> **Synthesis document** — synthesises all 20 Accepted ADRs into a single coherent picture.
> For rationale and alternatives see the individual ADRs in `docs/architecture/adr-*.md`.
> **Last updated**: 2026-04-17 (covers ADR-0001 through ADR-0020)

---

## 1. Overview

GXLib is a self-hosted, Windows-only game engine SDK written in C++20. Its core
value proposition is a **DXLib-compatible procedural API** (`gx::` namespace, free
functions that mirror `DrawGraph`, `PlaySoundMem`, `GetJoypadInputState`, etc.) layered
on top of a **modern DirectX 12 backend** — giving developers who graduate from DXLib
access to HDR output, Variable Rate Shading, Mesh Shaders, Sampler Feedback,
DirectStorage, GGPO-style rollback netcode, an archetype ECS, a Job System, GPU
particles, and a complete physics + animation + audio + AI stack, all without
abandoning the procedural authoring style they already know. The engine is not a
game; it is the SDK itself, and the authoritative reference is always the source
under `GXLib/` and the changelog in `CHANGELOG.md`.

---

## 2. Technology Stack

| Area | Choice | ADR |
|------|--------|-----|
| Language | C++20 (MSVC + Clang-cl, Windows) | ADR-0001 |
| Rendering backend | DirectX 12, Feature Level 12_1 min / 12_2 preferred | ADR-0002 |
| Public API style | DXLib-compatible procedural (`gx::` namespace) + `gx::App` class API | ADR-0003 |
| Audio | XAudio2 (bus-based mixer, 3D spatial, DSP, OGG streaming) | ADR-0010 |
| Scripting | Lua 5.4 + sol2 (sandboxed, hot-reloadable) | ADR-0005 |
| Physics | Custom in-house (GJK/EPA, constraints, cloth, ragdoll, character controller) | ADR-0009 |
| Networking | Custom reliable UDP, property replication, GGPO rollback | ADR-0013 |
| Asset pipeline | Hash-keyed AssetDatabase, DirectStorage + fallback, hot reload | ADR-0007 |
| Concurrency | Work-stealing JobSystem (singleton), no per-subsystem thread pools | ADR-0006 |
| Shaders | HLSL (DXC, SM 6.0 baseline; SM 6.5 for Mesh Shaders / DXR) | ADR-0002 |
| Scripting UI | Lua text scripts + visual NodeGraph (peer surfaces, same authority) | ADR-0005, ADR-0015 |
| Editor | GXLib_Editor static lib (GX_EDITOR=ON/OFF CMake flag) | ADR-0015 |
| Media | Windows Media Foundation (video playback + recording) | ADR-0020 |

---

## 3. Architecture Layers

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ PRESENTATION LAYER                                                           │
│  Graphics (FrameGraph · Deferred · Forward+ · HDR PostFX · GPU Particles)   │
│  Audio (XAudio2 mixer · 3D spatial · DSP · OGG · Reverb · Snapshots)       │
│  GUI (retained widget tree · ImGui · XML/CSS · DataBinding · UITween)       │
│  Animation (Skeleton · Blend Tree · IK · Motion Matching · GPU Skinning)    │
│  Movie (Media Foundation playback · VideoRecorder)                          │
├──────────────────────────────────────────────────────────────────────────────┤
│ FEATURE LAYER                                                                │
│  AI (BehaviorTree · GOAP · NavMesh × 3 · RVO · NavAgent)                   │
│  Networking (ReliableUDP · Replication · Prediction · Rollback · NAT)       │
│  Script (Lua 5.4 + sol2 · hot reload · sandboxed surface)                  │
│  Editor (PIE · Reflection · UndoSystem · NodeGraph · Panels)                │
│  Scene (Entity/Component · SceneManager · Persistence · Prefabs · Streaming)│
├──────────────────────────────────────────────────────────────────────────────┤
│ CORE LAYER                                                                   │
│  ECS (Archetype World · Query · System · EntityBridge OOP↔ECS)              │
│  Physics (PhysicsWorld2D/3D · GJK/EPA · Cloth · Ragdoll · CharacterCtrl)   │
│  Input (InputManager · Keyboard · Mouse · Gamepad/XInput · IME/IMM32)      │
│  EventBus (type-keyed pub/sub · Idempotent/SideEffect · replay-suppression) │
│  AssetDatabase (hash-keyed cache · provider chain · hot reload · DirectStorage)│
│  JobSystem (work-stealing · DAG deps · SubmitMainThread · ParallelFor)      │
├──────────────────────────────────────────────────────────────────────────────┤
│ FOUNDATION LAYER                                                             │
│  Math (Vector2/3/4 · Quaternion · Matrix4x4 · Tween · Random)              │
│  Container (PCH-free custom containers: Vector, HashMap, VectorMap …)       │
│  IO (FileWatcher · DirectStorage · Archive · Pak · BundleManager)           │
│  Compat (DXLib-compatible procedural API in gx:: namespace)                 │
│  GX (gx::App class-style facade)                                            │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Subsystem Map

| Subsystem | ADR | Module path | Description |
|-----------|-----|-------------|-------------|
| Documentation strategy | ADR-0001 | `docs/architecture/` | ADR-only docs; no GDDs; tr-registry.yaml for requirements |
| DX12 Backend | ADR-0002 | `GXLib/Graphics/Device/` | GraphicsDevice singleton; caps-gated HDR/VRS/Mesh Shader/DS |
| DXLib Compat API | ADR-0003 | `GXLib/Compat/` | Procedural `gx::` free functions mirroring DXLib signatures |
| ECS (archetype) | ADR-0004 | `GXLib/ECS/` | World, Archetype, Query, System, EntityBridge; 100k entity target |
| Lua scripting | ADR-0005 | `GXLib/Script/` | sol2 bindings, sandboxed surface, hot reload, VisualScript |
| Job System | ADR-0006 | `GXLib/Core/JobSystem.h` | Work-stealing, DAG deps, priority tiers, SubmitMainThread |
| Asset Database | ADR-0007 | `GXLib/Core/AssetDatabase.*`, `GXLib/IO/` | Hash-keyed cache, IFileProvider chain, hot reload, DirectStorage |
| Rendering pipeline | ADR-0008 | `GXLib/Graphics/{FrameGraph,Pipeline,PostEffect,3D}` | Deferred + Forward+, FrameGraph, HDR PostFX, DynRes |
| Physics | ADR-0009 | `GXLib/Physics/` | Custom 2D/3D worlds; GJK/EPA; cloth; ragdoll; CharacterController |
| Audio | ADR-0010 | `GXLib/Audio/` | XAudio2 bus mixer, 3D spatial, DSP chain, OGG, occlusion |
| Input | ADR-0011 | `GXLib/Input/` | InputManager per Window; Keyboard/Mouse/Gamepad/ActionMapping/IME |
| GUI | ADR-0012 | `GXLib/GUI/`, `GXLib/Compat/ImGuiManager.*` | Retained widget tree + ImGui coexistence; XML/CSS hot reload |
| Networking | ADR-0013 | `GXLib/IO/Network/` | ReliableUDP, replication, rollback, NAT, matchmaking, CloudSave |
| Animation pipeline | ADR-0014 | `GXLib/Graphics/3D/{Skeleton,Animator,…}` | Blend tree, state machine, IK, motion matching, GPU skinning |
| Editor | ADR-0015 | `GXLib/Editor/`, `GXLib/Core/{UndoSystem,NodeGraph,Reflect}` | PIE, reflection, undo, panels; `GX_EDITOR=ON/OFF` |
| EventBus | ADR-0016 | `GXLib/Core/EventBus.h` | Type-keyed singleton pub/sub; HandlerCategory; replay-suppression |
| Two-Layer Pillar | ADR-0017 | (architectural pillar) | L1 beginner-usable + L2 core-modifiable; 9-point PR checklist |
| AI | ADR-0018 | `GXLib/AI/` | BehaviorTree, GOAP, NavMesh × 3, RVO; Foundation-only deps |
| Scene | ADR-0019 | `GXLib/Core/Scene/`, `GXLib/Graphics/3D/SceneRenderer.*` | Entity/Component, SceneManager, persistence, prefabs, streaming |
| Movie pipeline | ADR-0020 | `GXLib/Movie/`, `GXLib/Graphics/VideoRecorder.*` | MF playback (video-only) + MP4 recording; no built-in audio sync |

---

## 5. Data Flow — One Frame

```
Win32 message pump
      │
      ▼
InputManager::Update()          ← keyboard / mouse / XInput / IME (main thread)
      │
      ▼
Game Logic / ECS Systems        ← SetParameter on Animator; script Tick; AI Tick
      │
      ▼
Animation tick (fixed dt)       ← StateMachine → BlendTree → IK → SpringBone
      │ RootMotion delta         └─ AnimationEventDispatcher → EventBus
      ▼
Physics step (fixed dt)         ← broadphase (workers) → narrow-phase (workers)
  IslandSolve (workers)            → integrate → mirror Transforms → ECS
      │
      ▼
NetworkReplicator::Tick()       ← diff ReplicatedProperty<T> → ReliableChannel
  (+ rollback re-simulation if needed; EventBus in replay-suppression mode)
      │
      ▼
EventBus::DispatchQueued()      ← flush worker-queued events (main thread)
      │
      ▼
FrameGraph compile + record     ← shadow passes → ComputeSkinning → GBuffer
  (workers parallel)               → Deferred shade → Forward+ → PostFX chain
      │                            → ToneMap/OutputTransform → UI → ImGui → Present
      ▼
JobSystem::ProcessMainThreadJobs() + ID3D12CommandQueue submit (main thread)
      │
      ▼
Present (SwapChain, main thread only)
```

---

## 6. Threading Model

GXLib uses a **single worker pool** provided by `gx::JobSystem`. No subsystem owns a private
thread pool; all parallel work flows through `JobSystem::Submit` / `SubmitAfter` / `ParallelFor`.

| Thread | What runs there |
|--------|----------------|
| **Main thread** | InputManager::Update, game logic, ECS command-buffer flush, AnimationEventDispatcher, EventBus Fire/Subscribe/Unsubscribe/DispatchQueued, NetworkManager API, UIContext::Update/Render, SceneManager, DX12 command-queue submission, Present, ProcessMainThreadJobs drain |
| **Worker threads** (N = hw_concurrency − 1) | Blend-tree evaluation, IK solves, motion matching search, physics broadphase refit + narrow-phase + island solve, FrameGraph command-list recording per pass, replication serialisation + snapshot build, asset async load / deserialization, OGG decode (Low priority), AudioOcclusion raycasts |
| **XAudio2 OS thread** (XAudio2-owned) | Mix buses, DSP insert chains, voice output — no heap allocation permitted on this path |
| **Winsock OS I/O threads** (OS-owned) | Socket send/recv queues — GXLib does not manage their lifetime |

**Key contracts:**
- `SubmitMainThread(fn)` is the only safe path for DX12 swap-chain calls and Lua state touches from workers.
- `EventBus::QueueFromWorker<T>` is the only sanctioned worker-to-main event route.
- Physics queries (`Raycast`, `OverlapSphere`) may be called concurrently with `PhysicsWorld::Step` via a versioned copy-on-write broadphase read view (ADR-0009 §14).
- Animation evaluation and physics broadphase run concurrently on workers (no shared writers).

---

## 7. Frame Budget (60 fps / 16.6 ms)

| Phase | Thread | Budget |
|-------|--------|--------|
| Input + game logic | Main | ~1.0 ms |
| Animation tick (≤ 20 Animators) | Workers (wall-clock) | ≤ 1.0 ms |
| Physics step (1000 bodies) | Workers (wall-clock) | ≤ 3.0 ms |
| Network replication tick (1000 entities) | Workers | ≤ 0.5 ms |
| EventBus dispatch + misc main-thread work | Main | ~0.5 ms |
| FrameGraph compile | Main | ≤ 0.1 ms |
| Command-list recording (4 workers) | Workers (wall-clock) | ≤ 1.5 ms |
| Light cluster build | Main/Worker | ≤ 0.3 ms |
| JobSystem scheduling overhead | — | ≤ 0.2 ms |
| Audio (XAudio2 thread) | XAudio2 | ≤ 1.5 ms |
| UI (Update + Render) | Main | ≤ 0.5 ms HUD / ≤ 2.0 ms menu |
| GPU frame (GBuffer + shade + PostFX) | GPU | ~13 ms target |
| **Total main-thread serial** | Main | **≤ ~7 ms** |

Rollback worst case: ≤ 8 × physics cost per spike (≤ 24 ms amortised spike, not per-frame).

---

## 8. Cross-Cutting Concerns

### EventBus
Single `gx::EventBus::Instance()` is the cross-system notification spine. Type-index routing
(`std::type_index(typeid(T))`). Handlers declare `HandlerCategory { Idempotent, SideEffect }`;
default is `SideEffect` (conservative). During rollback re-simulation `SetReplayMode(true)`
suppresses all `SideEffect` handlers so audio/VFX/network side effects do not duplicate. Worker
threads use `QueueFromWorker<T>`; direct `Fire<T>` from workers is forbidden.

### AssetDatabase
Central cache keyed by 64-bit FNV-1a `AssetId`. All file access routes through the
`IFileProvider` chain (Physical → Archive → Pak → Bundle). Consumers hold `AssetHandle<T>`
(generation-counted); hot reload flows `FileWatcher → AssetReloader → per-type rebinder`.
GPU-backed assets enter a ≥3-frame deferred-release quarantine on last ref.

### Reflection
Macro-based (`GX_REFLECT_BEGIN/END`), anonymous-namespace registrar, lives in `GXLib/Core/Reflect/`.
Available in runtime builds (not editor-exclusive). Drives `JsonSerializer` for save/load.
Macros must not appear in headers (ODR violation).

### Two-Layer Accessibility Pillar (ADR-0017)
Every public API must satisfy **both**:
- **Layer 1** (beginner): DXLib-shaped `gx::` procedural call exists, sensible defaults,
  int return codes with Logger error on failure, Doxygen with example.
- **Layer 2** (core-modifiable): extension points are public headers (never `detail::`),
  at least one in-repo example, Doxygen on the override contract, modification requires
  no engine recompile.

The pillar is enforced via 4 forbidden patterns and a 9-item per-PR review checklist in `docs/CLAUDE.md`.

---

## 9. Key Constraints

| Constraint | Origin | Detail |
|------------|--------|--------|
| **Windows-only** | ADR-0002 | DX12, XAudio2, Winsock2, IMM32 — no cross-platform abstraction |
| **No DLL boundary** for EventBus / Reflection | ADR-0016, ADR-0015 | `std::type_index` is per-module on MSVC; cross-module dispatch fails silently. GXLib must remain a static library configuration. |
| **Fixed timestep for physics + rollback** | ADR-0009, ADR-0013 | `dt = 1/60` s default; variable timestep in the solver is forbidden. Rollback re-simulation re-runs the full per-frame schedule (Input → Animation → Physics). |
| **No raw `std::thread`** | ADR-0006 | All worker parallelism goes through `JobSystem::Submit`. Forbidden pattern: spawning threads in subsystem code. |
| **All asset access through AssetDatabase** | ADR-0007 | `fopen`/`CreateFile` directly in subsystem code is forbidden. |
| **Fast-math disabled in physics / animation / rollback TUs** | ADR-0009, ADR-0013, ADR-0014 | `/fp:fast` breaks rollback determinism via FP reduction-order changes. |
| **No nondeterministic iteration in gameplay** | ADR-0013 | `std::unordered_*` / `gx::HashMap` iteration in gameplay logic is forbidden; use `gx::VectorMap` or archetype queries. |
| **DX12 types must not leak into public headers** | ADR-0002, ADR-0003 | CI grep gate: `<d3d12.h>` / `<dxgi*.h>` must not appear in `GXLib/Compat/*.h`. |
| **Editor excluded from shipping builds** | ADR-0015 | `GX_EDITOR=OFF` drops `GXLib_Editor` from the link; no `#include "Editor/..."` from runtime modules. |
| **Lua sandbox** | ADR-0005 | `os.execute`, `io.open`, `package.loadlib`, `dofile`, `loadfile` removed from every `sol::state`; `gxlib.io` is path-restricted. |
| **`MoviePlayer` is video-only** | ADR-0020 | Audio track is ignored; callers manage audio sync via AudioManager separately. |
| **Physics mutations are main-thread-only** | ADR-0009 | `AddBody`, `RemoveBody`, `ApplyForce` are main-thread or command-buffered; read queries are concurrent-safe. |

---

## 10. ADR Index

| # | Title | Status |
|---|-------|--------|
| ADR-0001 | Documentation Strategy for GXLib (SDK, ADR-Only, No GDDs) | Accepted |
| ADR-0002 | DirectX 12 Rendering Backend | Accepted |
| ADR-0003 | DXLib-Compatible Procedural API Layer | Accepted |
| ADR-0004 | Archetype-Based Entity Component System | Accepted |
| ADR-0005 | Lua Scripting Boundary (Lua 5.4 + sol2) | Accepted |
| ADR-0006 | Job System — Multi-threaded Task Scheduler | Accepted |
| ADR-0007 | Asset Database + Hot Reload Pipeline | Accepted |
| ADR-0008 | Rendering Pipeline (Deferred + Forward+ Hybrid, FrameGraph, PostFX, HDR) | Accepted |
| ADR-0009 | Physics Architecture (Custom In-House, 2D + 3D) | Accepted |
| ADR-0010 | Audio Architecture (XAudio2 Backend, Bus-Based Mixer, 3D Spatial + DSP) | Accepted |
| ADR-0011 | Input Architecture (InputManager + Per-Device Classes + ActionMapping + IMM32 IME) | Accepted |
| ADR-0012 | GUI Architecture (Retained-Mode Widget Tree + ImGui Coexistence + XML/CSS Layout) | Accepted |
| ADR-0013 | Networking Architecture (Reliable UDP, Property Replication, Prediction + Rollback, NAT) | Accepted |
| ADR-0014 | Animation Pipeline (Skeleton + Animator State Machine + Blend Tree + IK Suite + Motion Matching + GPU Skinning) | Accepted |
| ADR-0015 | Editor Architecture (Play-in-Editor, Reflection, Undo/Redo, Node Graph, Panels) | Accepted |
| ADR-0016 | EventBus / Cross-System Communication (Type-Safe Pub/Sub with Replay-Suppression Contract) | Accepted |
| ADR-0017 | Two-Layer Accessibility Pillar (Beginner-Usable + Core-Modifiable) | Accepted |
| ADR-0018 | AI Architecture (Behavior Tree, GOAP, NavMesh, RVO) | Accepted |
| ADR-0019 | Scene Architecture (Entity/Component, SceneManager, Persistence, Prefabs, Streaming) | Accepted |
| ADR-0020 | Movie Pipeline (Video Playback via Media Foundation + Video Recording) | Accepted |
