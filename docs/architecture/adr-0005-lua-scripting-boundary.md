# ADR-0005: Lua Scripting Boundary (Lua 5.4 + sol2)

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Scripting |
| **Knowledge Risk** | LOW — Lua 5.4 and sol2 3.x are stable and within the LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Script/` source tree (ScriptEngine, ScriptBindings, ScriptBindingsExtended, VisualScript), `CHANGELOG.md` Phases 1 and 5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Sandbox escape test (scripts cannot reach `os.execute`, `io.open`, `require` arbitrary modules); bindings stability under hot reload; Lua→C++ call overhead ≤ 1 μs for trivial bound functions |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0004 (ECS — Lua's primary entity manipulation goes through World) |
| **Enables** | Future ADRs on save/load of script state, in-game modding surface, editor visual-scripting (VisualScript node graph) |
| **Blocks** | None (code already exists since Phase 1; this ADR is retroactive) |
| **Ordering Note** | Must be Accepted before any ADR proposing user-facing game logic in Lua (modding, scripted events, quest DSLs) |

## Context

### Problem Statement
GXLib embeds Lua (since Phase 1) so gameplay authors and end-user modders can write logic without a C++ compile cycle. Phase 5 extended bindings to Physics, Audio, IO, ECS, and GUI. The scripting boundary is consequential: bindings that expose too much of the engine make the C++ API impossible to refactor without breaking scripts; bindings that expose too little force authors to write performance-critical paths in C++ anyway. This ADR records **which language, which binding layer, what the boundary exposes, and how sandboxing / hot reload / error handling work** — so that future binding additions follow the same rules.

### Constraints
- Must integrate with C++20 cleanly (template-heavy binding libraries like sol2 rely on modern C++)
- Must work alongside the ECS (`World::Spawn/Destroy/Query`, ADR-0004) and the OOP Node model
- Scripts must be hot-reloadable (FileWatcher integration, Phase 5)
- Must run sandboxed: untrusted mod code must not be able to read arbitrary files, spawn processes, or load native DLLs
- Deterministic call cost — no surprise stalls from JIT warm-up or GC pauses in the gameplay loop

### Requirements
- Scripts can: spawn/destroy ECS entities, read/write registered component types, play sounds, draw 2D/3D primitives through the Compat procedural API, subscribe to engine events
- Scripts cannot: escape the sandbox (no `os.execute`, `io.open` on arbitrary paths, no `package.loadlib`), access the raw `GraphicsDevice` / `ID3D12Device`, or bypass forbidden-pattern registry rules (ADR-0002 / ADR-0003 / ADR-0004)
- Lua→C++ call overhead must not dominate the gameplay frame (target ≤ 1 μs per trivial bound call; ≤ 20 μs for full ECS query from script)
- Hot reload of a script must not invalidate C++-held references to Lua values

## Decision

**GXLib uses Lua 5.4 as its sole embedded scripting language, with sol2 as the C++↔Lua binding layer. All bindings live under `GXLib/Script/` and present a curated, sandboxed surface that routes entity access through `World` (ECS), Compat functions (drawing/audio/input), and event subscription — never through raw engine internals.**

Concrete rules:

1. **Language: Lua 5.4.** No LuaJIT (licensing clarity + 5.4 features like integer type, goto, to-be-closed variables matter more than JIT speed for the embedded-scripting role). No additional languages — no Python, no C#, no JavaScript. One scripting surface to maintain.

2. **Binding library: sol2 (3.x).** Header-only, template-based, C++20-friendly. Provides `sol::state`, `sol::usertype<T>`, automatic overload resolution, and clean exception/error translation. Alternative libraries rejected below.

3. **Boundary philosophy: curated facade, not raw exposure.** Scripts see `gxlib.world`, `gxlib.audio`, `gxlib.input`, `gxlib.draw`, `gxlib.events`, `gxlib.physics`, `gxlib.gui`, `gxlib.io` — table namespaces that mirror the C++ public API but not the internal layout. Raw pointers, `GraphicsDevice`, `ID3D12Device`, archetype internals, and any type under a `detail::` namespace are never bound.

4. **Sandboxing is default-on.** The `ScriptEngine` creates each `sol::state` with a restricted global table:
   - Removed: `os.execute`, `os.remove`, `os.rename`, `os.exit`, `io.open`, `io.popen`, `io.tmpfile`, `package.loadlib`, `package.cpath`, `dofile`, `loadfile`
   - Kept: safe subset of `math`, `string`, `table`, `coroutine`, `os.time`, `os.date`, `os.clock`
   - File I/O goes through `gxlib.io` which is path-restricted to a per-script asset root (configured at `ScriptEngine::Create` time)
5. **ECS access routes through `World`.** Scripts call `gxlib.world:spawn{Transform=..., Velocity=...}`, `gxlib.world:destroy(entity)`, `gxlib.world:query({"Transform","Velocity"}):foreach(function(e, t, v) ... end)`. Archetype internals and raw `ComponentStorage` pointers are never exposed.

6. **Component types must register a Lua binding explicitly.** A component type `T` becomes script-visible only when a `sol::usertype<T>` is registered via `GXLib/Script/ScriptBindings.cpp`. New components default to C++-only; exposing them to Lua is a deliberate act.

7. **Error handling.** sol2 exceptions cross the boundary as Lua errors. The engine installs a top-level `pcall` wrapper around every script entry point; a script error never crashes the engine — it is logged via the Logger (Phase 1 categories) and the script is marked failed until reloaded. No silent swallowing.

8. **Hot reload.** A reloaded script replaces the Lua module in place; C++ holds only registered callbacks (`sol::protected_function` references tied to a generation counter), so a reload invalidates old callbacks cleanly rather than leaving dangling function references.

9. **Performance budget.** Script execution as a whole gets a 1 ms/frame budget at 60 fps (see Registry). Scripts that exceed it are logged; the engine does not forcibly preempt (cooperative model — Lua scripts are expected to yield via coroutines for long work).

10. **VisualScript** (`GXLib/Script/VisualScript.h`, editor node graph) compiles to the same sol2-bound surface — visual graphs emit Lua code rather than running on a separate runtime. One backend, two authoring modes.

### Architecture Diagram

```
Author authoring mode
   Lua text script                Visual node graph
        │                                │
        └──────────────┬─────────────────┘
                       ▼
           GXLib/Script/ScriptEngine   (sol::state, sandboxed globals,
              │                         hot-reload generation counter)
              ▼
           GXLib/Script/ScriptBindings (sol::usertype<T> registrations)
              │
              ├─► gxlib.world      ───► GXLib/ECS/World             (ADR-0004)
              ├─► gxlib.draw       ───► GXLib/Compat/*              (ADR-0003)
              ├─► gxlib.audio      ───► GXLib/Audio/*
              ├─► gxlib.input      ───► GXLib/Input/*
              ├─► gxlib.physics    ───► GXLib/Physics/*
              ├─► gxlib.gui        ───► GXLib/GUI/*
              ├─► gxlib.io         ───► GXLib/IO/*   (path-restricted)
              └─► gxlib.events     ───► GXLib/Core/EventBus
```

### Key Interfaces

C++ side:
- `gx::ScriptEngine` — owns one `sol::state` per isolated script context; configures sandbox
- `gx::ScriptEngine::Create(const ScriptEngineDesc& desc)` — sets asset root, reload policy, binding set
- `gx::ScriptEngine::Load(const char* path)` / `Reload(const char* path)`
- `gx::ScriptEngine::Call<R>(const char* fn, Args&&...)` — typed call returning `expected<R>`

Lua side (script-visible surface):
- `gxlib.world:spawn{...}`, `:destroy(entity)`, `:query({"Comp1","Comp2"}):foreach(fn)`
- `gxlib.draw.graph(x, y, handle, transFlag)` and other Compat mirrors
- `gxlib.events.subscribe("damage_dealt", fn)` / `.publish(...)`
- `gxlib.io.read_asset(relative_path)` — path-restricted to script asset root

## Alternatives Considered

### Alternative 1: LuaJIT instead of Lua 5.4
- **Description**: Use LuaJIT 2.1 for higher script throughput via trace-based JIT compilation
- **Pros**: Significantly faster for tight script loops; popular choice in shipping engines (WoW, Garry's Mod)
- **Cons**: Stuck at Lua 5.1 semantics — no integer type, no `goto`, no `<const>`/`<close>` from 5.4; LuaJIT development cadence is slow; MIT-licensed but with platform support caveats on some consoles; JIT warm-up introduces timing variability that complicates debugging
- **Rejection Reason**: GXLib's scripting role is gameplay glue, not inner loops. 5.4 language features and timing predictability outweigh JIT speed. Hot paths should be C++ anyway.

### Alternative 2: Hand-rolled Lua bindings (no sol2)
- **Description**: Write C bindings directly against the Lua C API
- **Pros**: Zero template bloat; absolute control over stack manipulation; no third-party dependency
- **Cons**: Every binding is boilerplate-heavy (push/pop, typecheck, userdata metatables); overload resolution is manual; easy to introduce stack imbalance bugs
- **Rejection Reason**: sol2 is header-only, stable, and well-tested. Hand-rolling loses hundreds of hours per release cycle for no measurable runtime win.

### Alternative 3: Multi-language scripting (Lua + C# / Python)
- **Description**: Embed both Lua (for modding) and a second language (C# via Mono, or Python) for first-party gameplay code
- **Pros**: Appeals to more author audiences; some teams prefer statically-typed scripting
- **Cons**: 2× the binding surface to maintain; 2× the sandbox/error/reload code paths; confusion about which language to use for what; runtime memory cost doubles
- **Rejection Reason**: Scope creep. GXLib targets DXLib-style indie game authoring — one scripting language is enough, and Lua covers both first-party and mod use cases.

## Consequences

### Positive
- One clear scripting surface: if it's not under `gxlib.*`, scripts can't reach it
- Sandbox-by-default protects end users from malicious mods
- sol2 template magic means new bindings are ~5-line registrations, not ~50-line stack-manipulation
- Hot reload works because the C++ side never holds raw function pointers — only sol2-tracked references
- VisualScript and text scripting share the same runtime, avoiding divergence

### Negative
- Adding a new component type to the ECS requires a Lua binding update if scripts need to touch it (deliberate friction)
- sol2's template error messages are notoriously long when bindings break (accepted as implementation cost)
- Lua 5.4's single-threaded nature means per-script isolation is required for parallelism — no multi-threaded Lua state per ADR-0005 policy
- Sandbox restrictions may surprise authors coming from "full Lua" environments; documented in author guide

### Risks
- **Sandbox escape via forgotten stdlib.** New Lua versions add functions (e.g., `os.setenv` hypothetically); the sandbox allowlist must be reviewed on every Lua version bump. *Mitigation*: explicit allowlist, not denylist; CI test asserts forbidden globals are `nil`.
- **Binding surface drifts from C++ API.** C++ rename without binding update leaves script callers broken. *Mitigation*: bindings live in `ScriptBindings.cpp` near the types they bind; CI grep gate flags C++ renames in types that have `sol::usertype` registrations.
- **Hot reload races with in-flight callbacks.** A reload during a script callback could invalidate the currently-running function. *Mitigation*: reload is deferred to a frame boundary; generation counter rejects stale callbacks.
- **sol2 compile time.** sol2 is heavy on templates. *Mitigation*: `Script/` module PCH, binding registrations split into `.cpp` files (not headers), thin header exposed to callers.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Requirement sourced from charter: "gameplay authors and modders can iterate without a C++ compile cycle" — satisfied by Lua + sol2 + hot reload + sandboxed surface |

## Performance Implications
- **CPU**: Lua execution + bindings budgeted at 1 ms/frame at 60 fps across all live scripts; trivial Lua→C++ call ≤ 1 μs
- **Memory**: One `sol::state` per script context (~200 KB baseline); bindings add ~50 KB of registered metatables
- **Load Time**: Script compilation is millisecond-scale; hot reload flush < 5 ms for typical scripts
- **Network**: N/A for this ADR (future replication of script state is a separate decision)

## Migration Plan

Not applicable — this ADR is retroactive. Scripting engine was introduced in Phase 1; Phase 5 broadened the bindings (Physics, Audio, IO, ECS, GUI). Going forward:

1. New C++ subsystems proposing script access must list the intended Lua surface in their ADR (what's exposed, what's private)
2. Adding a new binding module requires updating the sandbox allowlist review note
3. Any proposal to replace sol2 or add a second scripting language must supersede this ADR explicitly

## Validation Criteria
- CI test: `os.execute`, `io.open`, `package.loadlib`, `dofile`, `loadfile` are `nil` in a fresh `ScriptEngine` state
- CI test: `gxlib.io.read_asset` refuses `../` escapes from the configured asset root
- Microbenchmark: trivial Lua→C++ bound function call overhead ≤ 1 μs (averaged)
- Hot reload integration test: modify a script while running, FileWatcher triggers reload, old callbacks marked stale, new callbacks execute
- Sandbox regression test runs on every Lua version bump

## Related Decisions
- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — script `gxlib.world` surface mirrors this ADR's Key Interfaces exactly)
- CHANGELOG.md Phase 1 (Lua + sol2 initial integration)
- CHANGELOG.md Phase 5 (extended bindings: Physics, Audio, IO, ECS, GUI)
- `GXLib/Script/ScriptEngine.h`, `GXLib/Script/ScriptBindings.h` (source of truth)
