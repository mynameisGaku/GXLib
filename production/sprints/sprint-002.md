# Sprint 002: Production Onboarding + Manual QA Close-Out

> **Start**: 2026-04-19 (pending Pre-Production → Production stage flip)
> **Duration**: 1-2 sessions
> **Goal**: First Production-stage sprint. Close sprint-001 carry-overs (manual QA), harden the Editor CI gate, close Audio epic's IXAPO runtime gap, draft the SDK-equivalent Production DoD.

## Sprint Backlog

| # | Task | Epic | Type | Priority | Est | Owner |
|---|------|------|------|----------|-----|-------|
| 1 | FontManager Detach() manual crash test (window-close AV check) | compat-api | Manual QA | HIGH | 10m | user (at PC) |
| 2 | IXAPO Process() runtime audio verification (real playback) | audio | Manual QA | HIGH | 30m | user (at PC) |
| 3 | GX_EDITOR=OFF CI gate — add headless build job to `.github/workflows/build.yml` | editor | Config | MEDIUM | 1h | engine-programmer |
| 4 | Editor boundary lint — CI grep rule for `editor_included_from_runtime` forbidden pattern | editor | Config | MEDIUM | 30m | engine-programmer |
| 5 | GUI UIContext Compat wrapper (beginner-layer accessor + example) | gui | Logic | MEDIUM | 2h | gameplay-programmer |
| 6 | Deferred test hardening batch: AI-only link test, ScenePersistence fault-inject, MFPlatform refcount unit test | arch-fixes-2026-04-18 | Logic | LOW | 3h | engine-programmer |

Tasks 1 + 2 are grouped as "manual-QA batch" — they unblock together when the user is at their PC.

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
- [ ] FontManager manual test: no AV on window close, no crash dump in `crashes/`
- [ ] IXAPO runtime audio: `Process()` confirmed called during playback (log or print evidence)
- [ ] `.github/workflows/build.yml` has a `GX_EDITOR=OFF` headless job that passes
- [ ] `editor_included_from_runtime` grep rule added to CI (fails if violated)
- [ ] `gx::` has a Compat accessor for UIContext (enables DXLib-shaped consumers to build GUI)
- [ ] At least 2 of the 3 deferred regression tests from arch-fixes-2026-04-18 epic landed
- [ ] Full test-suite regression: 4957+ tests PASS (no count shrinkage)
- [ ] Sprint retrospective entry in `production/session-state/active.md`

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
