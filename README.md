# plasma-irlume

`plasma-irlume` is a KDE Plasma System Settings integration for the
[`irlume`](https://github.com/archledger/irlume) face-authentication engine on
Fedora KDE.

The repository contains the **Phase 6 / 1.0.0 experimental release**: a
discoverable Plasma 6 KCM, live Fedora and hardware diagnostics, guided profile
management, a fixed-operation KAuth helper, transactional authentication
planning and activation, post-apply verification, automatic rollback,
one-click verified disable, in-product TTY recovery, actionable error guidance,
redacted Markdown support-report export, and reproducible Fedora 44 packaging.

> **Experimental:** version 1.0.0 is not a production release until the
> real-hardware and clean-install release matrix in
> [the test matrix](docs/TEST-MATRIX.md) passes. Packaging readiness does not
> establish biometric or authentication safety on a particular computer.

Profile mutations are fail-closed behind irlume's proposed versioned JSON/JSONL
integration contract. The current irlume 0.6.1 release does not publish that
contract, so installed 0.6.x systems show profile management as unavailable
instead of parsing human output or using the private daemon protocol.
Authentication mutations use the same fail-closed policy: irlume 0.6.1 does
not advertise `login-transactions`, so the KCM cannot apply a live
authentication change on that release.

## Engine compatibility

Read-only diagnostics support the documented commands in irlume 0.6.x. The
diagnostic adapter invokes only fixed commands (`--version`, `status`, `doctor`,
and `login status`), parses a narrow set of known fields defensively, and fails
closed for other versions or malformed output.

The mutation adapters invoke only fixed machine-mode commands and validate
contract version, command, operation ID, monotonic sequence, terminal event,
bounded output, safe opaque profile IDs, and the absence of sensitive fields.
They never display or store frames and never accept a username, executable,
PAM path, or free-form command from QML. Enrollment is tested automatically; an
unverified new profile is deleted through the same typed contract.

Authentication activation is an irlume-owned plan → apply → verify transaction.
The privileged helper validates Fedora 44, the active display manager,
enrollment, password fallback, and Secure-tier eligibility. Failed verification
triggers rollback before an error reaches the KCM.

See:

- [Installation guide](docs/INSTALLATION.md)
- [User guide](docs/USER-GUIDE.md)
- [Engine contract](docs/ENGINE-CONTRACT.md)
- [Architecture boundary](docs/ARCHITECTURE.md)
- [Upstream API request](docs/UPSTREAM-API-REQUEST.md)
- [Phase 6 test matrix](docs/TEST-MATRIX.md)
- [Fedora packaging](packaging/fedora/README.md)
- [TTY recovery](docs/RECOVERY.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)
- [Fedora 44 KDE project plan](FEDORA_44_KDE_FACE_LOGIN_PROJECT_PLAN.md)

## Install on Fedora 44

Enable the separate engine repository and the plasma-irlume repository, then
install the KCM:

```bash
sudo dnf copr enable archledger/irlume
sudo dnf copr enable loofitheboss/plasma-irlume
sudo dnf install plasma-irlume
```

Open **System Settings → Security & Privacy → Face Login**, or run:

```bash
kcmshell6 kcm_irlume
```

Read the [installation guide](docs/INSTALLATION.md) before changing
authentication. It covers the supported platform, the current engine contract
gate, updates, removal, verification, and recovery preparation. The
[user guide](docs/USER-GUIDE.md) explains every page and the safe first-run
flow.

## Build and test

Required development packages include Qt 6, Extra CMake Modules, and the KF6
Auth, CoreAddons, I18n, Kirigami, and KCMUtils development packages.

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

Fedora 44 packaging requires the separate `irlume` security engine. The KCM
does not bundle the engine, biometric models, profiles, PAM modules, or camera
code. Read-only diagnostics support `irlume >= 0.6.0` and `< 0.7.0`; mutation
features remain fail-closed unless a reviewed engine advertises the structured
contract described in [the engine contract](docs/ENGINE-CONTRACT.md).

Build the source and binary RPM locally with:

```bash
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
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
