# ADR-0015: Editor Architecture (Play-in-Editor, Reflection, Undo/Redo, Node Graph, Panels)

## Status
Accepted

## Date
2026-04-16

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Editor / Tooling |
| **Knowledge Risk** | LOW — Play-in-Editor snapshot/restore, Command-pattern undo stack, macro-registered reflection, ImGui-hosted panels, and data-driven node-graph execution are all standard editor-tooling patterns inside LLM training data |
| **References Consulted** | `GXLib/Editor/{PlayInEditor,Gizmo,EntityPicker,PropertyInspector,SceneHierarchyPanel,AssetBrowser,ConsoleWindow,ShaderGraph,TerrainSculptor,TimelineEditor}.{h,cpp}` (current editor module), `GXLib/Core/UndoSystem.{h,cpp}` (ICommand + ValueCommand + stack, shared between engine and editor), `GXLib/Core/NodeGraph.{h,cpp}` (visual scripting runtime), `GXLib/Core/Reflect/{TypeInfo,TypeRegistry,PropertyMeta,ReflectMacros,JsonSerializer}.{h,cpp}` (reflection + JSON (de)serialisation), `GXLib/CMakeLists.txt` lines 189-197 (GXLib_Editor static lib — separately linkable, depends on GXLib_Graphics + GXLib_Scene + GXLib_Core, includes bundled ImGui), `GXModelViewer/` (reference host application — demonstrates how an external tool embeds GXLib_Editor), CHANGELOG Phases 1-3 (NodeGraph + Reflection + PIE + Undo/Redo) |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | PIE state-machine transitions are correct under all sequences (Stop→Play→Pause→Resume→Stop, Stop→Play→Stop, Stop→Play→Pause→Step→Resume→Stop); PIE snapshot restore returns entity transform/active state to pre-Play values; UndoSystem single-undo/redo consistency with ValueCommand chain; Reflection registration idempotent across TU include order; NodeGraph execution respects flow-pin order; JSON round-trip via reflection preserves all property values; shipping build with `GX_EDITOR=OFF` excludes all Editor symbols from link output; Editor panels do not steal focus from game input unless explicitly enabled (ADR-0012 §9 reaffirmation) |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0004 (ECS — SceneHierarchyPanel and EntityPicker navigate ECS entities via opaque EntityHandle), ADR-0005 (Lua scripting boundary — NodeGraph is a peer visual-scripting surface, not a replacement; both drive ICommand submissions), ADR-0007 (Asset Database / Hot Reload — AssetBrowser is the editor face of the same database; hot-reload events drive panel refresh), ADR-0008 (Rendering — Gizmo is a rendering pass; ShaderGraph authors materials that ADR-0008 Standard/Unlit/PBR consumes), ADR-0012 (GUI — editor panels render inside the same ImGui host as debug GUI; input routing uses ADR-0012 §9 steal-focus-guard), ADR-0014 (Animation — TimelineEditor edits AnimationClips defined there) |
| **Enables** | Future dedicated content-pipeline tools, in-editor playtest recording, live-tuning overlays, remote editor (networked PropertyInspector), editor plug-in surface for game-specific panels |
| **Blocks** | None (code already exists — retroactive) |
| **Ordering Note** | Low priority retroactive ADR. ADR-0017 Two-Layer Accessibility pillar classifies the Editor module at L2 ("core-modifiable, advanced"); it is not part of the beginner `gx::` Compat layer. Shipping games disable the editor at compile time. |

## Context

### Problem Statement

GXLib ships with a substantial tooling layer — Play-in-Editor (snapshot-restore), command-pattern undo/redo, macro-based runtime reflection, a visual-scripting node-graph runtime, a JSON serialiser over reflection, and ImGui-hosted panels (entity picker, property inspector, scene hierarchy, asset browser, console, gizmo, shader graph, terrain sculptor, timeline editor). All of this is statically linked today (`GXLib_Editor` static library), always built, depends on `GXLib_Graphics + GXLib_Scene + GXLib_Core`, and includes bundled ImGui.

No ADR has ever codified where "editor" ends and "runtime" begins, what the shipping-build exclusion model is, how Undo/Redo interacts with the rest of the engine, how reflection and node-graph relate to Lua scripting (ADR-0005), or how editor panel input coexists with ADR-0012 GUI focus rules. This ADR closes that gap retroactively, names the boundary, and documents the shipping-build opt-out.

### Constraints

