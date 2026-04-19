# ADR-0020: Movie Pipeline (Video Playback via Media Foundation + Video Recording)

## Status
Accepted

## Date
2026-04-17

## Engine Compatibility

| Field | Value |
|-------|-------|
| **Engine** | Custom — GXLib (self-hosted) |
| **Domain** | IO / Media |
| **Knowledge Risk** | LOW — Windows Media Foundation (IMFSourceReader, IMFSample) and frame-capture-to-MP4 encoding are well-documented Windows SDK patterns |
| **References Consulted** | `GXLib/Movie/MoviePlayer.{h,cpp}` (playback, 426 lines), `GXLib/Graphics/VideoRecorder.{h,cpp}` (recording, Phase 4), CHANGELOG Phase 4 |
| **Post-Cutoff APIs Used** | None |
| **Verification Required** | MP4/WMV/AVI playback on Windows 10 21H2 + Windows 11 with various codecs installed; seek accuracy; frame-timing correctness at non-standard frame rates; texture upload does not stall the GPU pipeline |

## ADR Dependencies

| Field | Value |
|-------|-------|
| **Depends On** | ADR-0001 (documentation strategy), ADR-0002 (DX12 backend — GraphicsDevice + TextureManager for decoded frame upload), ADR-0007 (Asset Database — declared dependency for the documented AssetDatabase bypass, see §7 Known Exception) |
| **Enables** | Future ADRs on cutscene systems, in-game video replay, streaming video textures |
| **Blocks** | None (code already exists; retroactive) |

## Context

### Problem Statement

GXLib has two media capabilities: `MoviePlayer` (video playback via Media Foundation decoding to GPU texture, ~426 lines) and `VideoRecorder` (frame capture + MP4 export, Phase 4). Neither has an ADR. This ADR codifies both as a single Movie Pipeline, documents the Media Foundation backend commitment, and calls out the known performance and feature limitations.

### Constraints

- Windows-only: Media Foundation is a Windows SDK API. No cross-platform path.
- `MoviePlayer` outputs RGB32 frames only. Decoded frames are uploaded to `TextureManager` as a new texture handle each frame (simple but not optimal).
- No audio track decoding — video-only playback. Audio must be handled separately via ADR-0010 AudioManager.
- `MFStartup`/`MFShutdown` are called per `Open`/`Close`, not globally. Multiple simultaneous `MoviePlayer` instances each init/shutdown MF independently.
- `VideoRecorder` captures rendered frames from the swap chain — it does not record gameplay replay data.

## Decision

**GXLib uses Windows Media Foundation as the sole video codec backend. `MoviePlayer` decodes video frames via `IMFSourceReader` and uploads them as RGB32 textures. `VideoRecorder` captures rendered frames and encodes to MP4 via Media Foundation Sink Writer. Both are simple, single-purpose classes with no internal threading — the caller drives them from the game loop. Performance is traded for implementation simplicity (acknowledged in code comments). Audio sync is the caller's responsibility via separate AudioManager playback.**

Concrete rules:

1. **MoviePlayer (playback).**
   - `Open(filePath, device, texManager)` initialises Media Foundation, opens the file, creates an `IMFSourceReader`.
   - `Update(device)` decodes one frame when the frame-interval timer elapses. Returns `true` if a new frame was decoded. BGRA→RGBA conversion + vertical flip in software. Uploads to TextureManager.
   - `GetTextureHandle()` returns the current frame's texture handle for rendering via `DrawGraph`.
   - `Play/Pause/Stop/Seek(seconds)` control playback state.
   - State machine: `Stopped → Playing → Paused → Playing → Stopped`.
   - Supported formats: MP4, WMV, AVI (whatever codecs Windows Media Foundation supports on the target machine).

2. **VideoRecorder (recording).**
   - **Capture point**: called by the host after the final render pass and
     **before** the Present barrier. `CaptureFrame` resource-barriers the
     back buffer assuming `D3D12_RESOURCE_STATE_RENDER_TARGET`; callers who
     have already transitioned to `PRESENT` state must move the capture
     earlier in the frame.
   - **Thread**: `CaptureFrame` + `IMFSinkWriter::WriteSample` run on the
     caller's thread (typically the main render thread). Media Foundation
     Sink Writer internally dispatches encoding onto its own worker pool;
     the caller does not block on encode completion.
   - Captures rendered frames from the swap chain at a specified frame rate.
   - Encodes to MP4 via Media Foundation Sink Writer.
   - Start/Stop API driven by the caller.

