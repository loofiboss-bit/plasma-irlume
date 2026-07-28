from __future__ import annotations

import json
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "tests/fixtures/irlume"
CONTRACT = FIXTURES / "contract-v1"


def load(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


class ContractOneFixtureTests(unittest.TestCase):
    def test_released_documents_have_valid_common_envelopes(self) -> None:
        expected = {
            "version.json": "version",
            "status.json": "status",
            "doctor.json": "doctor",
            "profiles-list.json": "profiles.list",
            "login-status.json": "login.status",
        }
        for name, command in expected.items():
            with self.subTest(name=name):
                document = load(CONTRACT / name)
                self.assertIsInstance(document, dict)
                assert isinstance(document, dict)
                self.assertEqual(document["contract_version"], 1)
                self.assertEqual(document["command"], command)
                self.assertIs(document["ok"], True)
                self.assertIsInstance(document["engine_version"], str)
                self.assertIsInstance(document["data"], dict)

    def test_handshake_advertises_contract_one_and_read_capabilities(self) -> None:
        version = load(CONTRACT / "version.json")
        assert isinstance(version, dict)
        data = version["data"]
        assert isinstance(data, dict)
        versions = data["contract_versions"]
        self.assertLessEqual(versions["min"], 1)
        self.assertGreaterEqual(versions["max"], 1)
        self.assertEqual(
            set(data["capabilities"]),
            {
                "version-json",
                "status-json",
                "doctor-json",
                "profiles-list-json",
                "login-status-json",
            },
        )

    def test_structured_error_has_no_success_data(self) -> None:
        document = load(CONTRACT / "error.json")
        assert isinstance(document, dict)
        self.assertIs(document["ok"], False)
        self.assertNotIn("data", document)
        self.assertEqual(document["error"]["code"], "daemon-unavailable")
        self.assertIs(document["error"]["retryable"], True)

    def test_released_fixtures_are_sanitized(self) -> None:
        forbidden = {
            "credential",
            "credentials",
            "device_path",
            "embedding",
            "embeddings",
            "frame",
            "frames",
            "image",
            "images",
            "password",
            "path",
            "user",
            "username",
        }

        def keys(value: object):
            if isinstance(value, dict):
                for key, item in value.items():
                    yield key.lower()
                    yield from keys(item)
            elif isinstance(value, list):
                for item in value:
                    yield from keys(item)

        for path in sorted(CONTRACT.glob("*.json")):
            with self.subTest(path=path.name):
                self.assertTrue(forbidden.isdisjoint(keys(load(path))))

    def test_historical_mutation_fixtures_are_not_runtime_inputs(self) -> None:
        cmake_inputs = "\n".join(
            path.read_text(encoding="utf-8")
            for path in ROOT.rglob("CMakeLists.txt")
            if "build" not in path.parts
        )
        self.assertNotIn("proposed-v1", cmake_inputs)
        self.assertNotIn("fixtures/irlume/events", cmake_inputs)


if __name__ == "__main__":
    unittest.main()
