# Story 001: Add HandlerCategory enum + Subscribe overload

> **Epic**: EventBus
> **Status**: Ready
> **Layer**: Core
> **Type**: Logic
> **Manifest Version**: 2026-04-17

## Context

**ADR**: docs/architecture/adr-0016-eventbus.md
**Requirement**: `TR-bus-002`
*(Handler categorisation: Subscribe<T>(cb, HandlerCategory) with Idempotent and SideEffect; one-arg Subscribe defaults to SideEffect.)*

**ADR Governing Implementation**: ADR-0016: EventBus / Cross-System Communication
**ADR Decision Summary**: Handlers carry a HandlerCategory (Idempotent or SideEffect). Default is SideEffect — conservative default ensures forgotten categorisation suppresses in replay rather than duplicating side effects.

**Engine**: GXLib Phase 5 | **Risk**: LOW
**Engine Notes**: None — pure C++20, no post-cutoff APIs.

**Control Manifest Rules (Core layer)**:
- Required: EventBus handlers must carry a HandlerCategory
- Required: Default is SideEffect for one-arg Subscribe
- Forbidden: eventbus_sideeffect_handler_uncategorised_in_rollback_game

---

## Acceptance Criteria

- [ ] `enum class HandlerCategory { Idempotent, SideEffect }` declared in EventBus.h
- [ ] `Subscribe<T>(cb, HandlerCategory cat)` overload added — stores category in Handler struct
- [ ] Existing `Subscribe<T>(cb)` one-arg overload defaults to `HandlerCategory::SideEffect`
- [ ] Handler internal struct extended with `HandlerCategory category` field
- [ ] All existing tests (EventBusTest in test_CoreSystems.cpp) still pass unchanged
- [ ] New test: subscribe with explicit Idempotent, verify handler.category stored correctly

---

## Implementation Notes

*From ADR-0016 §3:*

- Add `HandlerCategory` enum class above `EventHandle` in EventBus.h
- Extend the private `Handler` struct: add `HandlerCategory category = HandlerCategory::SideEffect;`
- Add a second `Subscribe<T>` overload that takes `(std::function<void(const T&)> cb, HandlerCategory cat)` — identical to existing except it sets `handler.category = cat`
- The existing one-arg overload internally calls the two-arg overload with `HandlerCategory::SideEffect`
- No behaviour change in Fire/DispatchQueued yet — that's Story 002

---

## Out of Scope

- Story 002: SetReplayMode replay suppression (uses the category field added here)
- Story 003: QueueFromWorker (uses the category field for worker-produced events)

---

## QA Test Cases

- **AC-1**: HandlerCategory enum exists
  - Given: `#include "Core/EventBus.h"`
  - When: declare `gx::HandlerCategory cat = gx::HandlerCategory::Idempotent;`
  - Then: compiles without error
  - Edge cases: both enum values (Idempotent, SideEffect) are distinct

- **AC-2**: Two-arg Subscribe stores category
  - Given: EventBus with Clear()
  - When: Subscribe<TestEvent>(handler, HandlerCategory::Idempotent)
  - Then: internal handler has category == Idempotent (verify via a replay-mode test in Story 002, or via a test-only accessor)

- **AC-3**: One-arg Subscribe defaults to SideEffect
  - Given: EventBus with Clear()
  - When: Subscribe<TestEvent>(handler) (one-arg)
  - Then: internal handler has category == SideEffect

- **AC-4**: Existing tests unchanged
  - Given: existing test_CoreSystems.cpp EventBusTest fixture
  - When: run all 5 existing EventBus tests
  - Then: all pass (no behaviour change)

---

## Test Evidence

**Story Type**: Logic
**Required evidence**: `tests/unit/eventbus/handler_category_test.cpp` — must exist and pass
**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: None (foundational story)
- Unlocks: Story 002, Story 003, Story 004
