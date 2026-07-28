# Architecture

Milestone 2 separates five unprivileged areas:

```text
KDE System Settings
  -> KFaceAuthKcm
     -> RefreshCoordinator
        -> NativeFaceAuthBackend
     -> SystemProbe
     -> CameraPreviewSession
        -> kfaceauth-camera-preview-worker
     -> VisionAnalysisSession
        -> kfaceauth-vision-worker

engine/
  -> protocol
  -> vision
  -> vision-worker
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

`VisionAnalysisSession` is separate from preview capture. A user must first
start preview and then explicitly request analysis. C++ copies only the current
bounded `QImage`, normalizes it to an accepted packed pixel format, and sends it
over private process pipes. It never exposes bytes to QML or uses a temporary
file. Each request has a monotonic generation; stale responses are discarded.
Page hiding, application deactivation, preview stop, cancellation, protocol
failure, timeout, or teardown kills the short-lived worker and clears both the
frame copy and result.

## Rust workspace

The workspace keeps the Milestone 1 status skeleton and adds bounded vision:

- `protocol` defines protocol version 1, a four-byte big-endian frame length,
  4 KiB requests, 16 KiB responses, closed request/response enums, and typed
  stable errors;
- `vision` owns model-manifest parsing, SHA-256 verification, checked frame
  validation, backend-neutral detector/quality types, cancellation, and the
  deterministic provider;
- `vision-worker` accepts one versioned length-framed analyze operation on
  stdin/stdout and never opens a socket or persistent store;
- `templates` reports persistence unavailable;
- `daemon` dispatches exactly one bounded status or capability request over
  caller-provided local streams;
- `cli` prints only the source skeleton's typed status and capabilities.

All crates forbid unsafe Rust and have no third-party dependencies. Build and
test are offline. The vision worker verifies the manifest and selected model
directory before provider initialization, but it does not download anything,
open sockets, access cameras, persist data, produce embeddings, or make
authentication decisions. It is an executable owned by the source and package,
not a daemon or system service.

## Model supply chain

`models/manifest.kfaceauth` is the machine-readable allow-list. It pins filename,
version, immutable origin, size, SHA-256, role, provider state, and license.
`models/files` contains only listed, license-reviewed artifacts.
`tools/verify_models.py` and Rust initialization both reject missing, renamed,
modified, duplicate, malformed, or unlisted files. Configure, build, test,
install, and runtime never fetch model data.

## Identity

The temporary product, KCM, application, QML, translation, worker, and support
report identifiers are defined in `cmake/ProjectIdentity.cmake`. Generated
desktop and plugin metadata derive from that file. No compatibility aliases are
installed.
