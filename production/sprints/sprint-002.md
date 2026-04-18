# Sprint 002: Production Onboarding + Manual QA Close-Out

> **Start**: 2026-04-19
> **Status**: In progress — agent-executable tasks all ✅ Done 2026-04-19 (same session as start); 2 manual-QA items carry to user
> **Duration**: Will close when tasks 1+2 complete (blocked on user-at-PC)
> **Goal**: First Production-stage sprint. Close sprint-001 carry-overs (manual QA), harden the Editor CI gate, close Audio epic's IXAPO runtime gap, draft the SDK-equivalent Production DoD.

## Sprint Backlog

| # | Task | Epic | Type | Priority | Est | Owner | Status |
|---|------|------|------|----------|-----|-------|--------|
| 1 | FontManager Detach() manual crash test (window-close AV check) | compat-api | Manual QA | HIGH | 10m | user (at PC) | ⏳ Blocked on user |
| 2 | IXAPO Process() runtime audio verification (real playback) | audio | Manual QA | HIGH | 30m | user (at PC) | ⏳ Blocked on user |
| 3 | GX_EDITOR=OFF CI gate — headless build job | editor | Config | MEDIUM | 1h | engine-programmer | ✅ Done — `.github/workflows/build.yml` `build-no-editor` job present |
| 4 | Editor boundary lint — CI grep rule for `editor_included_from_runtime` | editor | Config | MEDIUM | 30m | engine-programmer | ✅ Done — `.github/workflows/build.yml` `lint-editor-boundary` job present (also checks `reflection_macro_in_header`) |
| 5 | GUI UIContext Compat wrapper (beginner-layer accessor) | gui | Logic | MEDIUM | 2h | gameplay-programmer | ✅ Done — `gx::GetUIContext()` declared in `GXLib/Compat/GXLib.h:835`, implemented in `Compat_System.cpp:142` |
| 6 | Deferred test hardening batch | arch-fixes-2026-04-18 | Logic | LOW | 3h | engine-programmer | ✅ Done 2026-04-19 — see breakdown below |

### Task 6 breakdown (all ✅ Done 2026-04-19)
- **MFPlatform refcount unit test** → `Tests/unit/core/mf_platform_test.cpp` (7 tests, PASS)
- **ScenePersistence atomicity test** → `Tests/unit/scene/scene_persistence_atomic_test.cpp` (5 tests, PASS — includes fault-injection-via-bad-path)
- **AI foundation-only link isolation test** → `Tests/isolation/ai_foundation_only/main.cpp` + `CMakeLists.txt` — new executable that links ONLY `GXLib_AI + GXLib_Core` (no `GXLib_Graphics`). Builds + runs cleanly; regressions in `AI/*.cpp` Graphics dependencies will now fail the link step at build time.

Tasks 1 + 2 remain as the "manual-QA batch" — they unblock together when the user is at their PC.

## Epic Sequencing (sprint-002 + beyond preview)

Producer recommendation: foundational epics first, then cross-cutting, then
presentation-layer polish. Sprint-002 scope is deliberately narrow (close
carry-overs + one new epic advance) to establish Production-stage cadence.

Lookahead (not committed):
- **Sprint-003 candidate**: compat-api Doxygen + Compat_Particle regression close; asset-pipeline concurrent-load stress test.
- **Sprint-004 candidate**: animation epic verification sweep (SetGlobalBusBridge delivered via EventBus story-004; needs epic-level status update); ecs epic (EntityBridge single-world constraint verification).
- **Sprint-005 candidate**: Presentation polish — remaining GUI widget coverage; Audio Voice bus hardening (ADR-0010 future-work VC placeholder).

Full 17-epic backlog in `production/epics/index.md`. Sequencing revisits at every sprint close.

## Definition of Done

Sprint is complete when:
- [ ] FontManager manual test: no AV on window close, no crash dump in `crashes/` — user-at-PC
- [ ] IXAPO runtime audio: `Process()` confirmed called during playback (log or print evidence) — user-at-PC
- [x] `.github/workflows/build.yml` has a `GX_EDITOR=OFF` headless job that passes (pre-existing)
- [x] `editor_included_from_runtime` grep rule added to CI (fails if violated) (pre-existing)
- [x] `gx::` has a Compat accessor for UIContext (enables DXLib-shaped consumers to build GUI) (pre-existing)
- [x] At least 2 of the 3 deferred regression tests landed → all 3 landed 2026-04-19
- [x] Full test-suite regression: 4969/4969 tests PASS (was 4957 + 12 new)
- [x] Sprint retrospective entry in `production/session-state/active.md` (appended each milestone)

Sprint-002 is 7/9 DoD green; remaining 2 items await user-at-PC.

## Carry-over from sprint-001

Already summarised in tasks 1 + 2 above.

## Risk Register

| ID | Risk | Severity | Mitigation |
|----|------|----------|------------|
| R1 | `origin` still points to CCGS template → first Production push fails or leaks | Medium | User reconfigures `git remote set-url origin <user-repo>` before sprint close |
| R2 | Manual-QA items (1, 2) stay blocked if user cannot sit at PC this week | Low | Acceptable — tasks are well-scoped and can carry into sprint-003 without blocking architecture or gameplay work |
| R3 | GUI UIContext Compat API surface requires design decisions not yet in ADR-0012 | Medium | If scope expands, split task 5 into design + impl; update ADR-0012 if API surface choices are non-trivial |
| R4 | Deferred regression tests (task 6) may reveal genuine bugs in E1/E2/E4 fixes | Low | Existing 4957-test green state is strong baseline; fault-injection tests are "test the test" rather than "test the fix" |

## Retrospective Seeds (populate at sprint close)

- Did the Production cadence rhythm hold (one quality-ratcheting artifact per sprint)?
- Did the 40-min manual-QA batch actually unblock, or did it slip again?
- Is the 17-epic "Ready" queue getting sequenced, or just sitting in `index.md`?
