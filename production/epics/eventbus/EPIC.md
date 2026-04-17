# Epic: EventBus

> **Layer**: Core
> **ADR**: docs/architecture/adr-0016-eventbus.md
> **Architecture Module**: GXLib/Core/EventBus.h
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories eventbus`

## Overview

This epic covers the typed publish-subscribe EventBus introduced in Phase 2. The base publish/subscribe/unsubscribe API (TR-bus-001) and synchronous dispatch (TR-bus-005/006) are implemented. However, three requirements from ADR-0016 are NOT YET IMPLEMENTED: handler categories with priority ordering (TR-bus-002), replay-mode toggling for deterministic test replay (TR-bus-003), and thread-safe enqueue from worker threads (TR-bus-004). These gaps affect the Job System's ability to post events cross-thread and the Editor's ability to replay event sequences. This is the highest-priority implementation epic in the Core layer.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0016: EventBus | Typed pub-sub bus; handlers grouped by category for priority; replay mode for test determinism; worker-thread-safe enqueue | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-bus-001 | Typed Subscribe / Publish / Unsubscribe API | ✅ Implemented |
| TR-bus-002 | HandlerCategory enum with ordered dispatch (Physics → Logic → Render) | ❌ Not Yet |
| TR-bus-003 | SetReplayMode(bool) — queue all events, replay on demand | ❌ Not Yet |
| TR-bus-004 | QueueFromWorker — thread-safe event enqueue from Job System threads | ❌ Not Yet |
| TR-bus-005 | Synchronous immediate dispatch on main thread | ✅ Implemented |
| TR-bus-006 | Handler auto-unsubscribe via RAII token | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories eventbus` to break this epic into implementable stories.
