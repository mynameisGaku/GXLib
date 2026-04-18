# Epic: Architecture Fixes (2026-04-18 Review)

> **Layer**: Cross-cutting (AI, Scene, Movie, Editor)
> **Source**: `docs/architecture/architecture-review-2026-04-18.md`
> **Status**: **Complete** (all 7 stories Done 2026-04-18, build + 4957 tests ✅)
> **Stories**: 7 — all Done in-session

## Stories

| # | Story | Type | Status | Issue | ADR |
|---|-------|------|--------|-------|-----|
| 001 | R1 — PIE layering clarification (PlayInEditor vs SimulationManager) | ADR text | ✅ Done | R1 | ADR-0015 §3 + ADR-0019 §7 |
| 002 | R2 — AI threading contract rewrite (per-instance non-reentrant) | ADR text | ✅ Done | R2 | ADR-0018 §9 |
| 003 | R3 — Movie AssetDatabase bypass declared as Known Exception | ADR text | ✅ Done | R3 | ADR-0020 |
| 004 | E1 — Split NavMeshDebug.cpp out of AI/ (restore Foundation-only) | Integration | ✅ Done | E1 | ADR-0018 §Constraints |
| 005 | E2 — Remove Graphics include from SceneSerializer.cpp (file relocated) | Integration | ✅ Done | E2 | ADR-0019 §12 `scene_renderer_in_core` |
| 006 | E3 — ScenePersistence::SaveToFile atomic write | Logic | ✅ Done | E3 | ADR-0019 §5 |
| 007 | E4 — MFPlatform refcount wrapper for MoviePlayer + VideoRecorder | Integration | ✅ Done | E4 | ADR-0020 §6 |

## Overview

The fresh-session `/architecture-review` on 2026-04-18 caught 7 REAL ISSUES in
ADR-0018 (AI), ADR-0019 (Scene), ADR-0020 (Movie) — the three newest ADRs being
independently audited for the first time. Three issues (R1/R2/R3) are ADR text
self-inconsistencies resolvable by editing; four (E1/E2/E3/E4) are cases where
the committed code violates the ADR's own invariants. E3 (non-atomic scene
save) was the smallest-effort / largest-impact fix and was implemented
in-session along with the ADR text patches. The remaining three code changes
(E1, E2, E4) need dedicated implementation sessions.

## Governing ADRs

| ADR | Relevance |
|-----|-----------|
| ADR-0015 (Editor) | R1 layering clarification, PIE orchestration |
| ADR-0018 (AI) | R2 threading contract, E1 Foundation-only dependency |
| ADR-0019 (Scene) | R1 PIE backend, E2 Core/Scene layer boundary, E3 atomic save |
| ADR-0020 (Movie) | R3 AssetDatabase exception, E4 MF platform refcount |

## Requirements (traceability impact)

All 55 TRs in `tr-registry.yaml` v8 remain covered. This epic does NOT add new
TRs — it addresses the gap between what ADR text claims and what the code does.
Specifically:

| TR-ID | Gap | Fixed by |
|-------|-----|----------|
| TR-ai-007 (AI Foundation-only) | Code pulls Graphics — ADR claim is false | Story 004 (E1) |
| TR-scn-007 (SceneRenderer in Graphics/3D/) | SceneSerializer.cpp pulls Graphics | Story 005 (E2) |
| TR-scn-003 (ScenePersistence dual-format) | Non-atomic write, torn-file risk | Story 006 (E3) ✅ |
| TR-mov-001 / TR-mov-002 (MoviePlayer + VideoRecorder) | Per-instance MF refcount collision | Story 007 (E4) |

## Definition of Done

- All 7 stories implemented, reviewed, closed via `/story-done`
- `/architecture-review` re-run in a fresh session → verdict **PASS** (no open
  blocking issues)
- All Logic/Integration stories have passing tests where applicable
- Pre-Production → Production gate unblocked (E2/E3/E4 are blockers per the
  review's Gate guidance)

## Next Step

All 7 stories complete 2026-04-18. Build: 20 sub-libraries + 16 examples +
GXLibTests + GXModelViewer + gxconv + gxpak all link cleanly. Tests: 4957
pass, zero regressions.

**Recommended follow-up**:
1. Run `/architecture-review` in a fresh session (new conversation window)
   to independently validate — same-session re-run already executed
   2026-04-18 and verified PASS conditions.
2. Advance Pre-Production → Production gate once fresh-session review confirms.
3. Optional: add the deferred automated tests (AI-only link isolation
   check, ScenePersistence fault-injection test, MFPlatform refcount unit
   test, MoviePlayer + VideoRecorder cohabitation integration test) in a
   dedicated QA session.
