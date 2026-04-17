# Epic: Physics

> **Layer**: Core
> **ADR**: docs/architecture/adr-0009-physics-architecture.md
> **Architecture Module**: GXLib/Physics/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories physics`

## Overview

GXLib ships a custom in-house physics stack with dual 2D/3D worlds, GJK/EPA narrow-phase, sequential-impulse constraint solver, Verlet cloth, ragdoll, and a capsule-sweep character controller. All code exists since Phases 0-5 and is well-tested. The deterministic fixed-timestep contract (60 Hz default) and island-solve reduction order are binding for rollback netcode (ADR-0013). Remaining work is verification-focused: confirm determinism across CPU SKUs, validate constraint stability at high mass ratios, and ensure broadphase concurrent-read contract holds under Animation parallel execution (ADR-0014).

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0009: Physics Architecture | Custom 2D+3D, GJK/EPA, fixed timestep, deterministic island solve, JobSystem parallel broadphase | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| (charter) TR-chr-003 | Physics: rigid body, cloth, ragdoll, GJK/EPA | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from ADR-0009 verified
- Determinism golden-trace test passes on AMD + Intel
- Constraint stability at 100:1 mass ratio verified
- Broadphase concurrent-read under animation parallel confirmed

## Next Step

Run `/create-stories physics` to break this epic into implementable stories.
