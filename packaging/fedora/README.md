# Fedora packaging

The spec builds plasma-irlume 2.2.0 for Fedora 44 and requires irlume 0.7.0 or
newer. Compatibility uses Machine API Contract 1, so there is no upper engine
version bound.

Build locally:

```bash
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
```

Inspect and test the binary RPM:

```bash
rpm -qlp "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-2.2.0-1.fc44.$(uname -m).rpm"
rpm -qp --requires "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-2.2.0-1.fc44.$(uname -m).rpm"
packaging/fedora/rpm-smoke-test.sh \
  "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-2.2.0-1.fc44.$(uname -m).rpm"
```

The smoke test requires the KCM plugin and desktop file, forbids privileged
helper, system D-Bus, and Polkit payloads, verifies the absence of
authentication scriptlets, hashes a PAM sentinel before install, after
install, and after removal, and confirms that engine and user data survive.
