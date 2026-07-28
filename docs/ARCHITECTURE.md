# Architecture

plasma-irlume 2.2.0 is a read-only Plasma KCM:

```text
QML
  -> typed presentation models
  -> RefreshCoordinator
  -> FaceAuthBackend
  -> fixed production factory
  -> IrlumeBackend
  -> /usr/bin/irlume Contract 1
```

`RefreshCoordinator` assigns monotonic generations and accepts only current
signals. A newer request cancels the active process and replaces any pending
request, so one backend instance never owns more than one irlume process.

`IrlumeBackend` is a signal-driven `QProcess` state machine. It handshakes
first, then runs advertised read commands in fixed order. It uses no shell,
wait API, camera, or mutation command. Every command has a three-second
timeout and independent 256 KiB stdout and stderr limits.

Local OS, display-manager, EFI, and TPM facts are read with strict bounds on a
worker thread. `SystemProbe::evaluate()` remains a pure deterministic mapping.
Presentation models consume only their corresponding operation result.

On teardown, receivers are disconnected and generations become stale. A live
process is killed and reparented to the application for asynchronous reaping.
The GUI thread never waits for process shutdown.

There is no privileged helper, system D-Bus service, Polkit action, PAM
integration, daemon, camera implementation, or biometric processing in this
repository.
