# plasma-irlume

`plasma-irlume` is a KDE Plasma System Settings integration for the
[`irlume`](https://github.com/archledger/irlume) face-authentication engine on
Fedora KDE.

The repository is currently at **Phase 2: live Fedora and hardware
diagnostics**. It contains a discoverable Plasma 6 KCM, a typed `SystemState`
model, deterministic fake-adapter states for tests, live local probes, and
read-only Overview, Security, and Diagnostics pages.

There is no privileged helper, PAM mutation, enrollment workflow, or mutating
irlume adapter. The KCM cannot change authentication.

## Engine compatibility

Phase 2 supports the documented, read-only diagnostic commands in irlume
0.6.x. The adapter invokes only fixed commands (`--version`, `status`, `doctor`,
and `login status`), parses a narrow set of known fields defensively, and fails
closed for other versions or malformed output. It never exposes raw output,
profile names, usernames, device paths, images, or biometric data.

A future structured JSON/NDJSON API remains preferable, especially before
enrollment or authentication changes are implemented. It is an improvement
path rather than a blocker for read-only diagnostics.

See:

- [Engine contract](docs/ENGINE-CONTRACT.md)
- [Architecture boundary](docs/ARCHITECTURE.md)
- [Upstream API request](docs/UPSTREAM-API-REQUEST.md)
- [Fedora 44 KDE project plan](FEDORA_44_KDE_FACE_LOGIN_PROJECT_PLAN.md)

## Build and test

Required development packages include Qt 6, Extra CMake Modules, and the KF6
CoreAddons, I18n, Kirigami, and KCMUtils development packages.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
```

The proposed structured-contract checks use only Python's standard library:

```bash
python3 -m unittest discover -s tests -p 'test_*.py'
```

The fixtures under `tests/fixtures/irlume/observed-private/` are sanitized
source-derived evidence, not runtime inputs. Fixtures under
`tests/fixtures/irlume/proposed-v1/` describe the preferred future structured
API.

For a local installation prefix:

```bash
cmake --install build --prefix "$PWD/build/prefix"
XDG_DATA_DIRS="$PWD/build/prefix/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}" \
QT_PLUGIN_PATH="$PWD/build/prefix/lib64/qt6/plugins" \
kcmshell6 kcm_irlume
```

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
