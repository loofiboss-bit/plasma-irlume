from __future__ import annotations

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ReadOnlyHardeningTests(unittest.TestCase):
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
        self.assertFalse(
            (ROOT / "data/io.github.loofiboss_bit.plasma_irlume.actions").exists()
        )

        build_inputs = "\n".join(
            path.read_text(encoding="utf-8")
            for path in (
                ROOT / "CMakeLists.txt",
                ROOT / "src/CMakeLists.txt",
                ROOT / "src/backend/CMakeLists.txt",
                ROOT / "data/CMakeLists.txt",
                ROOT / "packaging/fedora/plasma-irlume.spec",
            )
        )
        for forbidden in ("KAuth", "AuthCore", "kauth_install", "kf6-kauth"):
            self.assertNotIn(forbidden, build_inputs)

    def test_production_factory_is_fixed(self) -> None:
        factory = (ROOT / "src/adapter/backendfactory.cpp").read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("/usr/bin/irlume")', factory)
        self.assertIn("std::make_unique<IrlumeBackend>", factory)


if __name__ == "__main__":
    unittest.main()