- The editor is NOT part of the shipped game runtime. Shipping game binaries MUST be able to exclude `GXLib_Editor` completely (link-time, not just runtime-gated) — `GX_EDITOR=OFF` build flag drops the static lib from the link line.
- ImGui is an editor-side dependency. The beginner `gx::` Compat layer (ADR-0017 L1) does not expose ImGui; the editor and the debug-only GUI overlays are L2.
- The engine runtime may use `UndoSystem` and `Reflection` standalone (they live in `GXLib/Core/` not `GXLib/Editor/`) — those are NOT editor-exclusive. The editor module ADDS editor-only panels on top.
- Undo/Redo must respect the ADR-0009 physics determinism boundary: undoing a command during a rollback re-simulation window is undefined behaviour. Editor is a PIE-paused-world tool; rollback is a live-play tool; they don't operate simultaneously.
- PIE snapshot captures the serialisable portion of scene state (transform, active flag, entity name). It does NOT capture non-replicated runtime state (particle emitters mid-run, audio voices, network sessions) — those reset when play exits.
- Reflection macros (`GX_REFLECT_BEGIN` … `GX_REFLECT_END`) must be include-order-independent. Two TUs defining the same struct's reflection is a duplicate-registration error and the compiler must catch it (anonymous-namespace registrar in the current implementation achieves this).
- NodeGraph and Lua (ADR-0005) are peer scripting surfaces with the same gameplay-authority boundary; neither can bypass the native systems they drive.
- Gizmo rendering is a separate render pass (additive, depth-tested against scene depth) inside the ADR-0008 FrameGraph; it MUST NOT write to gameplay-visible render targets in shipping builds (excluded via `GX_EDITOR=OFF`).

### Requirements

- **Play-in-Editor (PIE):**
  - `PIEState { Stopped, Playing, Paused }` with `EnterPlayMode / ExitPlayMode / Pause / Resume / StepFrame / Update`.
  - Snapshot + restore of scene entity transform/active/name on Stop.
  - Frame stepping in Paused state.
  - `OnEnterPlay` / `OnExitPlay` callbacks for host integration.
  - Config flags: `startPaused`, `captureInput`, `showDebugOverlay`, `simulatePhysics`, `simulateAI`, `simulateAudio`, `fixedTimeStep`.
- **Undo/Redo:**
  - `ICommand` interface with `Execute() / Undo() / GetDescription()`.
  - `ValueCommand<T>` template for property-change commands.
  - `UndoSystem` with push-on-execute stack, redo stack, `Clear`, `CanUndo / CanRedo`, top-of-stack description for UI.
  - Stack limit (default unbounded; host can cap).
- **Reflection:**
  - Macro-based registration (`GX_REFLECT_BEGIN / GX_REFLECT_FLOAT / GX_REFLECT_INT / GX_REFLECT_BOOL / GX_REFLECT_STRING / GX_REFLECT_FLOAT_RANGE / GX_REFLECT_END`).
  - `TypeInfo` holds factory + `Vector<PropertyMeta>` (name, type, offset, size, optional min/max).
  - `TypeRegistry` singleton (one registration per type; anonymous-namespace registrar pattern).
  - `PropertyMeta::type ∈ {Bool, Int, Float, String, ...}`.
  - `JsonSerializer` round-trips registered types.
- **NodeGraph (visual scripting runtime):**
  - `PinType { Flow, Bool, Int, Float, String, Vector3 }`.
  - `NodeDef` (name, input pins, output pins, execute lambda).
  - `NodeInstance` (per-node runtime state).
  - `NodeGraph` (instances + connections + execute-from-entry).
  - Flow pins sequence execution; data pins propagate values.
- **Editor Panels (ImGui-hosted):**
  - SceneHierarchyPanel, EntityPicker, PropertyInspector, AssetBrowser, ConsoleWindow, Gizmo, ShaderGraph, TerrainSculptor, TimelineEditor.
  - Each panel is a class with `Update(float dt)` and/or ImGui-draw methods; hosted by an embedding application (like `GXModelViewer`).
