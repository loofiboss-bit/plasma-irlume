# Threat boundary

Milestone 3 adds real local experimental face-presence analysis. It is not an
authentication system.

## Trusted in this milestone

- fixed, bounded local platform reads;
- the KCM's generation-aware asynchronous coordinator;
- the reviewed camera worker protocol and its strict bounds;
- the Rust protocol codec's closed message types and length checks.
- the model manifest allow-list and SHA-256 verification;
- the Rust vision frame parser's checked dimensions, stride, and payload
  arithmetic;
- short-lived private process pipes and monotonic analysis generations;
- the exact OpenCV 4.13 minor-version gate and narrow reviewed C ABI bridge;
- Rust validation of all native output before protocol encoding.

## Explicitly outside the trusted computing base

- embeddings, matching, and liveness;
- enrollment and deletion workflows;
- biometric templates, encryption keys, and persistence;
- PAM decisions or configuration;
- privileged helpers, system services, and policy installation;
- SELinux policy;
- network access, telemetry, model download, and remote inference.

These areas are represented as `Unsupported` or `Not implemented`. They have no
mutation API, command path, fallback heuristic, or permissive default.

## Fail-closed rules

- Missing native-engine status keeps the KCM responsive but never enables a
  biometric or PAM operation.
- Unsupported protocol versions and malformed or oversized frames are rejected.
- The Rust dispatcher recognizes only `Capabilities` and `Status`.
- The vision worker recognizes only one framed `Analyze` request, processes at
  most one frame, and terminates after a protocol violation or response.
- Model loading fails closed for a malformed manifest, missing/renamed/unlisted
  artifact, size mismatch, or SHA-256 mismatch. There is no fallback model.
- Camera presence and visible preview pixels never become a security,
  liveness, identity, or readiness claim.
- Zero/one/multiple-face and quality results are neutral guidance and never an
  authentication decision.
- Stale backend or system-probe generations cannot replace current state.
- Stale vision generations cannot replace the current result.
- Invalid native tensors, non-finite values, out-of-frame coordinates, worker
  crashes, timeouts, or resource failures produce no partial result.
- Production has no fake-provider selector or fallback.
- Package installation, upgrade, and removal contain no authentication
  scriptlets and do not touch PAM.

## Preview privacy

Preview capture is manual, time-limited, bounded, and ephemeral. Frames remain
in memory, are dropped under backpressure, and are cleared when capture stops.
Support reports contain only aggregate device counts, spectrum counts, stable
error codes, and dropped-frame counts. They exclude frames, labels, tokens, and
device nodes.

Vision analysis adds a second explicit consent boundary. One decoded frame is
copied only after the user requests analysis. It remains in parent/worker
memory, is not exposed to QML or written to disk, and is cleared when the
request or Camera Check session ends. Normal logs exclude pixels, rectangles,
landmarks, quality values, model contents, and user-supplied paths.

## Hostile input and native-library risk

The parent bounds decoded images before the worker sees them. The worker then
checks format, dimensions, stride, exact payload length, and arithmetic again.
Rust performs explicit RGB/RGBA/Gray conversion into a fresh tightly packed BGR
buffer. OpenCV never receives a pointer to the protocol buffer.

OpenCV and the C++ ABI increase memory-safety and parser risk. They are confined
to a short-lived, unprivileged, one-frame worker with private pipes, finite
timeouts, bounded output, no network, and no persistence. Native exceptions
cannot cross the C ABI. Linux core dumps are disabled before the worker reads a
request, reducing exposure of transient frames, model data, rectangles, and
landmarks after a crash.

The model manifest is a closed inventory. Rust verifies every listed artifact,
rejects unlisted files, pins exact YuNet metadata and SHA-256, and passes the
verified bytes to OpenCV. There is no filename-based native reload or substitute
model fallback.

OpenCV output is untrusted. Shape, type, row count, finite values, rectangles,
landmarks, scores, and image bounds are validated before at most eight
rectangles enter the existing protocol. Scores and landmarks are then
discarded. Cancellation kills the worker and clears the parent frame/result;
replacement analysis waits for the old process to exit.

Support reports remain aggregate and sanitized. They exclude frames, face
coordinates, landmarks, detector scores, per-user analysis results, model
tensors, user paths, and camera identifiers beyond the already reviewed
aggregate form. Detection remains separate from identity and authentication
because the project has no embedding, matching, enrollment, persistence,
liveness, or authentication API.
