# Epic: Lua Scripting

> **Layer**: Feature
> **ADR**: docs/architecture/adr-0005-scripting.md
> **Architecture Module**: GXLib/Script/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories scripting`

## Overview

This epic covers the Lua 5.4 scripting layer built on sol2, introduced in Phase 1 and extended with full Lua bindings for Phase 5 subsystems. The binding boundary (C++ → Lua) covers all major engine subsystems: ECS, Physics, Audio, Input, and Scene. All five core TRs are implemented. Remaining work is verification that the Phase 5 bindings (Job System, Hot Reload callbacks, IME events) are fully exercised by the scripting test suite and that error messages surfaced to Lua are actionable rather than raw C++ type names.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0005: Lua Scripting | Lua 5.4 + sol2 for consumer scripting; C++ core is not scriptable below the binding boundary; no coroutine-to-fiber bridge | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-scr-001 | Lua VM initialization and script execution | ✅ Implemented |
| TR-scr-002 | sol2 bindings for ECS (Entity, World, Query) | ✅ Implemented |
| TR-scr-003 | sol2 bindings for Physics, Audio, Input | ✅ Implemented |
| TR-scr-004 | sol2 bindings for Scene and Asset Database | ✅ Implemented |
| TR-scr-005 | Lua error handling with traceback to C++ call site | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories scripting` to break this epic into implementable stories.
