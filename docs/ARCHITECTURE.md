# Architecture

plasma-irlume 3.0.0 has two independent, unprivileged data paths:

```text
Read-only engine diagnostics             Native Camera Check
QML                                      QML
 -> presentation models                   -> CameraPreviewSession
 -> RefreshCoordinator                    -> private stdin/stdout pipes
 -> FaceAuthBackend                       -> preview worker
 -> /usr/bin/irlume Contract 1             -> Qt Multimedia + libudev
```

## Engine diagnostics

`RefreshCoordinator` assigns monotonic generations and accepts only current
signals. `IrlumeBackend` uses a signal-driven `QProcess`, fixed Contract 1 read
commands, three-second timeouts, and independent 256 KiB stdout/stderr limits.
It uses no camera or mutation command. A native preview failure cannot clear
or replace a valid irlume diagnostic result.

## Native camera process

`CameraPreviewSession` is the only KCM-side preview model. Its states are
`Idle`, `Discovering`, `Ready`, `Starting`, `Streaming`, `Stopping`, and
`Failed`. It exposes sanitized devices, current selection and spectrum, frame
availability, remaining time, dropped frames, status, and a stable error code.

The worker owns `QMediaDevices`, `QCamera`, `QMediaCaptureSession`, and
`QVideoSink`; the KCM never opens a video node. libudev classifies only
`ID_INFRARED_CAMERA=1` as IR and a reviewed capture capability as RGB.
Everything else is Unknown.

The parent and worker exchange length-framed CBOR v1 over private process
pipes. Both validate the session, monotonic sequence, record size, command
shape, device count, image dimensions, JPEG size, and spectrum. Commands have
no path or free-form argument. The worker retains at most one pending frame;
new frames replace older pending frames under backpressure.

The parent stops preview when the page is hidden or the application becomes
inactive. The worker independently enforces 60 seconds. A missing stop
acknowledgement causes a hard worker termination after one second. Crash,
protocol, startup, and stall failures clear the in-memory frame.

## Privilege and persistence boundary

The worker is installed as an ordinary executable in `/usr/libexec` without
setuid/setgid bits or file capabilities. It has no irlume adapter, privileged
helper, KAuth, Polkit, system service, network feature, audio capture, or
authentication API. Neither process persists frames or opaque device tokens.
Support and configuration paths do not receive preview data.
