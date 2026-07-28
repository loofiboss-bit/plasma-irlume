# Irlume Machine API Contract 1

## Compatibility decision

plasma-irlume requires `irlume >= 0.7.0` at runtime and negotiates the API
independently of the engine version string. `/usr/bin/irlume version --json`
must return a valid envelope whose contract range includes `1`. The engine
version is retained only as informational, redacted support state.

After the handshake, the adapter may invoke only:

| Capability | Fixed command |
| --- | --- |
| `status-json` | `irlume status --json --contract 1` |
| `doctor-json` | `irlume doctor --json --contract 1` |
| `profiles-list-json` | `irlume profiles list --json --contract 1` |
| `login-status-json` | `irlume login status --json --contract 1` |

An absent capability leaves that section unavailable. Unknown capabilities
and JSON properties are ignored. They never enable mutations.

## Validation and failure behavior

The adapter validates the echoed command, Contract 1, success/error envelope,
required types, documented enums, count consistency, and selected public
limits. A structured nonzero-exit error preserves its stable code. Empty or
malformed JSON, oversized output, timeout, process-start failure, wrong
contract, wrong command, contradictory fields, and sensitive unexpected
fields fail closed.

Commands use a fixed executable and argument lists, no shell, a deterministic
environment, a three-second timeout, and 256 KiB limits for each output
channel. Raw output never crosses into QML or support reports.

Unknown is kept distinct from zero and failure. In particular, absent
daemon-derived enrollment counts remain unknown.

## Unsupported operations

Contract 1 is read-only. Production code does not invoke enrollment streams,
authentication tests, scan/profile mutations, camera selection or tuning, or
login plan/apply/verify/rollback transactions. Presentation models return the
non-retryable `capability-unavailable` error without starting a subprocess.
The fixed KAuth action surface remains installed for a future backend, but its
helper currently returns the same error without running an engine command.

Fixtures under `tests/fixtures/irlume/proposed-v1/` and `events/` are historical
design artifacts only. No production target loads them.
