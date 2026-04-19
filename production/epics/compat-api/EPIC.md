# Epic: DXLib Compat API

> **Layer**: Foundation
> **ADR**: docs/architecture/adr-0003-compat-api.md
> **Architecture Module**: GXLib/Compat/
> **Status**: ✅ Complete (2026-04-19)
> **Coverage**: Compat wrappers for all major subsystems implemented; full Doxygen coverage; Compat_Particle regression test added

## Overview

DXLib-compatible procedural API layer (`GXLib/Compat/`). Provides a thin
procedural façade over GXLib's class-based internals so DXLib-sourced
code compiles with minimal changes (include + `gx::` namespace
qualification).

Delivered across Phases 0-5; Networking wrappers added 2026-04-17;
Compat_Particle defects fixed 2026-04-17 (AddEmitter return guard, count
precondition, GetDeltaTime); regression test added 2026-04-19.

Full Doxygen coverage confirmed 2026-04-19: 13 undocumented `*F` (float-
coordinate) drawing functions in `GXLib.h` received bilingual Doxygen
blocks. All 6 Compat headers now report zero undocumented public
functions.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0003: DXLib Compat API | Thin procedural façade; int return codes (0/-1); no exceptions across boundary; `gx::` namespace; `GX_*` prefix for GXLib-exclusive extensions | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-api-001 | Procedural compat wrappers for Graphics subsystem | ✅ Implemented |
| TR-api-002 | Procedural compat wrappers for Audio subsystem | ✅ Implemented |
| TR-api-003 | Procedural compat wrappers for Input subsystem | ✅ Implemented |
| TR-api-004 | Doxygen doc comments on all public compat headers | ✅ Implemented (2026-04-19) |

## Definition of Done

Reached 2026-04-19:
- [x] Compat wrappers present for Graphics / Audio / Input / Networking / Particle / System / File / Handle / Math / Screen / Text / Movie / GUI accessor / Model / Keyboard / Mouse / Gamepad / Misc
- [x] Return codes conform to DXLib convention: 0 success, -1 failure
- [x] GX_*-prefixed procedural entry points for GXLib-exclusive features (HDR, VRS, PostFX, network, IME)
- [x] Full Doxygen coverage on Compat/*.h
- [x] Compat_Particle regression test covers count<0 precondition (3 test cases in `Tests/unit/compat/compat_particle_test.cpp`)
- [x] No test regressions (part of 4977-test green baseline)

## Next Step

Epic complete. Future work (not gating): additional wrapper coverage as
more DXLib-sourced consumers request specific functions. Route via
`/quick-design` for simple additions.
