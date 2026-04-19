# Epic: Animation Pipeline

> **Layer**: Presentation
> **ADR**: docs/architecture/adr-0014-animation.md
> **Architecture Module**: GXLib/Graphics/3D/{Skeleton,Animator,AnimationEventDispatcher,...}
> **Status**: ✅ Complete (verified 2026-04-19)

## Overview

Skeletal animation pipeline delivered in Phase 5: Skeleton, AnimationClip,
Animator state machine, GPU skinning, blend trees, motion matching, spring
bones, full-body IK, animation layers, procedural animation. Implementation
is complete.

The EventBus bridge specified in ADR-0016 §7 (`AnimationEventDispatcher::
SetGlobalBusBridge`) was delivered as part of the EventBus epic story-004
(2026-04-17). Verified 2026-04-19 at `GXLib/Graphics/3D/AnimationEventDispatcher.h:76`:
`void SetGlobalBusBridge(bool on) { m_globalBusBridge = on; }` with matching
private member `bool m_globalBusBridge = false;`. Test coverage at
`Tests/unit/eventbus/animation_bridge_test.cpp` + `Tests/test_AnimationEventDispatcher.cpp`.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0014: Animation Pipeline | GPU-skinned skeletal animation; Animator state machine; blend trees; animation events routed through EventBus via bridge | LOW |
| ADR-0016: EventBus | AnimationEventDispatcher publishes events into the global bus via SetGlobalBusBridge (§7) | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-anim-001 | Skeleton + Animator state machine + blend tree + IK suite | ✅ Implemented |
| TR-anim-002 | Motion matching, root motion, spring bones, GPU skinning (compute shader) | ✅ Implemented |
| TR-anim-003 | AnimationEventDispatcher → EventBus bridge; ragdoll handoff atomic at frame boundary | ✅ Implemented (bridge delivered via EventBus story-004; ragdoll handoff documented in ADR-0014 §20) |

## Definition of Done

Reached 2026-04-19:
- [x] Animator state machine + blend tree + motion matching + IK — Phase 5 delivery
- [x] GPU skinning via compute shader — Phase 5 delivery
- [x] AnimationEventDispatcher — `GXLib/Graphics/3D/AnimationEventDispatcher.{h,cpp}`
- [x] SetGlobalBusBridge + AnimationEventFired — delivered via EventBus epic story-004
- [x] Tests — `test_AnimationEventDispatcher.cpp` + `unit/eventbus/animation_bridge_test.cpp` pass among 4969 total tests

## Next Step

Epic complete. Future work (not gating): formal story breakdown for any
fresh animation features introduced in later Phases. When this happens,
run `/create-stories animation`.
