# plasma-irlume

`plasma-irlume` is a KDE Plasma System Settings integration for the
[`irlume`](https://github.com/archledger/irlume) face-authentication engine on
Fedora KDE.

The repository is currently at **Phase 3: native enrollment and profile
management**. It contains a discoverable Plasma 6 KCM, live Fedora and hardware
diagnostics, a strict structured irlume process adapter, a typed `ProfileModel`,
and guided enrollment, recognition testing, appearance-scan, cancellation,
camera-recovery, and profile-deletion flows.

Profile mutations are fail-closed behind irlume's proposed versioned JSON/JSONL
integration contract. The current irlume 0.6.1 release does not publish that
contract, so installed 0.6.x systems show profile management as unavailable
instead of parsing human output or using the private daemon protocol. There is
still no privileged helper or PAM mutation; the KCM cannot change
authentication.

## Engine compatibility

Read-only diagnostics support the documented commands in irlume 0.6.x. The
diagnostic adapter invokes only fixed commands (`--version`, `status`, `doctor`,
and `login status`), parses a narrow set of known fields defensively, and fails
closed for other versions or malformed output.

Phase 3's mutation adapter invokes only fixed machine-mode commands, validates
contract version, command, operation ID, monotonic sequence, terminal event,
bounded output, safe opaque profile IDs, and the absence of sensitive fields.
It never displays or stores frames and never accepts a username, executable,
path, or free-form argument from QML. Enrollment is tested automatically; an
unverified new profile is deleted through the same typed contract.

See:

- [Engine contract](docs/ENGINE-CONTRACT.md)
- [Architecture boundary](docs/ARCHITECTURE.md)
- [Upstream API request](docs/UPSTREAM-API-REQUEST.md)
- [Phase 3 test matrix](docs/TEST-MATRIX.md)
- [Fedora 44 KDE project plan](FEDORA_44_KDE_FACE_LOGIN_PROJECT_PLAN.md)

## Build and test

Required development packages include Qt 6, Extra CMake Modules, and the KF6
CoreAddons, I18n, Kirigami, and KCMUtils development packages.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
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
