# ADR-0001: Documentation Strategy for GXLib (SDK, ADR-Only, No GDDs)

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted DX12) |
| **Domain** | Core / Meta (project documentation process) |
| **Knowledge Risk** | LOW — no engine APIs involved |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md` |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | None (process decision) |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | None |
| **Enables** | All future ADRs (ADR-0002 onwards) |
| **Blocks** | None |
| **Ordering Note** | Must be the first ADR — establishes how the rest of the docs are structured. |

## Context

### Problem Statement
GXLib is a DirectX 12-based 2D/3D game engine library with a DXLib-compatible API — an SDK/framework, not a game. The CCGS template's standard workflow is GDD-driven (game concept → systems → GDDs → ADRs → stories), which does not fit a project whose output is itself the engine rather than a game.

### Constraints
- 784 existing source files with established architecture (Phase 0–5)
- No GDDs exist; game-design concepts (mechanics, balance, progression) do not apply
- Template skills (`/create-stories`, `/story-readiness`, `/architecture-review`) expect `design/gdd/*.md` as requirement source
- `/gate-check` phase heuristics assume game-project milestones

### Requirements
- Must allow template skills (`/architecture-review`, `/create-stories`, etc.) to operate without GDDs
- Must preserve traceability from architectural decisions to implementation
- Must accommodate retroactive documentation of 5 existing phases
- Must scale to future subsystem additions without template friction

## Decision

**GXLib adopts ADR-only documentation. The `design/gdd/` directory is unused.**

- All architectural and design decisions are recorded as ADRs in `docs/architecture/adr-*.md`
- Each major subsystem (Graphics, Physics, ECS, Audio, IO, etc.) gets ≥1 ADR covering its core design
- "Requirements" in `tr-registry.yaml` are sourced from ADRs' `GDD Requirements Addressed` section, repurposed as **Architectural Requirements** (ADR instead of GDD origin)
- Public API contracts live in Doxygen comments in headers (`GXLib/**/*.h`) and are summarized per subsystem in `docs/engine-reference/gxlib/modules/[subsystem].md` as they stabilise
- `CHANGELOG.md` continues to serve as the phase-level release log

### Architecture Diagram

```
Source of truth chain (for GXLib):

  CHANGELOG.md (phase-level releases)
       │
       ▼
  docs/architecture/adr-*.md (decisions + rationale)
       │
       ▼
  docs/architecture/tr-registry.yaml (stable requirement IDs, ADR-sourced)
       │
       ▼
  production/epics/*/ (work items, reference ADR + TR-IDs)
       │
       ▼
  GXLib/**/*.h (Doxygen — API contract)
       │
       ▼
  docs/engine-reference/gxlib/modules/*.md (stabilised API snapshots)
```

### Key Interfaces
- ADR file structure: unchanged from CCGS template
- `tr-registry.yaml` entries: `requirement` field cites `ADR-NNNN §Decision` instead of `GDD: system.md §X`
- `/architecture-review`: GDD-coverage metric reports N/A; ADR-coverage becomes the primary metric

## Alternatives Considered

### Alternative 1: Per-subsystem lightweight GDDs
- **Description**: Author one GDD per GXLib subsystem describing its public API surface and invariants
- **Pros**: Full template compatibility; more review surface via `/review-all-gdds`
- **Cons**: Duplicates information already in Doxygen headers; significant authoring overhead for 16 subsystems; forces game-design vocabulary ("Player Fantasy", "Tuning Knobs") onto API design where it doesn't fit
- **Rejection Reason**: High cost, low added value over ADRs + Doxygen

### Alternative 2: Doxygen-only (no ADRs either)
- **Description**: Rely entirely on Doxygen-generated API reference; skip ADRs
- **Pros**: Zero process overhead
- **Cons**: Loses decision rationale, alternatives-considered record, and cross-subsystem constraints; template skills become effectively unusable
- **Rejection Reason**: Throws away the value of the CCGS template entirely

## Consequences

### Positive
- No GDD authoring overhead for an SDK where GDDs don't fit
- Architecture decisions are first-class and discoverable via `tr-registry.yaml`
- Future game projects *using* GXLib can still use the full GDD flow in their own repos

### Negative
- `/review-all-gdds`, `/design-review`, `/design-system`, `/map-systems`, `/team-narrative`, `/balance-check`, and similar game-design skills are unused
- `/gate-check` phase detection requires manual `production/stage.txt` override
- Onboarding contributors who know the CCGS template will need a pointer to this ADR

### Risks
- **Template skills may behave unexpectedly when `design/gdd/` is empty.**
  *Mitigation*: document the SDK workflow in `docs/CLAUDE.md` and reference this ADR.
- **Future addition of a sample game inside this repo would need its own `design/gdd/` tree.**
  *Mitigation*: not a concern today; address if/when it happens by creating a nested project under `sdk/` or `examples/`.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — no GDDs exist by design) | N/A | This ADR establishes the absence of GDDs as the working baseline |

## Performance Implications
- **CPU**: None (process decision, not runtime)
- **Memory**: None
- **Load Time**: None
- **Network**: N/A

## Migration Plan
1. Leave `design/gdd/` empty (do not delete — template skills expect the directory to exist)
2. Update `docs/CLAUDE.md` to note the SDK workflow (follow-up task)
3. Author retroactive ADRs for existing subsystems (ADR-0002 onwards, one per major subsystem)
4. Run `/architecture-review` after ~5 ADRs exist to bootstrap `tr-registry.yaml`

## Validation Criteria
- `/architecture-decision` runs without errors citing missing GDDs
- `/architecture-review` completes with GDD coverage marked N/A rather than failing
- `tr-registry.yaml` is populated with ADR-sourced requirements after 5+ ADRs

## Related Decisions
- CCGS template configuration: `CLAUDE.md`, `.claude/docs/technical-preferences.md`
- Engine reference: `docs/engine-reference/gxlib/VERSION.md`
- Adoption plan: `docs/adoption-plan-2026-04-15.md`
