# Architecture

Milestone 1 separates four unprivileged areas:

```text
KDE System Settings
  -> KFaceAuthKcm
     -> RefreshCoordinator
        -> NativeFaceAuthBackend
     -> SystemProbe
     -> CameraPreviewSession
        -> kfaceauth-camera-preview-worker

engine/
  -> protocol
  -> vision
  -> templates
  -> daemon
  -> cli
```

## KCM coordination

`NativeFaceAuthBackend` implements the provider-neutral `FaceAuthBackend`
interface. Requests, progress, completion, and cancellation carry a monotonically
increasing generation. `RefreshCoordinator` ignores stale generations, and the
backend cancels a pending generation before scheduling a newer one.

The production backend is fully queued and currently completes with
`native-engine-unavailable`. It does not use `QProcess`, a shell, a command
path, a socket, or network access. An injected availability probe exists only
for deterministic unit tests of the typed skeleton state.

`SystemProbe` runs bounded local reads through `QtConcurrent` and applies results
only when its generation is current. It reports OS, display-manager, Secure
Boot, native-engine, and explicit unsupported capability states. Unknown or
missing data is never interpreted as success.

## Camera path

`CameraPreviewSession` owns a separate unprivileged worker process. This is the
only runtime process started by the KCM, and it handles preview pixels only. It
does not share a transport, state, or implementation with the native engine.
The existing framed CBOR protocol, session identifiers, monotonic sequences,
bounds, timeouts, backpressure, cancellation, and frame clearing remain intact.

## Rust workspace

The workspace is source-only in Milestone 1:

- `protocol` defines protocol version 1, a four-byte big-endian frame length,
  4 KiB requests, 16 KiB responses, closed request/response enums, and typed
  stable errors;
- `vision` reports processing unavailable;
- `templates` reports persistence unavailable;
- `daemon` dispatches exactly one bounded status or capability request over
  caller-provided local streams;
- `cli` prints only the source skeleton's typed status and capabilities.

All crates forbid unsafe Rust and have no third-party dependencies. They do not
open sockets, access cameras, persist data, load models, or make authentication
decisions. The KCM does not invoke either Rust binary.

## Identity

The temporary product, KCM, application, QML, translation, worker, and support
report identifiers are defined in `cmake/ProjectIdentity.cmake`. Generated
desktop and plugin metadata derive from that file. No compatibility aliases are
installed.
