# Runtime selection

Status: Milestone 3 production decision, reviewed 2026-07-29.

## Decision

KFaceAuth uses Fedora 44's OpenCV 4.13 packages and
`cv::FaceDetectorYN` through a small, project-owned C ABI bridge. The Rust
worker remains the protocol, model-verification, preprocessing,
postprocessing, cancellation, and result-validation owner. The bridge owns
only the OpenCV C++ detector object and transfers fixed-layout rows containing
15 `float` values.

Fedora 44 provides OpenCV 4.13.0 in the official repositories. Builds require
`opencv-devel >= 4.13.0`; the installed worker links only the OpenCV core, DNN,
image-processing, and object-detection libraries that it uses. The build
rejects another OpenCV minor release until that implementation has been
reviewed. There is no bundled OpenCV binary, runtime downloader, Python
process, shell command, or network operation.

This is the smallest integration that keeps inference on Fedora-supported
system libraries without importing a large generated binding surface.

## Boundary and offline behavior

`engine/vision-opencv-sys` is the only crate allowed to contain `unsafe` Rust.
Its public Rust wrapper validates slices and numeric limits before calling the
C ABI. The C++ side:

- copies the caller's BGR pixels into an OpenCV-owned matrix;
- initializes `FaceDetectorYN` from the already verified in-memory ONNX bytes;
- selects the OpenCV CPU backend and target;
- accepts no filenames, URLs, device handles, or arbitrary operations;
- catches C++ exceptions before they can cross the C ABI;
- returns stable status values and bounded POD rows;
- disables process core dumps before model or frame processing;
- clears temporary model, pixel, and detection buffers where practical.

Configure and build use the system compiler, `pkg-config`, and already
installed Fedora packages. Cargo has no registry dependencies, so locked
offline builds are supported. Configure, build, test, installation, and
runtime never fetch a model or inference component.

The C++ standard-library and OpenCV ABI remain risks. They are contained in the
short-lived unprivileged worker, covered by native tests and ASan/UBSan builds,
and excluded from the long-lived KCM process. Worker crashes, malformed output,
timeouts, and cancellation fail closed and produce no fallback result.

## Considered alternatives

### Maintained Rust OpenCV bindings

The `opencv` crate exposes `FaceDetectorYN`, but it adds generated bindings,
build-time probing, and a substantially wider unsafe FFI surface than this
worker needs. Fedora 44 does not provide a packaged Rust OpenCV crate for this
application to consume offline. Vendoring the crate and its transitive build
surface would weaken the current dependency and audit model.

### Direct OpenCV C++ worker

A C++-only worker would remove one FFI hop, but it would duplicate or replace
the existing reviewed Rust framing, model-inventory, cancellation, and typed
result code. That is a larger security and maintenance change than a narrow
detector bridge.

### ONNX Runtime or another inference engine

No alternative offered a stronger Fedora 44 packaging case for this milestone.
Bundling a precompiled runtime, downloading it during a build, or using a
Python runtime is rejected. A new runtime would also require a separate
license, ABI, operator-support, preprocessing, and packaging review without
providing a material benefit for the already selected YuNet model.

## Security conclusion

The selected design introduces an explicit native FFI boundary, but keeps it
small, version-gated, testable, and inside a one-request process. Detection
output remains untrusted until Rust validates every finite score, rectangle,
and landmark. Face detection is neutral camera guidance; it is not identity,
liveness, enrollment, or an authentication decision.
