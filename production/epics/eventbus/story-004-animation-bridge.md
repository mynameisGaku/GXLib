# Story 004: AnimationEventDispatcher global bus bridge

> **Epic**: EventBus
> **Status**: ✅ Done (2026-04-17 — implemented + tested)
> **Layer**: Core
> **Type**: Integration
> **Manifest Version**: 2026-04-17

## Context

**ADR**: docs/architecture/adr-0016-eventbus.md
**Requirement**: `TR-bus-005`
*(AnimationEventDispatcher → global bus bridge via SetGlobalBusBridge(bool); bridged events emit AnimationEventFired on the global bus and respect categorisation.)*

**ADR Governing Implementation**: ADR-0016 §7 + ADR-0014 (Animation)
**ADR Decision Summary**: AnimationEventDispatcher is a per-AnimationPlayer local mechanism. When SetGlobalBusBridge(true) is called, each fired AnimationEvent additionally emits AnimationEventFired on the global EventBus, passing through replay-suppression.

**Engine**: GXLib Phase 5 | **Risk**: LOW

**Control Manifest Rules (Core layer)**:
- Required: EventBus is single cross-system pub/sub spine
- Forbidden: movie_audio_assumption (not directly relevant but categorisation discipline applies)

---

## Acceptance Criteria

- [ ] `struct AnimationEventFired { gx::String eventName; uint32_t clipId; float time; uint32_t playerId; }` declared
- [ ] `void SetGlobalBusBridge(bool on)` added to AnimationEventDispatcher
- [ ] `bool IsGlobalBusBridged() const` added
- [ ] When bridge is on, each fired AnimationEvent also calls `EventBus::Instance().Fire<AnimationEventFired>({...})`
- [ ] Global bus subscribers can categorise their handlers (SideEffect for SFX, Idempotent for state)
- [ ] When bridge is off, no global bus emission (local handlers only)
- [ ] Existing AnimationEventDispatcher tests pass unchanged

---

## Implementation Notes

*From ADR-0016 §7:*

- Add `bool m_globalBusBridge = false;` to AnimationEventDispatcher
- In `AnimationEventDispatcher::Update`, after invoking local handlers for each matched event, check `m_globalBusBridge` — if true, `EventBus::Instance().Fire<AnimationEventFired>({event.name, clipId, event.time, playerId})`.
- `AnimationEventFired` struct lives in a new header or alongside `AnimationEventDispatcher.h`.
- Local handlers remain OUTSIDE replay-suppression. The global bus handles respect categorisation as normal.
- `playerId` comes from the AnimationPlayer's entity ID or a user-set identifier.

---

## Out of Scope

- Story 001: HandlerCategory (must be DONE)
- Local handler replay-suppression is NOT part of this story (ADR-0016 explicitly says local handlers are outside the system)

---

## QA Test Cases

- **AC-1**: Bridge emits AnimationEventFired on global bus
  - Given: AnimationEventDispatcher with SetGlobalBusBridge(true); global bus subscriber for AnimationEventFired
  - When: Update through an event time
  - Then: global handler receives AnimationEventFired with correct eventName/clipId/time

- **AC-2**: Bridge off = no global emission
  - Given: AnimationEventDispatcher with SetGlobalBusBridge(false); global bus subscriber
  - When: Update through an event time
  - Then: global handler does NOT fire; local handler still fires

- **AC-3**: Global handler respects categorisation
  - Given: Subscribe<AnimationEventFired>(handler, SideEffect); SetReplayMode(true)
  - When: bridge fires AnimationEventFired
  - Then: handler is suppressed (SideEffect in replay)

---

## Test Evidence

**Story Type**: Integration
**Required evidence**: `tests/integration/eventbus/animation_bridge_test.cpp` — must exist and pass
**Status**: [ ] Not yet created

---

## Dependencies

- Depends on: Story 001 (HandlerCategory must exist for categorised subscribers)
- Unlocks: None
