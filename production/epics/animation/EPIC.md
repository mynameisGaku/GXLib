# Epic: Animation Pipeline

> **Layer**: Presentation
> **ADR**: docs/architecture/adr-0014-animation.md
> **Architecture Module**: GXLib/Graphics/3D/{Skeleton,Animator,...}
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories animation`

## Overview

This epic covers the skeletal animation pipeline: Skeleton, AnimationClip, Animator state machine, GPU skinning, and blend trees, delivered in Phase 5. Implementation is fully complete. One integration point specified in ADR-0016 (EventBus) is not yet implemented: `AnimationEventDispatcher` should call `SetGlobalBusBridge()` to route animation events (clip start, loop, end, marker hit) into the EventBus — the method does not exist yet. Until this bridge is wired, animation events are only available via direct callback registration and cannot be subscribed through the EventBus. No TR-level requirements were registered at project inception.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0014: Animation Pipeline | GPU-skinned skeletal animation; Animator state machine; blend trees; animation events routed through EventBus via bridge | LOW |
| ADR-0016: EventBus | AnimationEventDispatcher must implement SetGlobalBusBridge to publish events into the EventBus | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| — | No TR-level requirements registered; governed at ADR charter level | ⚠️ Partial |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories animation` to break this epic into implementable stories.
