# Story 003: Add QueueFromWorker thread-safe worker route

> **Epic**: EventBus
> **Status**: Ready
> **Layer**: Core
> **Type**: Integration
> **Manifest Version**: 2026-04-17

## Context

**ADR**: docs/architecture/adr-0016-eventbus.md
**Requirement**: `TR-bus-004`
*(Threading: Subscribe/Unsubscribe/Fire/DispatchQueued/Clear are main-thread-only. Worker threads use QueueFromWorker<T> SPSC lane only. Fire from worker is a forbidden pattern.)*

**ADR Governing Implementation**: ADR-0016: EventBus / Cross-System Communication
**ADR Decision Summary**: Worker threads use a dedicated QueueFromWorker<T> path backed by a shared mutex-guarded queue. Main thread drains this queue at the start of DispatchQueued. Fire<T> from a worker thread triggers a debug assertion.

**Engine**: GXLib Phase 5 | **Risk**: LOW

**Control Manifest Rules (Core layer)**:
- Required: EventBus Fire/Subscribe/Unsubscribe are main-thread-only
- Forbidden: eventbus_fire_from_worker_thread

---

## Acceptance Criteria

- [ ] `QueueFromWorker<T>(const T& event)` template method added — thread-safe, mutex-guarded
- [ ] Internal `m_workerQueue` (mutex + Vector<QueuedEvent>) added to EventBus
- [ ] `DispatchQueued()` drains worker queue first, then main-thread queue
- [ ] Worker-queued events pass through the same categorisation + replay-mode logic
- [ ] Debug assertion fires if `Fire<T>` is called from a non-main thread (thread-id check)
- [ ] Thread-safety: N worker threads each QueueFromWorker<T> concurrently — no data race

---

## Implementation Notes

*From ADR-0016 §5 (patched):*

- Add `std::mutex m_workerMutex` and `gx::Vector<QueuedEvent> m_workerQueue` to EventBus
- `QueueFromWorker<T>`: lock mutex, push QueuedEvent, unlock. Same format as `Queue<T>` entries.
- At the start of `DispatchQueued()`: lock mutex, swap m_workerQueue into a local, unlock, then dispatch each entry through the same handler-lookup + category-check + replay-mode logic.
- Main-thread queue dispatch follows after worker drain.
- `Fire<T>`: add `assert(IsMainThread())` in debug builds. Use `std::this_thread::get_id()` compared to a stored main-thread id captured at EventBus construction.
- Note: QueueFromWorker is a NEW API — it does not exist in the current EventBus.h. This story adds it.

---

## Out of Scope

- Story 001: HandlerCategory (must be DONE — categories used in dispatch)
- Story 002: ReplayMode (must be DONE — replay suppression applies to worker-queued events too)

---

## QA Test Cases

- **AC-1**: QueueFromWorker is thread-safe
  - Given: 4 worker threads, each calling QueueFromWorker<IntEvent>({value}) 100 times
  - When: all threads complete; main thread calls DispatchQueued
  - Then: handler fires exactly 400 times; no data race (TSan-clean)
  - Edge cases: 0 events queued → DispatchQueued is no-op

- **AC-2**: Worker queue drained before main queue
  - Given: QueueFromWorker<TestEvent>({1}); Queue<TestEvent>({2})
  - When: DispatchQueued
  - Then: handler receives event 1 before event 2

- **AC-3**: Worker-queued events respect replay mode
  - Given: Subscribe handler (SideEffect). QueueFromWorker<TestEvent>({})
  - When: SetReplayMode(true); DispatchQueued
  - Then: handler does NOT fire (SideEffect suppressed)

- **AC-4**: Fire from worker thread asserts (debug)
  - Given: debug build
  - When: worker thread calls Fire<TestEvent>({})
  - Then: assertion failure (debug break / crash)

---

## Test Evidence

**Story Type**: Integration
**Required evidence**: `tests/integration/eventbus/queue_from_worker_test.cpp` — must exist and pass (TSan-clean)
**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (HandlerCategory), Story 002 (ReplayMode)
- Unlocks: None
