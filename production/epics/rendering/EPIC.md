# Epic: Rendering Pipeline

> **Layer**: Presentation
> **ADR**: docs/architecture/adr-0008-rendering.md
> **Architecture Module**: GXLib/Graphics/{FrameGraph,Pipeline,PostEffect,3D}
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories rendering`

## Overview

This epic covers the high-level rendering pipeline built on the DX12 Backend: the FrameGraph (render pass DAG), the PBR deferred shading pipeline, and the PostEffect stack. Phase 4 added HDR output, VRS, Mesh Shaders, and GPU Particles. A custom shader model API and PostFX insertion points were added recently this session. All TRs are implemented. Remaining work is verification: the custom shader model API needs an end-to-end test in GXModelViewer, and PostFX insertion order needs documentation so consumers know the guaranteed evaluation sequence.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0008: Rendering Pipeline | FrameGraph-based deferred PBR; PostFX stack with insertion API; HDR output; VRS and Mesh Shaders as optional tiers | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-rnd-003 | Descriptor heap management shared between DX12 Backend and Rendering Pipeline | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories rendering` to break this epic into implementable stories.
