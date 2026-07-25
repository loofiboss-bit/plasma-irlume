# Irlume Adapter Compatibility

## Decision

Read-only diagnostics support irlume `>= 0.6.0, < 0.7.0`.

The released 0.6.x CLI is human-oriented, but its documented diagnostic
commands are sufficient for Phase 2 when consumed through a narrow,
version-gated adapter. Lack of JSON is no longer a project blocker.

| Engine release | Read-only diagnostics | Mutating workflows |
| --- | --- | --- |
| v0.6.0 | Supported by the 0.6.x parser | Not implemented |
| v0.6.1 | Supported by the 0.6.x parser | Not implemented |
| Other versions | Rejected until reviewed | Not implemented |

## Accepted surface

The adapter may invoke only:

| Command | Accepted fields |
| --- | --- |
| `irlume --version` | semantic engine version |
| `irlume status` | daemon reachability, enrollment presence, camera pair |
| `irlume doctor` | TPM, Secure Boot, required runtime readiness |
| `irlume login status` | read-only PAM wiring summary |

Commands run without a shell, use fixed arguments and C locale, time out after
three seconds, and cap captured output at 256 KiB.

Parsing is deliberately conservative:

- an absent or malformed field becomes `Unknown`;
- an unsupported version prevents all further interpretation;
- no exit code alone is treated as proof of readiness;
- raw command output is never displayed or copied into a support report;
- camera paths, usernames, profile names, and PAM paths are never retained.

## Tier mapping

- **Secure**: supported engine, running daemon, IR camera, and required
  liveness capability;
- **Convenience**: supported engine, running daemon, and RGB-only camera;
- **Unsupported**: missing/unsupported engine, unhealthy daemon, or no usable
  camera.

RGB-only state can never authorize login-screen activation.

TPM hardware presence and template-at-rest protection are separate facts.
Phase 2 reports TPM availability but does not claim that a template is sealed
unless irlume reports that state.

## Upgrade policy

Patch releases inside 0.6.x are accepted by the narrow parser. Unknown fields
are ignored and missing known fields become `Unknown`. A 0.7.0 or newer engine
requires an explicit adapter review and test update.

The proposed structured contract in `UPSTREAM-API-REQUEST.md` remains the
preferred migration target. Once available, it should replace prose parsing
behind `SystemProbe` without changing `SystemState` or QML.

## Evidence baseline

The 0.6.x parser is based on official v0.6.0 and v0.6.1 sources and command
documentation. Sanitized fixtures retain evidence for the private protocol
and the proposed future API; they are not production inputs.
