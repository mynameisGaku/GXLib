# Epic: Networking

> **Layer**: Feature
> **ADR**: docs/architecture/adr-0013-networking.md
> **Architecture Module**: GXLib/IO/Network/
> **Status**: Ready
> **Stories**: Not yet created — run `/create-stories networking`

## Overview

This epic covers GXLib's networking foundation: reliable UDP transport, connection management, and packet serialization introduced in Phase 3 and extended in Phase 4. Compat layer wrappers for networking were added this session. No TR-level requirements were registered at project inception. Two features are explicitly deferred to future scope per ADR-0013: encrypted UDP transport and dedicated server hosting. Remaining near-term work is integration testing of the newly added Compat wrappers and verifying that packet round-trip tests pass under simulated packet loss.

## Governing ADRs

| ADR | Decision Summary | Engine Risk |
|-----|-----------------|-------------|
| ADR-0013: Networking | Reliable UDP (custom ACK layer over raw UDP sockets); no TCP; encrypted transport and dedicated server deferred to v2 | LOW |

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

Run `/create-stories networking` to break this epic into implementable stories.
