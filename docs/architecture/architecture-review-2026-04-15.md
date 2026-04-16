# Architecture Review — 2026-04-15

> **Mode**: full (in-session caveat — ran in same session as ADR authoring; independence limited)
> **Engine**: Custom — GXLib (Phase 5)
> **GDDs reviewed**: 0 (intentional per ADR-0001)
> **ADRs reviewed**: 5 (all `Proposed`)

---

## Traceability Summary

| Metric | Count |
|--------|-------|
| Total architectural requirements | 29 |
| ✅ Covered | 20 |
| ⚠️ Partial | 2 |
| ❌ Gaps | 7 |

Requirements were extracted from each ADR's `## Context → Requirements` section, supplemented by charter-level subsystem requirements implied by `docs/engine-reference/gxlib/VERSION.md` subsystem map.

## Full Traceability Matrix

| TR-ID | Source | Requirement | Coverage | Status |
|-------|--------|-------------|----------|--------|
| TR-doc-001 | ADR-0001 | Template skills operate without GDDs | ADR-0001 | ✅ |
| TR-doc-002 | ADR-0001 | Traceability from decisions to implementation | ADR-0001 | ✅ |
| TR-doc-003 | ADR-0001 | Scales to future subsystem additions | ADR-0001 | ✅ |
| TR-rnd-001 | ADR-0002 | 60 fps @ 1080p, mid-range GPU, full pipeline | ADR-0002 | ✅ |
| TR-rnd-002 | ADR-0002 | HDR10/scRGB, VRS, Mesh Shaders, Sampler Feedback, DirectStorage, DXR | ADR-0002 | ✅ |
| TR-rnd-003 | ADR-0002 | Deferred + Forward+ hybrid pipeline | ADR-0002 (mention only) | ⚠️ Partial |
| TR-rnd-004 | ADR-0002 | Graceful feature fallback via caps check | ADR-0002 + forbidden `unchecked_optional_gpu_feature` | ✅ |
| TR-rnd-005 | ADR-0002 | DX12 types stay out of public headers | ADR-0002 + forbidden `dx12_type_in_public_header` | ✅ |
| TR-api-001 | ADR-0003 | Port DXLib program with minimal change | ADR-0003 | ✅ |
| TR-api-002 | ADR-0003 | DXLib return-code semantics (0/-1) | ADR-0003 | ✅ |
| TR-api-003 | ADR-0003 | Compat + class API coexist | ADR-0003 | ✅ |
| TR-api-004 | ADR-0003 | GXLib-exclusive features (HDR, VRS, PostFX) reachable from Compat | ADR-0003 + ADR-0002 | ✅ |
| TR-ecs-001 | ADR-0004 | 100k entities @ 60fps ≤ 2 ms/frame | ADR-0004 + budget | ✅ |
| TR-ecs-002 | ADR-0004 | Runtime component composition | ADR-0004 | ✅ |
| TR-ecs-003 | ADR-0004 | Query filtered by component set | ADR-0004 | ✅ |
| TR-ecs-004 | ADR-0004 | OOP Node ↔ ECS Entity bridge | ADR-0004 + interface `oop_ecs_entity_bridge` | ✅ |
| TR-ecs-005 | ADR-0004 | Lua can spawn/query via World | ADR-0004 + ADR-0005 | ✅ |
| TR-scr-001 | ADR-0005 | Hot-reloadable scripts | ADR-0005 | ✅ |
| TR-scr-002 | ADR-0005 | Sandboxed untrusted mods | ADR-0005 + forbidden `lua_sandbox_escape` | ✅ |
| TR-scr-003 | ADR-0005 | Lua→C++ trivial call ≤ 1 μs | ADR-0005 + budget | ✅ |
| TR-scr-004 | ADR-0005 | Deterministic call cost (no JIT surprises) | ADR-0005 | ✅ |
| TR-scr-005 | ADR-0005 | Script surface does not leak internals | ADR-0005 + forbidden `unregistered_script_binding_leak` | ✅ |
| TR-chr-001 | Charter | Post-effects / screen effects out-of-the-box | implied by ADR-0002 | ⚠️ Partial |
| TR-chr-002 | Charter | Audio / Music / DSP effects | — | ❌ Gap |
| TR-chr-003 | Charter | Physics (rigid body, cloth, ragdoll, GJK/EPA) | — | ❌ Gap |
| TR-chr-004 | Charter | Asset pipeline / Hot reload / DirectStorage flow | — | ❌ Gap |
| TR-chr-005 | Charter | Networking (Reliable UDP, replication) | — | ❌ Gap |
| TR-chr-006 | Charter | Job System / multi-threaded simulation | — | ❌ Gap |
| TR-chr-007 | Charter | Input (K/M, Gamepad with vibration, IME) | — | ❌ Gap |
| TR-chr-008 | Charter | GUI widget system + ImGui integration | — | ❌ Gap |

