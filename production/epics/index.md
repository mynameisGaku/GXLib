# Epic Index

All production epics for GXLib. Each epic tracks verification, gap closure, and new API
implementation against an accepted ADR. This is an ADR-only project (no GDDs per ADR-0001).

| Epic | Layer | ADR | Module | Status | Key Gap |
|------|-------|-----|--------|--------|---------|
| [DX12 Backend](dx12-backend/EPIC.md) | Foundation | ADR-0002 | GXLib/Graphics/Device/ | Ready | — |
| [DXLib Compat API](compat-api/EPIC.md) | Foundation | ADR-0003 | GXLib/Compat/ | ✅ Complete | Doxygen + regression test closed 2026-04-19 |
| [Archetype ECS](ecs/EPIC.md) | Core | ADR-0004 | GXLib/ECS/ | Ready | EntityBridge single-world constraint (documented) |
| [Job System](job-system/EPIC.md) | Core | ADR-0006 | GXLib/Core/JobSystem.h | Ready | — |
| [Asset Database + Hot Reload](asset-pipeline/EPIC.md) | Core | ADR-0007 | GXLib/Core/AssetDatabase.*, GXLib/IO/ | Ready | Concurrent load stress tests |
| [Input](input/EPIC.md) | Core | ADR-0011 | GXLib/Input/ | Ready | Hot-plug edge cases |
| [EventBus](eventbus/EPIC.md) | Core | ADR-0016 | GXLib/Core/EventBus.h | ✅ Complete | All 5 stories implemented + tested 2026-04-17 |
| [Animation Pipeline](animation/EPIC.md) | Presentation | ADR-0014 | GXLib/Graphics/3D/{Skeleton,Animator,AnimationEventDispatcher,...} | ✅ Complete | SetGlobalBusBridge verified 2026-04-19; tests under `test_AnimationEventDispatcher.cpp` + `unit/eventbus/animation_bridge_test.cpp` |
| [AI](ai/EPIC.md) | Feature | ADR-0018 | GXLib/AI/ | Ready | BT blackboard + Scene Persistence round-trip |
| [Networking](networking/EPIC.md) | Feature | ADR-0013 | GXLib/IO/Network/ | Ready | Compat wrapper integration tests |
| [Lua Scripting](scripting/EPIC.md) | Feature | ADR-0005 | GXLib/Script/ | Ready | Phase 5 binding test coverage |
| [Editor](editor/EPIC.md) | Feature | ADR-0015 | GXLib/Editor/, GXLib/Core/{UndoSystem,NodeGraph,Reflect} | ✅ Ready | CI gates landed: `build-no-editor` + `lint-editor-boundary` jobs in `.github/workflows/build.yml` |
| [Scene](scene/EPIC.md) | Feature | ADR-0019 | GXLib/Core/Scene/ | Ready | BT blackboard persistence; async load stress test |
| [Rendering Pipeline](rendering/EPIC.md) | Presentation | ADR-0008 | GXLib/Graphics/{FrameGraph,Pipeline,PostEffect,3D} | Ready | Custom shader model end-to-end test |
| [Audio](audio/EPIC.md) | Presentation | ADR-0010 | GXLib/Audio/ | Ready | IXAPO Process() dispatch verification |
| [GUI](gui/EPIC.md) | Presentation | ADR-0012 | GXLib/GUI/ | ✅ Ready | `gx::GetUIContext()` + accessor landed (Compat_System.cpp + GXLib.h) |
| ~~[Animation Pipeline](animation/EPIC.md)~~ | ~~Presentation~~ | — | — | (consolidated above) | |
| [Movie Pipeline](movie/EPIC.md) | Presentation | ADR-0020 | GXLib/Movie/ | Ready | **Zero test coverage** |
| [Architecture Fixes (2026-04-18)](arch-fixes-2026-04-18/EPIC.md) | Cross-cutting | ADR-0015/0018/0019/0020 | AI, Core/Scene, Movie, Editor | **✅ Complete** | 7 issues from 2026-04-18 fresh-session review — all fixed in-session, build + 4957 tests pass |

## Priority Order

Epics with implementation gaps (not just verification) should be scheduled first:

1. ~~**Architecture Fixes (2026-04-18)**~~ ✅ **COMPLETE 2026-04-18**
2. ~~**EventBus**~~ ✅ **COMPLETE 2026-04-17** — all 5 stories implemented + tested (HandlerCategory, ReplayMode, QueueFromWorker, AnimationBridge, DispatchOrder); epic docs synchronised 2026-04-19
3. **Animation Pipeline** — SetGlobalBusBridge is implemented as part of EventBus story-004 above; epic verification sweep recommended
4. **Movie Pipeline** — basic test coverage landed (`movie_player_test.cpp` 8 tests); extend as MP4/WMV fixtures become available
5. ~~**Editor**~~ ✅ CI gates landed 2026-04-17
6. ~~**GUI**~~ ✅ `gx::GetUIContext()` Compat accessor landed
7. **Audio** — IXAPO dispatch needs user-at-PC runtime verification (sprint-002 Task 2)

All other epics are verification and documentation closure.
