# Epic: Movie Pipeline

> **Layer**: Presentation
> **ADR**: docs/architecture/adr-0020-movie.md
> **Architecture Module**: GXLib/Movie/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories movie`

## Overview

This epic covers video playback (rendering pre-encoded video frames into the FrameGraph) and video recording (capturing swap chain frames to an encoded video file). Both were introduced in Phase 4. Both TRs are implemented at the feature level. The critical gap is that this subsystem has NO test coverage whatsoever — it is the only engine subsystem without at least a smoke test. This is a quality risk: playback and recording regressions will be invisible until they manifest in a consumer project. Adding baseline tests is the primary story for this epic.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0020: Movie Pipeline | Video playback via CPU-decoded frames submitted to GPU texture; recording via swap chain capture; codec handled externally (Media Foundation) | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-mov-001 | Video playback: decode and present video frames in FrameGraph | ✅ Implemented |
| TR-mov-002 | Video recording: capture swap chain frames to encoded output file | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories movie` to break this epic into implementable stories.
