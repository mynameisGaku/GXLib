# Epic: Scene

> **Layer**: Feature
> **ADR**: docs/architecture/adr-0019-scene.md
> **Architecture Module**: GXLib/Core/Scene/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories scene`

## Overview

This epic covers Scene lifecycle management: scene loading, unloading, additive loading, transition callbacks, and persistence (save/load of scene state to disk). The subsystem spans Phases 0 through 5 and has comprehensive test coverage. All eight core TRs are implemented. This is one of the most stable epics in the engine. Remaining work is confirming that the AI Behavior Tree blackboard serialization (ADR-0018 dependency) round-trips cleanly through Scene Persistence and that the SceneManager async-load path is stress-tested under frame-budget conditions.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0019: Scene | SceneManager owns scene lifetime; additive loading supported; persistence via binary snapshot; transitions are callback-driven | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-scn-001 | SceneManager: load, unload, get-active APIs | ✅ Implemented |
| TR-scn-002 | Additive scene loading (multiple scenes simultaneously) | ✅ Implemented |
| TR-scn-003 | Scene transition callbacks (OnUnload, OnLoad, OnActivate) | ✅ Implemented |
| TR-scn-004 | Scene persistence: serialize ECS World to binary snapshot | ✅ Implemented |
| TR-scn-005 | Scene persistence: deserialize and restore ECS World | ✅ Implemented |
| TR-scn-006 | Prefab instantiation within a scene | ✅ Implemented |
| TR-scn-007 | Scene asset references tracked by Asset Database | ✅ Implemented |
| TR-scn-008 | Async scene load via Job System | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories scene` to break this epic into implementable stories.
