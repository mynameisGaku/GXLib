# ADR-0016: EventBus / Cross-System Communication (Type-Safe Pub/Sub with Replay-Suppression Contract)

## Status
Accepted

## Date
2026-04-16

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Core / Cross-System Communication |
| **Knowledge Risk** | LOW — `std::type_index`-keyed pub/sub, type-erased `std::function` dispatch, deferred queue, and rewind/replay-suppression categorisation are standard patterns inside LLM training data |
| **References Consulted** | `GXLib/Core/EventBus.h` (sole source-of-truth today — header-only singleton), `Tests/test_CoreSystems.cpp` (EventBusTest fixture — Subscribe / Fire / Queue / DispatchQueued / TypeIsolation), `GXLib/Graphics/3D/AnimationEventDispatcher.{h,cpp}` (sibling mechanism — NOT the same bus), ADR-0013 §13 (forward-declared replay-suppression contract), ADR-0014 §17+§20 (animation event-side-effect producers), ADR-0009 §15 (deterministic reduction order the replay mode must preserve), CHANGELOG Phase 2 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Replay mode suppresses `SideEffect` handlers and lets `Idempotent` handlers run during rollback re-simulation; dispatch order is stable across rollback windows (type-keyed lookup + insertion-ordered handler vector); Subscribe/Unsubscribe/Fire is single-writer main-thread-only (no unordered concurrent mutation); `Clear()` during Shutdown does not leave dangling subscriber captures; type-isolation holds under template instantiation across multiple TUs |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0004 (ECS — publishers frequently emit events during ECS system updates and consume them in the same tick), ADR-0009 (Physics — §15 deterministic reduction order is the correctness frame that EventBus must not violate inside rollback), ADR-0013 (Networking — §13 forward-declared this ADR as the interface owner for replay-suppression), ADR-0014 (Animation — AnimationEventDispatcher routes per-clip events into EventBus when author opts in) |
| **Enables** | Decoupled gameplay systems (damage → VFX, pickup → audio, level-load → UI refresh); rollback-safe gameplay event notification; a single notification spine that analytics, achievements, and telemetry hooks can subscribe to without the subsystems knowing |
| **Blocks** | None (the current non-replay-aware implementation already ships; this ADR adds a categorisation contract that MUST land before any rollback-using game ships) |
| **Ordering Note** | ADR-0013 §13 says: "Until [ADR-0016] lands, this contract is a forward-declaration: implementation MUST be added before any rollback-using game ships." This ADR lifts that forward-declaration and freezes the contract shape. Implementation work tracked under TR-core-eventbus-replay below. |

## Context

### Problem Statement

GXLib has a header-only `EventBus` singleton (`GXLib/Core/EventBus.h`) that provides type-safe publish/subscribe with two modes (synchronous `Fire<T>` and deferred `Queue<T>` + `DispatchQueued`). It exists, it is tested (`test_CoreSystems.cpp` — Subscribe / Fire / Queue / DispatchQueued / TypeIsolation / MultipleSubscribers / Unsubscribe), and game code can use it today.

What it does NOT have, and what is urgently required before any rollback-using game ships (per ADR-0013 §13), is:

1. A **handler categorisation** (`Idempotent` vs `SideEffect`) so the dispatcher can decide what to suppress when gameplay frame N is re-simulated during rollback.
2. A **replay mode flag** (`SetReplayMode(bool)`) that gameplay tick in rollback sets for the duration of the re-simulation window.
3. A **threading contract** — the current `gx::HashMap<std::type_index, ...>` is not thread-safe and offers no serialisation guarantees, so callsites must be pinned to a specific thread for deterministic dispatch.
4. A **relationship with `AnimationEventDispatcher`** — ADR-0014 §17 names AnimationEventDispatcher as a side-effect producer but does not specify how those events reach the global EventBus, nor which direction the suppression flag flows.

This ADR codifies the existing shape, freezes the replay-suppression contract referenced by ADR-0013 §13, specifies the threading model, and documents the bridge to AnimationEventDispatcher.

### Constraints

