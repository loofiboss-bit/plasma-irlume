from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SecurityBoundaryTests(unittest.TestCase):
    def test_source_has_no_blocking_process_waits(self) -> None:
        source = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted((ROOT / "src").rglob("*"))
            if path.suffix in {".cpp", ".h"}
        )
        for forbidden in ("waitForStarted", "waitForReadyRead", "waitForFinished"):
            self.assertNotIn(forbidden, source)

    def test_privileged_surface_is_absent(self) -> None:
        self.assertFalse(any((ROOT / "src/helper").glob("*")))
        self.assertFalse(any((ROOT / "data").glob("*.actions")))

        build_inputs = "\n".join(
            path.read_text(encoding="utf-8")
            for path in (
                ROOT / "CMakeLists.txt",
                ROOT / "src/CMakeLists.txt",
                ROOT / "src/backend/CMakeLists.txt",
                ROOT / "data/CMakeLists.txt",
                ROOT / "packaging/fedora/kfaceauth.spec",
            )
        )
        for forbidden in ("KAuth", "AuthCore", "kauth_install", "kf6-kauth"):
            self.assertNotIn(forbidden, build_inputs)

    def test_native_backend_never_executes_an_engine_process(self) -> None:
        backend = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted((ROOT / "src" / "backend").glob("nativefaceauthbackend.*"))
        )
        self.assertNotIn("QProcess", backend)
        self.assertNotIn("/usr/bin/", backend)
        self.assertIn("unsupported-in-milestone-1", backend)
        self.assertIn("native-engine-unavailable", backend)

    def test_preview_worker_has_no_privileged_or_persistent_surface(self) -> None:
        preview = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted((ROOT / "src" / "preview").glob("*"))
            if path.suffix in {".cpp", ".h", ".txt"}
        )
        for forbidden in (
            "QSettings",
            "QSaveFile",
            "QNetworkAccessManager",
            "QAudio",
            "KAuth",
            "Polkit",
            "systemBus",
            "QSettings",
        ):
            self.assertNotIn(forbidden, preview)

        self.assertNotIn("deviceNode", (ROOT / "src/backend/camerapreviewsession.h").read_text(encoding="utf-8"))
        support = (ROOT / "src/backend/supportreport.cpp").read_text(encoding="utf-8")
        for forbidden in ("frame()", "device.token", "device.label", "selectedDeviceIndex"):
            self.assertNotIn(forbidden, support)

    def test_vision_bridge_has_no_persistent_or_privileged_surface(self) -> None:
        vision = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted(
                (ROOT / "src" / "backend").glob("visionanalysissession.*")
            )
        )
        for forbidden in (
            "QTemporaryFile",
            "QSaveFile",
            "QFile::write",
            "QSettings",
            "QNetworkAccessManager",
            "KAuth",
            "Polkit",
            "systemBus",
        ):
            self.assertNotIn(forbidden, vision)
        self.assertIn("QProcess::SeparateChannels", vision)
        self.assertIn("m_generation", vision)
        self.assertNotIn("qDebug", vision)
        self.assertNotIn("qInfo", vision)

    def test_milestone_ui_exposes_no_mutation_control(self) -> None:
        qml = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted((ROOT / "src" / "kcm" / "ui").rglob("*.qml"))
        )
        for forbidden in (
            ".enroll(",
            ".deleteProfile(",
            ".addAppearanceScan(",
            ".enableLoginScreen(",
            ".enableLockScreen(",
            ".disableNow(",
            ".selectPair(",
            ".setupEmitter(",
            ".tuneCamera(",
        ):
            self.assertNotIn(forbidden, qml)

    def test_no_legacy_identity_or_dependency_remains(self) -> None:
        legacy_engine = "ir" + "lume"
        forbidden_names = (
            legacy_engine,
            "plasma-" + legacy_engine,
            "kcm_" + legacy_engine,
        )
        checked_suffixes = {
            ".cpp",
            ".h",
            ".qml",
            ".txt",
            ".cmake",
            ".json",
            ".in",
            ".md",
            ".py",
            ".sh",
            ".spec",
            ".yml",
            ".toml",
            ".rs",
            ".po",
        }
        offenders: list[str] = []
        for path in ROOT.rglob("*"):
            if (
                not path.is_file()
                or ".git" in path.parts
                or ".mypy_cache" in path.parts
                or ".pytest_cache" in path.parts
                or any(part.startswith("build") for part in path.parts)
                or "target" in path.parts
                or path.suffix in {".gz", ".rpm"}
            ):
                continue
            relative = path.relative_to(ROOT)
            lowered_name = relative.as_posix().lower()
            if any(name in lowered_name for name in forbidden_names):
                offenders.append(str(relative))
                continue
            if path.suffix in checked_suffixes or path.name in {"CMakeLists.txt", "LICENSE"}:
                text = path.read_text(encoding="utf-8", errors="replace").lower()
                if any(name in text for name in forbidden_names):
                    offenders.append(str(relative))
        self.assertEqual(offenders, [])

    def test_rust_skeleton_has_no_network_or_persistence_surface(self) -> None:
        rust = "\n".join(
            path.read_text(encoding="utf-8").split("#[cfg(test)]", 1)[0]
            for path in sorted((ROOT / "engine").rglob("*.rs"))
        )
        for forbidden in (
            "std::net::",
            "tokio",
            "reqwest",
            "unsafe {",
            "create_dir",
            "write_all_at",
            "pam_start",
            "authenticate(",
            "enroll(",
        ):
            self.assertNotIn(forbidden, rust)
        self.assertIn("MAX_REQUEST_BYTES", rust)
        self.assertIn("MAX_RESPONSE_BYTES", rust)

    def test_vision_worker_has_no_network_auth_or_disk_write_surface(self) -> None:
        vision_rust = "\n".join(
            path.read_text(encoding="utf-8").split("#[cfg(test)]", 1)[0]
            for path in sorted((ROOT / "engine").rglob("*.rs"))
            if "vision" in path.parts
        )
        for forbidden in (
            "std::net::",
            "TcpStream",
            "UdpSocket",
            "reqwest",
            "tokio",
            "File::create",
            "OpenOptions",
            "fs::write",
            "pam_",
            "authselect",
            "systemd",
            "authenticate(",
            "FaceEmbedding",
            "PersistentEmbedding",
        ):
            self.assertNotIn(forbidden, vision_rust)
        self.assertIn("MAX_WIDTH", vision_rust)
        self.assertIn("MAX_HEIGHT", vision_rust)


if __name__ == "__main__":
    unittest.main()
