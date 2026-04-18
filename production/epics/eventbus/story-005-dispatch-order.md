# Story 005: Dispatch order determinism verification

> **Epic**: EventBus
> **Status**: ✅ Done (2026-04-17 — implemented + tested)
> **Layer**: Core
> **Type**: Logic
> **Manifest Version**: 2026-04-17

## Context

**ADR**: docs/architecture/adr-0016-eventbus.md
**Requirement**: `TR-bus-006`
*(Dispatch order = insertion order within handler vector; copy-before-iterate for re-entrancy safety; deterministic under rollback re-simulation.)*

**ADR Governing Implementation**: ADR-0016 §6
**ADR Decision Summary**: Handler vector is append-on-Subscribe, erase-on-Unsubscribe, in-order, never re-sorted. Fire<T> copies the vector before iterating (re-entrancy safe). During rollback, the same Subscribe order → same dispatch order.

**Engine**: GXLib Phase 5 | **Risk**: LOW

**Control Manifest Rules (Core layer)**:
- Required: ECS query iteration is deterministic (analogous contract for EventBus)
- Forbidden: eventbus_iteration_of_handler_map

---

## Acceptance Criteria

- [ ] 100 handlers subscribed in sequence → Fire → invocation order matches subscription order (0..99)
- [ ] Subscribe in reverse → Fire → invocation order matches reverse subscription order (99..0)
- [ ] Unsubscribe mid-vector does not reorder remaining handlers (erase-then-shift, not swap-with-last)
- [ ] Re-entrant Subscribe inside a handler takes effect on NEXT Fire, not current
- [ ] Re-entrant Unsubscribe inside a handler takes effect on NEXT Fire, not current
- [ ] DispatchQueued processes events in FIFO order

---

## Implementation Notes

*From ADR-0016 §6:*

The current implementation already copies the handler vector before iterating (`auto copy = it->second;` in Fire<T>). This story verifies that contract holds and adds explicit tests. No code changes expected unless tests reveal a bug.

Key verification points:
- The copy is a value copy of `gx::Vector<Handler>` — order-preserving
- `Unsubscribe` iterates all type vectors to find the matching ID — it does a sequential erase, not swap-with-last. Verify this in the implementation.
- `DispatchQueued` moves the queue into a local before iterating — FIFO guaranteed.

---

## Out of Scope

- Story 001-004: implementation work (this is verification-only)

---

## QA Test Cases

- **AC-1**: Insertion order preserved
  - Given: Clear(); Subscribe 100 handlers, each appending its index to a shared vector
  - When: Fire<TestEvent>({})
  - Then: shared vector == [0, 1, 2, ..., 99]

- **AC-2**: Reverse insertion order preserved
  - Given: Clear(); Subscribe 100 handlers in reverse (99, 98, ..., 0)
  - When: Fire<TestEvent>({})
  - Then: shared vector == [99, 98, ..., 0]

- **AC-3**: Unsubscribe preserves order of remaining
  - Given: Subscribe A, B, C (in that order); Unsubscribe B
  - When: Fire
  - Then: order is [A, C] — not [A, C] via swap-with-last (same result here, but verify no reorder)

- **AC-4**: Re-entrant Subscribe deferred
  - Given: Subscribe handler X that calls Subscribe(handler Y) inside its callback
  - When: Fire
  - Then: X runs; Y does NOT run during this Fire. Next Fire: both X and Y run.

- **AC-5**: DispatchQueued FIFO
  - Given: Queue event A, Queue event B, Queue event C
  - When: DispatchQueued
  - Then: handler receives A, B, C in that order

---

## Test Evidence

**Story Type**: Logic
**Required evidence**: `tests/unit/eventbus/dispatch_order_test.cpp` — must exist and pass
**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: None (verification of existing + Story 001 behaviour)
- Unlocks: None
