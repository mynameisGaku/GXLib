# Epic: AI

> **Layer**: Feature
> **ADR**: docs/architecture/adr-0018-ai.md
> **Architecture Module**: GXLib/AI/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories ai`

## Overview

This epic covers the AI subsystem: Behavior Trees, NavMesh pathfinding, and RVO (Reciprocal Velocity Obstacles) crowd steering delivered in Phases 1 and 2. All seven core TRs are implemented and this is the best-tested subsystem in the engine with six dedicated test files. Remaining work is verification-focused: confirming that Behavior Tree blackboard serialization round-trips correctly with the Scene Persistence system (ADR-0019 dependency) and that RVO handles edge cases with zero-radius agents.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0018: AI | Behavior Tree for decision logic; NavMesh for pathfinding (A* on triangulated mesh); RVO for local avoidance; no ML/neural inference in core | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-ai-001 | Behavior Tree node types: Sequence, Selector, Condition, Action, Decorator | ✅ Implemented |
| TR-ai-002 | Blackboard with typed key-value storage | ✅ Implemented |
| TR-ai-003 | NavMesh build from geometry and A* query | ✅ Implemented |
| TR-ai-004 | NavMesh dynamic obstacle registration | ✅ Implemented |
| TR-ai-005 | RVO agent registration and velocity solve | ✅ Implemented |
| TR-ai-006 | RVO integration with NavMesh steering output | ✅ Implemented |
| TR-ai-007 | Behavior Tree serialization (save/load) | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories ai` to break this epic into implementable stories.
