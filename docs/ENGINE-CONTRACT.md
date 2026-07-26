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
| Reviewed contract v2 release | Requires explicit review | Enabled only with every capability required by the selected workflow |
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

## V2 profile and preview surface

The profile adapter accepts the V2 event envelope and remains disabled unless
the version document advertises `profiles-json`, `events-jsonl`,
`position-report`, and `preview-ir-jpeg`.
The proposed fixed commands are:

```text
irlume version --json
irlume profiles list --json
irlume enroll --events=jsonl --preview=ir-jpeg --preview-max-fps=8 --preview-max-size=640x480
irlume auth test --events=jsonl --preview=ir-jpeg --preview-max-fps=8 --preview-max-size=640x480
irlume profiles add-scan --profile-id <opaque-id> --events=jsonl --preview=ir-jpeg --preview-max-fps=8 --preview-max-size=640x480
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

Only the dedicated enrollment session may consume `preview` events. Each event
must contain a session ID, monotonic sequence, JPEG of at most 128 KiB and
640 by 480, `ir` or `rgb` spectrum, exactly 478 normalized landmarks, normalized
face box, and typed `PositionReport`. It may never contain embeddings, match
scores, template material, credentials, usernames, or paths.

At most one preview session may exist. The consumer displays at most 8 frames
per second, keeps only the newest frame, and clears image memory on terminal
result, cancellation, timeout, page hide, or KCM destruction. Frames and
landmarks are excluded from logs, clipboard operations, crash text, support
reports, and disk.

## Phase 4 login-transaction surface

Authentication mutation remains disabled unless `irlume version --json`
returns a reviewed contract version and advertises `login-transactions`. The helper then
uses only:

```text
irlume login enable --scope lock-screen --json
irlume login enable --scope login-screen --json
irlume login disable --json
irlume login enable --scope <fixed-scope> --apply --plan-id <opaque-id> --json
irlume login disable --apply --plan-id <opaque-id> --json
irlume login verify --transaction-id <opaque-id> --json
irlume login rollback --transaction-id <opaque-id> --apply --json
```

The helper accepts only the two literal enable scopes. Plan and transaction IDs
must match a bounded opaque-ID grammar and originate from validated engine
responses. QML cannot provide them.

Every enable plan must report a supported Plasma Login Manager or SDDM target
that matches the helper's independent systemd detection, a healthy engine, an
enrolled profile, and preserved password fallback. Lock-screen plans may contain
only `pam-service:kde`; login-screen plans may contain only the active display
manager target. Login-screen plans additionally require the Secure tier. A
disable plan may contain only those two targets and remains an engine-owned
clean-state transaction.

Apply must preserve plan lineage and return an opaque transaction ID. Verify
must confirm daemon reachability, exact planned PAM targets, desired versus
actual state, and password fallback. Verification failure always attempts
rollback; an engine-reported completed rollback is accepted without repeating
it.

## V2 camera-configuration surface

Camera management remains disabled unless `irlume version --json` advertises
`camera-config-json`. The consumer uses only:

```text
irlume cameras list --json
irlume cameras select --pair-id <opaque-id> --apply --json
irlume cameras emitter-test --json
irlume cameras emitter-setup --apply --json
irlume cameras tune --apply --json
```

List output may contain at most 16 secure pairs and only bounded display
labels, opaque pair IDs, built-in and active booleans, and active-state
availability. Selection runs through KAuth and must be confirmed by an
independent list readback with exactly one matching active pair. Emitter setup
must be confirmed by a separate non-mutating emitter test with a control count
from 1 through 256. Capture tuning has a fixed engine-owned measurement count
and returns only `concurrent` or `sequential`, bounded retained-signal ratios,
bounded saved milliseconds, and a conclusive flag.

No operation accepts or returns a camera node, filesystem path, USB identity,
serial number, UVC selector, arbitrary round count, executable, environment
value, or shell argument.

## Evidence baseline

The 0.6.x parser is based on official v0.6.0 and v0.6.1 sources and command
documentation. Sanitized fixtures retain evidence for the private protocol
and the proposed future API; they are not production inputs.
