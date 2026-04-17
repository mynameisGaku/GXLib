# Epic: Input

> **Layer**: Core
> **ADR**: docs/architecture/adr-0011-input.md
> **Architecture Module**: GXLib/Input/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories input`

## Overview

This epic covers the unified Input subsystem providing keyboard, mouse, and gamepad (including vibration) support delivered in Phase 0 and extended through Phase 1. The system abstracts Win32 Raw Input and XInput behind a polling API compatible with DXLib conventions. No TR-level requirements were registered at project inception. Implementation is complete. Remaining work is documentation coverage and validating correct behavior when controllers are hot-plugged or disconnected mid-session.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0011: Input | Polling-based unified input; Win32 Raw Input for keyboard/mouse, XInput for gamepad; no action-map layer in core (left to consumer) | LOW |

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

Run `/create-stories input` to break this epic into implementable stories.
