# ADR-0010: Audio Architecture (XAudio2 Backend, Bus-Based Mixer, 3D Spatial + DSP)

## Status
Accepted

## Audio routing defects resolved 2026-04-19

IXAPO runtime verification (sprint-002 Task 2) surfaced three engine
defects in the AudioBus / SourceVoice / XAPOBridge wiring that rendered
the Layer-2 `IAudioEffect` extension functionally inert despite passing
unit tests. Fixed same-session:

1. **MusicPlayer SourceVoice bypassed AudioBus**.
   `CreateSourceVoice` was called with no `pSendList`, defaulting to
   MasteringVoice direct. Audio never flowed through the BGM bus's
   SubmixVoice → `Process()` never dispatched regardless of
   `AddEffect` success.
   Fix: added `MusicPlayer::SetOutputSubmixVoice(IXAudio2SubmixVoice*)`;
   `AudioManager::Initialize` calls it with
   `mixer.GetBGMBus().GetSubmixVoice()` after mixer init succeeds.
   `CreateSourceVoice` now uses a `XAUDIO2_VOICE_SENDS` list pointing
   at the BGM SubmixVoice.

2. **AudioBus effect chain `OutputChannels` hardcoded to 2**.
   `AudioBus::RebuildEffectChain` set
   `XAUDIO2_EFFECT_DESCRIPTOR::OutputChannels = 2` regardless of the
   SubmixVoice's actual channel count. On devices where MasteringVoice
   is stereo (2 ch) this accidentally matched; on any other device
   (mono, 5.1, 7.1) the mismatch would silently fail `SetEffectChain`.
   Fix: query `GetVoiceDetails` on the SubmixVoice and pass
   `InputChannels` to `OutputChannels`.