## Cross-ADR Conflicts

None detected.

- State ownership: only `graphics_device` registered; single owner (Graphics/Device)
- Interface contracts: ADR-0005 Lua surface correctly mirrors ADR-0003 Compat surface and ADR-0004 World API — no divergence
- Performance budgets: registered budgets (DX12 implicit full frame + ECS 2 ms + Script 1 ms) do not exceed 16.6 ms; no explicit CPU-split conflict
- Dependency cycles: none
- Architecture patterns: additive, non-overlapping
- Forbidden patterns: 9 registered, no overlap or contradiction

## ADR Dependency Order (topological)

```
Foundation:
  1. ADR-0001 — Documentation strategy

Depends on Foundation:
  2. ADR-0002 — DX12 backend choice

Depends on ADR-0001 + ADR-0002:
  3. ADR-0003 — DXLib-compatible procedural API
  4. ADR-0004 — Archetype-based ECS

Depends on ADR-0001 + ADR-0004:
  5. ADR-0005 — Lua scripting boundary
```

All 5 ADRs are currently `Proposed`. Stories referencing any of them will be auto-blocked by `/story-readiness` until Accepted. Recommendation: batch-accept after this review when comfortable with the decisions.

## Engine Compatibility

- All 5 ADRs carry the Engine Compatibility section
- `Post-Cutoff APIs Used` = "None" across all ADRs
- Version pin consistent: GXLib Phase 5 / 2026-04-15
- No deprecated-API references (no `deprecated-apis.md` — not applicable for self-hosted engine)

## GDD Revision Flags

N/A — no GDDs exist by design (ADR-0001). This section becomes relevant only if GDDs are introduced for sample games under `sdk/` or `examples/` per ADR-0001 Risks mitigation.

## Architecture Document Coverage

`docs/architecture/architecture.md` does not exist. Not required at 5 ADRs; becomes valuable once the ~8 missing subsystem ADRs are written so the doc can describe system layering holistically.

---

## Verdict: **CONCERNS**

| Verdict | Rationale |
|---------|-----------|
| ✗ PASS | 7 charter-level subsystem gaps remain; architecture not complete |
| ✓ CONCERNS | No blocking conflicts, no engine issues, existing ADRs well-formed |
| ✗ FAIL | No critical issues identified |

Existing 5 ADRs are safe to advance to `Accepted` individually or as a batch.

## Blocking Issues

None. The remaining gaps are expected — the architecture is only ~5/13 complete.

## Required ADRs (priority order)

**High priority — foundation for other ADRs:**
1. **ADR-0006: Job System / Multi-threaded scheduling** — ADR-0002 `ParallelRenderQueue` + future Physics/AI + Hot Reload depend on this
2. **ADR-0007: Asset Database / Hot Reload pipeline** — foundational for texture streaming, scene persistence (Phase 5), Lua script reload
3. **ADR-0008: Rendering Pipeline (Deferred+Forward+, PostFX, HDR workflow)** — upgrades TR-rnd-003 and TR-chr-001 from Partial to Covered

**Medium priority:**
4. ADR-0009: Physics architecture (GJK/EPA + constraints + cloth + ragdoll)
5. ADR-0010: Audio architecture (XAudio2 + 3D spatial + DSP effects)
6. ADR-0011: Input architecture (Keyboard/Mouse + Gamepad vibration + IME)

**Lower priority:**
7. ADR-0012: Networking architecture (Reliable UDP + replication + lag compensation)
8. ADR-0013: GUI widget system + ImGui integration

## Next Steps

1. Write highest-priority gap ADRs one at a time in a **fresh session** using `/architecture-decision [slug]`
2. Re-run `/architecture-review` after each ADR to verify coverage improves
3. Once all 13 priority ADRs are `Accepted`, run `/gate-check pre-production`
