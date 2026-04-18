# Story 001: R1 — PIE layering clarification (PlayInEditor vs SimulationManager)

> **Epic**: Architecture Fixes (2026-04-18 Review)
> **Status**: ✅ Done (2026-04-18, in-session)
> **Layer**: Editor + Core/Scene
> **Type**: ADR text (no code change)
> **Manifest Version**: 2026-04-18

## Context

**ADR**: `docs/architecture/adr-0015-editor.md` + `docs/architecture/adr-0019-scene-architecture.md`
**Requirement**: TR-edit-001 + TR-scn-005 (both already covered; clarifying ownership)
**Review source**: `docs/architecture/architecture-review-2026-04-18.md` §R1

**Problem**: Two ADRs independently claimed ownership of the Play-in-Editor
state machine and scene snapshot/restore:
- ADR-0015 §3-4: `PlayInEditor` owns PIE state machine + `PIESnapshot`.
- ADR-0019 §7: `SimulationManager` owns the identical state machine;
  `SceneSnapshot::Capture/Restore` is the actual capture mechanism.

Both descriptions were correct for different layers but nothing tied them
together. Readers following ADR-0015 searched for `PlayInEditor::EnterPlayMode`;
readers following ADR-0019 searched for `SimulationManager::Play`.

---

## Acceptance Criteria

- [x] ADR-0015 §3 explicitly states `PlayInEditor` delegates state-machine +
      snapshot mechanics to `SimulationManager` + `SceneSnapshot` (ADR-0019 §7).
- [x] ADR-0015 §3 notes that `SimulationManager` + `SceneSnapshot` are
      Core-layer (available when `GX_EDITOR=OFF`).
- [x] ADR-0019 §7 mirrors the reference: `SimulationManager` + `SceneSnapshot`
      are the underlying mechanism; `PlayInEditor` (ADR-0015) orchestrates
      editor UX on top.
- [x] ADR-0019 §7 explicitly calls out that games may use `SimulationManager`
      directly for rewind / checkpoint features without pulling in any Editor
      code.
- [x] No code change — layering was always intended; the ADRs just didn't
      express it.

---

## Implementation Notes

Completed in-session 2026-04-18. Changes:
- `docs/architecture/adr-0015-editor.md` §3 — prepended "Layering with ADR-0019"
  bullet describing `PlayInEditor` → `SimulationManager::Play()` delegation and
  `PIESnapshot` as a thin wrapper around `SceneSnapshot`.
- `docs/architecture/adr-0019-scene-architecture.md` §7 — prepended "Layering
  with ADR-0015" bullet describing the Core-layer responsibility split.

## Out of Scope

- Any code change to `PlayInEditor` or `SimulationManager` — current code
  already implements this layering (per the engine-programmer specialist
  review). This story is pure ADR-text alignment.

---

## Test Evidence

**Story Type**: ADR text only
**Required evidence**: `grep` confirmation that both ADRs contain the
cross-reference bullets. Not an automated-test story.
**Status**: ✅ Done
