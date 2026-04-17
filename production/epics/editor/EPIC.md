# Epic: Editor

> **Layer**: Feature
> **ADR**: docs/architecture/adr-0015-editor.md
> **Architecture Module**: GXLib/Editor/, GXLib/Core/{UndoSystem,NodeGraph,Reflect}
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories editor`

## Overview

This epic covers the Play-in-Editor runtime, the Undo/Redo system, the Node Graph editor, and the compile-time reflection macros that power editor introspection. All seven core TRs are implemented. Two CI/infrastructure gaps remain: the `GX_EDITOR=OFF` build configuration is not yet exercised in CI (so editor-only symbols could accidentally leak into shipping builds), and there is no automated check that reflection macros are not defined in .cpp files. These are not runtime bugs but represent compliance and build hygiene risks that must be closed before the engine leaves beta.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0015: Editor | Play-in-Editor via a parallel ECS World snapshot; Undo via Command pattern; reflection macros header-only; Editor symbols stripped when GX_EDITOR=OFF | LOW |

## Requirements

| TR-ID | Requirement | Status |
|-------|-------------|--------|
| TR-edit-001 | Play-in-Editor: enter/exit play mode with World snapshot restore | ✅ Implemented |
| TR-edit-002 | UndoSystem: Command pattern with undo/redo stacks | ✅ Implemented |
| TR-edit-003 | Node Graph editor widget (ImGui-backed) | ✅ Implemented |
| TR-edit-004 | Reflection macros: GX_REFLECT, GX_PROPERTY, GX_COMPONENT | ✅ Implemented |
| TR-edit-005 | Reflection data accessible at runtime for editor property panels | ✅ Implemented |
| TR-edit-006 | GX_EDITOR=OFF build strips all editor symbols (CI gate) | ⚠️ Partial |
| TR-edit-007 | CI check: reflection macros must not appear in .cpp files | ⚠️ Partial |

## Definition of Done

This epic is complete when:
- All stories implemented, reviewed, closed via `/story-done`
- All acceptance criteria from governing ADR verified
- Logic/Integration stories have passing tests in `tests/`
- Visual/Feel stories have evidence in `production/qa/evidence/`

## Next Step

Run `/create-stories editor` to break this epic into implementable stories.
