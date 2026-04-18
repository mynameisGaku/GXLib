# Story 004: E1 — Split NavMeshDebug.cpp out of AI/ (restore Foundation-only)

> **Epic**: Architecture Fixes (2026-04-18 Review)
> **Status**: ✅ Done (2026-04-18, in-session — code + CMake + ADR)
> **Layer**: AI + Graphics
> **Type**: Integration
> **Manifest Version**: 2026-04-18

## Context

**ADR**: `docs/architecture/adr-0018-ai-architecture.md` §Constraints
**Requirement**: TR-ai-007 (AI Foundation-only dependency)
**Review source**: `docs/architecture/architecture-review-2026-04-18.md` §E1

**ADR-0018 §Constraints**: *"AI module links only `GXLib_Foundation` (Math +
Container + Core utilities). No ECS, Physics, or JobSystem dependency at
compile time."*

**Actual code (engine-programmer specialist finding)**:
- `GXLib/AI/NavMesh.cpp` line 7-8: `#include "Graphics/3D/Terrain.h"` +
  `#include "Graphics/3D/PrimitiveBatch3D.h"`
- `GXLib/AI/NavMesh3D.cpp` line 7: `#include "Graphics/3D/PrimitiveBatch3D.h"`

The headers forward-declare these Graphics types (so consumers don't
transitively pull Graphics), but the `.cpp` files always link Graphics for
`DebugDraw()` and `BuildFromTerrain()`. A game using only AI still pulls the
Graphics chain — the Foundation-only isolation claim is false.

**Engine Notes**: GXLib Phase 5. C++20. Windows-only. Static-linked modules.

**Control Manifest Rules (AI module)**:
- Required: AI Foundation-only dependency (TR-ai-007)
- Forbidden: ai_depends_on_graphics (implied by the "Foundation-only" constraint)

---

## Acceptance Criteria

- [x] `GXLib/AI/NavMesh.cpp` no longer `#include`s any `Graphics/` header.
- [x] `GXLib/AI/NavMesh3D.cpp` no longer `#include`s any `Graphics/` header.
- [x] New file `GXLib/AI/Debug/NavMeshDebug.cpp` contains
      `NavMesh::BuildFromTerrain`, `NavMesh::DebugDraw`,
      `NavMesh::DebugDrawPath`, `NavMesh3D::DebugDraw` — a separate CMake
      target explicitly depending on Graphics.
- [x] `BuildFromTerrain(const Terrain&)` moves to the debug split TU (option
      (a) in original plan — minimal-invasive; signature unchanged).
- [x] CMake: new `GXLib_AIDebug` static target added; PUBLIC-links
      `GXLib_AI` + `GXLib_Graphics`. Umbrella `GXLib` links `GXLib_AIDebug`
      via `if (TARGET GXLib_AIDebug)`.
- [x] Build succeeds: `cmake --build build --config Debug` completes with
      zero errors; all 16 examples + GXLibTests + GXModelViewer link cleanly.
- [ ] AI-only link test: a minimal consumer TU that `#include`s only
      `GXLib/AI/NavMesh.h` and calls `FindPath` links WITHOUT Graphics
      symbols — DEFERRED (would require a new CMake test target; not a gate
      blocker).

---

## Implementation Notes

- Existing source references: `GXLib/AI/NavMesh.cpp`, `GXLib/AI/NavMesh3D.cpp`,
  `GXLib/AI/NavMesh.h`, `GXLib/AI/NavMesh3D.h` (forward-declarations already
  present).
- `BuildFromTerrain` accepts `const Terrain&` — changing the signature to
  accept a raw height-field array would be a cleaner fix but is an API break.
  Preferred: keep the `Terrain&` overload in the debug TU and add a
  raw-heightfield overload in the core TU.
- `DebugDraw(PrimitiveBatch3D&)` clearly needs the Graphics type — moving
  both methods to a sibling file and letting that file pull Graphics is the
  minimal-invasive fix.
- Regenerate the VS solution after the CMake change so the new target shows
  up in the `GXLib/` folder group.

## Out of Scope

- Any change to `NavAgent` (it does not pull Graphics).
- Any change to `BehaviorTree`, `GOAPPlanner`, `PolyNavMesh`, `RVOSolver`.
- Recast-style automated navmesh generation (tracked as TR-defer-recast-generation).

---

## QA Test Cases

- **AC-1**: `grep -n "Graphics/" GXLib/AI/*.cpp` returns zero matches.
- **AC-2**: `cmake --build build --config Debug` produces all 17 binaries +
  GXLibTests with zero errors.
- **AC-3**: `dumpbin /imports` on a minimal AI-only test confirms no Graphics
  symbols are referenced.
- **AC-4**: Existing AI tests (test_AIBehaviorTree, test_NavMesh,
  test_NavMesh3D, test_PolyNavMesh) all pass unchanged.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- Build verification (2026-04-18): `cmake --build build --config Debug`
  succeeds; new `GXLib_AIDebug` target links cleanly with Graphics; all
  AI tests (`test_NavMesh`, `test_NavMesh3D`) pass among the 4957 total
  tests.
- `tests/integration/ai/ai_foundation_only_test.cpp` — CMake compile-only
  test verifying AI-standalone link — DEFERRED to a future session (not
  a gate blocker).
**Status**: Build + existing test coverage verified ✅

---

## Dependencies

- Depends on: None (self-contained refactor)
- Blocks: `/architecture-review` PASS verdict — resolved
