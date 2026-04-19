# ADR-0007: Asset Database + Hot Reload Pipeline

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Core / IO / Asset pipeline |
| **Knowledge Risk** | LOW — asset-database patterns (GUID/hash-keyed, dependency-tracking, hot-reload via FileWatcher) are well-documented; DirectStorage 1.2 APIs used are within the LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Core/AssetDatabase.{h,cpp}`, `GXLib/Core/AssetMeta.{h,cpp}`, `GXLib/Core/AssetReloader.{h,cpp}`, `GXLib/Core/AssetRemapper.{h,cpp}`, `GXLib/IO/{FileWatcher,DirectStorageManager,Archive,PakFileProvider,BundleManager,AsyncLoadPipeline}.{h,cpp}`, `CHANGELOG.md` Phases 3/4/5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Round-trip test: modify an asset on disk → FileWatcher notifies → AssetReloader rebinds all consumers within 1 frame; DirectStorage fallback to standard ReadFile when NVMe/driver unavailable; Archive/Pak file provider correctness under random-access mod patches |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0002 (DX12 backend — DirectStorage GPU decompression target), ADR-0006 (Job System — async load jobs run on worker threads) |
| **Enables** | Texture Streaming Manager (Phase 5), Scene Persistence (Phase 3/5), Hot Reload Manager (Phase 5), Lua script reload (ADR-0005), future Editor live-editing workflows |
| **Blocks** | None (code already exists across Phases 3/4/5; this ADR is retroactive) |
| **Ordering Note** | Must precede any ADR that proposes a new asset type (meshes, animations, shaders, audio banks) — those ADRs register their type with the Asset Database and participate in hot reload under this ADR's rules |

## Context

### Problem Statement
GXLib manages many asset types across many storage formats: textures, meshes, animations, shaders, sounds, fonts, scripts, prefabs, scene files. They live in multiple places: loose files on disk, packed `.pak` archives, encrypted bundles, and GPU-friendly DirectStorage streams. Game code needs a single way to say "give me this asset" without caring whether the bytes come from disk, archive, pak, or DirectStorage, and — crucially — needs hot reload to work for all of them during development. Without a unified pipeline, every subsystem reinvents its own load/path/cache logic and hot reload is impossible to maintain.

This ADR records the architecture behind `GXLib/Core/AssetDatabase` + `AssetMeta` + `AssetReloader` + `AssetRemapper` (Phase 3 introduced AssetDatabase; Phase 5 added Hot Reload Manager). It answers: **what is the stable asset identity, what owns the cache, how are consumers notified of reloads, and what does the provider abstraction look like**.

### Constraints
- Hot reload is a dev-mode feature; shipping builds disable FileWatcher and always resolve through the Archive/Pak layer
- Asset types are open-ended — new types (GPU particle templates, scriptable objects) can be added without changing the pipeline
- DirectStorage is optional (not all consumer hardware supports it); pipeline must fall back to `ReadFile` path
- Consumers (Graphics, Audio, ECS, Lua) may hold references to assets for a long time; reload must not invalidate their pointers silently
- Must coexist with per-asset lifecycle rules: textures are GPU-resource-backed (need delayed release for in-flight frames); Lua modules are VM-bound (must reload inside the Lua thread, per ADR-0005/0006)
- Paths in source code are an anti-pattern — assets are addressed by stable ID, not filesystem path

### Requirements
- Every asset has a **stable AssetId** (64-bit hash of logical path, e.g. `hash("textures/player/body_albedo")`) — survives rename, rebuild, archive repackaging
- AssetDatabase owns the process-wide cache of loaded assets; one load per ID regardless of how many consumers request it
- Asynchronous loads go through `IO/AsyncLoader` → `JobSystem` worker threads
- DirectStorage used when available for GPU-bound assets (textures, meshes); automatic fallback otherwise
- FileWatcher → AssetReloader pipeline rebinds consumers when an underlying file changes (dev mode only)
- Dependency tracking: if asset A references B (e.g., material → texture), reloading B re-notifies A's consumers
- Remapping support (`AssetRemapper`) — mods can replace an AssetId's backing file without code changes

## Decision

**GXLib uses a hash-keyed Asset Database (`Core/AssetDatabase`) as the single cache and identity authority. File bytes flow through a pluggable `IFileProvider` chain (Physical → Archive → Pak → Bundle). Async loads go through `IO/AsyncLoader` which submits to the Job System. Hot reload flows FileWatcher → AssetReloader → per-type rebinder → subscribed consumers via a generation-counted handle. DirectStorage is used when GPU decompression is available, with automatic fallback.**

Concrete rules:

1. **Asset identity — `AssetId`.** 64-bit FNV-1a hash of a normalised logical path (`"textures/player/body_albedo"` — forward slashes, no extension unless disambiguation needs it, lower-case). Collisions are extremely rare; collision detected at insert time → error in dev, log in release.

2. **`AssetDatabase` is the single cache.** One singleton. Maps `AssetId → AssetRecord`. An `AssetRecord` holds: current-bytes pointer (or typed asset object), load state (Unloaded/Loading/Ready/Failed), ref count, generation counter, type tag, and dependency list.

3. **Type-erased storage, typed accessors.** `AssetDatabase::Get<T>(AssetId)` returns `AssetHandle<T>` — a generation-counted opaque handle. The database stores assets as `shared_ptr<void>` + type tag; the typed accessor checks the tag and casts. Consumers never hold raw pointers across frames — they hold `AssetHandle<T>`, which internally goes through the database.

4. **Provider chain for bytes.** `IFileProvider` interface with concrete implementations:
   - **PhysicalFileProvider** — loose files on disk (dev mode default)
   - **ArchiveFileProvider** — `.gxa` archive files
   - **PakFileProvider** — `.pak` archives (simple binary, fast)
   - **BundleManager** — encrypted / compressed bundles
   - Providers are stacked in priority order; dev mode prefers Physical (so loose-file edits hot-reload); ship mode prefers Archive/Pak.

5. **Async load pipeline.** `AsyncLoader::Load(assetId)` returns a future/handle. Internally: submit a `JobSystem::Submit` job that resolves the provider chain, reads bytes, runs the type-specific deserializer, and stores the result in the database on completion. GPU-bound assets take the DirectStorage path when available.

6. **DirectStorage integration.** `DirectStorageManager` wraps DirectStorage 1.2 queues. Textures and meshes flow directly to GPU memory with optional GPU-side decompression (BC7, GDeflate). When DirectStorage is unavailable, `AsyncLoader` uses the standard `ReadFile` → staging upload path transparently.

7. **Hot reload (`AssetReloader`, dev mode).**
   - `FileWatcher` (Windows `ReadDirectoryChangesW`, overlapped polling since Phase 5) observes the Physical provider root
   - A debounced change event triggers `AssetReloader::HandleFileChange(path)`
   - The reloader resolves `path → AssetId` (via reverse map maintained at load time), bumps the record's generation, reloads bytes via the provider chain, runs the type's reload handler (GPU resource rebuild, Lua module refresh, etc.)
   - Consumers who hold `AssetHandle<T>` see the new content on next `Resolve()`; stale handles (wrong generation) can be detected if the consumer cares
   - Ship builds: FileWatcher + AssetReloader are compiled out (ifdef or dead-stripped).

8. **Dependency tracking.** `AssetDatabase::AddDependency(dependent, dependee)` records that reloading `dependee` should notify `dependent`'s reload handler. Used by: material → texture, scene → prefabs, prefab → script, etc.

9. **`AssetRemapper` for mods.** A remap table can route `AssetId X → path Y`, intercepting provider resolution. Mods override base-game assets by declaring remaps; no code changes needed.

10. **Reference counting + deferred release.** GPU-backed assets enter a "frames-in-flight quarantine" on last ref release (≥ 3 frames, matching swap chain depth) before actual GPU memory release, to avoid writing-while-rendering hazards.

11. **Logical paths are data, not code.** Source files may NOT `#include` or hard-code filesystem paths. All access goes through `AssetId` resolved from a data-driven manifest or from a typed ID constant defined in a per-game asset-ID header generated from the manifest.

