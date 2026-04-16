# ADR-0013: Networking Architecture (Reliable UDP, Property Replication, Prediction + Rollback, NAT Traversal)

## Status
Accepted

## Date
2026-04-16

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Networking |
| **Knowledge Risk** | LOW — UDP/TCP sockets, ACK-bitfield reliable channels, sequence numbers + RTT, snapshot interpolation, GGPO-style rollback, ICE/STUN-style NAT traversal, and TLS/WebSocket handshakes are well-documented patterns within LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/IO/Network/*` source tree (UDP/TCP/TLS/WebSocket sockets, NetworkManager, ReliableChannel, NetworkReplicator, ReplicatedProperty, NetworkPrediction, RollbackNetcode, InterestManagement, NATTraversal, MatchmakingLobby, HTTPClient, CloudSave), CHANGELOG Phases 3/4 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | Reliable channel ACK-bitfield correctness under 30% loss; RTT estimator stability under jitter spikes; rollback determinism across CPU SKUs (FP associativity); **motion matching SIMD-path determinism across CPU SKUs (AVX2 vs scalar fallback) inside rollback window — distance computation must produce identical "winners" before tie-breaking applies, per ADR-0014 §17 + §13 below**; InterestManagement spatial query cost under 1000 entities; TLS cert validation against revoked CAs; NAT traversal hole-punch success rate against common router NATs; bandwidth budget under 32-player snapshot rate; **EventBus replay-suppression correctness — side-effect handlers do not re-fire on rewound frames once forthcoming EventBus ADR lands** |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (doc), ADR-0004 (ECS — replicated state lives on ECS components and queries), ADR-0006 (Job System — packet send/receive, serialization, and snapshot building submit here), ADR-0007 (Asset Database — RPC payload and replicated-property schemas may reference asset ids), ADR-0009 (Physics — rollback re-simulation re-runs PhysicsWorld::Step deterministically per ADR-0009 fixed-timestep rule) |
| **Enables** | Future ADRs on dedicated server framework, voice chat (audio voice bus from ADR-0010), authoritative anti-cheat, cross-platform play, peer-to-peer mesh topologies |
| **Blocks** | None (code already exists across Phases 3/4; retroactive) |
| **Ordering Note** | The determinism contract in ADR-0009 (`variable_timestep_in_physics_solver`, deterministic island ordering) is binding for rollback re-simulation here |

## Context

### Problem Statement
A modern Windows game library needs a complete, in-process networking stack so games can ship multiplayer without bolting on a third-party netcode middleware. GXLib built this incrementally across Phases 3 (UDP/TCP foundation, NetworkManager, ReliableChannel) and Phase 4 (NetworkReplicator, NetworkPrediction, RollbackNetcode, InterestManagement, NATTraversal, MatchmakingLobby, TLS/WebSocket, CloudSave, HTTPClient). This ADR codifies the layering, the reliability model, the replication contract, the prediction/rollback split, and how networking integrates with ECS, Job System, and Physics — so future netcode features extend a stable spine instead of adding parallel implementations.

### Constraints
- Windows-only sockets (Winsock2); same scope as ADR-0002 / ADR-0011
- Must coexist with the Job System (ADR-0006) — no networking-owned thread pool beyond OS socket I/O threads; serialization, snapshot building, and replication tick run as Jobs
- Replicated state lives on ECS components (ADR-0004) — opaque entity references, no raw pointers in payloads
- Rollback re-simulation re-runs `PhysicsWorld::Step` — must respect ADR-0009 fixed-timestep + deterministic-island-ordering rules
- TLS for any external endpoint (matchmaking, cloud save, HTTPS); plaintext only on trusted local-LAN UDP/TCP gameplay channels
- No nondeterminism in serialized payloads — std::unordered_map iteration order, pointer addresses, and time(NULL) are forbidden in serializers
- NAT traversal must not assume UPnP success — fall back to relay
- Game shipping builds may strip the matchmaking / cloud-save modules independently from the core gameplay-network modules

### Requirements
- **Sockets**: UDP, TCP, TLS (cert-pinned + system-trust), WebSocket (TLS-upgraded handshake), HTTP/HTTPS client
- **ReliableChannel**: ACK-bitfield reliable layer over UDPSocket; sequence numbers; exponential backoff retransmit; RTT estimator (smoothed); per-channel packet types; receive-buffer up to 1024 sequences
- **NetworkManager**: server (`StartServer(port, maxClients)`) + client (`Connect(host, port)`) modes; per-client RPC dispatch (`RegisterRPC(id, handler)`); broadcast / unicast send
- **NetworkReplicator**: property replication on ECS entities — author marks fields with `ReplicatedProperty<T>`; replicator diffs and ships changes to interested peers
- **NetworkPrediction**: client-side input prediction + server reconciliation (snapshot + acknowledged-input lookup)
- **RollbackNetcode**: GGPO-style — predict opponent input, rollback + re-simulate on misprediction; bounded rollback window (default 8 frames)
- **InterestManagement**: per-client visibility filter (spatial / radius / explicit subscription) — only replicate what the client cares about
- **NATTraversal**: STUN-style discovery + UDP hole punching; relay fallback
- **MatchmakingLobby**: lobby create/join/list/ready/start; room metadata; HTTPS endpoint
- **HTTPClient**: GET/POST/PUT/DELETE; multipart; streaming downloads; TLS by default
- **CloudSave**: HTTPS-backed key/value blob persistence; per-account namespace
- **Bandwidth target**: 32-player gameplay session ≤ 50 KB/s downstream per client at 30 Hz snapshot rate

## Decision

**GXLib networking is layered: a Sockets layer (UDP/TCP/TLS/WebSocket) at the bottom; a Reliability layer (`ReliableChannel`) and a Session layer (`NetworkManager` + RPC) above; a Replication layer (`NetworkReplicator` + `ReplicatedProperty<T>` on ECS) and an Optimisation layer (`InterestManagement`); a Prediction/Rollback pair (`NetworkPrediction` + `RollbackNetcode`) for responsive gameplay; a Connectivity layer (`NATTraversal`, `MatchmakingLobby`); and a Service layer (`HTTPClient`, `CloudSave`). Reliable gameplay traffic uses UDP + ACK-bitfield, not TCP. State replicates over ECS via opaque `EntityHandle` ids — no raw pointers cross the wire. Rollback re-runs `PhysicsWorld::Step` under ADR-0009's fixed-timestep + deterministic-island-ordering guarantee. TLS is mandatory for all non-LAN endpoints. Networking jobs (serialize, build snapshot, dispatch RPCs) submit to the Job System.**

Concrete rules:

1. **Layered architecture (bottom-up).**
   ```
   Sockets         UDPSocket  TCPSocket  TLSSocket  WebSocket  HTTPClient
   Reliability     ReliableChannel (ACK bitfield, seq, RTT)
   Session         NetworkManager (server/client, RPC dispatch)
   Replication     NetworkReplicator + ReplicatedProperty<T>
   Optimisation    InterestManagement
   Prediction      NetworkPrediction + RollbackNetcode
   Connectivity    NATTraversal + MatchmakingLobby
   Service         CloudSave (HTTPS blob KV)
   ```
   Each layer sits on the one below; no skipping.

2. **Sockets (`UDPSocket`, `TCPSocket`, `TLSSocket`, `WebSocket`, `HTTPClient`).**
   - All Winsock2-backed. Non-blocking with internal poll thread (one OS thread per socket type, OS-owned) — gameplay code reads/writes via `Send()` / `Recv()` queues, not blocking calls.
   - `TLSSocket` uses Windows Schannel; cert validation against system trust store + optional pinning.
   - `WebSocket` does its own RFC 6455 handshake over `TCPSocket` or `TLSSocket`.
   - `HTTPClient` uses TLS by default; HTTP allowed only for `127.0.0.1` and explicit allowlist.

3. **Reliability (`ReliableChannel`).**
   - Header: 32-bit magic `GXRP`, type, size, seq, ack, ackBits (32 bits = 32-most-recent receipt confirmation).
   - Configurable: initial retransmit 100 ms, max 1000 ms (exponential), max 10 retransmits, 1024-deep sequence buffer.
   - RTT smoothed via classic SRTT/RTTVAR (RFC 6298-ish).
   - Per-packet type field separates RPCs / snapshots / control.
   - Loss statistics surfaced for adaptive bitrate decisions.

4. **Session (`NetworkManager`).**
   - Modes: server (`StartServer(port, maxClients)`) / client (`Connect(host, port)`). One mode per process.
   - `ClientId` = 32-bit assigned on connect; stable for the session.
   - RPC: `RegisterRPC(id, RPCHandler)`; dispatch on receive; payloads are `gx::Vector<uint8_t>` — caller serializes.
   - Heartbeat every 1 s; client disconnect after 5 s of silence.

5. **Replication (`NetworkReplicator` + `ReplicatedProperty<T>`).**
   - Authors mark fields on ECS components: `ReplicatedProperty<float> health{100.0f};`.
   - Replicator iterates each tick (default 30 Hz, configurable per-property), diffs against last-sent value, emits delta packets on the ReliableChannel.
   - Property identity = `(EntityHandle, propertyId)`. EntityHandle = ECS opaque (idx32+gen32) per ADR-0004 — never raw pointers.
   - Per-property settings: tick rate, reliability (reliable / unreliable / lossy-but-acked), authority (server-authoritative / client-authoritative / shared).
   - Spawn / despawn replicate via dedicated control packets carrying the entity's archetype seed.

6. **Interest management.**
   - `InterestManagement::SetVisibilityFilter(clientId, fn)` — per-client predicate over entity AABBs / tags.
   - Default filters: radius around player, explicit subscription, always-replicated (e.g. global game state).
   - Spatial queries reuse the broadphase from ADR-0009 (PhysicsWorld AABB-tree) where the entity has a physics body; non-physics entities are tracked in a smaller dedicated AABB tree.

7. **Prediction (`NetworkPrediction`).**
   - Client predicts its own input locally each tick; sends input packet (with sequence) to server.
   - Server simulates authoritatively, sends back snapshot + last-acknowledged-input-sequence.
   - On mismatch (predicted state ≠ server state for entities the client predicted), reconcile: snap to server state, replay queued unacknowledged inputs.

8. **Rollback (`RollbackNetcode`).**
   - Bounded rollback window (default 8 frames; configurable).
   - Each frame snapshots the gameplay state (ECS world delta + physics body state).
   - Predicts opponent input from last-known input.
   - On receiving real opponent input differing from prediction: rewind state to that frame, replay forward applying the corrected input. Re-simulation re-runs `PhysicsWorld::Step` per ADR-0009's fixed-timestep contract; determinism guarantees same result.
   - Forbidden: any nondeterministic operation in gameplay tick during rollback window (no `time(NULL)`, no `std::unordered_map` iteration in gameplay logic, no random without a replicated seed).

9. **NAT traversal (`NATTraversal`).**
   - STUN-style external-address discovery via configured STUN servers.
   - UDP hole-punching for peer-to-peer: both peers send simultaneously to each other's discovered external endpoint via the `MatchmakingLobby` rendezvous.
   - Fallback: relay through a server (TURN-style) when hole-punch fails. Fallback is automatic; gameplay code does not branch.
   - UPnP: opportunistic only — never assumed.

10. **Matchmaking & lobby (`MatchmakingLobby`).**
    - HTTPS REST against a configurable backend.
    - Lobby create/list/join/leave/ready/start with room metadata.
    - Player presence and ready-state heartbeated; idle players timeout.

11. **HTTPS service (`HTTPClient`, `CloudSave`).**
    - `HTTPClient` is the foundation — async, TLS by default.
    - `CloudSave` is a thin KV blob store (per-account namespace) over HTTPS — used for save data across machines, leaderboard write-back, etc. Endpoints configurable per-game.

12. **Threading & Job System.**
    - Socket I/O threads are OS-owned (Winsock overlapped IO); GXLib does not manage their lifetime beyond Initialize/Shutdown.
    - Per-frame networking work — serialize ReplicatedProperty deltas, build snapshots, dispatch incoming RPCs — runs as Jobs on JobSystem (ADR-0006); main-thread-final-apply queues land via `SubmitMainThread`.
    - Application code's networking calls (Send / RegisterRPC / Connect) are main-thread.

13. **Determinism rules (binding for rollback gameplay).**
    - Fixed timestep enforced (per ADR-0009 `variable_timestep_in_physics_solver`).
    - **Physics island solve is deterministic-reduction-ordered per ADR-0009 §15** — island Jobs are parallel but the final accumulation merges in island-ID order on the main thread after a barrier, so reduction order is independent of Job completion order or worker count.
    - All gameplay random uses `gx::Random` seeded from a replicated seed (per session); no `std::rand` / `std::random_device` in gameplay logic.
    - No iteration over hash containers (`std::unordered_*`, `gx::HashMap`) in gameplay logic — use `gx::VectorMap` / archetype iteration (deterministic) instead.
    - No floating-point fast-math in physics / rollback translation units (enforced by `nondeterministic_reduction_in_rollback_physics_stage` forbidden pattern).
    - ECS query result ordering is deterministic by entity-id within an archetype (per ADR-0004).
    - **Animation tick is included in rollback re-simulation** (per ADR-0014 §1 binding frame schedule). Re-simulation re-runs the per-frame schedule in order — Input → Animation tick → Physics step → (replication apply) — not just the physics step. Animation respects the same fixed-dt + no-fast-math + deterministic-IK-seed rules (per ADR-0014 §17).
    - **EventBus dispatch during rollback re-simulation runs in replay-suppression mode**: AnimationEventDispatcher events (footstep SFX, hit-flash VFX, gameplay damage windows) and any other side-effect-bearing EventBus events that fired during the original frame N MUST NOT re-fire when frame N is re-simulated. Implementation: gameplay tick during rollback sets `EventBus::SetReplayMode(true)`; subscribed handlers categorised as `idempotent` (state mutation only) still run; handlers categorised as `side_effect` (audio play, particle spawn, network send) are suppressed. Producer-side categorisation is the EventBus contract; consumers cannot opt out. **The `EventBus::SetReplayMode` API and `HandlerCategory { Idempotent, SideEffect }` enum are NOT YET specified in any ADR — a forthcoming EventBus ADR (working title ADR-0016 EventBus / Cross-System Communication) will codify the interface stub. Until that ADR lands, this contract is a forward-declaration: implementation MUST be added before any rollback-using game ships, and `nondeterministic_reduction_in_rollback_physics_stage` style of forbidden_pattern will track its absence.**
    - **Motion matching SIMD-path pin**: ADR-0014's pose-feature nearest-neighbour search must use one fixed SIMD path (AVX2 or scalar — chosen at process start by the determinism-conservative path) for any Animator inside the rollback window. Mixing AVX2 and scalar runs across a rollback range produces ULP-level distance differences that can flip "winner" before stable tie-breaking applies.
    - **Rollback snapshot memory bound**: snapshot size = (live-entity-count × per-entity replicated-state size) + (active-body-count × ~256 B physics state) + (active-Animator-count × ~4 KB pose buffer). For the ADR-0004 charter ceiling of 100k live entities with ~64 B replicated state typical, an 8-frame window is ~50 MB worst case. Games approaching this scale should reduce the rollback window or filter snapshot scope to predicted-by-this-client entities only.

14. **Security baseline.**
    - TLS for all non-LAN endpoints (matchmaking, cloud save, HTTP services).
    - System trust store + optional cert pinning for first-party endpoints.
    - Server-authoritative model for competitive games — never trust client-sent state changes for security-relevant properties (currency, inventory ownership, position-with-collision-validation).
    - Plaintext UDP allowed only for LAN-only deployments; ship default is encrypted (DTLS / handshake-encrypted) — currently a known gap, future ADR.

15. **Bandwidth budget.**
    - Snapshot rate default 30 Hz; per-property rates can drop further (slow-changing props at 5 Hz, etc.).
    - Target ≤ 50 KB/s downstream per client in a 32-player session under typical interest filtering.
    - InterestManagement is the primary tool for hitting the budget — drop-then-optimise loop.

### Architecture Diagram

```
   Application code (main thread)
       │ NetworkManager.StartServer / Connect / RegisterRPC / Send
       ▼
   gx::NetworkManager  ─────────────►  RPC dispatch
       │
       ▼
   gx::ReliableChannel  (seq + ackBits + retransmit + RTT)
       │
       ▼
   gx::UDPSocket  /  gx::TCPSocket  /  gx::TLSSocket  /  gx::WebSocket
       │  (Winsock2 — OS-owned IO threads)
       ▼
   network

   Replication tick (Job on ADR-0006 worker):
       ECS query for entities with ReplicatedProperty<*>
            │
            ▼
       NetworkReplicator diffs ─► InterestManagement filters per-client
            │
            ▼
       Serialize → ReliableChannel.Send

   Prediction / Rollback (gameplay tick, main thread):
       NetworkPrediction (client-side input → simulate → reconcile on snapshot)
       RollbackNetcode (snapshot + rewind + replay; PhysicsWorld::Step deterministic)

   Connectivity / Service:
       NATTraversal (STUN + hole-punch + relay fallback)
       MatchmakingLobby (HTTPS REST)
       HTTPClient → CloudSave (HTTPS KV)
```

### Key Interfaces

- `gx::NetworkManager::StartServer(port, maxClients)`, `Connect(host, port)`, `RegisterRPC(id, handler)`, `Send(clientId, type, data)`, `Broadcast(type, data)`
- `gx::ReliableChannel::Send(packet)`, `Receive() → vector<ReceivedPacket>`, `GetRTTMs()`, `GetLossPercent()`
- `gx::UDPSocket / TCPSocket / TLSSocket / WebSocket` — open / close / send / recv / status
- `gx::ReplicatedProperty<T>` — implicit conversion + assignment; `SetTickRate(hz)`, `SetReliability(...)`, `SetAuthority(...)`
- `gx::NetworkReplicator::Tick(world, dt)`, `RegisterEntity(EntityHandle)`, `UnregisterEntity(EntityHandle)`
- `gx::InterestManagement::SetVisibilityFilter(clientId, fn)`
- `gx::NetworkPrediction::PredictInput(seq, input)`, `Reconcile(serverSnapshot)`
- `gx::RollbackNetcode::Snapshot()`, `RollbackTo(frame)`, `Resimulate(frames)`
- `gx::NATTraversal::DiscoverExternalEndpoint(stunHost)`, `HolePunch(peer)`, `RequestRelay()`
- `gx::MatchmakingLobby::Create(meta)`, `Join(roomId)`, `List(filter)`, `SetReady(bool)`, `Start()`
- `gx::HTTPClient::Get/Post/Put/Delete(url, body, headers) → Future<Response>`
- `gx::CloudSave::Read(key) → Future<bytes>`, `Write(key, bytes) → Future<void>`

## Alternatives Considered

### Alternative 1: Adopt a third-party netcode (Photon / Mirror / GameNetworkingSockets / ENet)
- **Description**: Wrap an established netcode middleware
- **Pros**: Battle-tested; rich features; community knowledge
- **Cons**: Photon is licensed/SaaS-coupled; Mirror is Unity-shaped; GameNetworkingSockets is Steam-coupled (Steamworks dep); ENet covers only the reliability layer; none integrate cleanly with our ECS / Job System / determinism contract; all add a non-trivial dependency
- **Rejection Reason**: In-house stack already exists across Phases 3/4 with native ECS / Job System integration. The cost is maintenance; the win is no licensing entanglement and no double-thread-pool problem.

### Alternative 2: TCP-only (skip UDP + reliability layer)
- **Description**: Use TCP for everything; rely on TCP's built-in reliability
- **Pros**: Simpler stack; no ACK bitfields to write
- **Cons**: TCP head-of-line blocking is fatal for real-time gameplay (one lost packet stalls all subsequent until retransmit completes); bufferbloat under congestion; can't selectively send unreliable packets (snapshots) cheaply
- **Rejection Reason**: Real-time gameplay needs UDP semantics. TCP is correct only for matchmaking / cloud / HTTP — exactly where we already use it.

### Alternative 3: Pure peer-to-peer (no server-authoritative mode)
- **Description**: Lockstep / shared-input-only model; no dedicated authority
- **Pros**: No server cost; works for 1v1 / small N
- **Cons**: Cheating-vulnerable (client trusts peer); doesn't scale past ~4 players; lockstep stalls on slowest peer; required for some genres but not as the only option
- **Rejection Reason**: GXLib must support both authoritative-server and rollback-P2P patterns. Choosing only one would foreclose half of multiplayer game design.

### Alternative 4: Drop rollback (ship only prediction + reconcile)
- **Description**: Server-authoritative + client prediction only; no GGPO-style rewind
- **Pros**: Simpler than rollback; less determinism burden
- **Cons**: Fighting / racing / fast-paced PvP genres need rollback for responsive feel; without it, those genres can't ship on this engine
- **Rejection Reason**: Rollback is hard but unique; cutting it would limit GXLib's genre fit. The determinism work is also valuable for save/load + replay regardless of netcode.

## Consequences

### Positive
- One coherent stack: socket → reliability → session → replication → interest → prediction → rollback. Each layer is a clean swap point.
- Native ECS integration via `ReplicatedProperty<T>` — author marks the field, replication just works
- Rollback + determinism unlock fighting / racing / fast PvP genres
- TLS / cert pinning baked in for all external services
- HTTP / WebSocket / TLS live in the same module — game services share a single client
- Job System integration means networking work scales across cores without a private thread pool

### Negative
- Determinism rules constrain gameplay code (no hash-container iteration, no `time(NULL)`, fixed timestep) — easy to violate without realising
- Rollback snapshot cost grows with game state size — bounded window helps but is not free
- Plaintext UDP gameplay is the current default for LAN — encrypted gameplay UDP is a future ADR
- NAT hole-punch success rate depends on router behaviour (symmetric NAT defeats most strategies); relay fallback is bandwidth cost
- Maintaining sockets/TLS/WebSocket/HTTP ourselves is a security surface — must track CVEs in Schannel and our own code

### Risks
- **Determinism drift across CPUs** in rollback re-simulation. *Mitigation*: ADR-0009 deterministic-island-ordering, fast-math disabled in physics/network TUs, replicated seed for `gx::Random`. Validation: golden-trace test on AMD + Intel.
- **Replication storm** when many ReplicatedProperty deltas land in one frame. *Mitigation*: per-property tick rate; coalescing within a tick; priority-based queue; max packets-per-frame cap.
- **Interest filter leak** — entity becomes visible to client mid-frame, client spawns it without prior history. *Mitigation*: spawn packet carries full state; replicator emits state from "newly visible" snapshot.
- **TLS cert validation bypass** if pinning misconfigured. *Mitigation*: pinning is opt-in over system-trust validation; fail closed on cert errors; debug builds log the cert chain.
- **Rollback chain too deep** under prolonged packet loss, causing perceptible rewind. *Mitigation*: bounded rollback window (default 8 frames); on overflow, freeze input prediction and snap to next server snapshot.
- **NAT traversal failure rate** in production. *Mitigation*: relay fallback, automatic; metric exposed for telemetry; matchmaking can prefer relay-friendly peers.
- **Bandwidth exceeds budget** under poor interest filtering. *Mitigation*: per-channel byte-rate metering; debug overlay shows top-N entities by replication cost.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-chr-005 ("Networking: Reliable UDP, replication, lag compensation") — elevated from Gap to Covered |

## Performance Implications

- **CPU**: Replication tick ≤ 0.5 ms/frame for 1000 entities × 30 Hz with interest filtering at 32 clients; rollback re-simulation cost ≈ N-frames × per-frame physics cost (≤ 8 × 3 ms = 24 ms worst case spike, amortised much less)
- **Memory**: Per-entity replication state ~64 bytes baseline; snapshot history for rollback ~state-size × window-frames; ReliableChannel sequence buffer 1024 × ~64 bytes
- **Load Time**: TLS handshake adds ~100-300 ms to first connect; matchmaking lobby join ~200 ms typical
- **Network**: Target ≤ 50 KB/s downstream per client at 32-player / 30 Hz with InterestManagement; budget enforced at the replicator metering layer

## Migration Plan

Not applicable — this ADR is retroactive across Phases 3/4. Going forward:

1. Encrypted UDP gameplay (DTLS or noise-protocol handshake) is a future ADR
2. Dedicated server framework (headless build flags, server-side InterestManagement tuning, container packaging) is a future ADR
3. Voice chat over the Voice audio bus (ADR-0010) is a future ADR — adds Opus codec + jitter buffer
4. Anti-cheat (server-side input validation, encrypted client telemetry) is a future ADR
5. Cross-platform play requires a non-Windows backend ADR for sockets

## Validation Criteria

- **Reliable channel under loss**: drop 30% of packets simulated — all retransmits succeed within 10 attempts; RTT estimator stays within ±20% of true RTT
- **Rollback determinism**: identical input streams from frame 0 produce byte-identical state at frame 10000 across AMD Ryzen + Intel Core
- **32-player bandwidth**: 32-client session with 200 dynamic entities under default InterestManagement — downstream ≤ 50 KB/s per client measured over 5 minutes
- **NAT hole-punch**: success rate ≥ 70% across home routers (full-cone, restricted-cone, port-restricted); 100% with relay fallback
- **TLS validation**: connecting to a server with revoked cert fails; with self-signed and pinning matching the pin succeeds; with self-signed and no pinning fails
- **Reconnect**: client disconnects mid-game, reconnects within 30 s — server snapshot brings client to current state without RPC duplication
- **Replication correctness**: server changes a ReplicatedProperty<float> on entity E; all interested clients reflect new value within 2 ticks; uninterested clients never see it
- **CloudSave**: write 1 MB blob, read back — bytes identical; concurrent writes from two clients resolve last-write-wins per documented behaviour

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0004 (ECS — replicated state lives on components; EntityHandle on the wire)
- ADR-0006 (Job System — serialize / snapshot / dispatch jobs run here)
- ADR-0007 (Asset Database — RPC and replication payloads may carry asset ids)
- ADR-0009 (Physics — rollback re-simulation re-runs PhysicsWorld::Step under fixed-timestep + deterministic-island-ordering)
- (Future) Encrypted-UDP gameplay ADR
- (Future) Dedicated Server framework ADR
- (Future) Voice chat ADR (uses ADR-0010 Voice bus)
- `GXLib/IO/Network/{NetworkManager,ReliableChannel,NetworkReplicator,ReplicatedProperty,NetworkPrediction,RollbackNetcode,InterestManagement,NATTraversal,MatchmakingLobby,HTTPClient,CloudSave,UDPSocket,TCPSocket,TLSSocket,WebSocket}.{h,cpp}` (source of truth)
- CHANGELOG.md Phases 3, 4
