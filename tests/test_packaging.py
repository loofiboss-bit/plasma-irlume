from __future__ import annotations

import re
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IDENTITY = ROOT / "cmake/ProjectIdentity.cmake"
SPEC = ROOT / "packaging/fedora/kfaceauth.spec"


class PackagingContractTests(unittest.TestCase):
    def test_central_identity_drives_project_metadata(self) -> None:
        identity = IDENTITY.read_text(encoding="utf-8")
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        metadata = (ROOT / "src/kcm/kcm_kfaceauth.json.in").read_text(
            encoding="utf-8"
        )
        desktop = (ROOT / "data/kcm_kfaceauth.desktop.in").read_text(
            encoding="utf-8"
        )
        spec = SPEC.read_text(encoding="utf-8")

        for declaration in (
            'set(KFACEAUTH_PROJECT_ID "kfaceauth")',
            'set(KFACEAUTH_VERSION "4.0.0")',
            'set(KFACEAUTH_DISPLAY_NAME "KFaceAuth")',
            'set(KFACEAUTH_KCM_ID "kcm_kfaceauth")',
            'set(KFACEAUTH_APP_ID "org.kde.kfaceauth")',
            'set(KFACEAUTH_I18N_DOMAIN "kcm_kfaceauth")',
            'set(KFACEAUTH_PREVIEW_WORKER "kfaceauth-camera-preview-worker")',
            'set(KFACEAUTH_VISION_WORKER "kfaceauth-vision-worker")',
            'set(KFACEAUTH_IDENTITY_WORKER "kfaceauth-identity-worker")',
            'set(KFACEAUTH_MODEL_DIRECTORY "kfaceauth/models")',
            'set(KFACEAUTH_MODEL_MANIFEST "manifest.kfaceauth")',
        ):
            self.assertIn(declaration, identity)

        self.assertIn(
            "project(${KFACEAUTH_PROJECT_ID} VERSION ${KFACEAUTH_VERSION}",
            cmake,
        )
        self.assertIn('"Name": "@KFACEAUTH_DISPLAY_NAME@ (Development Preview)"', metadata)
        self.assertIn('"Version": "@PROJECT_VERSION@"', metadata)
        self.assertIn("Exec=systemsettings @KFACEAUTH_KCM_ID@", desktop)
        self.assertRegex(spec, r"(?m)^Name:\s+kfaceauth$")
        self.assertRegex(spec, r"(?m)^Version:\s+4\.0\.0$")
        self.assertRegex(spec, r"(?m)^Release:\s+1")

    def test_spec_has_no_external_engine_or_privileged_runtime_dependency(self) -> None:
        spec = SPEC.read_text(encoding="utf-8")
        legacy_engine = "ir" + "lume"

        self.assertNotIn(legacy_engine, spec.lower())
        self.assertNotRegex(spec, r"(?im)^Requires:.*(?:face|biometric|pam)")
        self.assertNotRegex(spec, r"(?m)^%(?:pre|post|preun|postun|trigger)(?:\s|$)")
        self.assertNotIn("kf6-kauth", spec)
        self.assertIn("%{_libexecdir}/kfaceauth-camera-preview-worker", spec)
        self.assertIn("%{_libexecdir}/kfaceauth-vision-worker", spec)
        self.assertIn("%{_libexecdir}/kfaceauth-identity-worker", spec)
        self.assertIn("face_detection_yunet_2023mar.onnx", spec)
        self.assertIn("face_recognition_sface_2021dec.onnx", spec)
        for directory in (
            "%dir %{_datadir}/kfaceauth",
            "%dir %{_datadir}/kfaceauth/models",
            "%dir %{_datadir}/kfaceauth/models/files",
            "%dir %{_datadir}/kfaceauth/models/licenses",
            "%dir %{_datadir}/kfaceauth/models/provenance",
        ):
            self.assertIn(directory, spec)
        self.assertNotIn("fake-provider-v1.cfg", spec)
        for dependency in (
            "BuildRequires:  opencv-devel",
            "Requires:       opencv-core",
            "Requires:       opencv-dnn",
            "Requires:       opencv-imgproc",
            "Requires:       opencv-objdetect",
            "BuildRequires:  openssl-devel",
            "BuildRequires:  kf6-kwallet-devel",
            "Requires:       openssl-libs",
            "Requires:       kf6-kwallet",
        ):
            self.assertIn(dependency, spec)
        self.assertRegex(
            spec,
            r"(?m)^License:\s+GPL-3\.0-or-later AND MIT AND Apache-2\.0$",
        )
        self.assertIn("kcm_kfaceauth.so", spec)

    def test_source_archive_is_reproducible_in_shape_and_has_no_legacy_identity(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "kfaceauth-4.0.0.tar.gz"
            second = Path(directory) / "kfaceauth-4.0.0-second.tar.gz"
            subprocess.run(
                [
                    str(ROOT / "packaging/fedora/create-source-archive.sh"),
                    str(output),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            subprocess.run(
                [
                    str(ROOT / "packaging/fedora/create-source-archive.sh"),
                    str(second),
                ],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            )
            self.assertTrue(output.is_file())
            self.assertEqual(output.read_bytes(), second.read_bytes())
            self.assertEqual(output.stat().st_mode & 0o777, 0o644)
            with tarfile.open(output, "r:gz") as archive:
                names = archive.getnames()
            self.assertTrue(names)
            self.assertTrue(
                all(
                    name == "kfaceauth-4.0.0"
                    or name.startswith("kfaceauth-4.0.0/")
                    for name in names
                )
            )
            self.assertFalse(
                any("/redhat-linux-build/" in f"/{name}/" for name in names)
            )
            legacy_engine = "ir" + "lume"
            self.assertFalse(any(legacy_engine in name.lower() for name in names))

    def test_fedora_44_ci_covers_all_required_checks(self) -> None:
        workflows = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted((ROOT / ".github/workflows").glob("*.yml"))
        )

        for required in (
            "fedora:44",
            "clang-format --dry-run --Werror",
            "qmllint",
            "ctest",
            "unittest discover",
            "cargo fmt",
            "cargo clippy",
            "cargo test",
            "--offline",
            "verify_models.py",
            "rpmbuild -ba",
            "rpm-smoke-test.sh",
            "kfaceauth-[0-9]*.rpm",
            "kfaceauth-fedora-44",
        ):
            with self.subTest(required=required):
                self.assertIn(required, workflows)

        action_refs = re.findall(r"uses:\s+\S+@([0-9a-f]+)", workflows)
        self.assertTrue(action_refs)
        self.assertTrue(all(len(ref) == 40 for ref in action_refs))


if __name__ == "__main__":
    unittest.main()
