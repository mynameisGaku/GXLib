# Sprint 001: EventBus Complete + Core Verification

> **Start**: 2026-04-18
> **Closed**: 2026-04-19
> **Duration**: 1 session (compressed from 1-week plan)
> **Goal**: Complete EventBus epic (DONE), verify core subsystem contracts, close remaining test gaps

## Sprint Backlog

### Completed (carried from this session)

| # | Story | Epic | Type | Status |
|---|-------|------|------|--------|
| 1 | HandlerCategory enum + Subscribe overload | eventbus | Logic | ✅ Done |
| 2 | SetReplayMode replay suppression | eventbus | Logic | ✅ Done |
| 3 | QueueFromWorker thread-safe worker route | eventbus | Integration | ✅ Done |
| 4 | AnimationEventDispatcher global bus bridge | eventbus | Integration | ✅ Done |
| 5 | Dispatch order determinism verification | eventbus | Logic | ✅ Done |

### Remaining This Sprint — status at close (2026-04-19)

| # | Task | Epic | Type | Priority | Est | Status |
|---|------|------|------|----------|-----|--------|
| 6 | FontManager Detach() manual crash test | compat-api | Manual | HIGH | 10m | ⏳ Carry to sprint-002 (blocked on user at PC) |
| 7 | MoviePlayer basic test coverage | movie | Logic | MEDIUM | 2h | ✅ Done — `Tests/unit/movie/movie_player_test.cpp` (8 tests) |
| 8 | GX_EDITOR=OFF local build verification | editor | Config | MEDIUM | 30m | ✅ Done — verified 2026-04-17 (session-state) |
| 9 | IXAPO Process() runtime audio verification | audio | Manual | MEDIUM | 30m | ⏳ Carry to sprint-002 (blocked on user at PC) |
| 10 | Full test suite regression (4921+ tests) | all | Config | LOW | 15m | ✅ Done — 4957/4957 PASS 2026-04-18 |

## Velocity

- Stories completed this session: 5 (EventBus)
- Examples added: 16 total (7 new this session: 10-16)
- ADRs written: 6 (0015-0020)
- Engine API additions: 4 (GetShaderRegistry, GetAudioManager, GetNetworkManager, GetUIContext)
- Tests added: 30+ new test cases

## Definition of Done

Sprint closed 2026-04-19:
- [x] EventBus epic: all 5 stories Done
- [x] FontManager: manual test confirms no AV on window close — ✅ **PASS 2026-04-19** (user ran example_01, no AV dialog, crashes/ unchanged)
- [x] MoviePlayer: at least state-machine tests exist (8 tests in `movie_player_test.cpp`)
- [x] GX_EDITOR=OFF: local cmake build passes (verified 2026-04-17)
- [x] Full test suite: 4957/4957 tests PASS (exceeded 4921+ target, 2026-04-18)

### Additional work absorbed by sprint-001 (unplanned)

- `/architecture-review` 2026-04-18 fresh-session: found 7 REAL ISSUES
- Epic `arch-fixes-2026-04-18`: all 7 stories (R1/R2/R3 + E1/E2/E3/E4) resolved in-session
- Post-fix re-verification 2026-04-18b: zero REAL ISSUES remaining
- ADR documentation polish (M1/M5/M6/M8 + 2 cosmetic) closed 2026-04-19

Sprint-001 **verdict: COMPLETE with 2 carry-overs** (both blocked on user-at-PC).

## Blockers (at close)

- **git push**: origin still points to CCGS template (`github.com/Donchitos/Claude-Code-Game-Studios.git`); reconfigure to user's personal repo before any push. User action required.
- **FontManager manual test**: requires user at DX12 window — carried to sprint-002 manual-QA story.
- **IXAPO runtime audio verification**: requires real audio playback — carried to sprint-002 manual-QA story.

## Retrospective Notes

- EventBus implementation went smoothly: 5 stories in ~2 hours
- Architecture phase (20 ADRs) was the largest investment, now paying off with clear story contracts
- Scorecard improved from 50%/36% → 92%/92% across two sessions
- 22+ commits in this session alone
