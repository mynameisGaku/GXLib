# Epic: Audio

> **Layer**: Presentation
> **ADR**: docs/architecture/adr-0010-audio.md
> **Architecture Module**: GXLib/Audio/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories audio`

## Overview

This epic covers the XAudio2-backed audio subsystem: OGG playback, spatial (3D) audio, and DSP effects. Phase 5 added the Audio DSP pipeline. An IXAPO bridge (custom XAudio2 effect processor) was added this session to allow third-party DSP plugins. No TR-level requirements were registered at project inception. Implementation is complete. One item needs verification before this epic closes: the IXAPO bridge's `Process()` virtual dispatch path must be confirmed to route correctly at runtime — a unit test or GXModelViewer smoke test demonstrating a DSP effect applied via IXAPO is required as evidence.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0010: Audio | XAudio2 backend; OGG via stb_vorbis; HRTF spatial audio; DSP via IXAPO bridge for extensibility | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| — | No TR-level requirements registered; governed at ADR charter level | ⚠️ Partial |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories audio` to break this epic into implementable stories.
