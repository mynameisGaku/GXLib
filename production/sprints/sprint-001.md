# Sprint 001: EventBus Complete + Core Verification

> **Start**: 2026-04-18
> **Duration**: 1 week
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

### Remaining This Sprint

| # | Task | Epic | Type | Priority | Est |
|---|------|------|------|----------|-----|
| 6 | FontManager Detach() manual crash test | compat-api | Manual | HIGH | 10m |
| 7 | MoviePlayer basic test coverage | movie | Logic | MEDIUM | 2h |
| 8 | GX_EDITOR=OFF local build verification | editor | Config | MEDIUM | 30m |
| 9 | IXAPO Process() runtime audio verification | audio | Manual | MEDIUM | 30m |
| 10 | Full test suite regression (4921+ tests) | all | Config | LOW | 15m |

## Velocity

- Stories completed this session: 5 (EventBus)
- Examples added: 16 total (7 new this session: 10-16)
- ADRs written: 6 (0015-0020)
- Engine API additions: 4 (GetShaderRegistry, GetAudioManager, GetNetworkManager, GetUIContext)
- Tests added: 30+ new test cases

## Definition of Done

Sprint is complete when:
- [x] EventBus epic: all 5 stories Done
- [ ] FontManager: manual test confirms no AV on window close
- [ ] MoviePlayer: at least state-machine tests exist
- [ ] GX_EDITOR=OFF: local cmake build passes
- [ ] Full test suite: 4921+ tests PASS

## Blockers

- Push to remote blocked: origin points to CCGS template (Donchitos), not user's repo
- FontManager test requires user at PC with DX12 display

## Retrospective Notes

- EventBus implementation went smoothly: 5 stories in ~2 hours
- Architecture phase (20 ADRs) was the largest investment, now paying off with clear story contracts
- Scorecard improved from 50%/36% → 92%/92% across two sessions
- 22+ commits in this session alone
