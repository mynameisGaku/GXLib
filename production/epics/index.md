# Epic Index

All production epics for GXLib. Each epic tracks verification, gap closure, and new API
implementation against an accepted ADR. This is an ADR-only project (no GDDs per ADR-0001).

| Epic | Layer | ADR | Module | Status | Key Gap |
|------|-------|-----|--------|--------|---------|
| [DX12 Backend](dx12-backend/EPIC.md) | Foundation | ADR-0002 | GXLib/Graphics/Device/ | Ready | — |
| [DXLib Compat API](compat-api/EPIC.md) | Foundation | ADR-0003 | GXLib/Compat/ | Ready | Doxygen gaps; Compat_Particle regression |
| [Archetype ECS](ecs/EPIC.md) | Core | ADR-0004 | GXLib/ECS/ | Ready | EntityBridge single-world constraint (documented) |
| [Job System](job-system/EPIC.md) | Core | ADR-0006 | GXLib/Core/JobSystem.h | Ready | — |
| [Asset Database + Hot Reload](asset-pipeline/EPIC.md) | Core | ADR-0007 | GXLib/Core/AssetDatabase.*, GXLib/IO/ | Ready | Concurrent load stress tests |
| [Input](input/EPIC.md) | Core | ADR-0011 | GXLib/Input/ | Ready | Hot-plug edge cases |
| [EventBus](eventbus/EPIC.md) | Core | ADR-0016 | GXLib/Core/EventBus.h | Ready | **bus-002/003/004 NOT YET IMPLEMENTED** |
| [AI](ai/EPIC.md) | Feature | ADR-0018 | GXLib/AI/ | Ready | BT blackboard + Scene Persistence round-trip |
| [Networking](networking/EPIC.md) | Feature | ADR-0013 | GXLib/IO/Network/ | Ready | Compat wrapper integration tests |
| [Lua Scripting](scripting/EPIC.md) | Feature | ADR-0005 | GXLib/Script/ | Ready | Phase 5 binding test coverage |
| [Editor](editor/EPIC.md) | Feature | ADR-0015 | GXLib/Editor/, GXLib/Core/{UndoSystem,NodeGraph,Reflect} | Ready | GX_EDITOR=OFF CI gate; reflection macro CI check |
| [Scene](scene/EPIC.md) | Feature | ADR-0019 | GXLib/Core/Scene/ | Ready | BT blackboard persistence; async load stress test |
| [Rendering Pipeline](rendering/EPIC.md) | Presentation | ADR-0008 | GXLib/Graphics/{FrameGraph,Pipeline,PostEffect,3D} | Ready | Custom shader model end-to-end test |
| [Audio](audio/EPIC.md) | Presentation | ADR-0010 | GXLib/Audio/ | Ready | IXAPO Process() dispatch verification |
| [GUI](gui/EPIC.md) | Presentation | ADR-0012 | GXLib/GUI/ | Ready | UIContext not exposed via Compat layer |
| [Animation Pipeline](animation/EPIC.md) | Presentation | ADR-0014 | GXLib/Graphics/3D/{Skeleton,Animator,...} | Ready | AnimationEventDispatcher SetGlobalBusBridge missing |
| [Movie Pipeline](movie/EPIC.md) | Presentation | ADR-0020 | GXLib/Movie/ | Ready | **Zero test coverage** |

## Priority Order

Epics with implementation gaps (not just verification) should be scheduled first:

1. **EventBus** — 3 TRs not yet implemented (bus-002, bus-003, bus-004); blocks Job System cross-thread events and Editor replay
2. **Animation Pipeline** — SetGlobalBusBridge not implemented; blocks EventBus integration for animation events
3. **Movie Pipeline** — no test coverage; invisible regression risk
4. **Editor** — CI gates missing; build hygiene risk
5. **GUI** — UIContext Compat gap; affects DXLib-sourced consumer projects
6. **Audio** — IXAPO dispatch needs runtime verification

All other epics are verification and documentation closure.