- **Shipping-build exclusion:**
  - `GX_EDITOR` CMake option (default ON for tools / GXModelViewer, must be switchable to OFF for games).
  - When OFF, `GXLib_Editor` is not added to the build; `#include "Editor/..."` fails to compile (acceptable — games should not depend on editor headers).
  - `GXLib_Core::UndoSystem`, `GXLib_Core::NodeGraph`, `GXLib_Core::Reflect::*` remain available in runtime builds (they're not editor-exclusive).

## Decision

**GXLib's editor is a statically-linkable optional module (`GXLib_Editor`) containing ImGui-hosted panels, PIE control, gizmo rendering, shader graph, timeline, and terrain tools. It sits on a Core-layer foundation of Reflection, UndoSystem, and NodeGraph — those are NOT editor-exclusive and remain available in runtime-only builds. The editor module depends on Graphics + Scene + Core + bundled ImGui. Shipping games exclude the editor via `GX_EDITOR=OFF` at CMake time; the build drops the static lib from the link line. PIE snapshots serialisable scene state only; non-replicated runtime state resets on Stop. Undo/Redo is main-thread-only and must not execute inside a rollback re-simulation window. Reflection uses macro-based registration with an anonymous-namespace registrar for uniqueness. NodeGraph is a peer to Lua (ADR-0005) — same gameplay-authority rules; neither bypasses native systems.**

Concrete rules:

1. **Layering.**
   ```
   Editor Panels                   ImGui-hosted UI (GXLib/Editor/*Panel*, Gizmo, ShaderGraph, TerrainSculptor, TimelineEditor)
   Editor Services                 PlayInEditor, EntityPicker
   Core Foundation (NOT editor)    UndoSystem, NodeGraph, Reflect::{TypeInfo, TypeRegistry, PropertyMeta, JsonSerializer}
   ```
   The bottom tier is in `GXLib/Core/` — available in runtime builds. The top two tiers are in `GXLib/Editor/` — gated by `GX_EDITOR`.

2. **Editor vs runtime boundary.**
   - `GXLib/Editor/*` → `GXLib_Editor` static lib → excluded when `GX_EDITOR=OFF`.
   - `GXLib/Core/{UndoSystem,NodeGraph,Reflect/*}` → `GXLib_Core` → always built.
   - Rationale: games want runtime reflection (for save/load via JsonSerializer) and runtime UndoSystem (for in-game "undo last turn" features) and runtime NodeGraph (for ability trees, quest graphs) — those are Core, not Editor.

3. **Play-in-Editor (PIE).**
   - Single state machine: `Stopped → Playing → Paused → Playing → Stopped`.
   - `EnterPlayMode` captures a snapshot (entity transform + active + name) BEFORE entering Playing. `ExitPlayMode` restores from the snapshot. This is intentional shallow-snapshot; see (4).
   - `Pause` / `Resume` flip Playing ↔ Paused without touching the snapshot.
   - `StepFrame` advances one `fixedTimeStep` while Paused — calls `Scene::Update(fixedTimeStep)`; does NOT re-enter Playing.
   - `Update(dt)` advances only when Playing.
   - Callbacks: `OnEnterPlay` / `OnExitPlay` let the host re-sync ImGui window state, pause/resume audio, etc.
   - Thread: PIE control is main-thread-only.

4. **PIE snapshot scope.**
   - Snapshot captures: entity id, name, active flag, transform (position/rotation/scale).
   - Snapshot does NOT capture: particle emitter state, audio voice state, network sessions, ReliableChannel buffers, live physics body velocity/angular-velocity (only pose), Lua interpreter state, NodeGraph data pins, ECS component field values beyond transform.
   - Rationale: a full snapshot would require serialising the entire runtime world, which is what save-game is for — and ADR-0007 owns that. PIE is "quick play test," not "save slot." Authors who need wider snapshot coverage implement `Scene::SerializeFull/RestoreFull` separately (future ADR).
   - Consequence: exiting play mode leaves the world at the pre-play pose but particles, audio, and other non-replicated state reset. This is documented behaviour, not a bug.

5. **Undo/Redo.**
   - `ICommand::Execute` runs the action. `ICommand::Undo` reverses it. `GetDescription()` names it (shown in Edit > Undo menu).
   - `UndoSystem::Execute(unique_ptr<ICommand>)` runs and pushes onto the undo stack. Redo stack is cleared on any new Execute.
   - `ValueCommand<T>` wraps a setter lambda + old/new values — the 80% case for property edits in PropertyInspector.
   - Stack is main-thread-only. Host applications call `Execute / Undo / Redo` in response to UI input.
   - **Forbidden**: calling `Undo / Redo` while `EventBus::IsReplayMode()` is true (ADR-0016) — undoing a command during rollback re-simulation corrupts gameplay state. Enforced by debug assertion (`undo_during_rollback_replay` forbidden pattern).
   - Stack size: unbounded by default; host can cap by calling `Clear` on entering PIE Play mode (common convention — undo stack is meaningless across Play boundaries).

6. **Reflection.**
   - Registration via `GX_REFLECT_BEGIN(Type) … GX_REFLECT_<kind>(name, member) … GX_REFLECT_END(Type)` macros.
   - Macros expand to an anonymous-namespace `Type##_Registrar` whose constructor calls `gx::TypeRegistry::Register(typeInfo)`. Anonymous namespace gives TU-local uniqueness; static initialisation runs once per process.
   - `TypeInfo` owns `Vector<PropertyMeta>` (name, type enum, byte offset, size, optional range metadata).
   - `TypeRegistry::Get<T>()` returns `const TypeInfo*` or nullptr.
   - **Forbidden**: declaring `GX_REFLECT_BEGIN(T) … END(T)` in a HEADER (violates ODR — every including TU re-registers). Reflection declarations live in a single `.cpp` per type (`type_reflection.cpp` convention).
   - PropertyMeta offset + size enable reflection-driven generic property UI (PropertyInspector reads the type's PropertyMetas and draws widgets) and reflection-driven (de)serialisation (JsonSerializer).

7. **JsonSerializer.**
   - Lives in `Core/Reflect/JsonSerializer.{h,cpp}`. Writes/reads a `TypeInfo`-registered struct as JSON using the PropertyMeta offsets.
   - NOT a general-purpose JSON library — only round-trips registered types.
   - Used by: save-game systems (if the game opts into reflection-driven save), PIE extended snapshot (future), editor's "Save As" for scene authoring.
   - Keys = property name; values = type-specific JSON primitive.

8. **NodeGraph.**
   - `NodeDef` is the template: named, typed pins (in/out), an `Execute(NodeInstance&)` lambda.
   - `NodeInstance` is an instantiated node — holds current pin values and per-instance state.
   - `NodeGraph` holds all instances and connections; `Execute(entryNode)` walks flow pins in order, propagating data pin values through connections.
   - Peer to Lua (ADR-0005) — both are scripting surfaces. A node graph's Execute lambda is native (C++) — it can CALL Lua if the game chooses, but NodeGraph itself is not a Lua VM and does not depend on sol2.
   - Gameplay authority: a node graph's lambdas respect the same authority boundary as Lua — they cannot bypass physics, networking, or ECS via side channels. They use the same sanctioned APIs.
   - Editor counterpart (`GXLib/Editor/ShaderGraph.cpp`) is an ImGui-based authoring surface that emits a `NodeGraph`. The runtime is Core; the authoring UI is Editor.

9. **Editor Panels.**
   - Each panel is a self-contained class in `GXLib/Editor/`. Common contract: `Update(float dt)` (non-draw logic) and `DrawImGui()` (or equivalent; sometimes fused into `Update`).
   - Panels hold **pointers** to engine state (Scene*, EntityPicker*, etc.) — they do not own engine state.
   - Panels use the UndoSystem for every mutating action. A click in PropertyInspector that changes a float value produces a `ValueCommand<float>` pushed onto the host's UndoSystem.
   - Panels respect ADR-0012 §9 focus-steal guard — when the user is playing a PIE session with `captureInput=true`, editor panels do NOT capture keyboard/mouse unless the user explicitly clicks them.
   - Panels are visible only when their host chooses to render them. The editor module does not own a presentation loop — `GXModelViewer` (or another host) is the driver.

10. **ImGui dependency.**
    - Bundled in `GXLib/ThirdParty/imgui/` and linked into `GXLib_Editor`.
    - The `GX_EDITOR=ON` build configures the ImGui backend (DX12 + Win32) at engine startup. `GX_EDITOR=OFF` skips ImGui entirely — smaller binary, no ImGui draw passes in the frame graph.
    - Games that want a debug ImGui overlay without the full editor panels can depend on `GXLib_GUI` directly (ADR-0012 §6 debug overlay) — that's the non-editor ImGui path.

11. **Build integration.**
    - CMake option: `GX_EDITOR` (default ON).
    - `GX_EDITOR=ON` → `add_library(GXLib_Editor STATIC ${EDITOR_SOURCES})`; games link against `GXLib_Graphics GXLib_GUI GXLib_Editor`.
    - `GX_EDITOR=OFF` → skip the `add_library` step; games link against `GXLib_Graphics GXLib_GUI` only.
    - `GXLib_Core` (which contains UndoSystem, NodeGraph, Reflect) is always built — it is not editor-gated.
    - Presets: a future `CMakePresets.json` entry `game-shipping` sets `GX_EDITOR=OFF` + `GX_BUILD_EXAMPLES=OFF` + release config. Tracked as a follow-up, not binding in this ADR.

12. **Shipping-build invariants (enforced by CI / reviewer).**
    - No `GXLib/Editor/*.h` `#include` in any `GXLib/{Graphics,Audio,Physics,Network,Scene,AI,Input}/*` source or header. Enforced by grep-based CI rule (`editor_included_from_runtime` forbidden pattern).
    - No ImGui `#include` outside `GXLib/Editor/`, `GXLib/GUI/` (the debug-overlay path), and `GXLib/ThirdParty/`.
    - `GX_EDITOR=OFF` build must succeed — a failing link with `GX_EDITOR=OFF` is a blocker for any release.
    - Gizmo rendering pass is compiled out when `GX_EDITOR=OFF`; frame-graph stub returns a no-op pass.

13. **Forbidden patterns.**
    - `editor_included_from_runtime` — `#include "Editor/..."` appears outside `GXLib/Editor/` or an editor-gated host (GXModelViewer).
    - `reflection_macro_in_header` — `GX_REFLECT_BEGIN(T)` in a `.h`. Violates ODR; causes duplicate static initialisers.
    - `undo_during_rollback_replay` — `UndoSystem::Undo / Redo` called while `EventBus::IsReplayMode()` is true.
    - `pie_snapshot_assumed_deep` — game code assumes PIE Stop restores non-transform state (audio voices, particle state, network). Use the game's own save-load path for that scope.
    - `imgui_outside_editor_or_debug_gui` — bundled ImGui symbols referenced outside the two sanctioned paths.

### Architecture Diagram

```
   Host application (GXModelViewer, editor tool, or tools-enabled game build)
       │
       │  Constructs panels, owns Scene, UndoSystem, PlayInEditor
       ▼
   ┌─────────────────────────────────────────────────────────────────────┐
   │ GXLib_Editor  (GX_EDITOR=ON)                                        │
   │                                                                     │
   │  Panels (ImGui-hosted):                                             │
   │    SceneHierarchyPanel  EntityPicker  PropertyInspector             │
   │    AssetBrowser  ConsoleWindow  Gizmo                               │
   │    ShaderGraph  TerrainSculptor  TimelineEditor                     │
   │                                                                     │
   │  Services:                                                          │
   │    PlayInEditor (state machine + snapshot/restore)                  │
   │                                                                     │
   │  Dependencies: GXLib_Graphics, GXLib_Scene, GXLib_Core, ImGui       │
   └─────────────────────────────────────────────────────────────────────┘
           │                                     │
           │ uses                                 │ uses
           ▼                                     ▼
   ┌──────────────────────┐            ┌───────────────────────────┐
   │ GXLib_Core           │            │ GXLib_Scene / Graphics    │
   │                      │            │                           │
   │  UndoSystem          │            │  Scene, Entity, Transform │
   │   ├─ ICommand        │            │  Renderer, FrameGraph     │
   │   └─ ValueCommand<T> │            │  AssetDatabase            │
   │                      │            └───────────────────────────┘
   │  NodeGraph runtime   │
   │   ├─ NodeDef         │    Runtime-available: games may depend
   │   └─ NodeInstance    │    on UndoSystem / NodeGraph / Reflect
   │                      │    without any editor inclusion.
   │  Reflect             │
   │   ├─ TypeInfo        │
   │   ├─ TypeRegistry    │
   │   ├─ PropertyMeta    │
   │   ├─ ReflectMacros   │
   │   └─ JsonSerializer  │
   └──────────────────────┘

   Shipping game (GX_EDITOR=OFF):
       - GXLib_Editor  : not built, not linked
       - GXLib_Core    : linked (UndoSystem, NodeGraph, Reflect usable)
       - ImGui         : only if game opts into GXLib_GUI debug-overlay path
```

### Key Interfaces

```cpp
namespace gx {

// --- Core (always available) ---

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual gx::String GetDescription() const = 0;
};

template<typename T>
class ValueCommand : public ICommand { /* ... */ };

class UndoSystem {
public:
    void Execute(std::unique_ptr<ICommand> command);
    bool Undo();
    bool Redo();
    void Clear();
    bool CanUndo() const;
    bool CanRedo() const;
    size_t GetUndoCount() const;
    size_t GetRedoCount() const;
    gx::String GetUndoDescription() const;
    gx::String GetRedoDescription() const;
};

enum class PinType { Flow, Bool, Int, Float, String, Vector3 };
class NodeDef {/* AddInputPin, AddOutputPin, SetExecute, … */};
class NodeInstance {/* pin values, per-instance state */};
class NodeGraph {/* AddNode, Connect, Execute(entry) */};

namespace Reflect {
    class TypeInfo;
    class TypeRegistry;  // Register / Get<T>()
    struct PropertyMeta; // name, type, offset, size, range
    class JsonSerializer;
}

// --- Editor (gated by GX_EDITOR=ON) ---

enum class PIEState : uint32_t { Stopped = 0, Playing, Paused };
struct PIEConfig { bool startPaused, captureInput, showDebugOverlay,
                   simulatePhysics, simulateAI, simulateAudio;
                   float fixedTimeStep; };
struct PIESnapshot { uint32_t entityCount, componentCount; bool hasData; };

class PlayInEditor {
public:
    void SetConfig(const PIEConfig&);
    PIEState GetState() const;
    bool IsPlaying() const; bool IsPaused() const; bool IsStopped() const;
    void SetScene(Scene*);
    void EnterPlayMode();
    void ExitPlayMode();
    void Pause();
    void Resume();
    void StepFrame();
    void Update(float dt);
    PIESnapshot TakeSnapshot(const Scene&) const;
    void RestoreSnapshot(Scene&, const PIESnapshot&) const;
    bool HasSnapshot() const;
    std::function<void()> OnEnterPlay, OnExitPlay;
};

// Panels: SceneHierarchyPanel, EntityPicker, PropertyInspector,
//         AssetBrowser, ConsoleWindow, Gizmo, ShaderGraph,
//         TerrainSculptor, TimelineEditor — each a class with
//         Update(dt) and/or DrawImGui() entry point.

} // namespace gx
```

## Alternatives Considered

### Alternative 1: Runtime-flag-gated editor (single binary, `-editor` launch arg)

- **Description**: Build editor symbols into every binary; gate activation by a CLI flag or runtime check. Ship binaries include the editor — just never activate it.
- **Pros**: One build configuration; simpler CMake; in-field support can activate the editor on any installed build.
- **Cons**: Bloats shipping binaries with ImGui + panel code + gizmo render pass (~2-5 MB depending on panels); enlarges attack surface (ImGui font loader as an input-parsing code path in shipping); PS/Xbox/mobile cert models prefer minimised binaries; requires runtime branches on every editor hook. We are Windows-only today but this closes future-platform doors for no current benefit.
- **Rejection Reason**: Compile-time exclusion is cleaner, smaller, and matches the "editor is a tool, not part of the game" mental model. Debugging support for shipping builds uses `GX_DEV` flags separately, not the editor.

### Alternative 2: Editor as a separate process connecting to a running game

- **Description**: Editor runs in its own process, connects to the game via localhost TCP or named pipe; game instruments a thin remote-control server.
- **Pros**: Zero editor code in shipping binaries; editor can be iterated independently of engine build; enables remote editing workflows.
- **Cons**: Cross-process protocol is a large design surface; initial in-process editor is already built and working; shipping the protocol version-locked means coordinated updates; gameplay programmer experience is worse (roundtrip through IPC).
- **Rejection Reason**: In-process editor already exists and works well for the current single-developer / small-team workflow. Remote-editor may be a future ADR as the team scales or platforms diversify, but not a reason to throw out the current code.

### Alternative 3: Third-party editor framework (ImGui + Dear Im3D + something like Hazel or bring Unity/Unreal editor tooling)

- **Description**: Replace GXLib/Editor with a well-known external editor framework.
- **Pros**: More mature panels; less code to maintain; wider community.
- **Cons**: None integrate with GXLib's ECS / Scene / Reflection / Undo / NodeGraph without a multi-month adapter layer; Unity/Unreal editor tooling is copyrighted; Hazel / similar OSS projects are themselves evolving. Our panels are already specific to GXLib's domain model (EntityPicker uses ADR-0004 EntityHandle, etc.).
- **Rejection Reason**: The adapter cost exceeds the maintenance cost of the current editor code. ImGui itself is already a third-party component doing the heavy lifting — we're not writing UI primitives ourselves.

### Alternative 4: No editor at all — author scenes via hand-edited JSON + hot-reload

- **Description**: Treat the engine as headless SDK; drop PIE, panels, gizmos; rely on JsonSerializer + ADR-0007 hot-reload for iteration.
- **Pros**: Massive scope reduction; simpler engine; no ImGui dependency at all.
- **Cons**: Hand-editing JSON for transforms is orders of magnitude slower than dragging a gizmo; no visual feedback loop while tuning; terrain sculpting via JSON coordinates is not a real workflow; small teams need the editor to be productive.
- **Rejection Reason**: Editor productivity pays for itself on any non-trivial scene. The pillar (ADR-0017) explicitly supports advanced users at L2 — the editor IS that L2 surface for content authoring.

## Consequences

### Positive

- Clean separation: editor code cannot leak into runtime binaries (CI rule + CMake exclusion).
- Core-layer `UndoSystem` / `Reflection` / `NodeGraph` are available to runtime code (save/load, in-game undo, ability graphs) without dragging in the editor.
- PIE's shallow snapshot semantics keep the feature simple; games needing deep snapshot implement it via their own save-game path (owned by ADR-0007).
- Panels use UndoSystem uniformly — "Undo" works across every editor action for free.
- Reflection-driven PropertyInspector means adding a new component type doesn't require a new panel — just `GX_REFLECT_*` the fields.
- NodeGraph peers Lua with identical authority rules — game designers pick the scripting surface they prefer without hidden capability differences.

### Negative

- CMake complexity: games must know to set `GX_EDITOR=OFF` for shipping; out-of-the-box default is ON. Mitigated by a documented `game-shipping` preset (future).
- Reflection macros in headers are a footgun (duplicate static initialisers) — caught by the `reflection_macro_in_header` forbidden pattern but authors sometimes try it anyway.
- PIE shallow snapshot surprises authors who expect audio/particle/network state to reset on Stop — documented, not auto-fixable without the full save-game scope.
- In-process editor consumes GPU memory for ImGui font atlas and panel render targets — negligible at desktop scale but visible in debug-perf budgets.
- The `GX_EDITOR=OFF` path is less frequently exercised than `=ON` — CI must build both to prevent drift.

### Risks

- **Editor leaks into runtime**: an engine programmer `#include "Editor/..."` from a Graphics .cpp, breaking `GX_EDITOR=OFF` builds. *Mitigation*: `editor_included_from_runtime` forbidden pattern; CI grep check; `GX_EDITOR=OFF` build runs in every CI pipeline.
- **Undo across PIE boundary**: user Undoes a command that was issued mid-Play, after Stop restored pre-Play state — command targets an object that was reverted. *Mitigation*: hosts (like GXModelViewer) convention of `UndoSystem::Clear()` on `OnEnterPlay` callback; documented.
- **Reflection ODR breakage**: a reflection macro snuck into a header compiles fine in a single-TU test, blows up in a multi-TU game as duplicate static initialisers. *Mitigation*: forbidden pattern + CI grep + documented convention (`type_reflection.cpp` per-type).
- **NodeGraph cycle**: a graph author creates a flow-pin cycle, execution loops indefinitely. *Mitigation*: execution visit limit (default 10k node-executions per `Execute(entry)` call); assertion + early return.
- **Gizmo leaks into shipping frame graph**: ADR-0008 FrameGraph has a Gizmo pass slot; if `GX_EDITOR=OFF` doesn't null it out, shipping does an extra pass. *Mitigation*: `#if GX_EDITOR` guard around the pass registration; CI `GX_EDITOR=OFF` + frame-graph inspection test.
- **ImGui focus theft**: panels absorb keyboard input while a PIE session is running with `captureInput=true`. *Mitigation*: ADR-0012 §9 focus-steal guard; PIE explicitly disables panel focus while Playing unless user clicks a panel.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-chr-009 ("Editor: PIE, reflection, undo, scene authoring") — elevated from Gap to Covered. Also registers TR-edit-reflection, TR-edit-undo, TR-edit-nodegraph, TR-edit-pie as separate requirements pinning the individual editor subsystems. |

## Performance Implications

- **CPU (editor active)**: ImGui + panels cost ~0.5-1.5 ms/frame at typical authoring density (3-6 panels visible). Gizmo render pass ~0.1 ms. PIE `Update` forwards directly to `Scene::Update` — no additional per-frame cost.
- **CPU (GX_EDITOR=OFF)**: zero — no editor symbols linked, no gizmo pass, no ImGui.
- **Memory (editor active)**: ImGui font atlas ~2 MB; per-panel scratch ~100 KB-1 MB; `PIEEntityState` snapshot ~72 bytes × entity count (≤ 7.2 MB at ADR-0004 100k ceiling — acceptable for editor use).
- **Memory (GX_EDITOR=OFF)**: only Core-layer UndoSystem stack (unbounded by default; host-capped) + NodeGraph instances (per-game) + Reflect registry (~1 KB per registered type).
- **Binary size**: `GX_EDITOR=ON` adds ~3-5 MB to the shipping artifact (ImGui + panels + ShaderGraph + TimelineEditor). `GX_EDITOR=OFF` avoids all of it.
- **Load time**: editor panels init at first paint (~20-50 ms for ImGui backend init). Non-editor builds skip entirely.

## Migration Plan

Not applicable — this ADR is retroactive. Going forward:

1. Add `GX_EDITOR` CMake option with default ON and wire it into `GXLib/CMakeLists.txt` around the `GXLib_Editor` library definition.
2. Add a `game-shipping` CMake preset that sets `GX_EDITOR=OFF` + `GX_BUILD_EXAMPLES=OFF` + release config + LTCG.
3. Add the `editor_included_from_runtime` CI check (grep rule).
4. Add a CI job that builds with `GX_EDITOR=OFF` — blocks merge on failure.
5. Document the `GX_EDITOR=OFF` path in the game-shipping section of the engine reference.
6. Audit existing reflection registrations to confirm none live in headers (`reflection_macro_in_header` sweep).
7. Add `undo_during_rollback_replay` assertion in debug builds.
8. (Future ADR) Deep scene snapshot for PIE — full reflection-driven serialise/restore, or hook into ADR-0007 save/load.

## Validation Criteria

- **PIE state transitions**: unit test walks every legal transition (Stopped→Playing→Paused→Playing→Stopped, Stopped→Playing→Paused→Step→Resume→Stopped); asserts state at each step.
- **PIE snapshot restore**: unit test — enter play mode, mutate entity position, exit play mode, assert position restored.
- **UndoSystem**: test single Undo + Redo with ValueCommand<float>; test Execute-after-Undo clears redo stack.
- **UndoSystem × PIE boundary**: test — host calls `UndoSystem::Clear()` in `OnEnterPlay`; undo stack size = 0 after transition.
- **UndoSystem × rollback**: debug-build assertion fires when `Undo()` called with `EventBus::IsReplayMode()` true.
- **Reflection registration**: multi-TU test — two TUs include the same reflected type; `TypeRegistry::Get<T>()` returns same pointer; no duplicate-registration crash. Header-registered macro test (should fail to compile or produce a link-level diagnostic).
- **JsonSerializer round trip**: register a struct with bool/int/float/string/range props; serialise; deserialise into fresh instance; assert field equality.
- **NodeGraph execution**: flow-pin chain of 5 nodes; Execute from entry; assert all 5 ran in order; cycle test asserts execution visit limit halts cleanly.
- **CMake exclusion**: `GX_EDITOR=OFF` build succeeds; `nm` / `dumpbin` confirms no Editor symbols in the link output; ImGui font atlas not in the binary.
- **Runtime reflection availability**: `GX_EDITOR=OFF` build can still `GX_REFLECT_*` a type and round-trip via JsonSerializer.
- **Shipping invariant check**: CI grep rule passes — no `#include "Editor/..."` outside `GXLib/Editor/`.
- **PIE input capture coexistence (ADR-0012 §9)**: PIE with `captureInput=true` — editor panels do not receive keyboard input while the PIE viewport has focus.
- **Gizmo shipping**: `GX_EDITOR=OFF` frame graph does NOT include a Gizmo pass — verified by frame-graph introspection test.

### DLL Boundary Limitation

Reflection uses `std::type_index(typeid(T))` via `TypeRegistry`, and the anonymous-namespace registrar pattern produces TU-local static initialisers. Both are safe for the current static-library configuration. If GXLib is ever packaged as a DLL: (1) `TypeRegistry::Get<T>()` from a different module than the registrar will fail silently (different `type_info` per module on MSVC), and (2) the same type registered in multiple modules will produce duplicate entries. Migration to a central registration init function would be required.

### PIE Rotation Round-Trip Note

`PIEEntityState` stores rotation as three Euler floats (`rotX/Y/Z`). If `Transform` internally stores a Quaternion, the `GetRotation()` → Euler → `SetRotation(euler)` → Quaternion round-trip may introduce floating-point rounding in the gimbal-lock region. This is a known limitation of the shallow snapshot approach; deep snapshot (future ADR) would serialize the native Quaternion directly.

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — SceneHierarchyPanel and EntityPicker operate on opaque EntityHandles)
- ADR-0005 (Lua scripting boundary — NodeGraph is a peer scripting surface; same authority rules apply)
- ADR-0007 (Asset Database / Hot Reload — AssetBrowser is the editor face of the same database; hot-reload drives panel refresh)
- ADR-0008 (Rendering pipeline — Gizmo is a render pass; ShaderGraph emits materials consumed by ADR-0008 Standard/Unlit/PBR)
- ADR-0012 (GUI — §9 focus-steal guard; panels run inside the same ImGui host as debug overlay)
- ADR-0014 (Animation — TimelineEditor edits AnimationClips and binds to AnimationPlayer)
- ADR-0016 (EventBus — UndoSystem MUST NOT execute during `EventBus::IsReplayMode()` rollback window)
- ADR-0017 (Two-Layer Accessibility Pillar — Editor is L2 "core-modifiable advanced" surface, not beginner `gx::`)
- `GXLib/Editor/{PlayInEditor,Gizmo,EntityPicker,PropertyInspector,SceneHierarchyPanel,AssetBrowser,ConsoleWindow,ShaderGraph,TerrainSculptor,TimelineEditor}.{h,cpp}`
- `GXLib/Core/{UndoSystem,NodeGraph,Reflect/*}.{h,cpp}`
- `GXModelViewer/` (reference host application)
- `GXLib/CMakeLists.txt` lines 189-197 (GXLib_Editor static-library definition)
- CHANGELOG.md Phases 1-3
