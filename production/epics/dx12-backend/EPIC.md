# Epic: DX12 Backend

> **Layer**: Foundation
> **ADR**: docs/architecture/adr-0002-dx12-backend.md
> **Architecture Module**: GXLib/Graphics/Device/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories dx12-backend`

## Overview

This epic covers the DirectX 12 device abstraction layer that underpins all GPU work in GXLib. The device, command queue, swap chain, descriptor heap management, resource upload, and frame synchronization infrastructure were established in Phase 0 and extended in Phase 4 with advanced features (VRS, Mesh Shaders, Sampler Feedback, DirectStorage). All foundational TRs are implemented. Remaining work is gap closure: verifying that advanced DX12 feature detection handles hardware that does not support optional tiers, and confirming that all device-creation paths have regression test coverage.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0002: DX12 Backend | Use DirectX 12 as the sole rendering backend; DX11 fallback is explicitly out of scope | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-rnd-001 | DX12 device and command queue initialization | ✅ Implemented |
| TR-rnd-002 | Swap chain creation and frame presentation | ✅ Implemented |
| TR-rnd-003 | Descriptor heap management (CBV/SRV/UAV, RTV, DSV, Sampler) | ✅ Implemented |
| TR-rnd-004 | GPU resource upload and readback paths | ✅ Implemented |
| TR-rnd-005 | Advanced feature support: VRS, Mesh Shaders, Sampler Feedback, DirectStorage | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories dx12-backend` to break this epic into implementable stories.
