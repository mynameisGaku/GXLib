# ADR-0011: Input Architecture (InputManager + Per-Device Classes + ActionMapping + IMM32 IME)

## Status
Accepted

## Date
2026-04-16

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Input |
| **Knowledge Risk** | LOW — Win32 message pump, XInput, and IMM32 are stable APIs well within the LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Input/*` source tree, `GXLib/Core/Window.{h,cpp}`, CHANGELOG Phases 0/1 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | XInput hot-plug detection latency under 4 controllers churn; IME composition correctness across MS-IME / Google IME on Windows 10 21H2 + Windows 11 23H2; deadzone polarity on third-party (DInput-emulated) gamepads; key rebind capture across modifier keys |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (doc strategy), ADR-0002 (DX12 backend — establishes Windows-only scope), ADR-0003 (DXLib Compat — `gx::CheckHitKey` / `GetMouseInput` / `GetMousePoint` route here) |
| **Enables** | ADR-0012 (GUI — input ownership handoff between game and GUI), future ADRs on rebind UI, replay/determinism, multi-seat input |
| **Blocks** | None (code already exists across Phases 0/1; retroactive) |
| **Ordering Note** | GUI ADR must agree with the input-ownership rules recorded here (focus model, capture semantics) |

## Context

### Problem Statement
A modern Windows game library must aggregate keyboard, mouse, gamepad (XInput), and IME input behind a single facade that (a) mirrors DXLib's procedural shape for migration, (b) exposes per-device classes for richer use (trigger / press / release distinction, analog values, vibration), (c) supports logical action mapping with rebindable bindings, and (d) handles IMM32 composition for Japanese / CJK text input. GXLib built this incrementally in Phases 0 and 1. This ADR records the topology, the message-pump integration, and the input-ownership contract so subsequent ADRs (GUI, future replay, future rebind UI) can reference a fixed contract.

### Constraints
- Windows-only (matches ADR-0002 scope)
- Public API thread model: main thread only (input state is mutated only inside `InputManager::Update()`, which the application calls once per frame)
- Must not own its own thread or message pump — it hooks into `Window::AddMessageProc`
- DXLib Compat (ADR-0003) must be able to expose flat globals (`CheckHitKey`, `GetMouseInput`, `GetMousePoint`, `GetMouseWheelRotVol`, `GetJoypadInputState`) over this stack with DXLib semantics
- Non-throwing API; failure paths return zeros / `false` rather than exceptions
- IME state must coexist with text-input widgets (GUI ADR will consume `IMEHandler::GetCommittedText()` / `GetCompositionText()`)

### Requirements
- One `InputManager` per `Window`; aggregates keyboard, mouse, gamepad, action mapping
- Per-device classes (`Keyboard`, `Mouse`, `Gamepad`) own their own state and expose: current value, "pressed this frame" (trigger), "released this frame", press duration
- `Gamepad` uses XInput, supports up to 4 controllers, vibration (motor speeds), per-device deadzones (sticks 0.24, triggers 0.12 by default)
- `ActionMapping` lets game code define logical actions ("Jump", "Attack") and bind them to one or more physical inputs (key / mouse button / pad button / gamepad axis / mouse axis); JSON save/load
- `IMEHandler` wraps IMM32 — composition / candidate / committed text retrieval, conversion-window position setting
- `InputCapture` provides a key-rebind capture utility (Begin/Update → first input observed becomes a binding)
- `TouchInput` exists as a stub for future use; not a first-class input source under this ADR

## Decision

**GXLib uses a single `InputManager` (one per `Window`) that aggregates per-device state classes (`Keyboard`, `Mouse`, `Gamepad`) plus an `ActionMapping` layer and an `IMEHandler`. All input state is updated once per frame from the main thread via `InputManager::Update()`, fed by Win32 messages routed through `Window::AddMessageProc`. Gamepad polling uses XInput. DXLib Compat (ADR-0003) wraps this stack with flat global functions matching DXLib's procedural surface.**

Concrete rules:

1. **Single facade, per-device state.** `gx::InputManager` owns `m_keyboard`, `m_mouse`, `m_gamepad`, `m_actionMapping`. Game code accesses devices via `GetKeyboard()`, `GetMouse()`, `GetGamepad()`, `GetActionMapping()`. There is no input singleton — applications hold the InputManager (typically on `Application` / `Window`).

2. **Frame-driven update.** `InputManager::Update()` runs once per frame from the main thread before game logic. It refreshes "pressed this frame" / "released this frame" edges from accumulated message state and polls XInput for gamepad state. Reading state mid-frame is safe; writing happens only inside `Update()`.

3. **Win32 message hook, not a private pump.** `InputManager::Initialize(Window&)` calls `window.AddMessageProc(...)` to subscribe to keyboard, mouse, and IME messages. GXLib does NOT create its own message thread. Message handlers update internal state; nothing blocks the pump.

4. **Keyboard.** Tracks 256 virtual-key states. APIs: `IsPressed(vk)`, `IsTriggered(vk)` (rising edge), `IsReleased(vk)` (falling edge), `GetPressDuration(vk)`. DXLib `CheckHitKey(vk)` returns `IsPressed(vk) ? 1 : 0`.

5. **Mouse.** Position (client coordinates), button states (Left/Right/Middle, plus X1/X2), wheel rotation accumulator. APIs: `IsPressed(button)`, `IsTriggered(button)`, `IsReleased(button)`, `GetPosition()`, `GetWheelRotation()` (consumed-on-read accumulator). DXLib `GetMouseInput()` returns the bitmask `bit0=L, bit1=R, bit2=M`.

6. **Gamepad (XInput).**
   - Up to 4 controllers (`k_MaxPads = 4`). XInput `XInputGetState` polled every `Update()`.
   - Buttons: `PadButton::*` constants alias XInput bitmasks directly — no remap layer.
   - Sticks normalized to [-1, +1]; deadzone applied (default `k_StickDeadZone = 0.24`).
   - Triggers normalized to [0, +1]; deadzone applied (default `k_TriggerDeadZone = 0.12`).
   - Vibration: `SetVibration(pad, leftMotor, rightMotor)` — durations are caller-managed (no auto-stop).
   - Hot-plug: an unconnected pad returns false from `IsConnected(pad)` and zero state.
   - DXLib `GetJoypadInputState`-style helpers exposed via the Compat layer (ADR-0003).

7. **ActionMapping.**
   - Defined per InputManager; binding map: `string → vector<InputBinding>`.
   - `InputBinding` carries `type` (KeyboardKey / MouseButton / GamepadButton / GamepadAxis / MouseAxis), `keyCode`, `axisId`, `deadZone`, `scale`, `padIndex`.
   - APIs: `IsActionPressed("Jump")`, `IsActionTriggered("Jump")`, `GetActionAxis("Move")`, `DefineAction(name, bindings)`, `LoadFromJson(path)`, `SaveToJson(path)`.
   - Multiple bindings per action are OR-combined for binary actions; max-magnitude for axes.
   - Action names are case-sensitive `gx::String` keys.

8. **IMEHandler (IMM32).**
   - Tracks composition state machine: `Inactive` → `Composing` → `Committed` → (`ClearCommitted()` returns to `Inactive`).
   - Surfaces `GetCompositionText()` (live, may change), `GetCommittedText()` (frozen until cleared), `GetCandidates()` (page + selected index).
   - `SetCompositionWindowPos(x, y)` repositions the IME candidate window relative to the focused text field.
   - GUI text widgets MUST poll `IMEHandler::GetState()` and `ClearCommitted()` after consuming text.
   - Activation/deactivation of IME (e.g., when entering an English-only password field) is the consumer's responsibility — IMEHandler does not dictate focus policy.

9. **InputCapture.** Single-shot key-rebind helper. `Begin(cancelKey)` → `Update()` polls all devices each frame; first non-cancel input becomes the captured `InputBinding`. `ApplyToAction(mapping, name, bindingIndex)` writes it back. State is exposed via `CaptureState` enum (`Idle / Waiting / Captured / Cancelled`).

10. **TouchInput.** Class exists in `GXLib/Input/TouchInput.{h,cpp}` as a stub. Touch is **not a supported input source under this ADR.** Applications must not depend on it. A future ADR can elevate it if/when GXLib targets touch-capable Windows hardware (Surface, etc.).

11. **Input ownership contract (preview).** Game vs GUI input ownership is the GUI ADR's responsibility (ADR-0012). The contract this ADR commits to: InputManager surfaces raw / aggregate device state every frame; consumers (GUI, game logic) decide how to interpret focus and capture. InputManager itself does not "consume" events on behalf of any consumer.

12. **Compat layer integration.** `gx::CheckHitKey`, `gx::GetMouseInput`, `gx::GetMousePoint`, `gx::GetMouseWheelRotVol`, `gx::GetJoypadInputState`, `gx::SetJoypadVibration` (and similar) wrap the corresponding InputManager methods with DXLib return-code semantics (0/1 booleans, bitmasks, return ints). See ADR-0003.

13. **Determinism note.** Input is **not** deterministic by default — XInput polling and OS message timing introduce non-determinism. A future Replay/Determinism ADR may add an `IInputSource` indirection so recorded streams can drive the same `InputManager` API in playback. This ADR does not add that indirection; it commits to making it possible to insert later without breaking the public API.

### Architecture Diagram

```
   Win32 messages (WM_KEYDOWN, WM_MOUSEMOVE, WM_IME_*, ...)
                 │
                 ▼
   gx::Window::AddMessageProc  ──►  InputManager hooked handlers
                                          │
                                          ▼
   gx::InputManager  (one per Window)
       ├── Keyboard       (VK[256] state, edges, durations)
       ├── Mouse          (pos, buttons, wheel-accumulator)
       ├── Gamepad        (XInput poll × 4, deadzone, vibration)
       ├── ActionMapping  (logical action → InputBinding[], JSON I/O)
       └── IMEHandler     (IMM32: composing / committed / candidates)

   Update() flow (once / frame, main thread):
       ├── advance Keyboard edges from accumulated message state
       ├── advance Mouse edges + flush wheel accumulator
       ├── XInputGetState × 4 → Gamepad state
       └── ActionMapping pulls fresh device state on query

   gx::InputCapture  (utility — runs alongside InputManager for rebind UI)

   gx::Compat (ADR-0003)
       └── CheckHitKey / GetMouseInput / GetMousePoint / GetMouseWheelRotVol /
           GetJoypadInputState / SetJoypadVibration
                          ──►  InputManager
```

### Key Interfaces

- `gx::InputManager::Initialize(Window&)`, `Update()`, `Shutdown()`
- `Keyboard& GetKeyboard()`, `Mouse& GetMouse()`, `Gamepad& GetGamepad()`, `ActionMapping& GetActionMapping()`
- `int CheckHitKey(int vk)`, `int GetMouseInput()`, `void GetMousePoint(int*, int*)`, `int GetMouseWheel()` — DXLib-shaped convenience
- `Gamepad::IsConnected(pad)`, `IsPressed(pad, button)`, `IsTriggered(pad, button)`, `GetStick(pad, axisId)`, `GetTrigger(pad, axisId)`, `SetVibration(pad, l, r)`
- `ActionMapping::DefineAction(name, bindings)`, `IsActionPressed(name)`, `IsActionTriggered(name)`, `GetActionAxis(name)`, `LoadFromJson(path)`, `SaveToJson(path)`
- `IMEHandler::Initialize(hwnd)`, `ProcessMessage(msg, wParam, lParam)`, `GetState()`, `GetCompositionText()`, `GetCommittedText()`, `GetCandidates()`, `ClearCommitted()`, `SetCompositionWindowPos(x, y)`
- `InputCapture::Begin(cancelKey)`, `Update(kbd, mouse, gpad)`, `GetState()`, `GetResult()`, `ApplyToAction(mapping, name, bindingIndex)`

## Alternatives Considered

### Alternative 1: Global static input (DXLib-style only)
- **Description**: Skip the per-device classes; just expose `gx::CheckHitKey` / `gx::GetMouseInput` etc. as the only API
- **Pros**: Smallest possible surface; matches DXLib 1:1
- **Cons**: No per-button trigger/release semantics, no analog access, no rebinding, no IME — every consumer has to reinvent these on top of poll-only globals
- **Rejection Reason**: DXLib parity is provided via the Compat layer over per-device classes (ADR-0003); the per-device classes give modern features for free without losing the flat API

### Alternative 2: Event-driven only (subscribe to input events)
- **Description**: Surface input as event callbacks; no per-frame poll API
- **Pros**: Cleaner for UI consumption; no missed events
- **Cons**: DXLib heritage demands per-frame poll (`CheckHitKey` etc.); event-only forces every game loop to reinvent state caching
- **Rejection Reason**: Hybrid is cheap — InputManager polls and caches state; consumers can also subscribe to message handlers if they want event semantics. A pure-event API loses the porting story.

### Alternative 3: SDL / GLFW / SDL_GameController as the input backend
- **Description**: Wrap a cross-platform input library
- **Pros**: Free non-XInput gamepad support (DInput, HID), cross-platform when GXLib eventually goes non-Windows
- **Cons**: Adds a 1-2 MB dependency; SDL's input model conflicts with our message-hook pattern; we'd still need IMM32 wrapper; XInput is sufficient for the Xbox-controller-shaped pads that dominate Windows gaming
- **Rejection Reason**: GXLib is Windows-only by ADR-0002 scope; XInput covers 90%+ of consumer pads; the dependency cost outweighs the marginal device support

### Alternative 4: Raw Input (WM_INPUT) for keyboard/mouse instead of WM_KEYDOWN/WM_MOUSEMOVE
- **Description**: Use Raw Input API for higher-resolution mouse + multi-keyboard support
- **Pros**: Higher mouse polling rate; per-device routing
- **Cons**: Doesn't deliver IME messages (still need WM_IME_*); duplicates state if also handling regular messages; complexity for marginal benefit at this stage
- **Rejection Reason**: Standard messages are sufficient for current scope; Raw Input can be added later as an opt-in mode without breaking the public InputManager API

## Consequences

### Positive
- One facade, one update tick — game loops have a single place to plug input in
- Per-device classes give modern semantics (edges, durations, analog) that DXLib lacks
- ActionMapping makes rebindable controls a one-liner instead of a per-game project
- IMM32 IME wrapped once — text widgets across the engine consume the same `IMEHandler` API
- Compat layer (ADR-0003) keeps DXLib porting painless
- No private threads — input piggybacks on the existing Window message pump; deterministic insertion point for replay later

### Negative
- Windows-only: porting GXLib elsewhere needs a new InputManager backend
- XInput-only gamepad support: HOTAS sticks, racing wheels, third-party DInput-only pads are unsupported until DInput / Raw Input fallback added
- TouchInput stub may mislead consumers — must be clearly documented as unsupported
- IME activation policy is delegated to consumers (text widgets); inconsistent UX possible if widgets disagree
- ActionMapping JSON format is informal — a schema-versioned format is a future improvement

### Risks
- **XInput hot-plug latency** — connection state can take several frames to settle. *Mitigation*: poll every frame; consumers must check `IsConnected()` before reading state.
- **IME message ordering across MS-IME and third-party IMEs** can differ subtly. *Mitigation*: state machine documented (`Inactive → Composing → Committed`) with verification in test scenes for MS-IME, Google IME, ATOK.
- **Modifier-key capture in InputCapture** can produce ambiguous bindings (e.g., Shift+A captured as Shift then A). *Mitigation*: capture combines simultaneous modifiers in a single binding; documented in InputCapture header.
- **Thread-safety drift** if a consumer reads input from a worker thread. *Mitigation*: documented main-thread-only contract; debug-build assertion on cross-thread reads.
- **Determinism gap** for replay / netcode. *Mitigation*: forward-compat note above; future ADR adds `IInputSource` indirection without breaking InputManager API.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-chr-007 ("Input: K/M, Gamepad, IME") — elevated from Gap to Covered |

## Performance Implications

- **CPU**: Per-frame `Update()` ≤ 0.05 ms on mid CPU: keyboard/mouse edge advance is cache-resident state machines; XInput × 4 polls ≤ 0.03 ms total; ActionMapping queries are O(bindings-per-action) hashmap lookups
- **Memory**: ~4 KB per InputManager (KB state + mouse + 4× gamepad + action map baseline); IMEHandler buffers grow with composition string length (typically <1 KB)
- **Load Time**: ActionMapping JSON load ≤ 5 ms for typical config
- **Network**: N/A (future netcode replay path is a separate ADR)

## Migration Plan

Not applicable — this ADR is retroactive. Input subsystem landed in Phase 0 (Keyboard/Mouse/InputManager), Phase 1 (Gamepad with vibration, IMEHandler, ActionMapping, InputCapture). Going forward:

1. New device classes register inside InputManager (e.g., a future `RawInput` class would live alongside Keyboard/Mouse, not replace them)
2. DInput / HID gamepad fallback would extend Gamepad with a backend abstraction; the public Gamepad API stays stable
3. Replay/determinism work introduces `IInputSource` indirection — Live, Recorded, Replayed sources all feed the same InputManager API
4. Touch elevation requires a new ADR superseding the "stub" rule here

## Validation Criteria

- 4-controller stress: 4 gamepads connected, all reporting; vibration on/off cycling at 30 Hz with no message-pump stall
- IME composition: type a Japanese sentence with MS-IME — composition string mirrors live, committed text appears once per `Commit`, candidate page navigation works
- Rebind capture: capture every keyboard key (excluding system keys), all 14 XInput buttons, and L/R triggers + sticks — all map to the correct `InputBinding`
- ActionMapping JSON round-trip: define 20 actions with 60 total bindings, save → reload → verify identical
- Compat parity: `gx::CheckHitKey(VK_SPACE)` returns 1/0 matching `InputManager::GetKeyboard().IsPressed(VK_SPACE)` for 1000 frames of varied input
- Hot-plug: disconnect a controller mid-game — `IsConnected(pad)` flips to false within 5 frames; reconnect — flips back to true within 5 frames
- Thread-safety: debug build asserts when input read from non-main thread

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0002 (DX12 backend — establishes Windows-only scope)
- ADR-0003 (DXLib Compat — `gx::CheckHitKey` etc. wrap InputManager)
- (Future) ADR-0012 (GUI + ImGui — input ownership / focus rules)
- (Future) Replay/Determinism ADR — `IInputSource` indirection
- `GXLib/Input/{InputManager,Keyboard,Mouse,Gamepad,ActionMapping,IMEHandler,InputCapture,TouchInput}.{h,cpp}` (source of truth)
- `GXLib/Core/Window.{h,cpp}` (`AddMessageProc` host)
- CHANGELOG.md Phases 0, 1