3. **Performance trade-off (documented, accepted).**
   - `MoviePlayer` recreates a GPU texture resource every decoded frame. This is O(1) per frame but involves a `CreateCommittedResource` + `Unmap` cycle that could be avoided with a ring-buffer of pre-allocated upload textures.
   - Accepted because: video playback is typically for cutscenes (not 60 fps gameplay), implementation is simple and reliable, and the overhead is bounded (one texture creation per video frame, typically 24-30 Hz).

4. **No audio track.**
   - `MoviePlayer` decodes video streams only. The audio track is ignored.
   - Callers who need audio-synced video must start a separate audio playback (via `PlayMusic` or `AudioManager`) timed to `MoviePlayer::GetPosition()`.

5. **Module placement.**
   - `MoviePlayer` is in `GXLib/Movie/` (IO domain).
   - `VideoRecorder` is in `GXLib/Graphics/` (requires swap chain access).
   - Both link `mfplat.lib`, `mfreadwrite.lib`, `mf.lib`, `mfuuid.lib`.

6. **Forbidden patterns.**
   - `movie_audio_assumption` — do not assume `MoviePlayer` plays audio; it doesn't. Use AudioManager separately.
   - `mf_global_init` — do not call `MFStartup` / `MFShutdown` directly from any subsystem. All MF-consuming code (currently `MoviePlayer` and `VideoRecorder`, plus any future MF-using subsystem) MUST go through `gx::MFPlatform::Acquire()` / `Release()` (Core layer). The wrapper owns the process-global MF refcount; bypassing it causes premature teardown when multiple subsystems coexist.
   - `movie_in_rollback_window` — `MoviePlayer::Update` uses wall-clock frame-interval timing; do NOT drive a `MoviePlayer` inside an ADR-0013 rollback re-simulation window. MoviePlayer is intended for cutscenes, which typically pause gameplay and therefore do not intersect rollback.

### Resolution log

- 2026-04-18 (story E4 / 007): `mf_global_init` contract implemented. `MFPlatform` added at `GXLib/Core/MFPlatform.{h,cpp}`; `MoviePlayer` and `VideoRecorder` migrated.

## Testing Scope

Added 2026-04-19. Current automated test coverage for `MoviePlayer` /
`VideoRecorder`:

**Covered** (`Tests/unit/movie/movie_player_test.cpp` — 8 tests):
- Default state + getter correctness (size 0, duration 0, no texture)
- State-machine transitions without `Open()` (Play/Pause/Stop/Seek/Close
  are no-ops in Stopped state)
- Double-Close safety
- `MovieState` enum uniqueness

**Not covered** (requires an MP4 fixture): `Open()` + `Update()` decode
flow, `Seek()` keyframe proximity, `GetPosition()` after decode, frame
texture handle validity, `VideoRecorder::CaptureFrame` swap-chain
interaction.

Full Open/Decode/Seek integration tests are **deferred** — tracked as
`TR-defer-movie-integration-tests` in `architecture-traceability.md`.
Three fixture-generation strategies were evaluated (commit a 100 KB
binary, CMake-detected ffmpeg generation, Media-Foundation in-proc
generation) and all have trade-offs (binary churn, CI dep fragility,
test-infra complexity) that outweigh the value for an SDK project.
Consumer games that ship cutscenes are the natural source of realistic
fixtures. Revisit when such a consumer exists.

For now, `MoviePlayer` + `VideoRecorder` rely on:
- The 8 unit tests above for state-machine correctness.
- `GXModelViewer` reference-app smoke testing for end-to-end decode
  (manual, user-at-PC).
- The `MFPlatform` refcount unit tests (`Tests/unit/core/mf_platform_test.cpp`)
  for cohabitation correctness (regression guard for E4).

## Known Exception: AssetDatabase Bypass

MoviePlayer (`Open(filePath, …)`) intentionally bypasses ADR-0007 AssetDatabase.
The reason is technical, not architectural: Media Foundation's `IMFSourceReader`
requires a seekable OS file path or an `IMFByteStream`, neither of which the
current AssetDatabase provider chain produces for streamed video.

