# ADR-0006: Job System — Multi-threaded Task Scheduler

## Status
Accepted

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Core / Concurrency |
| **Knowledge Risk** | LOW — task-stealing schedulers (fork/join, work-stealing) are well-documented; C++20 `std::jthread`, `std::atomic`, and `std::function` are within the LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Core/JobSystem.{h,cpp}`, `CHANGELOG.md` Phase 5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Scaling test on 4-, 8-, and 16-core CPUs — throughput should scale near-linearly up to worker count; no priority inversion under High-priority jobs; no deadlock when `SubmitAfter` graph depth ≥ 8 |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy) |
| **Enables** | ECS parallel system execution (ADR-0004 extension), `ParallelRenderQueue` (ADR-0002), future Physics multi-threaded broadphase, Hot Reload async reloads, `IO/` async file loading, `Graphics/Pipeline/` command-list recording on worker threads |
| **Blocks** | None (code already exists since Phase 5; this ADR is retroactive) |
| **Ordering Note** | Foundational for any ADR that proposes parallel execution. Should be Accepted before rendering-pipeline or physics ADRs add work-generation stories. |

## Context

### Problem Statement
GXLib's workloads — rendering command recording, physics simulation, AI decisions, asset loading, particle simulation — scale with entity count and draw complexity. Without a shared scheduler, each subsystem is forced to either run on the main thread (bottleneck) or spin up its own thread pool (resource waste, priority contention, lifecycle chaos). A shared Job System lets every subsystem submit fine-grained work, lets the scheduler balance across available cores, and keeps worker-thread ownership out of subsystem code.

The existing `GXLib/Core/JobSystem.h` (Phase 5) already exposes `Submit`, `SubmitAfter` (dependencies), `ParallelFor`, and priority tiers. This ADR records the architectural choice behind that API — what scheduling model, what dependency semantics, what threading contract subsystems must follow — so future subsystem ADRs don't reinvent their own threading.

### Constraints
- C++20, MSVC + Clang-cl, Windows-only
- Must coexist with DX12 command-list recording (D3D12 is thread-safe for different `ID3D12CommandList` objects but not shared state)
- Must coexist with Lua (Lua 5.4 states are NOT thread-safe — one `sol::state` per thread, or main-thread-only; per ADR-0005)
- Must not dominate frame time on idle frames (workers should sleep, not spin, when no work is ready)
- Must support graph-shaped dependencies (not just linear chains) — typical case: "after physics step AND after AI tick, run animation blend"

### Requirements
- Submit O(1) per job on the caller side (no allocation on hot path where possible)
- Scale near-linearly to the number of physical cores for embarrassingly-parallel workloads (`ParallelFor`)
- Graph-shaped dependencies: one job can have multiple predecessors; one job can have multiple successors
- Priority tiers: High (frame-critical, pre-empts Normal), Normal (default gameplay), Low (background — asset bake, profiler, telemetry)
- Main thread affinity: some jobs (e.g., any DX12 swap-chain call, any Lua state touch) must run on a specific thread — the system must support main-thread-only jobs
- No re-entrance into the Job System from within a job that holds an unrelated lock (to avoid deadlock risk)
- Clean shutdown: `Shutdown()` drains the queue and joins workers; no leaked threads

## Decision

**GXLib uses a task-stealing (work-stealing) scheduler with per-worker deques, fixed worker thread count (default: hardware concurrency minus 1, reserving one core for the main thread), graph-shaped dependency tracking via a reference-counted job handle, and three priority tiers. One singleton `JobSystem` serves the whole process. Subsystems submit jobs; they do not own worker threads.**

Concrete rules:

1. **Scheduler model: work-stealing.** Each worker thread has its own LIFO deque. A worker takes from its own deque's top (LIFO — fresher work, better cache locality); an idle worker steals from another worker's deque bottom (FIFO on the victim — avoids cache contention). High-priority jobs go to a shared priority queue consulted first by every worker. Low-priority jobs go to a shared background queue consulted only when the per-worker deque and priority queue are empty.

2. **Worker count.** Default = `std::thread::hardware_concurrency() - 1` (reserve one core for the main thread). User may override at `Initialize(uint32_t workerCount)`. Workers are created once, live for the process lifetime, and are joined on `Shutdown()`.

3. **Job handle.** `JobHandle` is a 64-bit opaque ID (generation-counted). `IsValid()` returns true only for handles returned by a successful submit that has not been invalidated. Destroying a handle does NOT cancel the job — cancellation is not supported (jobs are short; caller handles early-exit internally).

4. **Dependencies.** `SubmitAfter(JobHandle dep, JobFunction fn)` and `SubmitAfter(span<JobHandle> deps, JobFunction fn)` build a DAG. A job is made runnable only when all of its predecessor counters reach zero. Graph cycles are detected in debug builds and rejected; release builds assume correct DAG.

5. **Priority tiers.**
   - **High** — frame-critical, consulted before per-worker deques; intended for "this must run this frame, not next" like swap-chain-adjacent prep
   - **Normal** — default for gameplay / simulation / rendering — goes into the submitting worker's deque
   - **Low** — background work (asset bake, screenshot encode, profiler flush) — runs only when the system would otherwise be idle
   Priority is advisory; the scheduler does not forcibly preempt.

6. **`ParallelFor(begin, end, batchSize, fn)`.** Splits `[begin, end)` into chunks of `batchSize` and submits each chunk as a Normal-priority job. Returns a single `JobHandle` that is done when all chunks are done. `Wait(h)` is callable from any thread including a worker (workers help drain other work while waiting, to avoid deadlock).

7. **Main thread jobs.** A separate `SubmitMainThread(JobFunction fn)` queue exists. Jobs submitted here run only when the main thread calls `JobSystem::ProcessMainThreadJobs(budget_ms)` — typically once per frame. This is the sole path for DX12 swap-chain calls and for any Lua state touch (per ADR-0005's single-state-per-thread rule).

8. **Locking and re-entrance.** Jobs must not re-enter the Job System while holding a lock that another job might want. The scheduler itself is lock-free on the deque hot path (lock-free Chase-Lev deque); cross-worker queues use short spinlocks.

9. **No fibers.** Pure thread-per-worker. Fibers add debugger, profiler, and TLS complexity that outweighs the benefit for GXLib's workload shapes.

10. **Shutdown.** `Shutdown()` marks the system as draining, lets workers finish in-flight jobs, refuses new submits, then joins. Idempotent.

### Architecture Diagram

```
Subsystem (ECS system, Renderer, Physics, ...)
   │
   │  auto h = gx::JobSystem::Instance().Submit(fn);
   │  auto h2 = gx::JobSystem::Instance().SubmitAfter(h, fn2);
   │  gx::JobSystem::Instance().ParallelFor(0, N, 64, fn_i);
   ▼
