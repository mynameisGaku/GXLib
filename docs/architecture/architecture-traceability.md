# Architecture Traceability Index

> **Last Updated**: 2026-04-18 (fresh-session review — 3 independent agents)
> **Engine**: Custom — GXLib (Phase 5)
> **Source of requirements**: ADR `## Context → Requirements` sections + charter-level subsystem needs (ADR-only project per ADR-0001; no GDDs)

## Coverage Summary

| Status | Count | % |
|--------|-------|---|
| ✅ Covered | 55 | 100% |
| ⚠️ Partial | 0 | 0% |
| ❌ Gap | 0 | 0% |
| **Total (registry v8)** | **55** | **100%** |

## Full Matrix

The complete TR × ADR matrix is maintained in:
- `tr-registry.yaml` — stable TR IDs, source ADR, requirement text
- `architecture-review-2026-04-18.md` — latest authoritative coverage matrix with issue flags

## Known Gaps

**None at the charter level.** All 10 charter subsystems (Documentation, Rendering,
DXLib Compat, ECS, Scripting, Job System, Assets, Physics, Audio, Input, GUI,
Networking, Animation, Editor, EventBus, AI, Scene, Movie) have Accepted ADRs with
registered TRs.

## Open Blocking Issues — ✅ ALL RESOLVED 2026-04-18b

The 2026-04-18 review flagged 7 REAL ISSUES. All closed in-session 2026-04-18
with build + 4957-test verification:

| Tag | ADR(s) | Nature | Fix type | Status |
|-----|--------|--------|----------|--------|
| R1 | 0015 + 0019 | Dual PIE state-machine ownership | ADR text | ✅ Closed 2026-04-18 |
| R2 | 0018 §9 | Threading contract self-contradicts | ADR text | ✅ Closed 2026-04-18 |
| R3 | 0020 | Bypasses ADR-0007 AssetDatabase, undeclared | ADR text | ✅ Closed 2026-04-18 |
| E1 | 0018 | `NavMesh.cpp/NavMesh3D.cpp` pull Graphics/ (violates Foundation-only) | Code + CMake + ADR | ✅ Closed 2026-04-18 (new `GXLib_AIDebug` target) |
| E2 | 0019 | `SceneSerializer.cpp` includes `Graphics/3D/GraphicsComponents.h` | Code + CMake | ✅ Closed 2026-04-18 (file relocated to `Graphics/3D/`) |
| E3 | 0019 | `ScenePersistence::SaveToFile` non-atomic write | Code + ADR | ✅ Closed 2026-04-18 (temp-rename pattern) |
| E4 | 0020 | `MFStartup/MFShutdown` per-instance → process-wide refcount collision | Code + ADR | ✅ Closed 2026-04-18 (new `Core/MFPlatform`) |

Full details: `architecture-review-2026-04-18.md` (findings) + `architecture-review-2026-04-18b.md` (post-fix re-verification).

## Deferred polish (non-blocking, optional)

4 minor concerns + 2 cosmetic items remain from the 2026-04-18 review. None
block gate advancement. Listed in `architecture-review-2026-04-18b.md`
"Remaining minor items" section. Addressable in a future documentation-polish
session if desired.

## Deferred / Forward-Looking Items

These are NOT charter gaps — they are future ADR topics flagged for tracking only:

| TR-ID | Topic | Trigger | Notes |
|-------|-------|---------|-------|
| TR-defer-pie-deep-snapshot | PIE deep snapshot (audio/particle/network/full ECS) | When authoring scope outgrows transform-only | ADR-0015 §4 documents the current shallow contract |
| TR-defer-encrypted-udp | Encrypted-UDP gameplay (DTLS/noise) | Before first public multiplayer title | ADR-0013 §14 flags this as a future ADR |
| TR-defer-dedicated-server | Dedicated server framework | Before first title requiring headless server build | ADR-0013 Migration Plan |
| TR-defer-voice-chat | Voice chat over ADR-0010 Voice bus | Before first VC-using title | ADR-0013 Migration Plan + ADR-0010 |
| TR-defer-anticheat | Server-authoritative anti-cheat | Before any competitive public multiplayer | ADR-0013 Migration Plan |
| TR-defer-cross-platform | Non-Windows socket backend | Before any non-Windows port | ADR-0013 Migration Plan + ADR-0002 Windows-only scope |
| TR-defer-remote-editor | Remote editor (separate process via IPC) | When team / platforms diversify | ADR-0015 Alternative 2 rejection rationale |
| TR-defer-ecs-ai-bridge | ECS-AI bridge components (BTComponent / NavAgentComponent) | When a game needs 1000+ AI agents batch-ticked via ECS + JobSystem | ADR-0018 Migration Plan |
| TR-defer-full-orca | Full ORCA LP solver for RVO | When crowd density exceeds 100 agents and oscillation is visible | ADR-0018 §7 simplified model |
| TR-defer-recast-generation | Automated navmesh generation from 3D scene geometry | When manual triangle-mesh authoring becomes impractical | ADR-0018 Migration Plan |

### Promoted from deferred to active follow-up (2026-04-18)

| TR-ID | Topic | Action |
|-------|-------|--------|
| TR-defer-game-shipping-preset | `game-shipping` CMake preset (GX_EDITOR=OFF + LTCG) | **Active follow-up** — ADR-0015 §11 Migration Plan step 2 declares this as immediate work. Assign to the next SDK infra sprint. |

### Completed and removed from deferred list

| TR-ID | Topic | Closed by | Date |
|-------|-------|-----------|------|
| ~~TR-defer-scene-architecture~~ | Scene subsystem ADR | ADR-0019 | 2026-04-17 |
| ~~TR-defer-movie-pipeline~~ | Movie subsystem ADR | ADR-0020 | 2026-04-17 |

## Superseded Requirements

None.

## History

| Date | Covered % | Notes |
|------|-----------|-------|
| 2026-04-15 | 69% | Initial review after 5 foundation ADRs (0001–0005) |
| 2026-04-16 | 85% | After ADR-0006 (Job), 0007 (AssetDB), 0008 (Rendering), 0009 (Physics), 0010 (Audio); 5 charter gaps remain |
| 2026-04-16 (run 2) | 94% | After ADR-0011 (Input), 0012 (GUI), 0013 (Networking); Editor + Animation retroactive gaps |
| 2026-04-16 (run 3 — TD review) | 94% | ADR-0014 (Animation); ADR-0013 §13 forward-declares EventBus |
| 2026-04-17 (overnight autonomous) | 100% | ADR-0015 (Editor) + ADR-0016 (EventBus) close all charter gaps |
| 2026-04-17 (fresh-session review) | 100% | ADR-0015/0016 patched per engine specialist findings, all 17 Accepted |
| 2026-04-17 (ADR-0018 AI) | 100% (45 TRs) | ADR-0018 AI Architecture; Scene + Movie subsystem gaps remain |
| 2026-04-17 (ADR-0019/0020) | 100% (55 TRs) | Scene + Movie ADRs close remaining subsystem gaps. Registry v7. |
| 2026-04-18 (fresh-session review) | 100% (55 TRs) | Coverage stable. 7 REAL ISSUES + 8 MINOR CONCERNS detected in ADR-0018/0019/0020 (first independent audit). Verdict: CONCERNS. See `architecture-review-2026-04-18.md`. |
| 2026-04-18b (post-fix re-verification) | 100% (55 TRs) | All 7 REAL ISSUES resolved in-session (R1/R2/R3 ADR-text + E1/E2/E3/E4 code+CMake+ADR). Build ✅ (20 sub-libs, 16 examples, GXModelViewer) + 4957 tests pass. Verdict: **CONCERNS (gate-unblocked)** — 4 minor + 2 cosmetic documentation items remain, none block advancement. See `architecture-review-2026-04-18b.md`. |
