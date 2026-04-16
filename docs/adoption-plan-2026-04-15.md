# Adoption Plan

> **Generated**: 2026-04-15
> **Project phase**: Technical Setup (engine configured, no design/architecture artifacts)
> **Engine**: Custom — GXLib (DirectX 12, C++20, Windows)
> **Template version**: v1.0+

GXLib is a game engine SDK, not a game project. The standard CCGS template assumes
a GDD-driven game project, so this plan adapts the workflow: architecture-first,
subsystem reverse-documentation, ADR-heavy. Work through these steps in order.
Re-run `/adopt` anytime to check remaining gaps.

---

## Step 1: Fix Blocking Gaps

None detected.

---

## Step 2: Fix High-Priority Gaps

### 2a. Update stale engine reference in docs/CLAUDE.md
`docs/CLAUDE.md` still says "Current engine: see `docs/engine-reference/godot/VERSION.md`".
The project is now on GXLib (custom), and the Godot file does not exist.
**Fix**: Edit `docs/CLAUDE.md` line near the bottom to point at
`docs/engine-reference/gxlib/VERSION.md`.
**Time**: 2 min
- [ ] docs/CLAUDE.md updated to reference gxlib/VERSION.md

### 2b. Decide SDK documentation strategy (GDDs vs ADR-only)
GXLib has no GDDs because it is not a game. Two viable paths:

- **Path A — ADR-only** (recommended for SDK): skip `design/gdd/`, document all
  architecture in `docs/architecture/adr-*.md`. One ADR per major subsystem
  (DX12 backend, DXLib compat layer, ECS, Lua boundary, Job System, etc.).
  `tr-registry.yaml` tracks architectural requirements instead of GDD requirements.
- **Path B — Treat subsystems as "systems"**: author lightweight subsystem GDDs
  in `design/gdd/` describing public API surface, invariants, and acceptance tests.
  More overhead but gives `/design-review`, `/review-all-gdds`, and `/create-stories`
  something to work with.

**Fix**: Make a decision and record it (can be an ADR itself — ADR-0001:
"Documentation strategy for SDK project").
**Time**: 30 min
- [ ] Documentation strategy decided and recorded

### 2c. Create foundational ADRs
Record the major architecture decisions that are already baked into the code but
not documented. Recommended first ADRs:

1. DX12 backend choice (replacing DXLib's DX11 reliance)
2. DXLib-compatible procedural API design (`Compat/` layer)
3. ECS architecture (archetype-based, Phase 4 addition)
4. Lua 5.4 + sol2 scripting boundary
5. Job System design (Phase 5)
6. Asset Database + Hot Reload pipeline
7. Rendering pipeline (Deferred + Forward+, HDR, VRS, Mesh Shaders)

Use `/architecture-decision` for each — or `/reverse-document architecture [subsystem]`
to generate drafts from the existing source code.
**Time**: 1 session per ADR (≈30–60 min each with reverse-doc)
- [ ] ADR-0001 created
- [ ] ADR-0002 created
- [ ] ADR-0003 created
- [ ] (continue per subsystem)

### 2d. Create control manifest
After a handful of ADRs exist, run `/create-control-manifest` to produce a flat
rules sheet programmers can check against. For an SDK, rules like "never include
`windows.h` in public headers", "all DX12 resources must flow through
`GraphicsDevice`", "no STL containers in public ABI" belong here.
**Time**: 30 min
- [ ] docs/architecture/control-manifest.md created

---

## Step 3: Bootstrap Infrastructure

### 3a. Populate TR registry
`docs/architecture/tr-registry.yaml` exists as an empty skeleton. After ADRs
exist, run `/architecture-review` to bootstrap entries from the ADRs
(and GDDs if Path B was chosen in Step 2b).
**Time**: 1 session
- [ ] tr-registry.yaml populated with initial requirements

### 3b. Create sprint tracking file (optional for SDK)
Run `/sprint-plan update` to create `production/sprint-status.yaml`. SDK projects
can skip this if work is tracked per-subsystem/phase in CHANGELOG.md instead.
**Time**: 5 min
- [ ] production/sprint-status.yaml created (or decision to skip recorded)

### 3c. Set authoritative project stage
Run `/gate-check technical-setup` (or the appropriate phase) to write
`production/stage.txt`. The auto-detect heuristic assumes game projects and is
unreliable for an SDK with 784 source files but no GDDs.
**Time**: 5 min
- [ ] production/stage.txt written

---

## Step 4: Medium-Priority Gaps

### 4a. Engine reference completeness
`docs/engine-reference/gxlib/VERSION.md` was created but is a summary only.
Consider adding per-subsystem API snapshots as the API stabilises — these act
as a "public API contract" reference that the LLM can cite.
**Time**: 1 session per subsystem
- [ ] Public API reference for Graphics module
- [ ] Public API reference for Physics module
- [ ] (continue per stabilised subsystem)

---

## Step 5: Optional Improvements

None blocking. Add as project matures.

---

## What to Expect from Existing Stories

No stories exist yet — nothing to preserve. When stories are eventually created
for SDK work (e.g. "Add SSR to Graphics pipeline"), they will be fully compliant
with the current template format from the start.

---

## Re-run

Run `/adopt` again after completing Step 2 to verify all high gaps are resolved.
The new run will reflect the current state of the project.