- Header-only (`EventBus.h`) for zero-TU-cost `#include` in any subsystem. Implementation extensions MUST stay header-only or move to a single `.cpp` without breaking the single-translation-unit-template-instantiation property that gives us type-isolation.
- `std::type_index`-keyed — no manual event ids; type identity IS the routing key. (No way to bypass without breaking TypeIsolation tests.)
- No dynamic allocation inside `Fire<T>` on the hot path after first subscription (acceptable: we allocate in `Subscribe` and `Queue`; `Fire` copies the handler vector for re-entrancy safety — that's a known allocation and it stays).
- Determinism: dispatch order MUST be deterministic within a single tick. Re-ordering inside rollback re-simulation MUST produce the same sequence as the original frame's dispatch.
- No concurrent mutation from worker threads (see Threading below). Cross-thread event *production* uses `Queue<T>` with an explicit main-thread drain, NOT direct `Fire<T>`.
- The ADR-0013 rollback contract is binding: `SetReplayMode(true)` during re-simulation MUST suppress every `SideEffect`-categorised handler and MUST run every `Idempotent`-categorised handler unchanged.

### Requirements

- **Core API (already shipped):** `Subscribe<T>(cb) → EventHandle`, `Unsubscribe(handle)`, `Fire<T>(event)`, `Queue<T>(event)`, `DispatchQueued()`, `Clear()`, `Instance()`.
- **Replay-suppression API (added by this ADR):**
  - `enum class HandlerCategory { Idempotent, SideEffect };`
  - `Subscribe<T>(cb, category) → EventHandle` overload — category is part of the subscription, not the event.
  - `Subscribe<T>(cb)` (one-argument existing overload) defaults to `SideEffect` — conservative default; makes forgotten categorisation behave correctly during rollback (suppressed, never duplicated).
  - `SetReplayMode(bool on)`, `IsReplayMode() const` — flag read inside dispatch; gameplay tick inside rollback wraps the re-sim window.
  - During replay mode, `Fire<T>` and `DispatchQueued` invoke only `Idempotent`-categorised handlers; `SideEffect`-categorised handlers are skipped entirely (not queued, not deferred — skipped).
- **Threading contract:**
  - `Subscribe`, `Unsubscribe`, `Fire`, `Queue`, `DispatchQueued`, `Clear` — main-thread-only.
  - Worker-thread (ADR-0006 JobSystem) event production uses a `QueueFromWorker<T>` path that pushes into a shared mutex-guarded queue (matching the JobSystem's own shared-queue model), which main-thread's `DispatchQueued` drains before running the main-thread queue. This path is the ONLY sanctioned cross-thread route. `QueueFromWorker<T>` is a **new API** to be added to EventBus — it does not exist in the current header-only implementation.
  - `Fire<T>` from a worker thread is a forbidden pattern (`eventbus_fire_from_worker_thread`).
- **AnimationEventDispatcher bridge:** AnimationEventDispatcher is a per-AnimationPlayer local mechanism (by-name callback registry on a single clip). When an AnimationPlayer opts into the global bus (new method `AnimationEventDispatcher::SetGlobalBusBridge(bool)`), each fired AnimationEvent is additionally `Fire<AnimationEventFired>`-emitted on the global EventBus. The global bridge's handlers respect categorisation; dispatcher-local handlers remain outside the replay-suppression system (author's responsibility).
- **Determinism:** dispatch order within a type = insertion order into the handler vector. Handler vector is never reordered. `Clear()` is the only way to drop the vector wholesale.
- **No iteration of `m_handlers`** by gameplay code — it's a `gx::HashMap` and iteration order is unspecified. Single-point lookups by `std::type_index` are deterministic (they're not iterating).
- **No pointer/reference capture of events past the dispatch call** — handlers receive `const T&` and must copy what they want to retain.
- **Subscribe returning `EventHandle`** is guaranteed stable and unique for the process lifetime (monotonic `m_nextId`).

## Decision