### Architecture Diagram

```
Game code:
   auto tex = assets.Get<Texture>(AssetId("textures/player/body_albedo"));
   assets.AddDependency(materialId, textureId);

gx::AssetDatabase   (singleton — single cache + identity)
   └── AssetRecord map: AssetId → { state, gen, type_tag, shared_ptr<void>, deps }

      ▲                                            ▲
      │ (consumers hold AssetHandle<T>)            │ (reload notifications)
      │                                            │
   ┌──┴──────────────┐                    ┌────────┴──────────┐
   │ AsyncLoader     │                    │ AssetReloader     │  (dev mode only)
   │ (submit → Job)  │                    │                   │
   └────┬────────────┘                    └────────▲──────────┘
        │                                          │
        ▼                                          │  path → AssetId (reverse map)
   Provider chain (priority-ordered):              │
     1. PhysicalFileProvider  ◄──── FileWatcher ───┘  (dev: watch loose files)
     2. ArchiveFileProvider
     3. PakFileProvider
     4. BundleManager
        │
        ▼
   Bytes → type-specific deserializer
        │
        ├─► CPU path: memcpy → staging upload → GPU resource
        └─► GPU path (when supported): DirectStorageManager
                                         → DirectStorage 1.2 queues
                                         → GPU-side BC7 / GDeflate decompress
                                         → GPU memory

gx::AssetRemapper (optional, mod support)
   sits in front of the provider chain; AssetId lookups first hit the remap table
```

