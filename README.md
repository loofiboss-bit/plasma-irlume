# KFaceAuth

KFaceAuth is a temporary neutral name for a standalone native KDE
face-authentication project. Milestone 2 adds bounded local vision analysis and
does **not** authenticate users.

The current KCM provides:

- asynchronous native-engine status with explicit unavailable and unsupported
  states;
- bounded local system probes;
- reusable Kirigami status and diagnostics components;
- local RGB/infrared camera discovery and an explicitly started, 60-second
  in-memory preview;
- a second explicit action that copies one current frame through private pipes
  to a short-lived, unprivileged Rust vision worker;
- typed zero, one, or multiple-face presence and neutral image-quality results;
- a selected, hash-pinned YuNet detector with offline verification and complete
  model provenance.

Vision analysis is local and experimental. The packaged YuNet weight is not yet
used for real inference: the worker reports its deterministic fake provider
explicitly until a reviewed Rust ONNX Runtime integration is available. Face
detection is not identification, image-quality guidance is not liveness, and no
security tier is claimed.

There is no recognition, face matching, identity threshold, enrollment,
biometric persistence, PAM module, privileged helper, system service, SELinux
policy, network access, telemetry, or runtime model download.

## Privacy boundary

The camera preview remains a separate unprivileged worker:

- capture starts only after an explicit click and stops after 60 seconds;
- leaving Camera Check, deactivating the KCM, worker failure, or teardown stops
  capture and clears the frame;
- frames are bounded to 640×480, 8 fps, and 128 KiB JPEG;
- analysis requires a separate click, sends at most one decoded 640×480 frame,
  and is cancelled when preview or the Camera Check session ends;
- discovery is limited to 16 devices and uses reviewed udev properties;
- raw frames are never exposed to QML or written to disk;
- frames, face rectangles, quality results, and device identifiers never enter
  configuration, normal logs, or support reports.

Finding a camera does not establish identity, liveness, security, enrollment,
or authentication readiness. Neither does a face-presence result.

## Build and verify

```bash
cmake --fresh -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml

cargo fmt --manifest-path engine/Cargo.toml --all -- --check
cargo clippy --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked --offline -- -D warnings
cargo test --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked --offline
python3 tools/verify_models.py --root models
```

See [architecture](docs/ARCHITECTURE.md),
[threat boundary](docs/THREAT-BOUNDARY.md),
[build guide](docs/BUILDING.md),
[camera protocol](docs/CAMERA-PREVIEW-PROTOCOL.md),
[vision protocol](docs/VISION-PROTOCOL.md),
[model decision](docs/MODEL-SELECTION.md), and the
[roadmap](docs/ROADMAP.md).

## License

Project code is GPL-3.0-or-later. The selected YuNet model is MIT. See
[LICENSE](LICENSE) and [model selection](docs/MODEL-SELECTION.md).
