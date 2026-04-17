# Architecture Traceability Index

> **Last Updated**: 2026-04-17 (overnight autonomous pass — ADR-0015 + ADR-0016)
> **Engine**: Custom — GXLib (Phase 5)
> **Source of requirements**: ADR `## Context → Requirements` sections + charter-level subsystem needs (ADR-only project per ADR-0001; no GDDs)

## Coverage Summary

| Status | Count | % |
|--------|-------|---|
| ✅ Covered | 45 | 100% |
| ⚠️ Partial | 0 | 0% |
| ❌ Gap | 0 | 0% |
| **Total** | **45** | **100%** |

## Full Matrix

(See `architecture-review-2026-04-16b.md` for the last authoritative coverage matrix; ADR-derived TRs in `tr-registry.yaml`. This overnight pass adds ADR-0015 + ADR-0016, closing the two remaining charter gaps and lifting ADR-0013 §13's forward-declaration to a concrete ADR binding.)

### Newly-covered requirements (2026-04-17)

| TR-ID | Requirement | ADR | Notes |
|-------|-------------|-----|-------|
| TR-chr-009 | Editor (Play-in-Editor / Undo/Redo / Node Graph / Reflection / panels) | ADR-0015 | Retroactive; GX_EDITOR=OFF shipping exclusion contract codified |
| TR-edit-pie | Play-in-Editor state machine + snapshot/restore | ADR-0015 §3, §4 | Shallow snapshot (transform/active/name); deep snapshot deferred to future |
| TR-edit-undo | Command-pattern Undo/Redo stack | ADR-0015 §5 | Core-layer, not editor-exclusive; forbidden during rollback replay mode |
| TR-edit-reflection | Macro-registered runtime type reflection | ADR-0015 §6, §7 | `GX_REFLECT_*` macros; JsonSerializer uses same registry |
| TR-edit-nodegraph | Visual-scripting NodeGraph runtime | ADR-0015 §8 | Peer to ADR-0005 Lua; same gameplay-authority rules |
| TR-core-eventbus-core | EventBus type-indexed pub/sub (existing) | ADR-0016 §1-§6 | Header-only singleton, main-thread-only, insertion-order dispatch |
| TR-core-eventbus-replay | EventBus replay-suppression contract | ADR-0016 §3, §4 | Lifts ADR-0013 §13 forward-declaration; HandlerCategory enum; SetReplayMode |
| TR-core-eventbus-worker | EventBus worker-thread production route | ADR-0016 §5 | QueueFromWorker SPSC lane; Fire from worker is a forbidden pattern |
| TR-anim-eventbus-bridge | AnimationEventDispatcher → global bus bridge | ADR-0016 §7 | SetGlobalBusBridge(bool); AnimationEventFired event type |

## Known Gaps

None at the charter level. Forward-looking / deferred items tracked below.

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
| TR-defer-game-shipping-preset | `game-shipping` CMake preset (GX_EDITOR=OFF + LTCG) | Before first non-internal shipping build | ADR-0015 Migration Plan |
| TR-defer-ecs-ai-bridge | ECS-AI bridge components (BTComponent / NavAgentComponent) | When a game needs 1000+ AI agents batch-ticked via ECS + JobSystem | ADR-0018 Migration Plan |
| TR-defer-full-orca | Full ORCA LP solver for RVO | When crowd density exceeds 100 agents and oscillation is visible | ADR-0018 §7 simplified model |
| TR-defer-recast-generation | Automated navmesh generation from 3D scene geometry | When manual triangle-mesh authoring becomes impractical | ADR-0018 Migration Plan |
| TR-defer-scene-architecture | Scene subsystem ADR (SceneManager / Prefabs / Persistence) | Medium priority retroactive — code exists since Phase 2 | Architecture review 2026-04-17 subsystem gap |
| TR-defer-movie-pipeline | Movie subsystem ADR (video playback / recording) | Low priority retroactive — code exists since Phase 4 | Architecture review 2026-04-17 subsystem gap |

## Superseded Requirements

None.

## History

| Date | Covered % | Notes |
|------|-----------|-------|
| 2026-04-15 | 69% | Initial review after 5 foundation ADRs (0001–0005) |
| 2026-04-16 | 85% | After ADR-0006 (Job), 0007 (AssetDB), 0008 (Rendering), 0009 (Physics), 0010 (Audio); 5 charter gaps remain (Networking, Input, GUI, Editor, Animation) |
| 2026-04-16 (run 2) | 94% | After ADR-0011 (Input), 0012 (GUI), 0013 (Networking); 2 charter gaps remain (Editor, Animation, both retroactive) |
| 2026-04-16 (run 3 — TD review) | 94% | ADR-0014 (Animation) closes Animation; ADR-0013 §13 forward-declares EventBus ADR (pending) |
| 2026-04-17 (overnight autonomous) | 100% | ADR-0015 (Editor) + ADR-0016 (EventBus) close all remaining charter gaps and lift the ADR-0013 §13 forward-declaration. |
| 2026-04-17 (fresh-session review) | 100% | ADR-0015 + ADR-0016 patched (engine specialist findings), promoted to Accepted. All 17 ADRs Accepted. |
| 2026-04-17 (ADR-0018 AI) | 100% (45 TRs) | ADR-0018 AI Architecture (Accepted). +7 TRs (ai-001..007). Subsystem gaps reduced to Scene + Movie. |
