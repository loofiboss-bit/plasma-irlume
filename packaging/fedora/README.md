# Fedora packaging

The spec targets Fedora 44 and builds plasma-irlume 1.0.0 as an experimental
package. `irlume` remains a separate upstream security dependency and is never
vendored into this source or RPM.

## Local build

Install the build dependencies declared in `plasma-irlume.spec`, then run:

```bash
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
rpmlint packaging/fedora/plasma-irlume.spec
```

The archive timestamp defaults to the current commit timestamp. Set
`SOURCE_DATE_EPOCH` explicitly to reproduce an archive from another controlled
timestamp.

Run the isolated package lifecycle check against the resulting architecture
RPM:

```bash
packaging/fedora/rpm-smoke-test.sh \
  "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-1.0.0-1.fc44.$(uname -m).rpm"
```

The smoke test uses a temporary RPM root and `--nodeps`. It validates payload
ownership and non-mutating uninstall behavior; it is not a substitute for the
live Fedora 44 KDE and real-hardware release matrix.

## Dependency and lifecycle policy

- Runtime diagnostics require `irlume >= 0.6.0` and `< 0.7.0`.
- Profile and authentication mutations additionally require the reviewed
  structured capabilities documented in `docs/ENGINE-CONTRACT.md`.
- Installing, upgrading, or erasing this RPM runs no irlume, PAM, authselect,
  or profile command.
- The RPM owns only the KCM, KAuth helper, desktop metadata, D-Bus policy,
  Polkit action, available translations, and project documentation.
- Engine-owned profiles and authentication state intentionally survive removal
  of the GUI.

## Release policy

Version 1.0.0 is published only as an experimental package. COPR availability
proves that the package can be built and distributed; it does not satisfy the
real-hardware, live PAM, display-manager, or recovery gates in
`docs/TEST-MATRIX.md`. Do not describe the package as production-ready until
those results have been recorded.
