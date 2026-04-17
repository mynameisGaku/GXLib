# Epic: Archetype ECS

> **Layer**: Core
> **ADR**: docs/architecture/adr-0004-ecs.md
> **Architecture Module**: GXLib/ECS/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories ecs`

## Overview

This epic covers GXLib's archetype-based Entity Component System introduced in Phase 4. The World, Query, and System primitives are fully implemented. All five core TRs are satisfied. One known architectural constraint remains documented: `EntityBridge` uses a process-global static to expose the ECS world to legacy Compat-layer code — this is an accepted trade-off per ADR-0004, not a bug, but it must be called out in the API docs so consumers understand the single-world restriction.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0004: Archetype ECS | Adopt archetype-based ECS (not sparse-set) for cache-efficient batch queries; single World per process | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-ecs-001 | Archetype-based World with add/remove component APIs | ✅ Implemented |
| TR-ecs-002 | Type-safe Query iteration with filter support | ✅ Implemented |
| TR-ecs-003 | System registration and ordered execution | ✅ Implemented |
| TR-ecs-004 | EntityBridge: expose World to Compat layer | ✅ Implemented |
| TR-ecs-005 | ECS integration tests covering archetype migration | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories ecs` to break this epic into implementable stories.
