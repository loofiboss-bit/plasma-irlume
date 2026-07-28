# Fedora packaging

The spec builds plasma-irlume 3.0.0 for Fedora 44. It requires
`irlume >= 0.7.0`, builds against Qt Multimedia and libudev (`systemd-devel`),
and lets RPM generate the corresponding native runtime dependencies.

```bash
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
```

Inspect and test the binary RPM:

```bash
rpm -qlp "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-3.0.0-1.fc44.$(uname -m).rpm"
rpm -qp --requires "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-3.0.0-1.fc44.$(uname -m).rpm"
packaging/fedora/rpm-smoke-test.sh \
  "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-3.0.0-1.fc44.$(uname -m).rpm"
```

The smoke test requires `/usr/libexec/plasma-irlume-camera-preview-worker`,
rejects setuid/setgid bits and file capabilities, forbids privileged helpers,
system D-Bus and Polkit payloads, rejects authentication scriptlets, hashes a
PAM sentinel through install/remove, and confirms engine and user data survive.
