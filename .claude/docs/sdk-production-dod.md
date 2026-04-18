# SDK Production Stage — Definition of Done

GXLib is a library/SDK, not a game. Standard game-production gate criteria
(vertical slice playtest, player fantasy delivery, core-loop fun, content
pipeline, balance tuning, QA sign-off) do not map directly. This document
records the SDK-equivalent Production expectations so future gate checks,
sprint plans, and reviews don't flag N/A items as CONCERNS.

## Equivalent DoD criteria (game → SDK)

| Game DoD expectation | SDK equivalent | Measurement |
|----------------------|----------------|-------------|
| Vertical slice playable | Reference app + sample suite builds + runs without crashes | `GXModelViewer` + `examples/01-16` all launch; `cmake --build` green |
| Vertical slice playtested | Reference app exercises the subsystems end-to-end | 16 examples collectively cover 2D, 3D, input, audio, ECS, physics, animation, GUI, networking, IK, custom shaders, custom assets, custom audio DSP, custom widgets |
| Player fantasy delivered | Developer ergonomics: DXLib-sourced consumer can port in <1 day | `Compat/` layer coverage + ADR-0017 Two-Layer Accessibility L1 surface |
| Core-loop fun validated | Developer ergonomics validated via reference app | `GXModelViewer` as a working reference; "feel" is the SDK user's iteration speed |
| Content pipeline stable | Asset-DB schema + file formats stable; versioned on breaking change | ADR-0007 AssetDatabase contract; `.gxscene`/`.gxscbin` dual-format versioning |
| Balance tuning | Performance budgets locked per subsystem | `docs/architecture/architecture.md` §7 frame-budget table (main thread ≤ 7.2 ms / 16.6 ms) |
| QA sign-off by qa-lead | Test suite green + reference-app smoke + CHANGELOG entry + ADR for breaking change | 4957+ tests PASS + CHANGELOG.md discipline + ADR-0001 documentation strategy |
| Daily playtest | Daily clean build + test-suite run | CI: `.github/workflows/build.yml`; local: `cmake --build build --config Debug && GXLibTests.exe` |
| Scope creep detection | ADR drift (not GDD drift) | `/architecture-review` cadence; 4 fresh-session reviews to date catching 3-7 real issues each |
| Cut candidates | Defer features to later Phases (Phase timeline is the release train) | `docs/engine-reference/gxlib/VERSION.md` Phase Timeline |

## Production-stage sprint cadence

Expected artifacts per sprint (ratcheting stability, not feature count):
- At least 1 commit ratcheting ADR / CHANGELOG / reference-app / subsystem-smoke
- Test-suite regression count non-decreasing (4957+ baseline as of 2026-04-18)
- Sprint retrospective row appended to `production/session-state/active.md`
- Next sprint drafted before current sprint closes

## Gate-check recalibrations

Gate-check skill items that are automatically N/A for GXLib:
- `design/gdd/` — ADR-only project per ADR-0001
- `design/art/art-bible.md` — SDK has no visual art product
- `design/ux/` (including HUD, interaction patterns) — SDK has no user-facing UI; the developer-facing `GXLib/Editor/` ImGui panels are documented in ADR-0015 instead
- Character visual profiles — N/A
- Vertical-slice playtest sessions × 3 — replaced by reference-app exercise count
- `/review-all-gdds` — no GDDs to review; `/architecture-review` is the parallel for ADRs
- Player-journey document — N/A
- Difficulty curve doc — N/A
- Localization gates — N/A until any shipping game uses the SDK with localized strings (at which point the consumer game project has its own localization)

Gate-check items that still apply:
- ADR coverage, Foundation-layer completeness, architecture-traceability zero gaps
- Test framework + CI workflow + test regression green
- Control manifest current + every Accepted ADR's forbidden patterns captured
- Performance budgets defined + per-subsystem frame-time budgets locked
- Engine reference pinned, version-stamped, engine-compatibility sections in all ADRs

## Success criteria for the Pre-Production → Production stage flip

Met 2026-04-19:
- All 20 ADRs Accepted, 55 TRs 100% covered
- Zero REAL ISSUES from the latest fresh-session `/architecture-review`
- 4957/4957 tests PASS, zero regressions
- Reference app (`GXModelViewer`) + 16 examples all build cleanly
- Control manifest 2026-04-17 current against all ADRs
- First Production sprint (sprint-002) drafted before flip
