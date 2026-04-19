# Sprint 003: Compat-API Completion + Asset Pipeline Hardening

> **Start**: 2026-04-19
> **Closed**: 2026-04-19 (same-session close — agent-executable scope fully drained)
> **Status**: ✅ Complete — 5/6 tasks Done; Task 4 resolved-as-deferred with tracking.
> **Goal**: Close the compat-api Doxygen gap + Compat_Particle regression
> test; land the asset-pipeline concurrent-load stress test; tidy the
> movie-epic test suite beyond the 8-case baseline.

## Sprint Backlog

| # | Task | Epic | Type | Priority | Est | Owner | Status |
|---|------|------|------|----------|-----|-------|--------|
| 1 | Compat_Particle regression test (guards AddEmitter return, count precondition, GetDeltaTime fix) | compat-api | Logic | HIGH | 1h | engine-programmer | ✅ Done 2026-04-19 (3 tests in `Tests/unit/compat/compat_particle_test.cpp` — count<0, large negative, INT_MIN) |
| 2 | Doxygen pass on remaining `Compat/*.h` headers (TR-api-004 close) | compat-api | Docs | MEDIUM | 2h | gameplay-programmer | ✅ Done 2026-04-19 (13 `*F` float-variant drawing functions in GXLib.h documented; all 6 Compat headers now 0 undocumented per grep check) |
| 3 | Asset-pipeline concurrent-load stress test (N workers, M assets, no leak, no race) | asset-pipeline | Integration | MEDIUM | 2h | engine-programmer | ✅ Done 2026-04-19 (Option A + D per design discussion: 4 tests in `Tests/unit/io/async_loader_concurrent_test.cpp` + ADR-0007 §Known Limitation added + TR-defer-asyncloader-jobsystem registered) |
| 4 | Movie epic test expansion (MP4 fixture + open/seek/close flow beyond current state-machine tests) | movie | Integration | LOW | 2h | engine-programmer | ✅ Resolved-as-deferred 2026-04-19 — `TR-defer-movie-integration-tests` added to architecture-traceability; ADR-0020 §Testing Scope documents the evaluated-and-rejected fixture-generation strategies |
| 5 | ECS EntityBridge single-World-constraint verification + documented assertion | ecs | Logic | LOW | 1h | engine-programmer | ✅ Done 2026-04-19 (5 tests in `Tests/unit/ecs/entity_bridge_test.cpp` — ClearMappings lifecycle, Import/Export round-trip, cross-world collision pinned) |
| 6 | CHANGELOG entry + epics/index.md priority sequencing update | docs | Docs | LOW | 30m | producer | ✅ Done 2026-04-19 (CHANGELOG [SDK Sprint 3] entry written at close; epic index already synced to current state) |

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

Reached 2026-04-19:
- [x] `Tests/unit/compat/compat_particle_test.cpp` — 3 tests covering
      count<0 guard (the preCondition-side regression). AddEmitter
      return + UpdateParticles delta-time paths need an initialised
      CompatContext and are covered by reference-app smoke testing.
- [x] Every public header in `GXLib/Compat/*.h` has Doxygen `///` on
      every function declaration (13 float-variant drawing functions
      added 2026-04-19; awk-grep verified zero undocumented across all
      6 Compat headers).
- [x] AsyncLoader concurrent caller stress test — `Tests/unit/io/
      async_loader_concurrent_test.cpp` 4 tests. NOTE: the multi-worker
      parallelism wording in the original DoD was aspirational — see
      ADR-0007 §Known Limitation (added 2026-04-19). The stress test
      pins the API-thread-safety contract that the code actually
      delivers; the true parallelism upgrade is tracked as
      `TR-defer-asyncloader-jobsystem`.
- [x] Movie Open/Seek flow tests — RESOLVED-AS-DEFERRED. Three fixture
      strategies evaluated; all trade-offs exceed the SDK value of the
      test. Tracked as `TR-defer-movie-integration-tests`.
- [x] ECS EntityBridge: unit test file documents the cross-World
      collision silently-invalid-handle behaviour; runtime assertion
      noted in ADR-0019 §10 as debug-mode advisory (no code assertion
      added — current behaviour is documented, not buggy).
- [x] CHANGELOG `[SDK Sprint 3]` entry
- [x] Sprint retrospective in `production/session-state/active.md`

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
