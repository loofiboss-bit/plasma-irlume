# Building and testing

## Fedora 44 dependencies

```bash
sudo dnf install \
  cargo clang-tools-extra cmake extra-cmake-modules gcc-c++ ninja-build rust \
  kf6-kcmutils-devel kf6-kcoreaddons-devel kf6-ki18n-devel \
  kf6-kirigami-devel qt6-qtbase-devel qt6-qtdeclarative-devel \
  qt6-qtmultimedia-devel systemd-devel
```

No external face-authentication package is required.

## CMake and Qt tests

```bash
cmake --fresh -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
```

## Rust checks

The Rust workspace has no external crates and can be built offline after the
toolchain is installed.

```bash
cargo fmt --manifest-path engine/Cargo.toml --all -- --check
cargo clippy --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked -- -D warnings
cargo test --manifest-path engine/Cargo.toml \
  --workspace --all-targets --locked
```

## Staged installation

```bash
DESTDIR="$PWD/stage" cmake --install build
find stage -type f -o -type l
```

The staged payload contains the KCM, desktop metadata, translations, and camera
preview worker. The Rust skeleton is deliberately not installed in Milestone 1.
