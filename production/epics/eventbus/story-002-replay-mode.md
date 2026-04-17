# Story 002: Implement SetReplayMode replay suppression

> **Epic**: EventBus
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Manifest Version**: 2026-04-17

## Context

**ADR**: docs/architecture/adr-0016-eventbus.md
**Requirement**: `TR-bus-003`
*(Replay-suppression: SetReplayMode(bool) / IsReplayMode() — SideEffect handlers skipped in replay, Idempotent handlers run; Queue during replay is a no-op. Lifts ADR-0013 §13 forward-declaration.)*

**ADR Governing Implementation**: ADR-0016: EventBus / Cross-System Communication
**ADR Decision Summary**: SetReplayMode(true) during rollback re-simulation suppresses SideEffect handlers and preserves Idempotent handlers. Queue<T> during replay mode is a no-op (prevents deferred side-effect re-fire after replay ends).

**Engine**: GXLib Phase 5 | **Risk**: LOW

**Control Manifest Rules (Core layer)**:
- Required: Rollback SetReplayMode(true) suppresses SideEffect handlers
- Forbidden: nondeterministic_reduction_in_rollback_physics_stage (EventBus must not violate)

---

## Acceptance Criteria

- [ ] `void SetReplayMode(bool on)` and `bool IsReplayMode() const` added to EventBus
- [ ] `m_replayMode` bool member added (default false)
- [ ] `Fire<T>`: when m_replayMode is true, skip handlers with category == SideEffect
- [ ] `DispatchQueued`: same suppression logic per handler
- [ ] `Queue<T>`: when m_replayMode is true, event is NOT enqueued (silent no-op)
- [ ] After SetReplayMode(false), normal dispatch resumes — all handlers fire
- [ ] Existing tests pass unchanged (replay mode defaults to false)

---

## Implementation Notes

*From ADR-0016 §4:*

```cpp
// In Fire<T>, before invoking each handler:
if (m_replayMode && handler.category == HandlerCategory::SideEffect)
    continue;

// In Queue<T>, at the top:
if (m_replayMode) return;  // no-op during replay
```

- The rollback gameplay tick wraps the re-simulation window:
  ```
  EventBus::Instance().SetReplayMode(true);
  // ... re-simulate frames ...
  EventBus::Instance().SetReplayMode(false);
  ```
- This is the contract ADR-0013 §13 depends on.

---

## Out of Scope

- Story 001: HandlerCategory (must be DONE before this story)
- Story 003: QueueFromWorker (independent — uses same category field)

---

## QA Test Cases

- **AC-1**: ReplayMode skips SideEffect
  - Given: Subscribe handler A (Idempotent), handler B (SideEffect). counter_A = counter_B = 0.
  - When: SetReplayMode(true); Fire<TestEvent>({})
  - Then: counter_A == 1, counter_B == 0
  - Edge cases: SetReplayMode(false); Fire again → counter_A == 2, counter_B == 1

- **AC-2**: Queue is no-op during replay
  - Given: Subscribe handler C (SideEffect). counter_C = 0.
  - When: SetReplayMode(true); Queue<TestEvent>({}); SetReplayMode(false); DispatchQueued()
  - Then: counter_C == 0 (event was never enqueued)

- **AC-3**: Default replay mode is false
  - Given: fresh EventBus (Clear)
  - When: IsReplayMode()
  - Then: false

- **AC-4**: Idempotent handlers run N times during replay
  - Given: Subscribe handler (Idempotent). counter = 0.
  - When: SetReplayMode(true); Fire 3 times; SetReplayMode(false)
  - Then: counter == 3 (Idempotent always runs)

---

## Test Evidence

**Story Type**: Logic
**Required evidence**: `tests/unit/eventbus/replay_mode_test.cpp` — must exist and pass
**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (HandlerCategory must exist)
- Unlocks: None (but completes the ADR-0013 §13 contract)
