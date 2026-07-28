# plasma-irlume

`plasma-irlume` is a KDE Plasma System Settings frontend for the separately
packaged [`irlume`](https://github.com/archledger/irlume) face-authentication
engine on Fedora KDE.

Version 2.2 is a hardened, read-only integration. It negotiates irlume Machine
API Contract 1 asynchronously and presents typed readiness, profile summaries,
camera capability, and authentication-wiring status. It does not implement a
biometric engine.

## Engine compatibility

The backend starts with the fixed handshake:

```text
/usr/bin/irlume version --json
```

Contract 1 is selected when the advertised contract range includes `1`.
Compatibility is based on the contract and capabilities, not
`engine_version`; future engine versions remain acceptable when they implement
Contract 1. The following fixed read-only commands are capability-gated:

```text
irlume status --json --contract 1
irlume doctor --json --contract 1
irlume profiles list --json --contract 1
irlume login status --json --contract 1
```

Profile enrollment and maintenance, camera configuration, authentication
tests, and login/PAM changes are disabled because Contract 1 exposes no
reviewed mutation capability. Attempts fail locally with
`capability-unavailable` and never start an undocumented engine command.
Only one engine process may run per backend instance. Refreshes are
generation-tagged, latest-request-wins, time-limited, and independently bound
stdout and stderr to 256 KiB.

See [Backend abstraction](docs/BACKEND-ABSTRACTION.md),
[Engine contract](docs/ENGINE-CONTRACT.md), and
[Native engine roadmap](docs/NATIVE-ENGINE-ROADMAP.md).

## Install on Fedora 44

```bash
sudo dnf copr enable archledger/irlume
sudo dnf copr enable loofitheboss/plasma-irlume
sudo dnf install plasma-irlume
```

Open **System Settings → Security & Privacy → Face Login**, or run:

```bash
kcmshell6 kcm_irlume
```

Installing, upgrading, or removing the KCM does not activate, deactivate, or
rewrite authentication. The package does not bundle irlume, models, profiles,
PAM modules, a daemon, camera code, a privileged helper, a system D-Bus
service, or a Polkit action.

## Build and test

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
```

Optionally qualify an already installed engine without network access:

```bash
scripts/check-installed-irlume-contract.py
```

Build Fedora packages locally:

```bash
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
```

Sanitized fixtures under `tests/fixtures/irlume/contract-v1/` represent the
released public Contract 1. They are validated offline against the exact
vendored irlume 0.7.0 Draft 2020-12 schema. Proposed mutation fixtures are
historical design artifacts and are never consumed by production code.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
