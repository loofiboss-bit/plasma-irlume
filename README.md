# plasma-irlume

`plasma-irlume` is a KDE Plasma System Settings frontend for the separately
packaged [`irlume`](https://github.com/archledger/irlume) face-authentication
engine on Fedora KDE.

Version 3.0 adds **Camera Check**: local RGB/IR discovery and a manually
started, 60-second camera preview. Camera access is isolated in the
unprivileged `/usr/libexec/plasma-irlume-camera-preview-worker`. Frames remain
in memory and are never used for face detection, enrollment, liveness,
security classification, or authentication.

The existing irlume Machine API Contract 1 integration remains asynchronous
and read-only. **Face Profiles** and **Access** display engine state but cannot
modify profiles, configure PAM, or make authentication decisions.

## Security and privacy boundary

- Preview starts only after an explicit click and stops after 60 seconds.
- Leaving Camera Check, deactivating System Settings, a worker failure, or KCM
  teardown stops capture and clears the current frame.
- The worker receives only fixed discovery/start/stop commands through private
  process pipes. The protocol is length-framed CBOR v1 with a session ID and
  monotonic sequences.
- Discovery is limited to 16 devices. Preview is limited to 640×480, 8 fps,
  and 128 KiB JPEG per frame with latest-frame backpressure.
- RGB and IR labels come from reviewed udev properties. A name never creates
  an IR or security claim; unclassified devices are **Unknown**.
- No image, device identifier, or preview state is written to configuration,
  logs, support reports, or disk.
- The package contains no root helper, KAuth/Polkit action, system D-Bus
  service, daemon, PAM scriptlet, model, or biometric processing.

See [Architecture](docs/ARCHITECTURE.md),
[worker protocol](docs/CAMERA-PREVIEW-PROTOCOL.md),
[engine contract](docs/ENGINE-CONTRACT.md), and
[native roadmap](docs/NATIVE-ENGINE-ROADMAP.md).

## Engine compatibility

The fixed handshake is `/usr/bin/irlume version --json`. When Contract 1 is
advertised, the KCM may run only these capability-gated read commands:

```text
irlume status --json --contract 1
irlume doctor --json --contract 1
irlume profiles list --json --contract 1
irlume login status --json --contract 1
```

Compatibility follows the contract and capabilities rather than an upper
engine-version bound. The RPM requires `irlume >= 0.7.0`.

## Install on Fedora 44

```bash
sudo dnf copr enable archledger/irlume
sudo dnf copr enable loofitheboss/plasma-irlume
sudo dnf install plasma-irlume
```

Open **System Settings → Security & Privacy → Face Login**, or run
`kcmshell6 kcm_irlume`.

## Build and test

Development packages include Qt 6 Multimedia and libudev:

```bash
sudo dnf install cmake extra-cmake-modules gcc-c++ ninja-build \
  kf6-kcmutils-devel kf6-kcoreaddons-devel kf6-ki18n-devel \
  kf6-kirigami-devel qt6-qtbase-devel qt6-qtdeclarative-devel \
  qt6-qtmultimedia-devel systemd-devel
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
```

CI tests camera behavior with an injected provider and requires no physical
camera. Real-hardware qualification is a separate Fedora 44 release gate.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
