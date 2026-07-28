# Fedora packaging

`kfaceauth.spec` builds the Milestone 2 KCM, camera preview worker, and
short-lived Rust vision worker for Fedora 44. It installs the verified YuNet
detector and provenance records without enabling real inference. It has no
external face-authentication engine dependency and installs no service, PAM
module, policy, embedding, or biometric storage.

## Build

```bash
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/kfaceauth.spec \
  --define "_sourcedir $PWD"
```

The source archive is reproducible in ordering, ownership, timestamps (when
`SOURCE_DATE_EPOCH` is set), mode normalization, and top-level directory.

## Inspect

```bash
rpm -qlp "$HOME"/rpmbuild/RPMS/*/kfaceauth-4.0.0-1*.rpm
rpm -qp --requires "$HOME"/rpmbuild/RPMS/*/kfaceauth-4.0.0-1*.rpm
packaging/fedora/rpm-smoke-test.sh \
  "$(find "$HOME/rpmbuild/RPMS" -name 'kfaceauth-[0-9]*.rpm' -print -quit)"
```

The smoke test performs isolated install, upgrade, and removal. It checks the
KCM, desktop file, both workers, model checksum, permissions/capabilities,
absence of privileged payloads and scriptlets, unchanged PAM sentinel, and
preserved unrelated user data.