### Key Interfaces

C++ (recorded contract):

- `gx::AssetId` — 64-bit value (FNV-1a of normalised path)
- `gx::AssetHandle<T>` — opaque, generation-counted; `Resolve()` returns `T*` or nullptr if failed/unloaded; `IsStale()` returns true if reloaded since last resolve
- `gx::AssetDatabase::Instance()` — singleton
- `AssetHandle<T> Get<T>(AssetId)` — sync load if not present (blocks); prefer async for hot paths
- `future<AssetHandle<T>> LoadAsync<T>(AssetId)` — submits via `AsyncLoader` → `JobSystem`
- `void AddDependency(dependent, dependee)` — reload graph edge
- `void RegisterType<T>(TypeDesc)` — plug in a new asset type's deserializer + reload handler + GPU-resource policy
- `IFileProvider` interface — `bool Resolves(AssetId)`, `span<byte> ReadSync(AssetId)`, async variant
- `DirectStorageManager::UploadTexture(AssetId, dstResource)` — DirectStorage path with fallback

## Alternatives Considered

### Alternative 1: Path-based asset access (no Asset DB)
- **Description**: Subsystems receive filesystem paths and load themselves via `fopen` / `CreateFile`
- **Pros**: Dead simple; zero indirection
- **Cons**: Every subsystem duplicates load logic; no shared cache (same texture loaded 5×); no hot reload; rename breaks every call site; mod support requires hacking every subsystem
- **Rejection Reason**: Defeats centralisation. The pipeline exists to eliminate these costs.

### Alternative 2: Per-type asset managers (TextureManager, MeshManager, SoundManager each own their cache)
- **Description**: Each subsystem owns its own type-specific cache; no cross-type database
- **Pros**: Cleaner per-type APIs; no type-erased storage complexity
- **Cons**: Dependency tracking (material → texture) requires cross-manager coordination that nobody owns; mod remapping must be replicated per manager; hot reload logic duplicated; shared-path addressing is awkward across managers
- **Rejection Reason**: Dependency and hot-reload cross-cutting concerns force a central authority. Cheaper to pay type-erasure cost once.

### Alternative 3: Ship DirectStorage as the only path (no fallback)
- **Description**: Require DirectStorage-capable hardware
- **Pros**: Simpler IO layer; best load performance
- **Cons**: Cuts off NVMe-less users; driver support is inconsistent; breaks the "runs on minimum-spec" promise of ADR-0002
- **Rejection Reason**: DirectStorage is a performance optimisation, not a requirement. Fallback is mandatory.

### Alternative 4: Immutable assets (no hot reload)
- **Description**: Once loaded, assets never change within a process
- **Pros**: No generation counter; no dependency graph; no FileWatcher
- **Cons**: Dev iteration requires full restart for every asset edit; Lua script reload (ADR-0005 requirement) impossible; texture/shader tweaking becomes painful
- **Rejection Reason**: Hot reload is the single biggest dev-iteration win an engine can offer. Non-negotiable.

## Known Limitation: AsyncLoader single-worker implementation

**Current state** (verified 2026-04-19 against `GXLib/IO/AsyncLoader.{h,cpp}`):
`AsyncLoader` owns **one** private worker thread (`std::thread m_workerThread;`)
that dequeues `LoadRequest`s from an internal mutex+cv protected queue.
Requests from arbitrary caller threads are serialised through that single
worker. `AsyncLoader` does **not** currently dispatch to `JobSystem`
(ADR-0006) despite the §Requirements wording that says async loads go
"through IO/AsyncLoader → JobSystem worker threads."

**Consequence**: concurrent `Load()` calls from N caller threads are
serialised on the AsyncLoader internal thread. True multi-worker
parallelism for asset I/O is **not** currently available — the ADR
claim is aspirational for the current code.

**What IS thread-safe (verified by `Tests/unit/io/async_loader_concurrent_test.cpp`
added 2026-04-19)**:
- Concurrent `Load()` calls from many threads (mutex-guarded enqueue).
- Concurrent `GetStatus()` queries.
- `Update()` pumping on the main thread while worker is loading.

**Migration path**: replace the internal worker thread with a `JobSystem::
Submit` per `Load()`. Completion queue stays — `Update()` on the main
thread still fires callbacks. Tracked as `TR-defer-asyncloader-jobsystem`
in `architecture-traceability.md`. Not gated on any current consumer —
scheduled as a Sprint-005-or-later epic.

Added 2026-04-19 to close the sprint-003 Task 3 design finding.

## Consequences

