# Fedora packaging

The Fedora 44 RPM builds the KCM and three ordinary-user workers. It installs
the exact verified YuNet FP32 and SFace FP32 artifacts with manifest, licenses,
and immutable provenance. Fedora supplies OpenCV 4.13, OpenSSL 3, and KF6
KWallet; none is bundled.

## Build and reproduce

```bash
SOURCE_DATE_EPOCH=0 packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/kfaceauth.spec --define "_sourcedir $PWD"
```

Repeat the source archive twice and compare SHA-256. Repeat SRPM/RPM builds at
the same normalized rpmbuild path and compare bytes. The complete prepared
source set is offline: Cargo is locked/offline and no model is downloaded.

## Inspect

```bash
rpm_path="$(find "$HOME/rpmbuild/RPMS" -name 'kfaceauth-[0-9]*.rpm' -print -quit)"
rpm -qpl "$rpm_path"
rpm -qp --requires "$rpm_path"
rpm -qp --scripts "$rpm_path"
rpm -qp --dump "$rpm_path"
rpm2cpio "$rpm_path" | cpio -itv
rpmlint "$rpm_path"
packaging/fedora/rpm-smoke-test.sh "$rpm_path"
```

Inspect worker modes/ownership, ELF `NEEDED` entries, file capabilities,
scriptlets, model hashes, and license/provenance payload. The package must have
no PAM file/module, authselect mutation, service unit, privileged helper,
setuid/capability, evaluator, fake provider, or authentication scriptlet.

Release qualification additionally uses ordinary dependency-resolved
`dnf install`, upgrade, and remove in a clean Fedora 44 environment. `--nodeps`
is never clean-install evidence. Installation/upgrade must not create a
profile or KWallet key. Removal must not inspect or delete user-home data;
profile deletion is only an explicit application action.
