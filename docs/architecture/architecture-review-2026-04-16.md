# Architecture Review — 2026-04-16

**Engine**: GXLib (self-hosted, Phase 5, pinned 2026-03-01)
**ADRs reviewed**: 10 (all `Proposed`)
**GDDs reviewed**: 0 (ADR-only project per ADR-0001)
**Verdict**: **CONCERNS** — coverage strong; 5 charter-level gaps remain (Networking, Input, GUI, Editor, Animation). No blocking conflicts.

---

## Traceability Summary

- Total requirements considered: 27 (22 ADR-derived TRs in `tr-registry.yaml` + 5 charter-level subsystem requirements from `VERSION.md`)
- ✅ Covered: 22 registered + 6 charter
- ❌ Charter gaps: 5

### Charter-Level Coverage

| TR (charter) | Domain | ADR | Status |
|---|---|---|---|
| TR-chr-001 PostFX out-of-the-box | Rendering | ADR-0008 | ✅ |
| TR-chr-002 Audio / Music / DSP | Audio | ADR-0010 | ✅ |
| TR-chr-003 Physics: rigid body, cloth, ragdoll, GJK/EPA | Physics | ADR-0009 | ✅ |
| TR-chr-004 Asset pipeline / Hot reload / DirectStorage | Core/IO | ADR-0007 | ✅ |
| TR-chr-006 Job System | Core | ADR-0006 | ✅ |
| TR-rnd-003 Deferred + Forward+ hybrid pipeline | Rendering | ADR-0008 | ✅ (was Partial in 2026-04-15 review) |
| TR-chr-005 Networking (Reliable UDP, replication, lag-comp) | Networking | — | ❌ GAP |
| TR-chr-007 Input (K/M + Gamepad + IME) | Input | — | ❌ GAP |
| TR-chr-008 GUI + ImGui integration | UI | — | ❌ GAP |
| TR-chr-009 Editor (PIE / Undo / Node Graph) | Editor | — | ❌ GAP |
| TR-chr-010 Animation (IK / blend trees / motion matching) | Animation | — | ❌ GAP |

All 22 registered ADR-derived TRs (`doc-001..003`, `rnd-001..005`, `api-001..004`, `ecs-001..005`, `scr-001..005`) remain ✅ Covered.

---

## Cross-ADR Conflict Detection

**No conflicts detected.** Specific cross-checks:

- **Performance budgets** stay within 16.6 ms total: Job ≤2 ms, Physics ≤3 ms, Rendering CPU+command-list ≤~4 ms, Audio ≤1.5 ms, AssetDB Get ≤0.1 μs amortised. Headroom preserved for game logic.
- **State ownership** is single-authority per subsystem. ECS components (ADR-0004) hold only opaque handles to physics bodies (ADR-0009 `BodyHandle`), assets (ADR-0007 `AssetHandle<T>`), and audio voices (ADR-0010 `VoiceHandle`) — no double ownership.
- **Interface contracts** are consistent: `AssetDatabase`, `JobSystem`, and `FrameGraph` are referenced uniformly by ADR-0008 (rendering), ADR-0009 (physics), and ADR-0010 (audio).
- **Threading model** is consistent: main-thread-only command queue submit (ADR-0002 / ADR-0008), Chase-Lev work-stealing on a single shared pool (ADR-0006), audio mix on XAudio2-owned thread with double-buffered params (ADR-0010), Lua main-thread-only execution (ADR-0005).
- **Hot reload** flow is consistent: AssetReloader (ADR-0007) drives texture, shader (ADR-0008), material (ADR-0009 `PhysicsMaterial`), sound (ADR-0010), and Lua module (ADR-0005) reloads through the same FileWatcher → reverse-map → typed reload-handler pipeline.

## ADR Dependency Order (topologically sorted)

**Foundation**
1. ADR-0001 — Documentation strategy
2. ADR-0002 — DX12 backend choice (deps: 0001)

**Core**
3. ADR-0006 — Job System (deps: 0001, 0002)
4. ADR-0007 — Asset Database + Hot Reload (deps: 0001, 0002, 0006)
5. ADR-0004 — Archetype-based ECS (deps: 0001)

**Systems**
6. ADR-0008 — Rendering Pipeline (deps: 0001, 0002, 0006, 0007)
7. ADR-0009 — Physics Architecture (deps: 0001, 0004, 0006)
8. ADR-0010 — Audio Architecture (deps: 0001, 0006, 0007, 0009)

**Compat / Script**
9. ADR-0003 — DXLib Compat API layer (deps: 0001, 0002)
10. ADR-0005 — Lua Scripting boundary (deps: 0001, 0004, 0006, 0007)

All `Depends On` references resolve to existing ADRs. **No dependency cycles.** Note: all ADRs are still `Proposed`; per `docs/CLAUDE.md` ADR lifecycle, stories cannot reference them until they are `Accepted`. Promotion to `Accepted` is the next gate after this review.

## Engine Compatibility

- All 10 ADRs include the Engine Compatibility section ✅
- Engine version (GXLib Phase 5, pinned 2026-03-01) referenced consistently ✅
- No deprecated API references (none documented in `docs/engine-reference/gxlib/`)
- No post-cutoff API conflicts — all decisions sit on stable, training-data-covered surfaces (DX12, XAudio2, C++20, Lua 5.4 + sol2, OGG/Vorbis, GJK/EPA, Verlet, Chase-Lev, FNV-1a, Freeverb, etc.)

### Engine Specialist Consultation
Skipped — review run in same session as authoring; specialist re-validation should run in a fresh session before promoting ADRs to `Accepted`.

## GDD Revision Flags
N/A — ADR-only project per ADR-0001 (no GDDs in scope).

## Architecture Document Coverage
`docs/architecture/architecture.md` not present. All architectural decisions live in individual ADRs per the ADR-only strategy (ADR-0001). No orphans.

---

## Verdict: CONCERNS

The architecture is internally consistent and free of conflicts, with foundation, core, and major systems (rendering, physics, audio, ECS, scripting, compat, asset/IO, job system) covered. Five charter-level subsystems still have working code with no codified ADR — they should be retroactively documented before pre-production work depends on their behaviour.

### Blocking Issues
None — all gaps are retroactive (code already shipped).

### Required ADRs (priority order)
1. **ADR-0011 Input Architecture** — Keyboard/Mouse + Gamepad (vibration) + IMM32 IME. Closes TR-chr-007.
2. **ADR-0012 GUI + ImGui Integration** — Widget system, XML/CSS loader, ImGui editor backend. Closes TR-chr-008.
3. **ADR-0013 Networking** — Reliable UDP channel, NetworkReplicator, lag compensation, rollback netcode. Closes TR-chr-005.
4. **ADR-0014 Animation Pipeline** — Skeleton, AnimationLayer, BlendTree, IK (FullBody/FootIK/LookAt), Motion Matching, Spring Bone. Closes TR-chr-010.
5. **ADR-0015 Editor / Play-in-Editor** — PIE simulation freeze/resume, Undo/Redo, Node Graph editor, Reflection-driven Property Inspector. Closes TR-chr-009.

After all five are written, re-run `/architecture-review` in a fresh session to validate full coverage, then run `/gate-check pre-production` (or skip directly to ADR `Accepted` promotion since this is an SDK-without-stories project).

---

## Related
- `docs/architecture/adr-0001-documentation-strategy.md` … `adr-0010-audio-architecture.md`
- `docs/architecture/tr-registry.yaml`
- `docs/architecture/architecture-traceability.md`
- `docs/architecture/architecture-review-2026-04-15.md` (prior review — 5 ADRs, verdict CONCERNS, 7 gaps)