### Positive
- One cache, one identity, one load path — consumers just call `Get<T>(id)`
- Hot reload works for every asset type without per-subsystem hot-reload code
- DirectStorage fast path free when hardware supports it; automatic fallback when it doesn't
- Mods drop in via `AssetRemapper` with no engine code changes
- Dependency graph means reloading a texture refreshes materials → scenes automatically
- Logical paths decouple source code from on-disk layout — shipping repackaging doesn't break code

### Negative
- Type-erased storage (`shared_ptr<void>` + type tag) is a small runtime cost per access (checked in dev, can be assumed in release)
- Generation counter on every `Resolve()` is a per-call overhead (but cheap — single load)
- Hot reload must be architected into every asset type (reload handlers) — not free; each new type pays this cost
- `AssetId` collisions are possible (2⁶⁴ space but FNV-1a is not cryptographic); handled at insert time
- DirectStorage 1.2 has platform quirks (requires Windows 11 for best experience, Windows 10 with driver); error paths are long

### Risks
- **Hot reload corrupts in-flight frames** when a GPU resource is released mid-render. *Mitigation*: deferred-release quarantine (≥ 3 frames for swap-chain depth) before actual destruction.
- **Lua script reload races with a running script callback.** *Mitigation*: reload handler schedules the swap via `JobSystem::SubmitMainThread` (per ADR-0006), landing it between Lua entry points.
- **FileWatcher missed events under heavy disk churn.** *Mitigation*: overlapped polling (Phase 5 rewrite), debounced event coalescing, periodic full-scan fallback option.
- **Mod remap points at a missing file.** *Mitigation*: remap resolution logs and falls back to original AssetId; does not crash.
- **Generation counter wraparound after 2³² reloads.** *Mitigation*: not a concern at human iteration rates; counter is 32-bit per record, 32-bit handle index separate.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Charter TR-chr-004 ("Asset pipeline / Hot reload / DirectStorage flow") — satisfied by this ADR |

## Performance Implications
- **CPU**: `Get<T>` resolve path ≤ 100 ns in steady state (hashmap lookup + generation check); async-load submit O(1); dependency graph update O(edges-per-asset)
- **Memory**: AssetDatabase overhead ~48 bytes per live record; FileWatcher ~4 KB; DirectStorage queues ~2 MB baseline
- **Load Time**: DirectStorage GPU decompression cuts initial-load times by 40–70% on capable hardware; fallback path matches a hand-written sync loader
- **Network**: N/A (future CDN-backed provider is a separate future ADR)

## Migration Plan

Not applicable — this ADR is retroactive. AssetDatabase landed in Phase 3; DirectStorage integration in Phase 4; Hot Reload Manager + FileWatcher rewrite in Phase 5. Going forward:

1. New asset types register with `AssetDatabase::RegisterType<T>(...)` at subsystem init time
2. New file providers implement `IFileProvider` and plug into the provider chain at a declared priority
3. Any proposal to add a new IO backend (e.g., network CDN provider) authors its own ADR and slots in as an `IFileProvider`
4. Direct `fopen` / `CreateFile` in subsystem code becomes a Forbidden Pattern (see registry)

## Validation Criteria
- AssetId hash collision test: load 10,000 assets, assert zero collisions
- Round-trip hot reload test: touch a texture file, assert all consumer materials re-render within 1 frame
- DirectStorage fallback test: disable DirectStorage at runtime, assert texture loads still succeed via staging upload path with identical final GPU state
- Mod remap test: override a base-game texture via `AssetRemapper`, assert `Get<Texture>(baseId)` returns the mod's bytes
- Dependency propagation test: reload a texture referenced by 3 materials → all 3 materials' reload handlers fire exactly once
- FileWatcher stress test: rename/move/modify 1,000 files in 1 second → no dropped events after debounce window
- Deferred release test: release a GPU-backed asset mid-frame, assert GPU memory is freed at frame + swap-chain-depth boundary, not before

## Related Decisions
- ADR-0001 (Documentation strategy)
- ADR-0002 (DX12 backend — DirectStorage is a DX12-era feature; forbidden pattern `unchecked_optional_gpu_feature` applies to DirectStorage caps gating)
- ADR-0005 (Lua scripting — script reload flows through AssetReloader + main-thread dispatch)
- ADR-0006 (Job System — async loads submit here; reload handlers may use `SubmitMainThread`)
- `GXLib/Core/AssetDatabase.{h,cpp}`, `GXLib/Core/AssetReloader.{h,cpp}`, `GXLib/IO/{FileWatcher,DirectStorageManager,AsyncLoadPipeline,Archive,PakFileProvider,BundleManager}.{h,cpp}` (source of truth)
- CHANGELOG.md Phases 3, 4, 5
