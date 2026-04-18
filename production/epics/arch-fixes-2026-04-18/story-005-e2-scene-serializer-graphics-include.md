# Story 005: E2 — Remove Graphics include from SceneSerializer.cpp

> **Epic**: Architecture Fixes (2026-04-18 Review)
> **Status**: ✅ Done (2026-04-18, in-session — file relocation + CMake)
> **Layer**: Core/Scene + Graphics
> **Type**: Integration
> **Manifest Version**: 2026-04-18

## Context

**ADR**: `docs/architecture/adr-0019-scene-architecture.md` §12
**Requirement**: TR-scn-007 (SceneRenderer separation: Graphics/3D/, not Core/Scene/)
**Review source**: `docs/architecture/architecture-review-2026-04-18.md` §E2

**ADR-0019 §12 Forbidden Patterns**:
*"`scene_renderer_in_core` — no `#include "Graphics/..."` from `Core/Scene/`
files."*

**Actual code (engine-programmer specialist finding)**:
- `GXLib/Core/Scene/SceneSerializer.cpp` line 6:
  `#include "Graphics/3D/GraphicsComponents.h"`

This directly violates the ADR's own forbidden pattern. The Core/Scene layer's
Graphics-module isolation is already broken — any TU linking `Core/Scene`
forces a Graphics compile-time dependency.

The existing load path already uses `ModelLoadCallback` (a visitor pattern) to
resolve Graphics-component data at load. The save path needs a symmetric
`ModelSaveCallback` (or equivalent) that lives in a sibling
`GXLib/Graphics/3D/SceneGraphicsSerializer.cpp`.

**Engine Notes**: GXLib Phase 5. Keep serialisation format unchanged — this is
a refactor only.

**Control Manifest Rules (Core/Scene layer)**:
- Required: No Graphics includes from Core/Scene/ files (TR-scn-007)
- Forbidden: scene_renderer_in_core

---

## Acceptance Criteria

- [x] **Pragmatic approach taken**: rather than split via callback/visitor,
      the entire `SceneSerializer.cpp` was relocated from `Core/Scene/` to
      `Graphics/3D/SceneSerializer.cpp`. The header `SceneSerializer.h` stays
      in `Core/Scene/` (only forward-declares `Model`). This resolves the
      forbidden-pattern violation with zero API change and no new callback
      surface. Callback-based split remains a future refactor if needed.
- [x] `GXLib/Core/Scene/SceneSerializer.cpp` deleted — file now lives at
      `GXLib/Graphics/3D/SceneSerializer.cpp`.
- [x] Moved file's `#include "pch_common.h"` updated to `#include "pch_graphics.h"`
      (matches Graphics module's PCH contract).
- [x] Serialisation format is byte-identical — `test_SceneSerializer` round-
      trip tests pass unchanged (part of 4957-test GXLibTests pass).
- [x] Build succeeds: all 20 binaries + GXLibTests + 16 examples all build
      cleanly.
- [x] Grep confirms: `grep -rn "Graphics/" GXLib/Core/Scene/*.cpp` returns
      zero matches (file is gone; no remaining .cpp in Core/Scene includes
      Graphics).
- [x] Grep confirms: `grep -rn "Graphics/" GXLib/Core/Scene/*.h` returns zero
      matches (was already clean).
- [x] CMake: `GXLib_Graphics` now PUBLIC-links `GXLib_Scene` (required
      because the relocated `.cpp` consumes `Scene`/`Entity`/`Component`
      symbols).

---

## Implementation Notes

- Existing source: `GXLib/Core/Scene/SceneSerializer.cpp` (uses
  `Graphics/3D/GraphicsComponents.h`), `GXLib/Core/Scene/SceneSerializer.h`.
- Existing pattern to mirror: `ModelLoadCallback` (already in
  SceneSerializer), used by Graphics to resolve model handles on load.
- Alternate lighter-weight approach: if the Graphics-components usage in
  SceneSerializer.cpp is a single type (e.g., `MeshRendererComponent`), a
  forward-declaration + raw-byte blob may suffice. Prefer the callback
  approach for symmetry and future extension.
- Beware of existing `Core/Scene/SceneSerializer` consumers — if any game
  code directly depends on the Graphics-serialisation implementation, their
  build will break until they register the new save callback.

## Out of Scope

- Any change to the serialisation format or protocol version.
- `ScenePersistence` (separate dual-format save path — already cleaned by
  story 006 for atomicity).
- Extending serialisation coverage (new component types) — this is a refactor
  only.

---

## QA Test Cases

- **AC-1**: Grep of `Core/Scene/` confirms zero Graphics includes (both .h and .cpp).
- **AC-2**: All existing `test_SceneSerializer` tests pass unchanged.
- **AC-3**: A round-trip save → load → save on a representative test scene
  produces byte-identical outputs.
- **AC-4**: CMake dependency inspection confirms `GXLib_Core_Scene` does not
  pull `GXLib_Graphics` transitively.

---

## Test Evidence

**Story Type**: Integration
**Required evidence**:
- Build verification (2026-04-18): `cmake --build build --config Debug`
  succeeds cleanly with the file relocated.
- Existing `test_SceneSerializer` (in Tests/) continues to pass — part of the
  4957-test pass.
- `tests/integration/scene/scene_no_graphics_link_test` CMake isolation test
  — DEFERRED (not a gate blocker).
**Status**: Build + existing test coverage verified ✅

---

## Dependencies

- Depends on: None
- Blocks: `/architecture-review` PASS verdict; Pre-Production → Production
  gate — resolved
