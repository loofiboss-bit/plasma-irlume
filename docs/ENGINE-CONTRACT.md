# Irlume Adapter Compatibility

## Decision

Read-only diagnostics support irlume `>= 0.6.0, < 0.7.0`.

The released 0.6.x CLI is human-oriented, but its documented diagnostic
commands are sufficient for Phase 2 when consumed through a narrow,
version-gated adapter. Lack of JSON is no longer a project blocker.

| Engine release | Read-only diagnostics | Profile workflows |
| --- | --- | --- |
| v0.6.0 | Supported by the 0.6.x parser | Unavailable: no public structured contract |
| v0.6.1 | Supported by the 0.6.x parser | Unavailable: no public structured contract |
| Reviewed contract v1 release | Requires explicit review | Enabled only with advertised `profiles-json` and `events-jsonl` |
| Other versions | Rejected until reviewed | Rejected until reviewed |

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

## Phase 3 machine surface

The profile adapter is implemented against contract version 1 and remains
disabled unless the version document advertises both required capabilities.
The proposed fixed commands are:

```text
irlume version --json
irlume profiles list --json
irlume enroll --events=jsonl
irlume auth test --events=jsonl
irlume profiles add-scan --profile-id <opaque-id> --events=jsonl
irlume profiles delete --profile-id <opaque-id> --json
```

These spellings are not claimed to exist in irlume 0.6.x. They freeze the
consumer-side safety boundary and must be aligned with a reviewed official
upstream release before its capability probe can pass.

Enrollment events must end in a complete profile, cancellation with engine
cleanup, or failure with engine cleanup. The KCM then performs a non-mutating
claimed-user test. A failed or unsafe verification triggers deletion of only
the newly returned opaque profile ID. No process output may contain camera
frames, images, embeddings, template material, credentials, passwords,
usernames, or filesystem/device paths.

## Evidence baseline

The 0.6.x parser is based on official v0.6.0 and v0.6.1 sources and command
documentation. Sanitized fixtures retain evidence for the private protocol
and the proposed future API; they are not production inputs.
