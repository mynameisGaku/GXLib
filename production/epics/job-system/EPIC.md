# Epic: Job System

> **Layer**: Core
> **ADR**: docs/architecture/adr-0006-job-system.md
> **Architecture Module**: GXLib/Core/JobSystem.h
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories job-system`

## Overview

This epic covers the multi-threaded Job System introduced in Phase 5. The system provides a work-stealing thread pool with fiber-style continuations used by the Graphics, Physics, and Audio subsystems for parallel update scheduling. Implementation is complete at the charter level — no TR-level requirements were registered at project inception. Remaining work is documentation coverage and verifying that all consumers of `JobSystem` handle cancellation and shutdown ordering correctly under stress.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0006: Job System | Work-stealing thread pool, N worker threads (default: hardware_concurrency − 1), no fiber switching to keep debugging tractable | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| — | No TR-level requirements registered; governed at ADR charter level | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories job-system` to break this epic into implementable stories.
