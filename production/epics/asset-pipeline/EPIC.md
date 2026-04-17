# Epic: Asset Database + Hot Reload

> **Layer**: Core
> **ADR**: docs/architecture/adr-0007-asset-pipeline.md
> **Architecture Module**: GXLib/Core/AssetDatabase.*, GXLib/IO/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories asset-pipeline`

## Overview

This epic covers the Asset Database (a JSON/binary resource registry with handles and reference counting) and the Hot Reload system (FileWatcher + DirectStorage integration for live asset refresh without engine restart). Both subsystems were delivered across Phases 2 and 5 and are fully operational. No TR-level requirements were registered at project inception. Remaining work is closing documentation gaps and adding stress tests for concurrent asset loads during Hot Reload cycles.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0007: Asset Database + Hot Reload | Central asset registry with handle-based access; hot reload driven by FileWatcher events and DirectStorage async loads | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| — | No TR-level requirements registered; governed at ADR charter level | ✅ Implemented |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories asset-pipeline` to break this epic into implementable stories.
