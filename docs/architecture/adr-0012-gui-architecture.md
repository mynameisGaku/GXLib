# ADR-0012: GUI Architecture (Retained-Mode Widget Tree + ImGui Coexistence + XML/CSS Layout)

## Status
Accepted

## Date
2026-04-16

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | UI |
| **Knowledge Risk** | LOW — retained-mode widget tree, Flexbox layout, CSS-subset selectors, and Dear ImGui DX12/Win32 backends are well-documented and stable |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/GUI/*` source tree, `GXLib/Compat/ImGuiManager.{h,cpp}`, `GXLib/ThirdParty/imgui/`, `Assets/ui/*.xml` + `*.css`, CHANGELOG Phases 0/1/3/5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Capture-target-bubble event ordering across nested widgets; XML/CSS hot reload (via AssetReloader) preserves widget identity by id; ImGui input ownership swap (game ↔ ImGui) under simultaneous click + IME composition; UIRenderer batching efficiency at 1000+ widgets; data binding propagation under property storms |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (doc), ADR-0002 (DX12 backend — UIRenderer issues DX12 draws), ADR-0007 (Asset DB — fonts, sprites, XML/CSS files flow through here with hot reload), ADR-0008 (Rendering — UI composites after PostFX, at native resolution per `dynamic_resolution_on_ui_targets`), ADR-0011 (Input — UIContext consumes InputManager state with the input-ownership rules established here) |
| **Enables** | Future Editor ADR (PIE / inspector / node graph all built on the same widget stack), accessibility features (screen-reader bridge, key-only navigation), localized-text widgets |
| **Blocks** | None (code already exists across Phases 0/1/3/5; retroactive) |
| **Ordering Note** | Editor ADR (future ADR-0015) extends ImGui usage; the input-ownership contract defined here is binding for it |

## Context

### Problem Statement
GXLib needs two distinct UI surfaces in the same process:
1. **Game UI** — retained-mode, designer-authorable (XML markup + CSS styling), animatable, data-bindable, with focus model and event propagation. This is what shipped games render to players.
2. **Developer UI** — immediate-mode, code-driven, used for editor panels, debug overlays, and the GXModelViewer reference tool. This is Dear ImGui.

These two have to coexist (editor and game-view in the same window during play-in-editor), share the same input source (InputManager from ADR-0011), and route through the same rendering backend without fighting over render targets or input focus. This ADR codifies the GUI module's architecture, the widget set, the markup/style pipeline, the input-ownership contract, and the ImGui boundary.

### Constraints
- Windows-only (per ADR-0002)
- Native-resolution UI: must not scale with DynamicResolution (per ADR-0008 forbidden pattern `dynamic_resolution_on_ui_targets`)
- Asset-backed: fonts, sprites, XML, CSS flow through AssetDatabase (per ADR-0007 `subsystem_direct_file_io`)
- Input from InputManager only — no private input pump (per ADR-0011 `private_input_thread`)
- Main-thread only — widget tree mutation, layout, render, and event dispatch all run on the main thread
- IME-aware: text input widgets must consume `IMEHandler` composition + commit per ADR-0011
- Public widget API in namespace `gx::GUI`; ImGui kept in its `ImGui::` namespace (third-party)

### Requirements
- Retained widget tree with `UIContext` as the root manager
- Widget hierarchy: container widgets (`Panel`, `Canvas`, `ScrollView`, `Dialog`, `TabView`, `ListView`) and leaf widgets (`Button`, `TextWidget`, `Image`, `TextInput`, `Slider`, `CheckBox`, `RadioButton`, `DropDown`, `ProgressBar`, `Spacer`)
- Flexbox-style layout (direction, justify, align, gap, padding, margin)
- 3-phase event dispatch: Capture → Target → Bubble (DOM-style) — `stopPropagation` semantics
- Focus management — keyboard / gamepad d-pad navigation
- XML markup loader (`GUILoader` + `XMLParser`) with id-based widget lookup (`UIContext::FindById`)
- CSS-subset stylesheet (`StyleSheet`) — selector support: type / id / class; properties: color, background, padding, margin, border, font, size constraints, layout
- Data binding (`DataBinding`) — observable property → widget property (one-way + two-way for inputs)
- Animations (`UITween`) — duration + easing + property channels
- Drag-and-drop (`DragDropManager`) — drag source / drop target widgets
- Accessibility (`Accessibility`) — names + roles for screen-reader bridge (Phase 5 partial)
- ImGui coexistence — `ImGuiManager` wraps `imgui_impl_dx12` + `imgui_impl_win32`
- Hot reload of XML / CSS / fonts via AssetDatabase

## Decision

**GXLib ships two complementary UI stacks. (1) A retained-mode widget tree under `gx::GUI` (UIContext + Widget hierarchy + XML/CSS layout + DataBinding + UITween) for shipping game UI. (2) Dear ImGui via `gx::ImGuiManager` for developer/editor UI. Both consume input from the single `InputManager` (ADR-0011) under a deterministic ownership contract: ImGui claims input first if any ImGui window is focused or hovered; otherwise the retained tree's UIContext gets it via 3-phase capture/target/bubble dispatch; otherwise input falls through to game code. UIRenderer composites the retained tree at native resolution after PostFX; ImGui draws on top through its own DX12 backend. XML markup, CSS stylesheets, fonts, and sprite atlases all flow through AssetDatabase (ADR-0007) with hot reload.**

Concrete rules:

1. **`gx::GUI::UIContext` is the retained-tree root.** One per Window. Owns the root `Widget`, the active `StyleSheet`, the focus pointer, and the binding registry. `Update(dt, input)` runs once per frame: input handling → event dispatch → layout → animation tick. `Render()` then submits draws to `UIRenderer`.

2. **Widget hierarchy.** All widgets derive from `gx::GUI::Widget`. Containers hold children; leaf widgets do not. Composition over inheritance — visual variants (icon button vs text button) are constructed, not subclassed.

3. **Layout: Flexbox subset.** Each widget has `LayoutProps` (direction, justify-content, align-items, gap, padding, margin, width/height/min/max). Layout runs bottom-up (compute intrinsic sizes), then top-down (assign final positions). Layout is dirty-tracked; only changed subtrees recompute.

4. **Event dispatch: Capture → Target → Bubble.** Pointer (mouse / touch) and keyboard events dispatch in DOM order. Each handler can `stopPropagation` (cancel the rest of the phase) or `preventDefault` (suppress built-in behaviour). Focus changes via `SetFocus(widget)` route subsequent keyboard events to the focused widget's bubble path.

5. **Focus model.** One focused widget per UIContext. Keyboard Tab / Shift+Tab cycles focus via the tree's tab order (DOM order by default; widgets can override `tabIndex`). Gamepad d-pad navigation is opt-in via `Widget::AcceptsDpadFocus`.

6. **XML markup + CSS subset.**
   - `GUILoader::LoadFromXML(assetId)` → `unique_ptr<Widget>` tree. Tags map 1:1 to widget classes (`<Button>`, `<Panel>`, ...). Attributes set widget properties. `id` and `class` attributes drive selectors.
   - `StyleSheet::LoadFromCSS(assetId)` parses a CSS subset: selectors `Type`, `#id`, `.class`, descendant; properties limited to the layout/visual set documented in `Style.h`.
   - Inline `style="..."` attribute supported on any widget tag.
   - Authored examples live in `Assets/ui/*.xml` + `*.css`; reference: `guimenu_demo.xml/css`, `menu.xml/css`.

7. **DataBinding.** `Bind<T>(observable, widget, property)` registers a one-way binding; text inputs use `BindTwoWay`. Observables are property storage objects (`Property<T>`); changes notify subscribed widgets next frame (deferred to layout phase to coalesce storms).

8. **UITween.** Property animation: `UITween::Play(widget, property, from, to, duration, easing)`. Easings: linear, ease-in/out (cubic), elastic, back. Concurrent tweens on the same property: latest wins; previous is cancelled.

9. **DragDropManager.** Widgets opt in via `SetDragSource(payloadFactory)` / `SetDropTarget(acceptFn, dropFn)`. The manager tracks drag state, surfaces a hover ghost, and calls the drop callback on release over a valid target.

10. **Accessibility.** Each widget exposes `name`, `role`, `description`. `Accessibility::EnumerateTree(uiContext)` produces the screen-reader-bridge model. Full screen reader integration is out of scope for this ADR; what's recorded here is the data model commitment — every widget must be name-able and role-tagged so a future bridge ADR can wire to UIA / NVDA without retrofit.

11. **`UIRenderer` is the draw backend.** Batched quad/text/sprite renderer over the GXLib Graphics layer (ADR-0008). Runs as a FrameGraph pass scheduled **after** PostFX tone-map, **before** ImGui. Operates at native resolution (per `dynamic_resolution_on_ui_targets`).

12. **ImGui via `gx::ImGuiManager`.**
   - Lives in `GXLib/Compat/ImGuiManager.{h,cpp}`.
   - Wraps `imgui_impl_dx12` and `imgui_impl_win32` (vendored in `GXLib/ThirdParty/imgui/backends/`).
   - Initialised with the GXLib `GraphicsDevice` and SwapChain; gets its own descriptor heap range.
   - Renders as the **last** FrameGraph pass before Present.
   - Hooks into Window message pump alongside InputManager (ImGui needs WM_CHAR, WM_KEY*, WM_MOUSE*, WM_INPUT* for itself).
   - Game code uses ImGui via its native API (`ImGui::Begin/End`, etc.); ImGuiManager just owns lifecycle.

13. **Input ownership contract (binding for ADR-0011 + future Editor ADR).** Per-frame priority order:
    1. **ImGui** — if `ImGui::GetIO().WantCaptureMouse` (mouse) or `WantCaptureKeyboard` (keyboard) is true, the event is consumed by ImGui and not delivered to UIContext.
    2. **UIContext** — if a widget hit-test under the cursor exists, or a focused widget exists for keyboard events, the event dispatches via Capture/Target/Bubble. A handler may `stopPropagation` to consume.
    3. **Game code** — receives the event via direct `InputManager` queries only if neither ImGui nor UIContext consumed it. Game code SHOULD check `UIContext::IsHandlingInput()` and `ImGuiManager::IsCapturing()` before acting on click/key events.

14. **IME integration.** Text-input widgets (`TextInput`, future `TextArea`) consume `IMEHandler` per ADR-0011: read `GetCompositionText()` for live preview, append `GetCommittedText()` + `ClearCommitted()` on commit. `UIContext::ProcessCharMessage(wchar_t)` routes WM_CHAR to the focused text widget when IME is inactive.

15. **Asset integration (ADR-0007).** Fonts, sprite atlases, XML markup, CSS stylesheets all register as asset types with `AssetDatabase`. `AssetReloader` reload handlers:
    - **XML reload** — `GUILoader` rebuilds the subtree, preserving widget identity by `id` (so DataBinding subscriptions survive); widgets without a stable `id` are recreated.
    - **CSS reload** — `StyleSheet::Reparse`; UIContext re-applies; layout dirties.
    - **Font / sprite reload** — UIRenderer texture cache rebinds to new GPU resource; deferred-release the old (per ADR-0007 `immediate_gpu_resource_release_on_reload`).

16. **Threading.** Public GUI API is main-thread only. UIContext::Update / Render and all widget mutations happen on main. UITween parameter writes are also main-thread; the tween advance runs inside Update.

### Architecture Diagram

```
   gx::Window  ◄── WM_* messages ──► InputManager (ADR-0011)
                                          │
                                          ▼
   gx::GUI::UIContext  (per Window — retained tree)
       ├── Widget tree (root → containers → leaves)
       ├── StyleSheet (CSS subset)         ◄── AssetDB (XML/CSS hot reload, ADR-0007)
       ├── Focus pointer
       ├── Event dispatcher (Capture / Target / Bubble)
       ├── DataBinding registry
       ├── UITween animator
       └── DragDropManager

   gx::GUI::UIRenderer
       └── Batched draws → FrameGraph "UI2D" pass (post-PostFX, pre-ImGui)

   gx::ImGuiManager  (developer / editor UI)
       └── imgui_impl_dx12 + imgui_impl_win32
            └── FrameGraph "ImGui" pass (last before Present)

   Per-frame input ownership priority (binding contract):
       1. ImGui (WantCaptureMouse / WantCaptureKeyboard)  → consumes
       2. UIContext hit-test / focus                      → dispatch + may stopPropagation
       3. Game code via InputManager                      → fallback
```

### Key Interfaces

- `gx::GUI::UIContext::Initialize(UIRenderer*, w, h)`, `SetRoot(unique_ptr<Widget>)`, `Update(dt, InputManager&)`, `Render()`, `SetFocus(Widget*)`, `FindById(id)`, `SetStyleSheet(StyleSheet*)`, `OnResize(w, h)`, `ProcessCharMessage(wchar_t)`, `IsHandlingInput()`
- `gx::GUI::Widget` — base class; `LayoutProps`, `Style`, `id`, `classes`, `OnEvent(Event&)` virtual
- `gx::GUI::GUILoader::LoadFromXML(AssetId) → unique_ptr<Widget>`
- `gx::GUI::StyleSheet::LoadFromCSS(AssetId)`, `Reparse()`
- `gx::GUI::DataBinding::Bind<T>(Property<T>&, Widget*, propertyName)`, `BindTwoWay(...)`
- `gx::GUI::UITween::Play(widget, prop, from, to, duration, easing)`
- `gx::GUI::DragDropManager::SetDragSource(widget, factory)`, `SetDropTarget(widget, accept, drop)`
- `gx::GUI::Accessibility::EnumerateTree(UIContext&) → AccessibilityModel`
- `gx::ImGuiManager::Initialize(GraphicsDevice&, SwapChain&)`, `BeginFrame()`, `EndFrame()`, `IsCapturing() → bool`

## Alternatives Considered

### Alternative 1: ImGui only (drop the retained widget stack)
- **Description**: Use Dear ImGui for both editor and shipping game UI
- **Pros**: Single code path; no widget hierarchy to maintain; rapid prototyping
- **Cons**: Immediate-mode is a poor match for designer-authored, animated, data-bound game UI (main menu, HUD, dialogue boxes); no XML/CSS pipeline; hard to localize; accessibility is afterthought; performance scales poorly with thousands of widgets in shipping games
- **Rejection Reason**: Game UI authoring needs declarative markup + retained state for animations and data binding. ImGui is correct for tools, wrong for shipping UI.

### Alternative 2: Retained tree only (drop ImGui)
- **Description**: Build editor panels (GXModelViewer) on the retained widget stack
- **Pros**: One code path; no third-party dep
- **Cons**: Editor panels iterate weekly — immediate-mode is ~10× faster to prototype; ImGuizmo, ImNodes, ImPlot ecosystem we already vendor depends on ImGui; retained tree can't match ImGui's expressiveness for editor inspectors at low effort
- **Rejection Reason**: Editor productivity demands ImGui. Maintaining both stacks costs less than reimplementing ImGui's editor ergonomics in retained widgets.

### Alternative 3: Adopt RmlUi or Noesis as the retained backend
- **Description**: Wrap an established HTML/CSS-styled UI library
- **Pros**: Mature CSS support; large widget catalog; battle-tested
- **Cons**: Heavy dependency (RmlUi ~1 MB, Noesis commercial); RmlUi's render abstraction conflicts with our DX12/FrameGraph model; Noesis licensing incompatible with GXLib's "minimal friction" positioning; data binding model differs from what we want
- **Rejection Reason**: In-house retained tree is small enough (~3k LoC across `GUI/`) and integrates natively with our renderer, asset DB, and input model

### Alternative 4: Single shared focus model across retained + ImGui (no priority order)
- **Description**: Treat ImGui windows as widgets in the retained tree; one focus pointer
- **Pros**: Cleaner mental model
- **Cons**: ImGui maintains its own internal state machine that doesn't expose the hooks needed to integrate it as a child widget; bidirectional focus sync would race with ImGui's per-frame input gathering
- **Rejection Reason**: The priority-order contract is simpler and respects ImGui's actual API surface. ImGui's `WantCaptureMouse / WantCaptureKeyboard` flags are the documented integration point.

## Consequences

### Positive
- Designers author shipping UI with XML + CSS; no recompile required for layout/style edits (hot reload via AssetDB)
- DataBinding + UITween give a modern UI authoring story (observable state → reactive view)
- ImGui keeps editor / debug UI productive and gives us a free graph-editor / plot ecosystem
- Single InputManager source means one focus contract; input-ownership rules are explicit and testable
- UIRenderer at native resolution preserves UI legibility under DynamicResolution
- Accessibility data model is in from day one — even if the screen-reader bridge is future work

### Negative
- Two UI stacks to maintain — bug fixes in one don't help the other
- ImGui adds ~600 KB to binary; ImGuizmo / ImNodes / ImPlot bring more
- CSS subset will always be smaller than real CSS; designers used to web styling will hit limits
- Retained tree hot reload preserves identity only by `id`; widgets without ids are rebuilt and lose runtime state (focus, scroll position)
- IME composition window positioning requires text widgets to call `IMEHandler::SetCompositionWindowPos` — easy to forget

### Risks
- **Input-ownership leaks**: a game-code handler that doesn't check `UIContext::IsHandlingInput()` can fire on a click intended for UI. *Mitigation*: contract documented as binding; `UIContext::IsHandlingInput()` and `ImGuiManager::IsCapturing()` are cheap; debug overlay highlights the consuming layer per click.
- **Layout dirty-tracking misses** can cause stale positions. *Mitigation*: dirty propagates up on prop set; full-tree recompute on `OnResize`; debug toggle for force-full-relayout.
- **CSS selector ambiguity** with overlapping rules. *Mitigation*: specificity rules match standard CSS (id > class > type > inherited); declared in `StyleSheet` docs.
- **ImGui descriptor heap exhaustion** if game code allocates ImGui textures unboundedly. *Mitigation*: ImGuiManager owns a fixed-capacity descriptor range; logs and falls back to magenta texture when exhausted.
- **DataBinding storms** under rapid property changes. *Mitigation*: notifications coalesce into the next layout phase; same property reset to the same value within one frame is a no-op.
- **Hot reload corrupts input state** if a focused widget is removed mid-event. *Mitigation*: AssetReloader runs at frame boundary (per ADR-0007); UIContext clears focus if the focused widget's id is missing in the reloaded tree.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-chr-008 ("GUI widget system + ImGui integration") — elevated from Gap to Covered |

## Performance Implications

- **CPU**: UI Update + dispatch + layout ≤ 0.5 ms/frame for a typical HUD (≤ 100 widgets), ≤ 2 ms for a complex menu (≤ 1000 widgets). ImGui editor pass ≤ 1 ms/frame for the GXModelViewer panel set
- **GPU**: UIRenderer batched-quad draw ≤ 0.3 ms at 1080p for typical HUD; ImGui ≤ 0.5 ms at 1080p
- **Memory**: ~256 bytes per Widget baseline; CSS stylesheet ~16 KB for typical project; ImGui baseline ~2 MB (font atlas + draw vertex buffer)
- **Load Time**: XML widget tree load ≤ 5 ms for a 200-widget menu; CSS parse ≤ 2 ms
- **Network**: N/A

## Migration Plan

Not applicable — this ADR is retroactive across Phases 0/1/3/5. Going forward:

1. New widget classes derive from `gx::GUI::Widget` and register with `GUILoader::RegisterTag` for XML loadability
2. New CSS properties extend `Style.h` and `StyleSheet::Apply`; format stays a CSS subset
3. Screen-reader bridge (UIA on Windows) is a future ADR that consumes `Accessibility::EnumerateTree`
4. Mobile / touch-first widgets require touch elevation (currently forbidden by ADR-0011 `touch_as_first_class_input`)
5. ImGui upgrades follow the vendored copy under `GXLib/ThirdParty/imgui/`; the `imgui_impl_dx12` / `imgui_impl_win32` files are pinned to the same version

## Validation Criteria

- Layout correctness: load `Assets/ui/guimenu_demo.xml` + CSS — visual layout matches reference screenshot at 1080p, 1440p, 4K
- Hot reload: edit a CSS color in `menu.css` while running — tint updates within 1 frame; widget identity preserved by id (button click handlers remain wired)
- Event dispatch: 5-deep nested panels; click on innermost — capture order Panel→…→Button, target Button, bubble Button→…→Panel; `stopPropagation` at any phase halts correctly
- Focus traversal: Tab cycles all focusable widgets in DOM order; Shift+Tab reverses; gamepad d-pad with `AcceptsDpadFocus=true` navigates spatially
- IME integration: focus a `TextInput`, type Japanese with MS-IME — composition string previews, commit appends to text, candidate window positions to the caret
- Input ownership: click on an ImGui window over a UIContext button — only ImGui handles the click; click on UIContext button only — UIContext handles, game code does not see it
- DynamicResolution: scale 3D RT to 0.5× — UI remains crisp at native resolution, text legible
- Accessibility: `EnumerateTree` produces a non-empty model for `guimenu_demo.xml`; every widget has a name and role

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0002 (DX12 backend — UIRenderer issues DX12 draws)
- ADR-0007 (Asset Database — fonts/sprites/XML/CSS hot reload)
- ADR-0008 (Rendering Pipeline — UI runs at native resolution post-PostFX)
- ADR-0011 (Input Architecture — UIContext consumes InputManager; input-ownership contract)
- (Future) Editor ADR — extends ImGui usage for PIE / inspector / node graph
- `GXLib/GUI/{UIContext,Widget,UIRenderer,StyleSheet,GUILoader,XMLParser,DataBinding,UITween,DragDropManager,Accessibility}.{h,cpp}`
- `GXLib/GUI/Widgets/*.{h,cpp}` (17 widget classes)
- `GXLib/Compat/ImGuiManager.{h,cpp}`, `GXLib/ThirdParty/imgui/`
- `Assets/ui/{menu,guimenu_demo}.{xml,css}` (sample content)
- CHANGELOG.md Phases 0, 1, 3, 5
