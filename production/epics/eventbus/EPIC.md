# Epic: EventBus

> **Layer**: Core
> **ADR**: docs/architecture/adr-0016-eventbus.md
> **Architecture Module**: GXLib/Core/EventBus.h
> **Status**: ✅ Complete (implemented + tested 2026-04-17)
> **Stories**: 5 stories — all Done

## Stories

| # | Story | Type | Status | TR-ID | ADR |
|---|-------|------|--------|-------|-----|
| 001 | HandlerCategory enum + Subscribe overload | Logic | ✅ Done | TR-bus-002 | ADR-0016 §3 |
| 002 | SetReplayMode replay suppression | Logic | ✅ Done | TR-bus-003 | ADR-0016 §4 |
| 003 | QueueFromWorker thread-safe worker route | Integration | ✅ Done | TR-bus-004 | ADR-0016 §5 |
| 004 | AnimationEventDispatcher global bus bridge | Integration | ✅ Done | TR-bus-005 | ADR-0016 §7 |
| 005 | Dispatch order determinism verification | Logic | ✅ Done | TR-bus-006 | ADR-0016 §6 |

## Overview

Typed publish-subscribe EventBus (`GXLib/Core/EventBus.h`). Introduced in
Phase 2; extended per ADR-0016 on 2026-04-17 with `HandlerCategory`
(Idempotent / SideEffect), replay-mode suppression (`SetReplayMode`),
thread-safe worker-thread enqueue (`QueueFromWorker<T>`),
`AnimationEventDispatcher` global-bus bridge, and insertion-order dispatch
determinism. All 5 new TRs covered with dedicated tests under
`Tests/unit/eventbus/`.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0016: EventBus | Typed pub-sub bus; HandlerCategory (Idempotent/SideEffect); replay mode suppresses SideEffect; QueueFromWorker is the main-thread-only Fire's worker-thread producer route; AnimationEventDispatcher bridges to global bus | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-bus-001 | Typed Subscribe / Fire / Unsubscribe API | ✅ Implemented |
| TR-bus-002 | HandlerCategory (Idempotent / SideEffect) + two-arg Subscribe | ✅ Implemented |
| TR-bus-003 | SetReplayMode — SideEffect handlers suppressed during replay | ✅ Implemented |
| TR-bus-004 | QueueFromWorker — thread-safe event enqueue from Job System threads | ✅ Implemented |
| TR-bus-005 | AnimationEventDispatcher → global bus bridge (SetGlobalBusBridge) | ✅ Implemented |
| TR-bus-006 | Insertion-order deterministic dispatch | ✅ Implemented |

## Definition of Done

Reached 2026-04-17:
- All 5 stories implemented — `GXLib/Core/EventBus.h` exposes
  `HandlerCategory`, `SetReplayMode`, `QueueFromWorker`,
  `SetGlobalBusBridge`.
- All acceptance criteria from ADR-0016 verified via dedicated tests in
  `Tests/unit/eventbus/{handler_category,replay_mode,queue_from_worker,animation_bridge,dispatch_order}_test.cpp`.
- Tests pass as part of the 4957-test GXLibTests run.

## Next Step

Epic complete. EventBus is production-ready. Future extensions (if needed):
static vs dynamic handler prioritisation beyond category, handler-scoped
filters, etc. — no ADR-0016 gaps remaining.
