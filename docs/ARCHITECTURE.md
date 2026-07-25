# Architecture

## Phase 5 status

The KCM provides live, read-only Fedora, Plasma, display-manager, Secure Boot,
TPM, and irlume diagnostics. `SystemProbe` collects local platform facts and
invokes a fixed irlume 0.6.x command set. `SystemState` exposes typed,
presentation-safe state to QML.

`IrlumeProcess` and `ProfileModel` provide the Phase 3 enrollment and profile
workflow behind a versioned machine-contract gate. The current irlume 0.6.1
release does not publish that contract, so production refuses mutation until a
reviewed upstream release advertises both `profiles-json` and `events-jsonl`.
Deterministic adapters and contract fixtures remain test-only.

`AuthConfiguration` provides Phase 4 UI state and eligibility policy.
`KAuthActionRunner` is the only unprivileged-to-privileged boundary. The
root-owned `AuthHelper` exposes six fixed actions: preview, lock-screen enable,
login-screen enable, disable, verify, and rollback.

`SupportReport` provides the Phase 5 recovery boundary. It consumes only typed
state from `SystemState`, `ProfileModel`, and `AuthConfiguration`. It maps
bounded error codes to localized recovery actions, creates a redacted Markdown
report, copies recovery text through the desktop clipboard, and writes exports
atomically to the user's Documents directory. It never consumes raw irlume
stdout, journal entries, profile names, camera paths, or biometric data.

The Diagnostics page can request an immediate disable through
`AuthConfiguration::disableNow()`. This bypasses the optional UI preview, not
the security boundary: the same fixed KAuth disable action regenerates and
validates an irlume plan, applies it, verifies password fallback, and reports
failure without accepting QML arguments.

Display-manager migration is detected read-only. If the active Plasma Login
Manager has stale SDDM wiring, or the active SDDM has stale Plasma Login Manager
wiring, the state becomes `display-manager-migration` and activation remains
blocked until the old integration is disabled. The KCM never rewrites the
migration automatically.

## Read-only diagnostic boundary

```text
QML pages
  -> IrlumeKcm
    -> SystemState
      <- SystemProbe
        -> local Fedora/systemd/EFI/sysfs facts
        -> irlume 0.6.x fixed read-only commands
```

The supported irlume command set is:

- `irlume --version`;
- `irlume status`;
- `irlume doctor`;
- `irlume login status`.

The process runner supplies fixed arguments, a deterministic C locale, a
three-second timeout, and bounded output. It never accepts QML-provided
programs, arguments, paths, environment variables, or usernames.

The 0.6.x parser recognizes only fields needed for Phase 2. Missing or
malformed fields become typed `Unknown` values rather than optimistic
defaults. Versions outside 0.6.x fail closed.

## Data handling

Raw irlume output may contain the current username, profile names, and device
paths. It is parsed in memory and never crosses the backend boundary.

Only typed status and redacted summary fields reach QML. The support report is
generated from those typed fields, not from raw command output. It excludes:

- camera frames, images, and embeddings;
- profile names and scan identifiers;
- passwords, credentials, and TPM secrets;
- usernames, home paths, device paths, and PAM paths.

## Profile mutation boundary

```text
EnrollmentPage
  -> ProfileModel
    -> IrlumeProcess
      -> fixed irlume machine-mode commands
        -> irlumed owns camera, templates, atomic mutation, and cleanup
```

The process adapter accepts a fixed operation enum: capability probe, profile
list, enrollment, claimed-user recognition test, appearance scan, and selected
profile deletion. QML can pass only an opaque profile ID that already exists in
`ProfileModel`; it cannot pass a username, executable, path, environment value,
or arbitrary command argument.

Every event must match contract version 1, the expected command and operation
ID, a monotonic sequence, and exactly one terminal state. Output is bounded and
rejected if it contains frame, image, embedding, template, credential,
password, username, user, or path fields. Stderr is never presented.

Fresh enrollment is immediately followed by a claimed-user recognition test.
The test contract must state that it released no credential and modified no
profile. If the test fails or violates the contract, `ProfileModel` deletes the
new profile before reporting failure. Standalone tests never mutate profiles.
Cancellation is considered successful only after a typed terminal cancellation
event; an unconfirmed process exit is reported as unknown rather than claiming
the camera was released.

Profile operations continue to run as the desktop user. Authentication
activation uses a separate boundary:

```text
AuthenticationPage
  -> AuthConfiguration
    -> KAuthActionRunner
      -> root AuthHelper
        -> fixed irlume plan/apply/verify/rollback commands
          -> irlume owns PAM mutation and transaction recovery
```

The helper validates the structured engine capability, Fedora 44, independently
detects the active display manager from systemd, and checks enrollment, password
fallback, and requested security tier. The engine-reported display manager must
match that local result. Lock-screen plans may target only `pam-service:kde`;
login-screen plans may target only the active login manager; disable may target
those two services. A successful apply is not reported until daemon, exact
PAM-target, desired-state, and password-fallback checks pass. Failed
verification invokes engine-owned rollback.

The UI requires a successful read-only preview for the selected scope before
enabling Apply. The helper does not trust or reuse that preview: it regenerates
and validates a fresh state-bound plan inside the privileged transaction.

The GUI and helper never edit PAM files directly, never accept executable or
shell strings, and cannot enable face authentication for sudo or Polkit. The
implementation retains these controls:

- fixed operation enums instead of arbitrary commands;
- no GUI or general backend running as root;
- irlume-owned PAM planning and mutation;
- password fallback validation;
- post-apply verification and automatic rollback.

A structured upstream JSON/NDJSON API remains the preferred long-term
transport. The current CLI adapter is deliberately narrow and independently
tested so the transport can be replaced without changing the QML-facing
model.
