# Claude Code Game Studios -- Game Studio Agent Architecture

Indie game development managed through 48 coordinated Claude Code subagents.
Each agent owns a specific domain, enforcing separation of concerns and quality.

## Technology Stack

- **Engine**: Custom — GXLib (DirectX 12, Windows, DXLib-compatible API)
- **Language**: C++20
- **Version Control**: Git with trunk-based development
- **Build System**: CMake (CMakePresets.json)
- **Asset Pipeline**: GXLib Asset Database + custom resource pipeline

> **Note**: GXLib is itself the engine being developed. Route engine-level work
> to `engine-programmer`, consumer-facing API work to `gameplay-programmer`,
> rendering internals to `technical-artist`. Godot/Unity/Unreal specialists do
> not apply.

## Project Structure

@.claude/docs/directory-structure.md

## Engine Version Reference

@docs/engine-reference/gxlib/VERSION.md

## Technical Preferences

@.claude/docs/technical-preferences.md

## Coordination Rules

@.claude/docs/coordination-rules.md

## Collaboration Protocol

**User-driven collaboration, not autonomous execution.**
Every task follows: **Question -> Options -> Decision -> Draft -> Approval**

- Agents MUST ask "May I write this to [filepath]?" before using Write/Edit tools
- Agents MUST show drafts or summaries before requesting approval
- Multi-file changes require explicit approval for the full changeset
- No commits without user instruction

See `docs/COLLABORATIVE-DESIGN-PRINCIPLE.md` for full protocol and examples.

> **First session?** If the project has no engine configured and no game concept,
> run `/start` to begin the guided onboarding flow.

## Coding Standards

@.claude/docs/coding-standards.md

## Context Management

@.claude/docs/context-management.md

## SDK Production Stage (DoD recalibration)

@.claude/docs/sdk-production-dod.md
