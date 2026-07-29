# Building and testing

## Fedora 44 dependencies

```bash
sudo dnf install \
  cargo clang-tools-extra cmake extra-cmake-modules gcc-c++ ninja-build rust \
  kf6-kcmutils-devel kf6-kcoreaddons-devel kf6-ki18n-devel \
  kf6-kirigami-devel kf6-kwallet-devel \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtmultimedia-devel \
  opencv-devel openssl-devel systemd-devel
```

KFaceAuth links Fedora OpenCV 4.13, OpenSSL 3, and KWallet. It does not bundle
those system libraries. `systemd-devel` supplies libudev headers only; no
systemd unit or runtime service is added.

## Full local gates

```bash
cmake --fresh -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'

cargo fmt --manifest-path engine/Cargo.toml --all -- --check
cargo clippy --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked --offline -- -D warnings
cargo test --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked --offline

/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
msgfmt --check -o /dev/null po/sv/kcm_kfaceauth.po
find src tests/unit engine/vision-opencv-sys/native \
  engine/crypto-openssl-sys/native \
  -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
  | xargs -0 clang-format --dry-run --Werror
python3 tools/verify_models.py --root models
git diff --check
```

Cargo has no registry dependencies. Model weights are present in the complete
prepared source set. All Cargo operations use `--locked --offline`; no
configure, build, test, install, or runtime step downloads data.

For native ASan/UBSan coverage:

```bash
cmake --fresh -S . -B build-sanitized -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DKFACEAUTH_ENABLE_NATIVE_SANITIZERS=ON
cmake --build build-sanitized --parallel
ASAN_OPTIONS=detect_leaks=1 \
  ctest --test-dir build-sanitized --output-on-failure \
  -R yunet_bridge_native
```

## Aggregate evaluation

The non-installed evaluator requires an explicitly supplied, permissioned PPM
dataset manifest. It runs the same one-request identity service path in a
short-lived evaluator subprocess for worker latency and memory measurements.
It never captures a camera and outputs aggregate JSON only. See
[HARDWARE-QUALIFICATION.md](HARDWARE-QUALIFICATION.md).

## Staged installation

```bash
DESTDIR="$PWD/stage" cmake --install build
find stage -type f -o -type l
```

The payload includes the KCM, translation, camera/vision/identity workers,
YuNet/SFace models, licenses, provenance, and manifest. It contains no
evaluator, fake provider, PAM module, service, privileged helper, enrolled
profile, or key.