**Consequences of the bypass** (all acknowledged):
- `MoviePlayer` cannot participate in hot reload (ADR-0007 §3)
- `MoviePlayer` cannot be mod-remapped via `AssetRemapper` (ADR-0007 §5)
- Video files cannot be packed into `.gxa`/`.pak` archives — they must ship as
  loose files on disk
- Video paths are the only place in the engine where `fopen`-equivalent
  filesystem calls may appear outside an AssetDatabase provider. This is a
  documented exception to the Control-Manifest Foundation rule "no direct
  filesystem in subsystems."

**Future extension** (deferred, not committed): implement an `IMFByteStream`
adapter over `IFileProvider` so `MoviePlayer` can accept an `AssetId`. At that
point this exception is removed and the `mf_global_init` forbidden-pattern
rewrite is completed as one change. Until then, callers accept the above
limitations.

## Alternatives Considered

### Alternative 1: FFmpeg / libav for decoding
- **Pros**: Cross-platform; wider codec support; proven.
- **Cons**: Large dependency (~50 MB); LGPL licensing complexity; requires custom build for Windows. Media Foundation is already part of the Windows SDK (zero additional bytes).
- **Rejection Reason**: Windows-only project (ADR-0002). Media Foundation is built-in and sufficient for MP4/WMV/AVI. Cross-platform would require FFmpeg, but that's a future concern.

### Alternative 2: DirectShow for decoding
- **Pros**: Familiar; widely used in older Windows media apps.
- **Cons**: Deprecated by Microsoft in favour of Media Foundation. COM-heavy graph-builder model is complex. No modern codec support path.
- **Rejection Reason**: Media Foundation is the modern replacement and is the recommended path for new Windows development.

## Consequences

### Positive
- Zero external dependencies — Media Foundation is part of the Windows SDK.
- Simple API: Open → Play → Update (in game loop) → GetTextureHandle → DrawGraph.
- Video recording enables screenshot/replay features with minimal code.

### Negative
- No audio track decoding — forces dual-system sync for AV playback.
- Per-frame texture recreation is not optimal for high-resolution or high-frame-rate video.
- No test coverage for MoviePlayer (acknowledged gap).
- MF init/shutdown per instance is redundant if multiple MoviePlayers are used.

### Risks
- **Codec availability**: MP4/H.264 decoding depends on Windows-installed codecs. Fresh Windows installs include them, but enterprise-locked machines may not. *Mitigation*: log codec negotiation failures clearly.
- **Seek inaccuracy**: Media Foundation seek targets the nearest keyframe, not the exact timestamp. *Mitigation*: documented behaviour; callers should not depend on frame-precise seeking.

## GDD Requirements Addressed

| GDD System | Requirement | How This ADR Addresses It |
|------------|-------------|--------------------------|
| (None — ADR-only project per ADR-0001) | N/A | Retroactive codification of Movie subsystem. Registers TR-mov-001 and TR-mov-002. |

## Performance Implications

- **CPU**: Frame decode ≈ 0.5-2 ms per frame at 1080p (Media Foundation software path). BGRA→RGBA + flip ≈ 0.3 ms at 1080p.
- **GPU**: Texture upload ≈ 0.1 ms per frame at 1080p (staging → default copy).
- **Memory**: One decoded frame ≈ 8 MB at 1080p RGBA. IMFSourceReader internal buffers ≈ 10-30 MB depending on codec.

## Validation Criteria

- **Playback round-trip**: Open MP4 → Play → Update 60 frames → GetPosition > 0 → GetTextureHandle ≥ 0 → Close without leak.
- **Seek**: Seek to 50% of duration → GetPosition ≈ duration/2 (±1 keyframe interval).
- **State machine**: Stopped→Playing→Paused→Playing→Stopped transitions are clean.
- **Format support**: MP4 (H.264), WMV, AVI all open successfully on stock Windows 11.

## Related Decisions

- ADR-0001 (Documentation strategy)
- ADR-0002 (DX12 backend — GraphicsDevice + TextureManager)
- ADR-0010 (Audio — audio track playback must be handled separately)
- `GXLib/Movie/MoviePlayer.{h,cpp}`
- `GXLib/Graphics/VideoRecorder.{h,cpp}`
- CHANGELOG.md Phase 4
