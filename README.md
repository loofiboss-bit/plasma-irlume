# KFaceAuth

KFaceAuth 4.0.0 is a standalone native KDE local identity experiment. It can
enroll and locally compare the currently logged-in user's face. A `Match` is an
in-session UI result only: it does not unlock, authenticate, authorize, invoke
PAM or Polkit, or alter the session.

The KCM provides:

- explicit, bounded RGB/infrared preview and one-frame YuNet analysis;
- verified YuNet FP32 detection plus verified SFace FP32 alignment and
  128-value embedding extraction through Fedora OpenCV 4.13;
- deliberate enrollment of 3–5 samples, with a hard stored maximum of 8;
- one encrypted profile for the current numeric UID;
- a random AES-256-GCM vault key stored only in KDE KWallet;
- aggregate profile status, explicit deletion, and explicit unreadable reset;
- one-click, rate-limited local verification returning `Match`, `No match`,
  `Ambiguous`, or a typed unavailable result;
- short-lived, unprivileged workers with private pipes, strict bounds,
  cancellation, deadlines, no network, and no telemetry.

Embeddings are sensitive biometric data. No captured image is intentionally
persisted. Frames, landmarks, embeddings, keys, and similarity scores do not
reach QML, normal logs, the CLI, or support reports.

There is no liveness or presentation-attack detection, security tier, PAM
module/configuration, authselect change, SDDM/lock-screen integration,
sudo/Polkit integration, privileged helper, system service, TPM sealing,
background recognition, networking, or runtime model download. KWallet keys
are unavailable before login. FAR, FRR, bias, spoof resistance, and broad
hardware behavior remain unqualified.

The code and automated gates are prepared as a v4.0.0 release candidate.
Publication remains blocked until the required physical-camera, keyboard,
assistive-technology, and representative identity qualification is recorded.

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
[identity pipeline](docs/IDENTITY-PIPELINE.md),
[identity protocol](docs/IDENTITY-PROTOCOL.md),
[template vault](docs/TEMPLATE-VAULT.md),
[embedding decision](docs/EMBEDDING-MODEL-SELECTION.md),
[build guide](docs/BUILDING.md), and
[hardware qualification](docs/HARDWARE-QUALIFICATION.md).

## License

Project code is GPL-3.0-or-later. YuNet weights are MIT. SFace weights and
Fedora OpenCV are Apache-2.0. The exact model licenses and provenance are
shipped in the closed model inventory.
