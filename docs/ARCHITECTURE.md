# Architecture

## Phase 2 status

The KCM provides live, read-only Fedora, Plasma, display-manager, Secure Boot,
TPM, and irlume diagnostics. `SystemProbe` collects local platform facts and
invokes a fixed irlume 0.6.x command set. `SystemState` exposes typed,
presentation-safe state to QML.

Deterministic `FakeSystemStateAdapter` scenarios remain test-only. The
production KCM does not offer a fake-state selector.

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

## Mutation boundary

Phase 2 exposes no mutation API and has no privileged helper. The GUI cannot
enroll a face, open a camera, edit PAM, start services, or change
authentication.

Future enrollment and authentication phases must retain these controls:

- fixed operation enums instead of arbitrary commands;
- no GUI or general backend running as root;
- irlume-owned PAM planning and mutation;
- password fallback validation;
- post-apply verification and automatic rollback.

A structured upstream JSON/NDJSON API remains the preferred long-term
transport. The current CLI adapter is deliberately narrow and independently
tested so the transport can be replaced without changing the QML-facing
model.
