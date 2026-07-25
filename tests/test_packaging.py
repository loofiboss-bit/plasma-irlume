from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class PackagingContractTests(unittest.TestCase):
    def test_release_version_is_consistent(self) -> None:
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        metadata = (ROOT / "src/kcm/kcm_irlume.json").read_text(encoding="utf-8")
        spec = (ROOT / "packaging/fedora/plasma-irlume.spec").read_text(
            encoding="utf-8"
        )

        self.assertRegex(cmake, r"project\(plasma-irlume VERSION 1\.0\.0 ")
        self.assertIn('"Version": "1.0.0"', metadata)
        self.assertRegex(spec, r"(?m)^Version:\s+1\.0\.0$")

    def test_spec_keeps_engine_separate_and_version_gated(self) -> None:
        spec = (ROOT / "packaging/fedora/plasma-irlume.spec").read_text(
            encoding="utf-8"
        )

        self.assertRegex(spec, r"(?m)^Requires:\s+irlume >= 0\.6\.0$")
        self.assertRegex(spec, r"(?m)^Requires:\s+irlume < 0\.7\.0$")
        self.assertNotRegex(
            spec, r"(?m)^%(?:pre|post|preun|postun|trigger)(?:\s|$)"
        )

    def test_fedora_44_ci_covers_required_phase_six_checks(self) -> None:
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
            "rpmbuild -ba",
            "rpm-smoke-test.sh",
        ):
            with self.subTest(required=required):
                self.assertIn(required, workflows)

        action_refs = re.findall(r"uses:\s+\S+@([0-9a-f]+)", workflows)
        self.assertTrue(action_refs)
        self.assertTrue(all(len(ref) == 40 for ref in action_refs))


if __name__ == "__main__":
    unittest.main()