gx::JobSystem   (singleton)
   ├── High priority queue      (shared, MPMC, consulted first)
   ├── Per-worker LIFO deques   (work-stealing; caller submits here by default)
   ├── Low priority queue       (shared, consulted only when idle)
   ├── Main-thread-only queue   (drained by ProcessMainThreadJobs)
   └── Dependency tracker       (predecessor counters → on-ready dispatch)
         │
         ▼
Worker threads (std::thread × N)   ─── pinned or free (default: free)
   each: pop own deque LIFO → steal others' FIFO → poll shared queues → sleep on cv

                Main thread
                   │
                   └─► ProcessMainThreadJobs(budget) — drains main-thread queue
                                                      for DX12 swap-chain,
                                                      Lua state touches, etc.
```

### Key Interfaces

C++ (existing API, recorded here as the contract):

- `static gx::JobSystem& gx::JobSystem::Instance();`
- `bool Initialize(uint32_t workerCount = 0);` — 0 = auto (hw concurrency − 1)
- `void Shutdown();`
- `JobHandle Submit(JobFunction, JobPriority = Normal);`
- `JobHandle SubmitAfter(JobHandle dep, JobFunction, JobPriority = Normal);`
- `JobHandle SubmitAfter(std::span<const JobHandle> deps, JobFunction, JobPriority = Normal);`
- `JobHandle ParallelFor(size_t begin, size_t end, size_t batchSize, std::function<void(size_t)>);`
- `void Wait(JobHandle);` — caller cooperates, stealing from workers
- `bool IsComplete(JobHandle) const;`
- `JobHandle SubmitMainThread(JobFunction);`
- `void ProcessMainThreadJobs(float budget_ms);`

## Alternatives Considered

### Alternative 1: OS ThreadPool (Windows Thread Pool API)
- **Description**: Use `CreateThreadpoolWork` and Windows-provided pool
- **Pros**: No own thread management; OS handles dynamic sizing
- **Cons**: No per-task priority graph beyond what the OS offers; less control over worker count (important for avoiding contention with DX12 command queues); no built-in work-stealing semantics; difficult to integrate with custom dependency tracking
- **Rejection Reason**: Game-engine scheduling benefits from fixed worker count and explicit priority tiers; OS pool is optimised for async I/O, not per-frame compute bursts

### Alternative 2: Fiber-based scheduler (a-la Naughty Dog's "Parallelizing the Naughty Dog Engine")
- **Description**: Thousands of cooperative fibers on a small worker thread pool; Wait suspends the fiber, resumes another
- **Pros**: `Wait` inside a job is cheap (no new thread); elegant expression of nested parallelism
- **Cons**: Fibers interact badly with the MSVC debugger, with TLS (per-thread Lua state assumptions break), and with DX12 command-list recording (command lists have thread affinity). Fiber stack accounting is a footgun. `std::jthread` / C++20 thread support doesn't assume fibers.
- **Rejection Reason**: The TLS / debugger / DX12 friction outweighs the nested-parallelism elegance. Work-stealing with cooperative Wait (workers help drain while waiting) gives most of the benefit.

### Alternative 3: `std::async` / per-subsystem futures
- **Description**: Each subsystem uses `std::async(std::launch::async, ...)` as needed
- **Pros**: Zero code to write in the engine
- **Cons**: No shared pool — each subsystem competes for threads; no dependency graph; no priority tiers; `std::async` with `launch::async` spawns a thread per call on many implementations
- **Rejection Reason**: Gives up all scheduling control — explicit anti-pattern for game engines

### Alternative 4: Single shared MPMC queue (no work-stealing)
- **Description**: One global queue, workers pop from it
- **Pros**: Simpler; no deque complexity
- **Cons**: Global queue contention as worker count grows; poor cache locality (consecutive jobs from the same caller scatter across workers); standard game-engine benchmark shows work-stealing wins for game workloads above ~4 workers
- **Rejection Reason**: Doesn't scale past small core counts; negates the primary reason to have a Job System on 8+ core modern CPUs

## Consequences

### Positive
- Every subsystem writes serial-looking code + submit; the scheduler handles balancing
- Work-stealing means idle workers pick up slack automatically; no manual load-balancing per subsystem
- Dependency graph lets "physics → animation → rendering" express as handles without locks
- Main-thread-only queue gives a clean escape hatch for DX12 / Lua without polluting the general queue
- Fixed worker count is predictable for performance tuning

### Negative
- Work-stealing deque (Chase-Lev) is subtle lock-free code — bugs are hard to reproduce; requires thorough stress testing
- Priority tiers are advisory, not preemptive — a misbehaving Normal-priority job can starve a High-priority job briefly
- `Wait` from a worker thread that helps-drain means the call-graph becomes non-linear — debugging stack traces get stranger
- No cancellation: a submitted job always runs to completion (simplifies the scheduler, pushes early-exit into the job body)

### Risks
- **Deadlock from Wait chains when all workers are waiting on each other.** *Mitigation*: Wait from workers must steal — i.e., workers help drain other work before sleeping. Debug-build deadlock detector flags Wait-on-own-dependency.
- **Priority inversion** — Low job holds a lock, High job waits. *Mitigation*: document the forbidden pattern; never take a shared lock inside a Low-priority job.
- **DX12 command list thread violations.** *Mitigation*: architectural guarantee — command-list recording uses the per-worker `ID3D12CommandList` rule (one list per worker thread); all submission to `ID3D12CommandQueue` routes through the main thread via `SubmitMainThread`.
- **Lua state touched from worker.** *Mitigation*: registered forbidden pattern (`lua_state_cross_thread`) — Lua must stay on the owning thread; cross-thread Lua submits go through `SubmitMainThread`.
- **Oversubscription when user code spawns its own threads.** *Mitigation*: document: users should prefer Job System submit over raw `std::thread`; Forbidden pattern registered.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Charter-level TR-chr-006 ("Job System / multi-threaded simulation") — satisfied by this ADR |

## Performance Implications
- **CPU**: Job System overhead target ≤ 0.2 ms/frame for scheduling work at 60 fps with typical frame graph (~ 200 jobs). Worker sleep/wake latency ≤ 50 μs via condition variable.
- **Memory**: Per-worker deque (~ 4 KB fixed), shared queues (~ 16 KB), dependency tracker grows with outstanding job count (~ 48 bytes per in-flight handle)
- **Load Time**: Init cost ≈ worker spawn time (~ 1 ms total for 8 workers)
- **Network**: N/A

## Migration Plan

Not applicable — this ADR is retroactive. `JobSystem` was introduced in Phase 5 (2026-03-01). Going forward:

1. Any new subsystem proposing parallel work must submit through `JobSystem` — not raw `std::thread`
2. Any ADR proposing a new dependency graph or scheduler must supersede this one
3. DX12 command-list parallelism (ADR-0002's `ParallelRenderQueue`) builds on Job System via per-worker command lists + main-thread submit

## Validation Criteria
- Scaling test: `ParallelFor(0, 1,000,000, 1024, trivial_fn)` shows near-linear speedup up to `workerCount` threads on 4-/8-/16-core CPUs
- Dependency graph test: diamond DAG (A → B, A → C, B&C → D) executes in correct topological order under 10,000 randomised submissions
- Deadlock test: `Wait` called from worker does not deadlock when all workers are waiting and at least one job is ready
- Priority test: High-priority job submitted while 100 Normal jobs queued starts executing within 2 worker time slices
- Shutdown test: process exits cleanly within 100 ms after `Shutdown()` call with in-flight jobs
- Stress test: 100,000 submits with random dependencies, no thread sanitizer violations

## Related Decisions
- ADR-0001 (Documentation strategy)
- ADR-0002 (DX12 backend — `ParallelRenderQueue` builds on this; per-worker command lists require the fixed worker count contract)
- ADR-0004 (ECS — future parallel system execution will use Job System)
- ADR-0005 (Lua — `lua_state_cross_thread` forbidden pattern applies; main-thread queue is the Lua-safe path)
- `GXLib/Core/JobSystem.{h,cpp}` (source of truth)
- CHANGELOG.md Phase 5 (Job System initial introduction)
