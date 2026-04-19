# Sprint 003: Compat-API Completion + Asset Pipeline Hardening

> **Start**: 2026-04-19
> **Status**: In progress — Tasks 1/2/5 ✅ Done 2026-04-19; Tasks 3/4 carry to sprint-004 (non-blocking).
> **Duration**: 1-2 sessions
> **Goal**: Close the compat-api Doxygen gap + Compat_Particle regression
> test; land the asset-pipeline concurrent-load stress test; tidy the
> movie-epic test suite beyond the 8-case baseline.

## Sprint Backlog

| # | Task | Epic | Type | Priority | Est | Owner | Status |
|---|------|------|------|----------|-----|-------|--------|
| 1 | Compat_Particle regression test (guards AddEmitter return, count precondition, GetDeltaTime fix) | compat-api | Logic | HIGH | 1h | engine-programmer | ✅ Done 2026-04-19 (3 tests in `Tests/unit/compat/compat_particle_test.cpp` — count<0, large negative, INT_MIN) |
| 2 | Doxygen pass on remaining `Compat/*.h` headers (TR-api-004 close) | compat-api | Docs | MEDIUM | 2h | gameplay-programmer | ✅ Done 2026-04-19 (13 `*F` float-variant drawing functions in GXLib.h documented; all 6 Compat headers now 0 undocumented per grep check) |
| 3 | Asset-pipeline concurrent-load stress test (N workers, M assets, no leak, no race) | asset-pipeline | Integration | MEDIUM | 2h | engine-programmer | ✅ Done 2026-04-19 (Option A + D per design discussion: 4 tests in `Tests/unit/io/async_loader_concurrent_test.cpp` + ADR-0007 §Known Limitation added + TR-defer-asyncloader-jobsystem registered) |
| 4 | Movie epic test expansion (MP4 fixture + open/seek/close flow beyond current state-machine tests) | movie | Integration | LOW | 2h | engine-programmer | ⏳ Carry to sprint-004 (needs MP4 fixture generation strategy) |
| 5 | ECS EntityBridge single-World-constraint verification + documented assertion | ecs | Logic | LOW | 1h | engine-programmer | ✅ Done 2026-04-19 (5 tests in `Tests/unit/ecs/entity_bridge_test.cpp` — ClearMappings lifecycle, Import/Export round-trip, cross-world collision pinned) |
| 6 | CHANGELOG entry + epics/index.md priority sequencing update | docs | Docs | LOW | 30m | producer | ⏳ Partial — CHANGELOG [SDK Sprint 2] covers through this session; Sprint 3 entry deferred until sprint-003 fully closes with tasks 3+4 |

## Epic sequencing rationale

Sprint-003 consolidates the Foundation layer: the compat-api epic has
been "Ready" since inception but has outstanding Doxygen + regression-
test gaps, and the asset-pipeline epic's concurrent-load stress test was
called out as a gap from sprint-001 close. Both harden the SDK's public
contract before Production-stage feature work.

**Deliberately NOT in sprint-003**: the 2 carry-over manual-QA items
from sprint-002 (FontManager, IXAPO). They carry forward as a separate
"sprint-002 close-out" milestone whenever the user is at a PC; they do
not block sprint-003 start.

## Lookahead (not committed)

- **Sprint-004 candidate**: Animation epic verification (done? yes —
  may skip), ECS batch-AI-tick bridge components (TR-defer-ecs-ai-bridge
  promotion if triggers met).
- **Sprint-005 candidate**: Presentation polish — GUI widget catalogue
  expansion, Audio ReverbZone smoke tests, scene streaming cadence
  verification.
- **Beyond**: Tier-2 subsystem production work (real STUN server wiring,
  CloudSave backend implementation, dedicated-server headless build).
  These are listed as TR-defer-* items in
  `docs/architecture/architecture-traceability.md`.

Full 17-epic backlog in `production/epics/index.md`.

## Definition of Done

- [ ] `Tests/unit/compat/compat_particle_regression_test.cpp` — at minimum
      3 tests covering: AddEmitter return-value guard, count precondition,
      UpdateParticles delta-time correctness
- [ ] Every public header in `GXLib/Compat/*.h` has Doxygen `///` on
      every function declaration (grep-verifiable — see Task 2 AC)
- [ ] `Tests/integration/asset_pipeline/concurrent_load_test.cpp` — spawns
      ≥4 Jobs that each load ≥10 assets; assertion: no duplicate load, no
      leak (AssetDatabase refcount balanced), no TSan failure
- [ ] `Tests/unit/movie/movie_player_test.cpp` extended with Open/Seek
      flow tests using a small test MP4 fixture (add
      `Tests/fixtures/movie/tiny.mp4` ≤ 100 KB)
- [ ] ECS EntityBridge: runtime assertion + unit test that attaching
      from 2 distinct Worlds to the same Entity is detected
- [ ] CHANGELOG `[SDK Sprint 3]` entry
- [ ] Sprint retrospective in `production/session-state/active.md`

## Risk register

| ID | Risk | Severity | Mitigation |
|----|------|----------|------------|
| R1 | Test MP4 fixture bloats the repo | Low | Generate fixture programmatically in CMake if feasible; else use a 3-frame BMP→MP4 converter tool; cap fixture at 100 KB |
| R2 | Asset concurrent-load test is inherently racy; flake risk | Medium | Run stress test N=100 iterations in CI; flaky-test quarantine policy via `/test-flakiness` if flake rate > 1% |
| R3 | Doxygen pass finds undocumented APIs that require design clarification | Low | Document what is; open follow-up story if semantics need ADR revision |
| R4 | Sprint-002 carry-overs (FontManager/IXAPO) still pending at sprint-003 close | Low | Acceptable — non-blocking by construction; user at PC resolves opportunistically |

## Carry-over to sprint-004 (if sprint-003 overruns)

- Task 5 (ECS EntityBridge verification) has the lowest architectural
  urgency; safe to defer.
- Task 4 (MP4 fixture) can slip to sprint-004 if fixture authoring proves
  non-trivial.
- Tasks 1-3 are the hard sprint-003 commitments.

## Retrospective seeds

- How long did the Doxygen pass actually take vs. 2h estimate?
- Is the concurrent-load stress test catching real bugs, or is it
  ceremonial coverage?
- Which carry-over pattern is sustainable (2-3 manual-only items per
  sprint) vs. accumulating?
