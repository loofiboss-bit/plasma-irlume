# Fedora packaging

The spec targets Fedora 44 and builds plasma-irlume 2.1.0. The package is a
KDE frontend for the separately packaged `irlume >= 0.7.0` engine. It has no
upper engine-version bound because compatibility is negotiated through Machine
API Contract 1 and advertised capabilities.

The RPM does not bundle irlume, models, templates, PAM modules, daemon code, or
camera drivers.

## Local build

```bash
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
rpmlint packaging/fedora/plasma-irlume.spec
```

Run the isolated package lifecycle check against the resulting RPM:

```bash
packaging/fedora/rpm-smoke-test.sh \
  "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-2.1.0-1.fc44.$(uname -m).rpm"
```

The smoke test validates ownership and non-mutating uninstall behavior in a
temporary RPM root. Installing, upgrading, or removing this package runs no
irlume, PAM, authselect, or profile command. Engine-owned profiles and existing
authentication state survive removal of the GUI.

The package and tests do not establish biometric accuracy, liveness, or
hardware readiness. Those require separate real-hardware qualification.
