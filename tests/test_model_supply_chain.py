# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

import importlib.util
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

REPOSITORY = Path(__file__).resolve().parents[1]
VERIFIER_PATH = REPOSITORY / "tools" / "verify_models.py"
SPEC = importlib.util.spec_from_file_location("verify_models", VERIFIER_PATH)
assert SPEC is not None and SPEC.loader is not None
verify_models = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = verify_models
SPEC.loader.exec_module(verify_models)


class ModelSupplyChainTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / "models"
        shutil.copytree(REPOSITORY / "models", self.root)

    def assert_rejected(self) -> None:
        with self.assertRaises((OSError, verify_models.VerificationError)):
            verify_models.verify(self.root)

    def test_repository_supply_chain_is_complete(self) -> None:
        entries = verify_models.verify(REPOSITORY / "models")
        self.assertEqual(
            {entry.artifact_id for entry in entries},
            set(verify_models.REQUIRED_ARTIFACTS),
        )
        yunet = next(entry for entry in entries if entry.artifact_id == "yunet-2023mar")
        self.assertEqual(yunet.backend, "opencv-facedetectoryn")
        self.assertEqual(yunet.size, 232589)
        sface = next(entry for entry in entries if entry.artifact_id == "sface-2021dec")
        self.assertEqual(sface.backend, "opencv-facerecognizersf")
        self.assertEqual(sface.license, "Apache-2.0")
        self.assertEqual(sface.size, 38696353)
        self.assertEqual(
            sface.sha256,
            "0ba9fbfa01b5270c96627c4ef784da859931e02f04419c829e83484087c34e79",
        )

    def test_missing_file_is_rejected(self) -> None:
        (self.root / "files" / "face_detection_yunet_2023mar.onnx").unlink()
        self.assert_rejected()

    def test_renamed_file_is_rejected(self) -> None:
        source = self.root / "files" / "face_detection_yunet_2023mar.onnx"
        source.rename(source.with_name("renamed.onnx"))
        self.assert_rejected()

    def test_modified_file_is_rejected(self) -> None:
        path = self.root / "files" / "face_detection_yunet_2023mar.onnx"
        path.write_bytes(path.read_bytes() + b"modified")
        self.assert_rejected()

    def test_modified_sface_file_is_rejected(self) -> None:
        path = self.root / "files" / "face_recognition_sface_2021dec.onnx"
        with path.open("r+b") as artifact:
            artifact.seek(1024)
            original = artifact.read(1)
            artifact.seek(1024)
            artifact.write(bytes([original[0] ^ 1]))
        self.assert_rejected()

    def test_duplicate_sface_manifest_entry_is_rejected(self) -> None:
        manifest = self.root / verify_models.MANIFEST_NAME
        lines = manifest.read_text(encoding="utf-8").splitlines()
        sface = next(line for line in lines if line.startswith("sface-2021dec\t"))
        manifest.write_text("\n".join(lines + [sface]) + "\n", encoding="utf-8")
        self.assert_rejected()

    def test_unlisted_file_is_rejected(self) -> None:
        (self.root / "files" / "unlisted.bin").write_bytes(b"unlisted")
        self.assert_rejected()

    def test_manifest_rename_is_rejected(self) -> None:
        manifest = self.root / verify_models.MANIFEST_NAME
        manifest.rename(self.root / "manifest.old")
        self.assert_rejected()

    def test_manifest_metadata_cannot_substitute_runtime(self) -> None:
        manifest = self.root / verify_models.MANIFEST_NAME
        data = manifest.read_text(encoding="utf-8")
        manifest.write_text(
            data.replace("opencv-facedetectoryn", "other-runtime"),
            encoding="utf-8",
        )
        self.assert_rejected()


if __name__ == "__main__":
    unittest.main()
