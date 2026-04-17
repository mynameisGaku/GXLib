# Epic: GUI

> **Layer**: Presentation
> **ADR**: docs/architecture/adr-0012-gui.md
> **Architecture Module**: GXLib/GUI/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories gui`

## Overview

This epic covers the widget-based GUI system and its ImGui integration used by the editor and in-game HUD. Phase 5 added IME (IMM32) support for CJK text input. Implementation is complete. One known gap: `UIContext` is not yet exposed through the DXLib Compat layer, meaning projects using the procedural Compat API cannot create or manage GUI contexts without dropping down to the class-based API. The Compat stub belongs to the Compat API epic but the UIContext design decision is governed here. No TR-level requirements were registered at project inception.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0012: GUI | Retained-mode widget tree; ImGui for editor/debug overlays; IME via IMM32; UIContext manages widget lifetime | LOW |

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

Run `/create-stories gui` to break this epic into implementable stories.
