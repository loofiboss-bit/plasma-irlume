# Fedora packaging

`kfaceauth.spec` builds the Milestone 3 KCM, camera preview worker, and
short-lived Rust vision worker for Fedora 44. It installs the verified YuNet
detector and provenance records and uses Fedora OpenCV 4.13 for real inference.
It has no
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
CMake remaps source paths in the packaged Rust worker and its C++ bridge.
Byte-for-byte SRPM and RPM comparisons must use the same normalized rpmbuild
paths because RPM stores expanded build-script paths in the source package
header. Repeating the build at that normalized path must produce identical
source archives, SRPMs, and RPMs.
The spec builds with repository-resolved `opencv-devel` and declares the
specific OpenCV runtime libraries used by the worker. No OpenCV binary is
vendored. The existing `systemd-devel` build dependency supplies libudev
headers only; no systemd service or runtime dependency is introduced.

## Inspect

```bash
rpm -qlp "$HOME"/rpmbuild/RPMS/*/kfaceauth-4.0.0-1*.rpm
rpm -qp --requires "$HOME"/rpmbuild/RPMS/*/kfaceauth-4.0.0-1*.rpm
packaging/fedora/rpm-smoke-test.sh \
  "$(find "$HOME/rpmbuild/RPMS" -name 'kfaceauth-[0-9]*.rpm' -print -quit)"
```

The smoke test is a payload-focused isolated test. Release qualification must
additionally use ordinary `dnf install ./kfaceauth-*.rpm` in a clean Fedora 44
environment and report separately if dependency resolution fails. The smoke
test checks the KCM, desktop file, both workers, model checksum,
permissions/capabilities,
absence of privileged payloads and scriptlets, unchanged PAM sentinel, and
preserved unrelated user data.
