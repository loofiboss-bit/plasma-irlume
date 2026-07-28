# KFaceAuth

KFaceAuth is a temporary neutral name for a standalone native KDE
face-authentication project. Milestone 1 establishes safe architecture and
does **not** authenticate users.

The current KCM provides:

- asynchronous native-engine status with explicit unavailable and unsupported
  states;
- bounded local system probes;
- reusable Kirigami status and diagnostics components;
- local RGB/infrared camera discovery and an explicitly started, 60-second
  in-memory preview;
- a source-only Rust workspace with a versioned, bounded local protocol and
  typed status/capability responses.

The current KCM does not start an engine executable. The Rust skeleton is not
installed as a service and is not connected to the KCM in this milestone.
There is no face detection, recognition, liveness decision, enrollment,
biometric persistence, PAM module, privileged helper, SELinux policy, network
access, or runtime model download.

## Privacy boundary

The camera preview remains a separate unprivileged worker:

- capture starts only after an explicit click and stops after 60 seconds;
- leaving Camera Check, deactivating the KCM, worker failure, or teardown stops
  capture and clears the frame;
- frames are bounded to 640×480, 8 fps, and 128 KiB JPEG;
- discovery is limited to 16 devices and uses reviewed udev properties;
- frames and device identifiers never enter configuration, logs, reports, the
  native backend, or the Rust workspace.

Finding a camera does not establish identity, liveness, security, enrollment,
or authentication readiness.

## Build and verify

```bash
cmake --fresh -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml

cargo fmt --manifest-path engine/Cargo.toml --all -- --check
cargo clippy --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked -- -D warnings
cargo test --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked
```

See [architecture](docs/ARCHITECTURE.md),
[threat boundary](docs/THREAT-BOUNDARY.md),
[build guide](docs/BUILDING.md),
[camera protocol](docs/CAMERA-PREVIEW-PROTOCOL.md), and
[roadmap](docs/ROADMAP.md).

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
