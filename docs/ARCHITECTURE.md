# Architecture

The implemented dependency direction is:

```text
QML
  -> IrlumeKcm and typed presentation models
    -> FaceAuthBackend
      -> IrlumeBackend
        -> fixed irlume Contract 1 read-only commands
```

`FaceAuthBackend` owns backend-neutral result types: capabilities, availability,
status, doctor checks, profile summaries, login wiring, and stable errors.
`IrlumeBackend` centralizes process execution and JSON envelope validation.
`IrlumeKcm::refresh()` obtains one immutable `EngineSnapshot` and distributes it
to `SystemProbe`, `ProfileModel`, `CameraConfiguration`, and
`AuthConfiguration`. QML never sees raw JSON, subprocesses, contract command
names, paths, or usernames.

`SystemProbe` combines the typed backend snapshot with local Fedora, display
manager, Secure Boot, and TPM observations. Missing backend data stays
`Unknown`. Password fallback is never claimed as preserved because Contract 1
does not verify it.

Profile, camera, enrollment-preview, and authentication configuration objects
are retained as stable presentation APIs. Their mutation entry points are
disabled and fail locally. `AuthHelper` preserves only its fixed KAuth action
surface and does not execute an irlume mutation.

`SupportReport` consumes typed state only. It excludes raw engine output,
camera data, biometric material, profile names, usernames, device/PAM paths,
credentials, passwords, and TPM secrets.

The GUI runs unprivileged, never edits PAM, and never supplies an executable,
shell string, arbitrary argument, or environment value. Package lifecycle
scripts perform no authentication changes.
