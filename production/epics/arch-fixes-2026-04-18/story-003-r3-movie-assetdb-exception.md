# Story 003: R3 — Movie AssetDatabase bypass declared as Known Exception

> **Epic**: Architecture Fixes (2026-04-18 Review)
> **Status**: ✅ Done (2026-04-18, in-session)
> **Layer**: Movie
> **Type**: ADR text (no code change)
> **Manifest Version**: 2026-04-18

## Context

**ADR**: `docs/architecture/adr-0020-movie-pipeline.md`
**Requirement**: TR-mov-001
**Review source**: `docs/architecture/architecture-review-2026-04-18.md` §R3

**Problem**: `MoviePlayer::Open(filePath, ...)` bypasses ADR-0007
AssetDatabase (takes a raw path and opens via Media Foundation's internal file
I/O). The Control Manifest's Foundation layer "Required Patterns" explicitly
forbids direct filesystem access: *"All asset access goes through
AssetDatabase — no direct fopen/CreateFile in subsystem code."*

ADR-0020 did not declare this exception and did not list ADR-0007 in its
`Depends On` field — so the bypass looked like a policy violation rather than
an intentional technical constraint (Media Foundation requires a seekable OS
path or `IMFByteStream`).

---

## Acceptance Criteria

- [x] ADR-0020 `Depends On` lists ADR-0007 with a cross-reference to the
      exception section.
- [x] ADR-0020 gains a §"Known Exception: AssetDatabase Bypass" section
      describing:
      - The Media Foundation technical constraint.
      - The acknowledged consequences (no hot reload, no AssetRemapper, cannot
        be packed into `.gxa`/`.pak`, filesystem-direct I/O accepted).
      - The future extension path (`IMFByteStream` adapter over
        `IFileProvider`) as deferred, not committed.
- [x] Forbidden pattern `mf_global_init` annotated with ⚠️ note that the
      current per-instance implementation is a known defect (tracked by
      story 007 / E4).
- [x] New forbidden pattern `movie_in_rollback_window` added (from minor
      concern — MoviePlayer uses wall-clock timing, not rollback-safe).

---

## Implementation Notes

Completed in-session 2026-04-18. Changes:
- `docs/architecture/adr-0020-movie-pipeline.md`:
  - `Depends On` now includes ADR-0007.
  - §6 Forbidden patterns: `mf_global_init` annotated with defect warning and
    pointer to story E4; new `movie_in_rollback_window` pattern added.
  - New §7 "Known Exception: AssetDatabase Bypass" section.

## Out of Scope

- `IMFByteStream` over `IFileProvider` — deferred; tracked in the ADR as
  future extension (no TR registered yet).
- E4 MFPlatform refcount wrapper — separate story (007).

---

## Test Evidence

**Story Type**: ADR text only
**Required evidence**: `grep "Known Exception" docs/architecture/adr-0020*.md`
returns the new section.
**Status**: ✅ Done
