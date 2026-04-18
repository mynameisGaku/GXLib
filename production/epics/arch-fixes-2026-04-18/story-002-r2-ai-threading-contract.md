# Story 002: R2 — AI threading contract rewrite (per-instance non-reentrant)

> **Epic**: Architecture Fixes (2026-04-18 Review)
> **Status**: ✅ Done (2026-04-18, in-session)
> **Layer**: AI
> **Type**: ADR text (no code change)
> **Manifest Version**: 2026-04-18

## Context

**ADR**: `docs/architecture/adr-0018-ai-architecture.md`
**Requirement**: TR-ai-007 (AI threading model)
**Review source**: `docs/architecture/architecture-review-2026-04-18.md` §R2

**Problem**: ADR-0018 §9 contradicted itself:
1. "All AI API calls are main-thread-only by convention."
2. "Callers who want parallel AI ticks submit them as independent Jobs via
   ADR-0006. Since each BehaviorTree/NavAgent owns private state, this is safe."

"Main-thread-only by convention" disallowed parallel Jobs; "parallel ticks are
safe" permitted them. Implementers following the strict reading couldn't
parallelise AI; implementers following the permissive reading saw no sync on
shared structures and could introduce races.

The actual intent (verifiable from source: no synchronisation primitives exist
inside any AI class) is **per-instance non-reentrant** — each object is owned
by a single thread at a time; parallel *different* instances is safe.

---

## Acceptance Criteria

- [x] ADR-0018 §9 rewrites the threading contract as "per-instance
      non-reentrant" — no "main-thread-only by convention" sentence.
- [x] §9 explicitly permits parallel ticking via JobSystem when each Job owns a
      distinct instance.
- [x] §9 names single-thread usage as the default safe mode for non-batched
      games.
- [x] §9 adds an EventBus interaction note — AI Actions on worker Jobs MUST use
      `EventBus::QueueFromWorker<T>`, not `Fire<T>` (per ADR-0016 §5).
- [x] §Constraints wording ("Thread-safety is the caller's responsibility")
      remains — now consistent with the rewritten §9.
- [x] No code change — synchronisation primitives already absent from all AI
      classes; the intent was always per-instance non-reentrant.

---

## Implementation Notes

Completed in-session 2026-04-18. Changes:
- `docs/architecture/adr-0018-ai-architecture.md` §9 — full rewrite of the
  threading bullets. Removes "main-thread-only by convention"; explicit
  per-instance non-reentrant contract; cross-reference to ADR-0016 for
  EventBus worker-thread rule.

## Out of Scope

- Code change — none needed. ADR aligns with existing behaviour.
- ECS-AI bridge (future ADR per ADR-0018 Migration Plan) — not this story.

---

## Test Evidence

**Story Type**: ADR text only
**Required evidence**: `grep "per-instance non-reentrant"` in ADR-0018 returns
the updated §9. Not an automated-test story.
**Status**: ✅ Done