**The GXLib EventBus is the single sanctioned cross-system pub/sub spine for game code. It stays a header-only, type-index-keyed, main-thread-only singleton. Handlers carry a `HandlerCategory` (`Idempotent` / `SideEffect`; default `SideEffect`). A `SetReplayMode(bool)` flag, set for the duration of rollback re-simulation, suppresses `SideEffect` handlers and preserves `Idempotent` handlers. Worker-thread event production flows through a single `QueueFromWorker<T>` path drained by the main thread. AnimationEventDispatcher is a peer local system with a bridge into the global bus; the bridge respects categorisation.**

Concrete rules:

1. **One bus, one path.** `gx::EventBus::Instance()` is the single cross-system notification spine. No sibling `GameEventBus` / `UIEventBus` / `CombatEventBus` — separate instances fragment analytics, replay-suppression, and test harness setup.

2. **Type-index routing.** `std::type_index(typeid(T))` is the routing key. Event types are POD structs declared in the publisher's header and included by subscribers. No event-id registry. No string names. This is the only contract that scales without drift across subsystems.

3. **Handler categorisation is mandatory for rollback correctness.**
   ```cpp
   enum class HandlerCategory { Idempotent, SideEffect };
   ```
   - `Idempotent` — handler mutates deterministic replicated state or emits derived data for other systems. Running it twice for the same frame produces the same result. Examples: updating a cached query result, flipping an ECS component flag based on the event's data, recomputing a UI-displayed damage number.
   - `SideEffect` — handler causes externally-visible state change that MUST NOT repeat on re-simulation. Examples: `AudioBus::Play()`, particle spawn, network send, telemetry/analytics event, save-to-disk trigger, toast notification, haptic feedback.
   - **Default is `SideEffect`** when a subscriber calls the existing single-argument `Subscribe<T>(cb)`. Rationale: safer default. A missed categorisation of an actually-idempotent handler silently degrades rollback performance (handler just doesn't run on replay, gameplay state drifts slightly and re-converges next authoritative snapshot). A missed categorisation of an actually-side-effect handler as `Idempotent` duplicates SFX / particles / analytics — strictly worse, user-visible, a correctness bug.

4. **Replay mode.**
   ```cpp
   void SetReplayMode(bool on);
   bool IsReplayMode() const;
   ```
   - Gameplay tick in rollback wraps the entire re-simulation window:
     ```
     EventBus::Instance().SetReplayMode(true);
     for (int f = rewindFrame; f <= currentFrame; ++f) {
         // Animation tick → Physics step → replication apply → gameplay systems
     }
     EventBus::Instance().SetReplayMode(false);
     ```
   - Inside dispatch (`Fire<T>`, `DispatchQueued`), before invoking each handler:
     ```cpp
     if (m_replayMode && handler.category == HandlerCategory::SideEffect)
         continue;
     ```
   - `Queue<T>` during replay mode: event is NOT queued. Rationale: the queued payload would be drained AFTER replay mode ends, re-firing side effects that already ran during the live frame. This is correct-by-default; games that want replay-aware deferred events can flush `DispatchQueued` themselves inside the replay window.

5. **Threading.**
   - Main-thread-only for `Subscribe` / `Unsubscribe` / `Fire` / `DispatchQueued` / `SetReplayMode` / `Clear`.
   - Worker threads (ADR-0006 JobSystem) use a dedicated cross-thread queue:
     ```cpp
     template<typename T>
     void EventBus::QueueFromWorker(const T& event); // thread-safe, mutex-guarded
     ```
     **New API** — not yet in the current EventBus header. Backed by a single shared mutex-guarded queue (consistent with the JobSystem's own shared-queue + mutex model from ADR-0006). Main thread drains this queue at the start of `DispatchQueued`, before processing the main-thread-local queue. The drain path dispatches through the same categorisation + replay-mode logic.
   - `Fire<T>` from a worker thread is a forbidden pattern (`eventbus_fire_from_worker_thread`). Callers must either (a) `QueueFromWorker<T>` to let the main thread fire, or (b) `SubmitMainThread([]{ EventBus::Instance().Fire(...); })` via ADR-0006.

6. **Dispatch determinism.**
   - Handler vector is append-on-`Subscribe`, erase-on-`Unsubscribe`, in-order, never re-sorted.
   - `Fire<T>` copies the handler vector before iterating (existing behaviour — re-entrancy-safe against subscribers that subscribe/unsubscribe from inside a handler). The copy is order-preserving; re-entrant mutations affect the next `Fire` call, not the current one.
   - `DispatchQueued` moves the queue into a local, clears, then iterates — also re-entrancy-safe.
   - During rollback re-simulation, both handler vector order and queue order are the same as during the original simulation (same Subscribe calls → same insertion order → same iteration order). Replay-mode suppression filters but does not reorder.

7. **AnimationEventDispatcher bridge (ADR-0014 §17 producer).**
   - AnimationEventDispatcher remains per-`AnimationPlayer` and local (its own by-name handler map, unchanged).
   - New method on AnimationEventDispatcher: `void SetGlobalBusBridge(bool on)`. When on, each fired AnimationEvent additionally calls `EventBus::Instance().Fire(AnimationEventFired{ eventName, clipId, time, playerId })`.
   - Subscribers to `AnimationEventFired` on the global bus categorise their handlers normally. Game-code convention: the footstep-SFX handler is `SideEffect`; the damage-window-flag-flip handler is `Idempotent`.
   - Dispatcher-local handlers (callbacks registered via `AnimationEventDispatcher::RegisterHandler`) are OUTSIDE the replay-suppression system. Game code that registers local handlers doing audio / VFX in a rollback-using game is responsible for guarding them manually (or routing through the global bus instead — preferred).

8. **Queue semantics.**
   - `Queue<T>(event)` copies the event into a `shared_ptr<T>` (current behaviour, unchanged) and enqueues. Subscribers receive `const T&` on `DispatchQueued`.
   - `DispatchQueued` drains the worker-thread shared queue first, then the main-thread-local queue. Both are processed in FIFO order. Deterministic (same Subscribe order → same dispatch order).
   - No priority. Games that need priority implement it in their event type + subscriber filter, not in the bus.

9. **Subscription lifetime.**
   - `EventHandle` is a 64-bit monotonic id. Zero-initialised handle (`EventHandle{}`) is invalid.
   - `Unsubscribe` is O(N) across all handler vectors — accepted for simplicity; subscribers are short-lived at frame/scene scope, not per-entity per-frame.
   - `Clear()` drops all handlers and the queue. Called on shutdown; also callable between tests (existing pattern in `EventBusTest::SetUp/TearDown`).
   - Subscriber lambdas CAPTURE by value; capturing pointers to objects with shorter lifetime than the bus is a forbidden pattern (`eventbus_subscriber_captures_dangling_pointer`). Scope-bound subscribers must `Unsubscribe` in their destructor.

10. **Forbidden patterns (enforced via registry + reviews).**
    - `eventbus_fire_from_worker_thread` — `Fire<T>` or `DispatchQueued` called off the main thread. Use `QueueFromWorker<T>`.
    - `eventbus_second_instance` — constructing a second bus (parallel `GameEventBus` etc.). Use the singleton.
    - `eventbus_sideeffect_handler_uncategorised_in_rollback_game` — game uses ADR-0013 rollback AND a `SideEffect`-producing handler was registered via the one-arg `Subscribe<T>(cb)` overload (which defaults to `SideEffect`, but we want explicit categorisation in rollback-using games). Enforced by a build flag `GX_ROLLBACK_REQUIRES_EXPLICIT_CATEGORY` and a warning in debug.
    - `eventbus_subscriber_captures_dangling_pointer` — lambda capture of a raw pointer whose lifetime is shorter than the bus without matching `Unsubscribe`.
    - `eventbus_iteration_of_handler_map` — direct read of `m_handlers` outside the class (it would be a hash-container iteration — nondeterministic order).

### Architecture Diagram

```
   Main thread (gameplay tick)
       │
       │ EventBus::Instance().Fire<T>(e)
       │ EventBus::Instance().Queue<T>(e)
       │ EventBus::Instance().DispatchQueued()
       │ EventBus::Instance().SetReplayMode(on/off)
       ▼
   ┌─────────────────────────────────────────────────┐
   │ gx::EventBus (singleton, header-only)           │
   │                                                 │
   │  m_handlers: HashMap<type_index, Vector<H>>     │
   │       (Vector<H> is insertion-ordered)          │
   │                                                 │
   │  m_queue (main-thread)                          │
   │  m_workerQueue (shared, mutex-guarded)            │
   │                                                 │
   │  m_replayMode (bool)                            │
   │                                                 │
   │  Dispatch logic:                                │
   │    for handler in handlers[typeid(T)]           │
   │      if m_replayMode AND h.cat == SideEffect    │
   │        continue                                 │
   │      invoke h.callback(&event)                  │
   └─────────────────────────────────────────────────┘
       ▲
       │ EventBus::Instance().QueueFromWorker<T>(e)
       │ (mutex-guarded push into shared worker queue)
       │
   Worker thread (ADR-0006 JobSystem)

   Per-AnimationPlayer (peer local system):

   gx::AnimationEventDispatcher
       ├─ RegisterHandler(name, cb)     [local, always runs, NO replay-suppression]
       ├─ RegisterGlobalHandler(cb)     [local, always runs]
       └─ SetGlobalBusBridge(true)  ──► EventBus::Fire<AnimationEventFired>{...}
                                        (routed through replay-suppression)
```

### Key Interfaces

```cpp
namespace gx {

enum class HandlerCategory { Idempotent, SideEffect };

struct EventHandle { uint64_t id = 0; explicit operator bool() const; };

class EventBus {
public:
    static EventBus& Instance();

    // Existing — unchanged behaviour. Default category = SideEffect.
    template<typename T>
    EventHandle Subscribe(std::function<void(const T&)> cb);

    // New — explicit categorisation.
    template<typename T>
    EventHandle Subscribe(std::function<void(const T&)> cb, HandlerCategory cat);

    void Unsubscribe(EventHandle handle);

    template<typename T> void Fire(const T& event);   // main-thread-only
    template<typename T> void Queue(const T& event);  // main-thread-only
    void DispatchQueued();                             // drains worker lanes first, then main queue

    template<typename T> void QueueFromWorker(const T& event); // thread-safe, mutex-guarded (new API)

    void SetReplayMode(bool on);
    bool IsReplayMode() const;

    void Clear();
};

// AnimationEventDispatcher addition
class AnimationEventDispatcher {
    // ... existing API ...
    void SetGlobalBusBridge(bool on);
    bool IsGlobalBusBridged() const;
};

// Global-bus event type emitted by the bridge
struct AnimationEventFired {
    gx::String eventName;
    uint32_t clipId;
    float time;
    uint32_t playerId;
};

} // namespace gx
```

## Alternatives Considered

### Alternative 1: Rewind the handler state instead of suppressing side-effect handlers

- **Description**: Snapshot every subscriber's mutable state at frame start; on rollback, restore that state; let all handlers re-fire on replay — they reproduce their side effects harmlessly because the world is restored.
- **Pros**: One contract (rollback restores world); no category metadata on subscribers.
- **Cons**: "Side effect" by definition escapes gameplay state — `AudioBus::Play` hands off to XAudio2 voices we don't own; particle spawn enters the GPU pipeline; network `Send` hits the socket. Rewinding all of those requires integrating every downstream system into rollback, which is far more invasive than a 2-line check in dispatch. Also the replay would produce duplicate audio / visible VFX for every rolled-back frame.
- **Rejection Reason**: Categorisation is localised (one enum at Subscribe) and aligns with what each subsystem can actually undo. Full-state-rewind of downstream systems is a multi-year project with no incremental delivery path.

### Alternative 2: Explicit event-id registry instead of `type_index`

- **Description**: Each event declares an id (enum or constexpr hash of a name); handlers key on the id; type safety comes from a separate registry mapping id → type.
- **Pros**: Stable across process runs (useful for replay files / net-serialisation of event traces).
- **Cons**: Drift risk — two subsystems pick the same id; introduces a registry that every event-defining header must touch (breaks encapsulation); doubles the declaration burden for every new event; loses implicit one-definition-per-type uniqueness.
- **Rejection Reason**: `type_index`'s downside is "not stable across process runs," which matters for on-disk replay storage — but we do not persist EventBus traces today, and if we ever need to, we can add a one-direction type-to-name registry at that point. The maintenance cost of an id registry is paid every day; the benefit is zero today.

### Alternative 3: One bus per subsystem (`CombatEventBus`, `UIEventBus`, `NetworkEventBus`, …)

- **Description**: Each high-level system owns its own bus; cross-system notifications use explicit bridge subscribers.
- **Pros**: Locality — a UI handler only sees UI events; cleaner test isolation; easier to reason about per-subsystem.
- **Cons**: Analytics / telemetry / rollback-replay-mode has to be applied N times (one per bus). Cross-cutting features (logging, replay suppression, assertion hooks) fragment. Authors who want to emit "player died" for gameplay AND for UI AND for audio have to pick a home bus and bridge it, increasing cognitive load.
- **Rejection Reason**: The per-subsystem benefit is local; the cross-cutting cost (replay-suppression, telemetry, rollback) is global. One bus with categorisation gives us both rollback correctness and telemetry hook points for free.

### Alternative 4: Make all handlers idempotent by construction — outlaw side effects inside handlers

- **Description**: Handlers may only mutate gameplay state; side effects must go through a separate queued-SFX / queued-VFX system whose queue is replay-aware.
- **Pros**: Simpler bus (no category enum, no replay mode in dispatch).
- **Cons**: Shifts the same problem to N "queued-side-effect" systems (audio, particles, network, telemetry, save, haptic, …). Each becomes a categorisation point on its own; none of them share infrastructure. Authors end up writing the same "should I emit in replay mode?" guard N times, badly.
- **Rejection Reason**: Centralising the replay-suppression logic inside the bus — exactly where dispatch happens — is the one-place-to-get-it-right choice. Forcing authors to route every side effect through a parallel queued-X system leaks rollback awareness everywhere in the codebase.

## Consequences

### Positive

- Single spine for cross-system notification — analytics, replay-suppression, assertion hooks, and test harness setup all attach in one place.
- Rollback correctness is a 2-line check inside `Fire` / `DispatchQueued`, not a per-subsystem project.
- Type identity = routing key. No id registry to maintain, no drift between subsystems.
- Header-only: zero TU cost to `#include` the bus; zero link-order concerns.
- Default-to-`SideEffect` categorisation: a forgotten category fails safe (handler is skipped in replay — gameplay state re-syncs at next snapshot) rather than fails loud-and-duplicated (SFX plays twice).
- Worker-thread producers have a single sanctioned route (`QueueFromWorker`); no ad-hoc thread safety per callsite.

### Negative

- Category annotation discipline is required, and the default is conservative. Authors who forget to mark an `Idempotent` handler get degraded rollback convergence (benign but measurable).
- `Fire<T>` copying the handler vector for re-entrancy safety is O(N) per dispatch — fine at subscriber counts <100, potentially visible at 10k. Games approaching that scale should batch subscribers or move to a dedicated system.
- Global singleton means a test that wants hermetic isolation must call `Clear()` in SetUp/TearDown (existing pattern). Two tests running in parallel on the same process would collide; not a concern for the current test runner (single-process, serial).
- Dispatcher-local AnimationEventDispatcher handlers remain OUTSIDE the replay-suppression system; authors using those in rollback-using games must manually guard them (or prefer the global bridge).

### Risks

- **Category drift**: subscriber added without categorisation in a rollback-using game — handler silently defaults to `SideEffect`, gets suppressed in replay, gameplay state drifts slightly. *Mitigation*: `GX_ROLLBACK_REQUIRES_EXPLICIT_CATEGORY` debug warning; static analyser rule listing all `Subscribe<T>(cb)` (one-arg) callsites in rollback-using TUs; documented in the control manifest.
- **Worker-thread `Fire<T>` slips in** — concurrent hashmap access crashes or races. *Mitigation*: debug assertion on thread-id inside `Fire`; `eventbus_fire_from_worker_thread` forbidden pattern; code review rule.
- **Subscriber captures dangling pointer** when the subscriber outlives its target. *Mitigation*: forbidden pattern; RAII wrapper `ScopedSubscription` in tests + gameplay (small helper that Unsubscribes in its destructor — future enhancement, not blocking).
- **Queue grows unbounded** if no one calls `DispatchQueued` for many frames. *Mitigation*: runtime assert on queue size > budget (default 10k pending events); logged and flushed on Shutdown.
- **AnimationEventDispatcher local-handler misuse** — game author registers a local handler that plays SFX, ships with rollback, hears duplicate SFX. *Mitigation*: documented in ADR and in `AnimationEventDispatcher::RegisterHandler` docstring; lint rule suggesting the global bridge instead in rollback-using games.
- **Two-step subscribe race on shutdown** — last subscriber Unsubscribes on a different thread than Shutdown. *Mitigation*: Shutdown is main-thread-only (per this ADR's threading contract); asserted.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Lifts ADR-0013 §13 forward-declaration to Covered via new TR-core-eventbus-replay; closes `nondeterministic_reduction_in_rollback_physics_stage`-adjacent gap for event-driven side-effect handling. Also codifies TR-core-eventbus-core (the already-shipped interface) and TR-anim-eventbus-bridge (AnimationEventDispatcher → global bus bridge). |

## Performance Implications

- **CPU**: `Fire<T>` = 1 hashmap lookup + 1 vector copy + N handler invocations. Handler invocation is `std::function` overhead (~1-2 indirect calls) + user code. Typical game subscriber count per event type: 1-10. Expected cost ≤ 0.01 ms per `Fire` in steady state.
- **Memory**: `m_handlers` = #types × (HashMap entry + per-type handler vector). Typical game: ~50 event types × ~5 handlers each × (64 bytes per handler entry) = ~16 KB. `m_queue` scales with deferred-event backlog; budget default 10k pending entries ≈ max ~1 MB depending on payload size.
- **Load Time**: No load cost — header-only, singleton lazily constructed.
- **Rollback overhead**: Replay mode adds one branch per handler invocation. Over an 8-frame rollback window with ~100 events per frame and ~5 handlers each, that's 4000 branch predictions, fully predicted, negligible.
- **Worker queue cost**: `QueueFromWorker<T>` uses a shared mutex-guarded queue (consistent with the JobSystem's own shared-queue model). Mutex push cost ≈ 50-100 ns per event under low contention; acceptable for the expected worker-event volume (tens per frame, not thousands).
- **Fire<T> allocation**: The handler-vector copy in `Fire<T>` allocates on every call (`gx::Vector` copy constructor). At typical subscriber counts (1-10 handlers, ≤ 640 bytes), the SBO or arena allocator absorbs this. For event types fired 100+ times per frame, this becomes measurable (~10-50 μs cumulative). **Accepted trade-off** for re-entrancy safety. If profiling shows this as a bottleneck, migration to a generation-counter + deferred-erase pattern (no copy, mark-and-skip stale entries) is the documented escape hatch.

## Migration Plan

Not applicable for a greenfield context, but this ADR adds to an existing interface:

1. **Current code** (`Subscribe<T>(cb)` one-arg) continues to compile and run. Default category is `SideEffect`; behaviour outside rollback is identical.
2. **Inside rollback-using games**: authors should audit every existing `Subscribe<T>(cb)` callsite and convert genuinely-idempotent handlers to `Subscribe<T>(cb, HandlerCategory::Idempotent)`. The `GX_ROLLBACK_REQUIRES_EXPLICIT_CATEGORY` debug flag lists them.
3. **New worker-thread producers**: switch from `EventBus::Instance().Fire<T>(...)` in jobs (currently a forbidden pattern, but may exist in legacy code) to `EventBus::Instance().QueueFromWorker<T>(...)`. Main thread drains on next `DispatchQueued`.
4. **AnimationEventDispatcher migration**: existing local-handler users keep working. Rollback-using games should call `SetGlobalBusBridge(true)` on each AnimationPlayer and migrate their SFX/VFX handlers from `RegisterHandler` (local, no suppression) to `Subscribe<AnimationEventFired>(..., HandlerCategory::SideEffect)` (global, suppressed in replay).

## Validation Criteria

- **Core (already shipped):** `test_CoreSystems.cpp::EventBusTest::{SubscribeAndFire, MultipleSubscribers, Unsubscribe, QueueAndDispatch, TypeIsolation}` — all must keep passing.
- **Categorisation:** new test `EventBusTest::ReplayModeSkipsSideEffect` — register two handlers (Idempotent + SideEffect); `SetReplayMode(true)`; Fire; only Idempotent ran. `SetReplayMode(false)`; Fire; both ran.
- **Queue in replay mode:** new test `EventBusTest::ReplayModeSkipsQueueEnqueue` — `SetReplayMode(true)`; `Queue(event)`; `SetReplayMode(false)`; `DispatchQueued` — no handler runs (event was not queued).
- **Default categorisation:** new test `EventBusTest::SingleArgSubscribeDefaultsToSideEffect` — `Subscribe<T>(cb)` (one-arg); `SetReplayMode(true)`; Fire; handler does NOT run.
- **Worker queue:** new test `EventBusTest::QueueFromWorkerIsThreadSafe` — N worker threads each call `QueueFromWorker<T>`; main thread calls `DispatchQueued`; handler runs exactly N×(events-per-worker) times; no data races (TSan-clean).
- **Order determinism:** new test `EventBusTest::FireOrderIsInsertionOrder` — subscribe 100 handlers that append their id to a vector; Fire; vector == 0..99. Clear; subscribe in reverse; Fire; vector == 99..0.
- **AnimationEventDispatcher bridge:** new test `AnimationEventDispatcherTest::BridgeEmitsAnimationEventFiredOnGlobalBus` — register a global-bus subscriber for `AnimationEventFired`; set bridge on; play an animation through an event time; global handler fires with the correct name/clip/time/player.
- **Threading assertion (debug):** calling `Fire<T>` from a worker thread with DEBUG build asserts and crashes cleanly; release build silently UB (known, documented).
- **Rollback end-to-end:** with a minimal rollback scenario — gameplay tick emits 3 events per frame (1 Idempotent, 2 SideEffect) — SideEffect handlers fire exactly once per frame regardless of how many times that frame is re-simulated; Idempotent handlers fire N times where N is live + re-simulation count.

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — ECS system updates are the main producers/consumers of EventBus events; the ECS archetype determinism combines with EventBus insertion order to give a deterministic event stream per tick)
- ADR-0006 (Job System — the `QueueFromWorker<T>` route uses the same shared-queue + mutex model as the JobSystem itself)

### DLL Boundary Limitation

EventBus uses `std::type_index(typeid(T))` as routing key. On MSVC, `type_info` identity is per-module. If GXLib is packaged as a DLL, `Fire<MyEvent>` from DLL A will not reach handlers subscribed from DLL B (different `type_info` addresses for the same type). Migration to string-keyed or hash-keyed dispatch would be required before DLL packaging. This is not a concern for the current static-library configuration.
- ADR-0009 (Physics — §15 deterministic reduction order is what EventBus replay mode preserves for physics-caused events)
- ADR-0013 (Networking — §13 forward-declared this ADR; this ADR lifts the forward-declaration and binds the `HandlerCategory` + `SetReplayMode` interface)
- ADR-0014 (Animation — §17 AnimationEventDispatcher is a producer; this ADR specifies the bridge to the global bus)
- ADR-0017 (Two-Layer Accessibility Pillar — EventBus sits at L2 "core-modifiable" per pillar: advanced users Subscribe freely; beginners' `gx::` Compat wrappers do not expose it)
- `GXLib/Core/EventBus.h` (current source of truth for the core interface)
- `GXLib/Graphics/3D/AnimationEventDispatcher.{h,cpp}` (peer local system)
- `Tests/test_CoreSystems.cpp` (existing test fixture — `EventBusTest`)
- CHANGELOG.md Phase 2
