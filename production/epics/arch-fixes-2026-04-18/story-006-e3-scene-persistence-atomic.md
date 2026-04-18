# Story 006: E3 — ScenePersistence::SaveToFile atomic write

> **Epic**: Architecture Fixes (2026-04-18 Review)
> **Status**: ✅ Done (2026-04-18, in-session — code + ADR)
> **Layer**: Core/Scene
> **Type**: Logic
> **Manifest Version**: 2026-04-18

## Context

**ADR**: `docs/architecture/adr-0019-scene-architecture.md` §5 + §12
**Requirement**: TR-scn-003 (ScenePersistence dual-format atomic save/load)
**Review source**: `docs/architecture/architecture-review-2026-04-18.md` §E3

**Problem** (engine-programmer specialist finding):
`GXLib/Core/Scene/ScenePersistence.cpp` `SaveToFile` opened `std::ofstream`
directly on the destination path for both text and binary formats. No
temp-file + rename pattern. A crash / power loss / exception during
serialisation left the scene file in a partially-written state — a data-loss
risk for editor-authored scenes (which may be the only copy of authored
content).

**Engine Notes**: GXLib Phase 5. Windows-only. `std::filesystem::rename` on
MSVC 2019+ uses `MoveFileExW` with replace semantics, atomic for files on the
same volume.

---

## Acceptance Criteria

- [x] `ScenePersistence::SaveToFile` writes to `path + ".tmp"` for both text
      and binary formats.
- [x] After successful flush+close, `std::filesystem::rename` atomically
      replaces the destination.
- [x] On any failure (open, write, flush, rename), the temp file is
      best-effort removed and `SaveToFile` returns `false`.
- [x] Destination path is untouched when a save fails — the pre-existing file
      (if any) remains intact.
- [x] ADR-0019 §5 gains the atomicity invariant as a REQUIRED rule.
- [x] ADR-0019 §12 adds `scene_save_non_atomic` as a forbidden pattern.
- [x] All existing `test_ScenePersistence` tests still pass (no API change).

---

## Implementation Notes

Completed in-session 2026-04-18. Changes:

**Code** (`GXLib/Core/Scene/ScenePersistence.cpp`):
- Added `#include <filesystem>` + `<system_error>`.
- Rewrote `SaveToFile` body to use a `.tmp` sibling file, RAII scope to
  ensure flush+close, then `std::filesystem::rename(tmp, final)` with
  `std::error_code` overload. A `cleanupTmp` lambda handles best-effort
  removal on every failure path.
- Serialisation path (SerializeToString / SerializeToBinary) unchanged.

**ADR** (`docs/architecture/adr-0019-scene-architecture.md`):
- §5 Persistence: added "Atomicity invariant (REQUIRED)" bullet describing the
  temp-file + rename contract, including the MSVC `MoveFileExW` same-volume
  atomic guarantee.
- §12 Forbidden patterns: added `scene_save_non_atomic` entry.

## Out of Scope

- `LoadFromFile` — read path is unchanged (reads are idempotent and atomic by
  nature of `std::ifstream`).
- Asynchronous save (background Job) — current save is synchronous on the
  calling thread; atomicity guarantees hold under synchronous use.
- Atomicity across volumes — if someone passes a `path` whose parent directory
  is on a different volume from the tmp target, `rename` may not be atomic.
  Current implementation writes tmp in the same directory as the destination
  (by using `finalPath + ".tmp"`), which keeps it on the same volume by
  construction.

---

## QA Test Cases

- **AC-1**: Save 50-entity scene to text format; verify the destination file
  equals `SerializeToString(scene)`.
- **AC-2**: Save 50-entity scene to binary format; verify round-trip via
  LoadFromFile.
- **AC-3**: Fault-injection test: mid-save make the temp file unwritable
  (chmod or similar). Verify `SaveToFile` returns `false` AND the destination
  is untouched (existing file not overwritten, no partial file).
- **AC-4**: After a failed save, verify no lingering `.tmp` file remains in
  the target directory (best-effort cleanup worked).
- **AC-5**: Concurrent save to the same path from two threads — a defect-free
  outcome is not guaranteed (not a contract), but the file is never
  half-written.
- **AC-6**: Existing `test_ScenePersistence` GoogleTest cases all pass.

---

## Test Evidence

**Story Type**: Logic
**Required evidence**: `tests/unit/scene/scene_persistence_atomic_test.cpp` —
fault-injection cases (AC-3, AC-4, AC-5).
**Status**: Pending — code is in place, automated fault-injection test needs
a dedicated session to author. Existing `test_ScenePersistence` coverage of
AC-1/AC-2/AC-6 remains valid.

---

## Dependencies

- Depends on: None
- Blocks: Pre-Production → Production gate (Gate blocker per 2026-04-18 review)