3. **XAPOBridge registration flags insufficient** — user-observed
   root cause.
   `GetRegistrationProperties` set only `XAPO_FLAG_INPLACE_REQUIRED`.
   XAudio2 rejects XAPO registrations missing the standard
   `XAPO_FLAG_*_MUST_MATCH` set — `SetEffectChain` returns
   `XAUDIO2_E_INVALID_CALL (0x88960001)`. `AudioBus::AddEffect`
   succeeds as far as our code sees (HRESULT wasn't surfaced), but
   XAudio2 never dispatches the XAPO's `Process()`.
   Fix: `Flags = XAPO_FLAG_CHANNELS_MUST_MATCH
   | XAPO_FLAG_FRAMERATE_MUST_MATCH
   | XAPO_FLAG_BITSPERSAMPLE_MUST_MATCH
   | XAPO_FLAG_BUFFERCOUNT_MUST_MATCH
   | XAPO_FLAG_INPLACE_SUPPORTED` (Microsoft XAPO sample standard set).

All three defects are now regression-guarded by
`examples/11-custom-audio-dsp/` which displays the `SetEffectChain`
HRESULT and a live `Process() calls` counter on screen. If any of
these three regresses, the counter stays at 0 (visible in the example
window) rather than silently failing.

Additionally: `AudioBus` now exposes `GetLastEffectChainStatus()` and
`GetLastEffectChainOutputChannels()` diagnostic getters so any future
runtime verification can read the HRESULT without adding new
instrumentation.

**Out of scope but tracked**: `SoundPlayer` has the same SubmixVoice-
bypass issue as MusicPlayer had before the fix. SE-bus `IAudioEffect`
chains still don't receive audio. Tracked as
`TR-defer-soundplayer-bus-routing` in
`architecture-traceability.md`. Fix pattern is identical to
MusicPlayer's.

## Date
2026-04-15

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | Audio |
| **Knowledge Risk** | LOW — XAudio2 is stable and well-documented since Windows 8; OGG Vorbis streaming and standard DSP effect algorithms (biquad filters, Freeverb, delay lines, feedback-compressor) are within LLM training data |
| **References Consulted** | `docs/engine-reference/gxlib/VERSION.md`, `GXLib/Audio/*` source tree, CHANGELOG Phases 0/1/3/5 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | 3D attenuation curves on headphones vs 5.1 output; OGG streaming under seek; occlusion raycast cost under 256-emitter scene; reverb tail continuity across ReverbZone transitions; DSP chain stability at 48 kHz / 44.1 kHz device rates |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (doc strategy), ADR-0006 (Job System — audio mix runs on a dedicated worker under this), ADR-0007 (Asset Database — Sound / SoundBank / OGG flow through AssetDatabase), ADR-0009 (Physics — AudioOcclusion queries PhysicsWorld for raycasts) |
| **Enables** | Future ADRs on advanced DSP (convolution reverb, HRTF spatial), voice chat (networking), audio streaming from disk via DirectStorage, interactive music system |
| **Blocks** | None (code already exists across Phases 0/1/3/5; retroactive) |
| **Ordering Note** | Any gameplay ADR wanting sound playback references this ADR's Key Interfaces |

## Context

### Problem Statement
Audio for a modern game library must cover: one-shot SFX playback, 3D-spatialised emitters with distance attenuation, streamed music (OGG), environmental reverb zones, occlusion based on physics geometry, a bus/mixer hierarchy for master/music/SFX/voice balancing, DSP effects (low-pass, high-pass, reverb, delay, compressor) insertable on any bus, and mix snapshots for scene-state changes (menu vs in-game vs underwater). GXLib built this incrementally across Phases 0 / 1 / 3 / 5 on XAudio2. This ADR records the backend commitment, the mixer topology, the DSP architecture, and how audio integrates with the rest of the engine (Asset DB, Physics, Job System).

### Constraints
- Windows-only backend (aligns with DX12 scope, ADR-0002); no cross-platform audio abstraction needed
- Must hit consumer-expected audio latency — ~20 ms end-to-end for gameplay SFX
- Device rate is not fixed (44.1 kHz / 48 kHz / 96 kHz devices all possible); pipeline must resample correctly
- Audio callback runs on a dedicated OS thread (XAudio2 internal) — mix must not stall; all allocations off the audio path
- Must not own its own thread pool beyond what XAudio2 provides internally; offline work (DSP parameter changes, streaming decode) goes through Job System (ADR-0006)
- Sound assets / SoundBank / OGG stream definitions flow through AssetDatabase (ADR-0007) with hot reload support
- Occlusion queries to PhysicsWorld must be rate-limited (occlusion per-emitter ≤ 10 Hz) to avoid bogging the physics raycast budget

### Requirements
- 2D one-shot playback (DXLib `PlaySoundMem`-equivalent): load-once, play-many, low-latency
- 3D spatialised emitters with distance + cone attenuation, Doppler
- Streamed OGG music with gapless loop + crossfade
- Bus hierarchy: Master → {Music, SFX, Voice, Ambient} with per-bus volume + mute + solo
- Per-bus DSP insert chain (up to 4 effects per bus): low-pass, high-pass, Freeverb, delay, feedback-compressor
- ReverbZone volumes: enter/leave blends send level to a reverb bus
- AudioOcclusion: raycast emitter → listener; occluded emitters get low-pass + attenuated send
- AudioSnapshot: named mixer-state presets (Menu, InGame, Paused, Underwater) with smooth interpolation
- Voice-count budget: 64 simultaneous 3D voices + 8 music / streamed + unlimited via virtualisation (stealing)
- Must coexist with Compat layer (ADR-0003) — `gx::PlaySoundMem` routes here with DXLib-shaped semantics

## Decision

**GXLib uses XAudio2 as its sole audio backend on Windows. A bus-based mixer (`AudioMixer`) with 4 default buses routes all emitters. Each bus carries an insert chain of up to 4 DSP effects. 3D emitters use standard distance-attenuation curves + cone model + Doppler, with optional raycast-driven occlusion rate-limited to 10 Hz per emitter. OGG streamed music plays through a dedicated streaming voice with a decode Job on the Job System. Voice virtualisation steals inaudible voices when the 64-voice budget is exceeded. ReverbZones and AudioSnapshots blend mixer state smoothly. Sound assets flow through AssetDatabase.**

Concrete rules:

1. **Backend: XAudio2 (Windows SDK).** No WASAPI direct, no FMOD, no Wwise. See Alternatives.

2. **`AudioDevice` / `AudioManager`.** `IAudioDevice` is a thin wrapper over `IXAudio2` + `IXAudio2MasteringVoice`. `AudioManager` is the singleton facade — init/shutdown, emitter creation, bus routing, global volume. All public API routes through AudioManager.

3. **Bus topology (default).** `Master` → {`Music`, `SFX`, `Voice`, `Ambient`}. Each non-Master bus carries: volume, mute, solo, optional send to the `Reverb` bus, insert DSP chain (≤ 4 slots). Custom buses may be added via `AudioMixer::CreateBus("name", parent)`. `Reverb` is a special send-target bus carrying the Freeverb DSP; dry paths bypass it.

4. **3D spatial model.**
   - Distance attenuation curves: `Linear`, `Logarithmic`, `Inverse`, `Custom` (user-authored curve)
   - Cone model: inner angle (full gain), outer angle (outer gain factor), outer gain
   - Doppler on by default; per-emitter disable flag
   - Listener position + velocity + orientation from `AudioListener` component (one active listener)
   - Speaker panning via XAudio2 `X3DAudio` helper
   - HRTF is out of scope for this ADR (future addition)

5. **Occlusion (`AudioOcclusion`).**
   - For each 3D emitter within a configurable range (default 50 m), raycast emitter→listener against PhysicsWorld
   - Occlusion updates are rate-limited per emitter: max 10 Hz, staggered across emitters to smooth the budget
   - Raycasts submitted to `JobSystem` as Normal-priority batched jobs. Concurrent execution with `PhysicsWorld::Step` is permitted under ADR-0009 §14's versioned-snapshot read contract — occlusion does not need to wait for the between-step window. Occlusion may observe the prior-frame snapshot of the broadphase; this is acceptable given the 10 Hz update rate.
   - Results apply: direct send attenuation + low-pass cutoff; wet/reverb send NOT attenuated (reverb goes around occluders)
   - Uses forbidden pattern `unchecked_optional_gpu_feature`-style caps gating: occlusion can be globally disabled on low-end configs

6. **Streaming music (`MusicPlayer` + `OggStream`).** One OGG stream = one `IXAudio2SourceVoice` with a ring of 4 buffers (≈ 500 ms each). A decode Job (Low priority on Job System) refills buffers ahead of playback. Seek / loop / crossfade supported; the decoder queues the next OGG for crossfade without clicks.

7. **Voice virtualisation.** When live voice count exceeds 64: inaudible voices (below hearing threshold, or masked) are marked virtual — their clock advances but no sample output. On gaining audibility they un-virtualise from the correct play position. Deterministic selection: lowest priority + lowest effective volume steal first.

8. **DSP effects (`AudioDSP`, `AudioEffect`).** Implementations:
   - `LowPassFilter`, `HighPassFilter` — biquad
   - `Reverb` — Freeverb (8-comb + 4-allpass)
   - `Delay` — ring-buffer with feedback
   - `Compressor` — feedback, soft-knee, lookahead ≈ 5 ms
   Effects are inserted into a bus's chain at authoring time; runtime parameter changes are thread-safe via a lock-free parameter-swap structure (double-buffered params + atomic publish).

9. **`AudioSnapshot`.** Named mixer-state capture (bus volumes, sends, DSP params). `AudioManager::BlendToSnapshot(name, duration)` interpolates current state → snapshot over `duration` with an ease curve. One active blend at a time; queued snapshot-change overrides if duration finishes within ε.

10. **Asset integration.** `Sound` (PCM loaded, short) and `SoundBank` (multi-sample, streaming-optional) are asset types registered with AssetDatabase (ADR-0007). OGG stream files are AssetDatabase-referenced but never cached whole — only the path/handle is cached. Hot reload: AssetReloader rebuilds the PCM buffer or restarts the OGG stream at its current position where possible.

11. **Compat layer integration (ADR-0003).** `gx::PlaySoundMem(handle, playType, topPositionFlag)` wraps AudioManager::Play(). `gx::LoadSoundMem(path)` routes to AssetDatabase → Sound asset → returns int handle. DXLib return-code semantics preserved (0 success, -1 fail).

12. **Thread model.**
    - Public API callable from main thread only; most is thread-safe for read.
    - XAudio2 mixing runs on its own OS thread (XAudio2-owned; GXLib does not manage it)
    - OGG decode jobs submit to JobSystem Low priority
    - DSP parameter writes are atomic double-buffered; never lock the audio callback
    - No `new` / `delete` / `malloc` on the audio callback path

### Architecture Diagram

```
Game / Compat (gx::PlaySoundMem, emitter.Play(), etc.)
   │
   ▼
gx::AudioManager   (singleton facade)
   │
   ├──► gx::AudioMixer
   │      └── Master
   │           ├── Music      ◄── MusicPlayer (OggStream × N)
   │           ├── SFX        ◄── AudioEmitter (3D / 2D voices × ≤64)
   │           ├── Voice      ◄── (future: VoIP / dialogue)
   │           ├── Ambient    ◄── AudioEmitter (ambient beds)
   │           └── Reverb     ◄── send target (dry bypasses)
   │      Each bus: volume, mute, solo, ≤4 DSP inserts
   │
   ├──► AudioEmitter[]   (3D spatial: distance curve, cone, Doppler; listener from AudioListener)
   │
   ├──► AudioOcclusion   (periodic raycast emitter→listener via PhysicsWorld; rate-limited 10 Hz/emitter)
   │
   ├──► ReverbZone[]     (per-volume blend of reverb send)
   │
   └──► AudioSnapshot    (named mixer-state captures; interpolated blend)
                │
                ▼
          IAudioDevice  (IXAudio2 + IXAudio2MasteringVoice)
                │  (OS audio thread — XAudio2-owned — mixes buses)
                ▼
             device output
```

### Key Interfaces

- `gx::AudioManager::Instance()` — singleton
- `void Initialize(const AudioDesc&)`, `void Shutdown()`
- `SoundHandle Play(AssetId soundId, const PlayDesc&) → VoiceHandle`
- `AudioEmitter CreateEmitter(Vec3 pos, AssetId sound, ...)` for 3D
- `MusicPlayer::Play(AssetId oggId, bool loop, float crossfade_s)`
- `AudioBus* GetBus(string_view name)` — volume/mute/solo/DSP-insert
- `ReverbZone::Attach(Vec3 center, radius, reverb_params)`
- `AudioSnapshot::Capture("InGame")`, `BlendToSnapshot("Paused", 0.5f)`
- `AudioOcclusion::SetEnabled(bool)`, `SetRange(float meters)`

## Alternatives Considered

### Alternative 1: WASAPI direct (skip XAudio2)
- **Description**: Talk directly to WASAPI, implementing our own mixer + resampler + XMA/PCM mix
- **Pros**: Lowest possible latency; full control; no XAudio2 dependency
- **Cons**: Huge implementation cost for mixer, resampler, XAPO-equivalent effects, multi-rate handling, low-level buffer management; XAudio2 is already essentially a minimal Windows-audio mixer layer; reinventing it trades maintenance for ~0 ms latency win
- **Rejection Reason**: XAudio2 *is* the Windows game-audio layer; reinventing it is pure cost

### Alternative 2: FMOD Studio or Wwise integration
- **Description**: Ship as a thin wrapper over FMOD Studio or Audiokinetic Wwise
- **Pros**: Industry-standard tools; sound designers get authoring tools they already know; rich mid-game parameter system; network and voice modules
- **Cons**: Per-title licensing / royalty; heavy SDK footprint (~40-100 MB); adds a large dependency a DXLib-heritage SDK developer may not want; tooling pipeline is complex; ties the audio identity of GXLib to a third party; licensing for source distribution / open source at all ambiguous
- **Rejection Reason**: Licensing and footprint conflict with GXLib's "minimal friction" positioning. Users who want FMOD can integrate it themselves on top of GXLib; we don't force it.

### Alternative 3: Keep audio simple (no bus mixer, no DSP, no snapshots)
- **Description**: One-shot play API, 3D spatial, nothing else
- **Pros**: Simpler surface
- **Cons**: Every real game needs at minimum Master/Music/SFX bus control; without DSP the engine can't do underwater filter, menu reverb, combat duck — features users expect. Users would build them anyway, worse and fragmented.
- **Rejection Reason**: Minimalism here fails the "out-of-the-box" promise. GXLib's positioning requires these features baked in.

## Consequences

### Positive
- XAudio2 is the right-abstraction-level — minimal dependency, no licensing, lifelong Windows support
- Bus-based mixer gives authors the industry-standard mental model (Master / Music / SFX / Voice)
- DSP chain unlocks common effects (reverb, filter, delay, compressor) without per-game re-implementation
- Snapshots make mixer-state transitions (menu→game, underwater, paused) a one-line authoring call
- Occlusion via existing PhysicsWorld raycast reuses rather than rebuilds a spatial structure
- AssetDatabase-backed sounds get hot reload for free

### Negative
- Windows-only (same scope as DX12); porting to another OS needs a new backend under `IAudioDevice`
- XAudio2 is no longer a growth area at Microsoft — future-proofing depends on Windows backwards compat (generally excellent)
- Bus count + DSP count increases mix-thread CPU; need to keep effects SSE-accelerated where possible
- Occlusion raycasts compete with gameplay raycasts for physics-query budget
- HRTF / head-tracked spatial is out of scope for this ADR — future ADR can extend

### Risks
- **Audio callback underrun** if DSP or mix gets too expensive. *Mitigation*: effects SSE-accelerated; voice count hard-capped at 64; double-buffered param updates never block.
- **Parameter race** between main-thread writes and audio-thread reads. *Mitigation*: atomic double-buffered params; no locks on the audio path.
- **OGG streaming stall** if decode Job is starved. *Mitigation*: 4 × 500 ms buffers (2 s lookahead); decode Job at Low priority but replaces itself at high headroom before buffer underrun.
- **Occlusion raycast cost** dominates physics. *Mitigation*: per-emitter rate limit (10 Hz), staggered across emitters; can be globally disabled.
- **Snapshot blend fights with per-bus script changes.** *Mitigation*: snapshot blend multiplies, doesn't overwrite — final volume = snapshot-target × runtime-modifier. Scripts see predictable behaviour.
- **44.1 kHz vs 48 kHz device** correctness. *Mitigation*: XAudio2 resampler handles source-to-mastering-voice rate conversion; all internal buses operate at mastering-voice rate.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | TR-chr-002 ("Audio / Music / DSP effects") — elevated from Gap to Covered |

## Performance Implications
- **CPU**: Target ≤ 1.5 ms/frame total audio budget at 60 fps on mid-range CPU: mix + DSP ≤ 1.0 ms (XAudio2 thread), occlusion raycast batch ≤ 0.2 ms (shared with physics), main-thread audio API ≤ 0.3 ms
- **Memory**: ~2 MB XAudio2 baseline; per loaded Sound ≈ size of PCM; OGG streaming ≈ 4 × 500 ms × channels × 2 bytes
- **Load Time**: Sound loads go through AssetDatabase; PCM decode cost per sound
- **Network**: N/A for this ADR (VoIP is a future separate ADR)

## Migration Plan

Not applicable — this ADR is retroactive across Phases 0 / 1 / 3 / 5. Going forward:

1. New DSP effects register with `AudioDSP::RegisterEffect<T>()`; effect count per bus remains capped at 4 until a superseding ADR expands it
2. Spatial extensions (HRTF, head-tracked, 7.1.4 channel config) are future ADRs
3. Cross-platform backend (macOS CoreAudio, Linux PipeWire) requires a new `IAudioDevice` implementation; XAudio2 remains the Windows default under this ADR
4. VoIP / networked voice is a separate future ADR that adds a Voice-bus consumer

## Validation Criteria
- 64-voice stress: 64 simultaneous 3D emitters with occlusion enabled — audio thread CPU ≤ 1 ms, no underruns over 5-minute run
- OGG gapless loop: 3-minute track loops 10 times — no clicks or silence gaps at wraparound
- Snapshot blend: record audio while blending Menu→InGame over 0.5 s — no discontinuity, no volume jump
- Occlusion rate limit: 256 emitters in range, per-emitter raycast frequency stays ≤ 10 Hz measured over 60 s
- Hot reload: modify a Sound asset on disk; live playback transitions to new bytes at next play; in-flight voices finish on old bytes (not truncated)
- Device rate: run on 44.1 kHz and 48 kHz devices — output indistinguishable to A/B listening (resampler correctness)
- Compat: `gx::PlaySoundMem(handle, DX_PLAYTYPE_BACK, FALSE)` produces equivalent audible output to a DXLib-equivalent call on the same WAV

## Related Decisions
- ADR-0001 (Documentation strategy)
- ADR-0003 (DXLib Compat — gx::PlaySoundMem / LoadSoundMem route here with DXLib semantics)
- ADR-0006 (Job System — OGG decode jobs + occlusion raycast batches submit here)
- ADR-0007 (Asset Database — Sound / SoundBank / OGG path asset types)
- ADR-0009 (Physics — AudioOcclusion raycasts go through PhysicsWorld)
- `GXLib/Audio/{AudioManager,AudioMixer,AudioBus,AudioEmitter,AudioListener,AudioOcclusion,ReverbZone,AudioSnapshot,AudioDSP,AudioEffect,MusicPlayer,OggStream,Sound,SoundBank,AudioDevice}.{h,cpp}` (source of truth)
- CHANGELOG.md Phases 0, 1, 3, 5
