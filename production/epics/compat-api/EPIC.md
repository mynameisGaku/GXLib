# Epic: DXLib Compat API

> **Layer**: Foundation
> **ADR**: docs/architecture/adr-0003-compat-api.md
> **Architecture Module**: GXLib/Compat/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories compat-api`

## Overview

This epic covers the DXLib-compatible procedural API layer that allows developers familiar with DXLib (DirectX 11 era) to adopt GXLib without a full paradigm shift. The compat wrappers for all major subsystems are implemented. Known remaining gaps include incomplete Doxygen coverage on some compat headers and a recently fixed `Compat_Particle` wrapper that should be regression-tested. The UIContext exposure via the Compat layer is also outstanding (tracked under the GUI epic but the compat stub belongs here).

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0003: DXLib Compat API | Provide a thin procedural façade over GXLib's class-based internals so DXLib-sourced code compiles with minimal changes | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-api-001 | Procedural compat wrappers for Graphics subsystem | ✅ Implemented |
| TR-api-002 | Procedural compat wrappers for Audio subsystem | ✅ Implemented |
| TR-api-003 | Procedural compat wrappers for Input subsystem | ✅ Implemented |
| TR-api-004 | Doxygen doc comments on all public compat headers | ⚠️ Partial |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories compat-api` to break this epic into implementable stories.
