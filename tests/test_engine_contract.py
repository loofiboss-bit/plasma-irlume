# SPDX-License-Identifier: GPL-3.0-or-later

from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path
import unittest

from contract_support import (
    CompatibilityPolicy,
    STRUCTURED_POLICY,
    is_compatible,
    load_json,
    load_ndjson,
    parse_semver,
    public_envelope_errors,
    walk_json,
)


TESTS_DIR = Path(__file__).resolve().parent
FIXTURES = TESTS_DIR / "fixtures" / "irlume"
OBSERVED = FIXTURES / "observed-private"
PROPOSED = FIXTURES / "proposed-v1"


class FixtureSyntaxTests(unittest.TestCase):
    def test_every_json_fixture_parses(self):
        paths = sorted(FIXTURES.rglob("*.json"))
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(path=path.relative_to(FIXTURES)):
                self.assertIsNotNone(load_json(path))

    def test_every_ndjson_record_parses_as_an_object(self):
        paths = sorted(FIXTURES.rglob("*.ndjson"))
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(path=path.relative_to(FIXTURES)):
                events = load_ndjson(path)
                self.assertTrue(events)
                self.assertTrue(all(isinstance(event, dict) for event in events))


class PublicEnvelopeTests(unittest.TestCase):
    def test_all_proposed_json_fixtures_use_the_common_envelope(self):
        for path in sorted(PROPOSED.glob("*.json")):
            with self.subTest(path=path.name):
                self.assertEqual(public_envelope_errors(load_json(path)), [])

    def test_expected_read_operations_have_fixtures(self):
        expected = {
            "version",
            "status",
            "doctor",
            "profiles.list",
            "login.status",
            "cameras.list",
            "cameras.emitter-test",
        }
        observed = {
            load_json(path)["command"]
            for path in PROPOSED.glob("*.json")
            if load_json(path)["command"] in expected
        }
        self.assertEqual(observed, expected)

    def test_malformed_json_is_rejected_by_the_parser(self):
        with self.assertRaises(json.JSONDecodeError):
            json.loads('{"contract_version": 1,')

    def test_missing_and_unknown_contract_versions_are_rejected(self):
        version = load_json(PROPOSED / "version.json")

        missing = deepcopy(version)
        del missing["contract_version"]
        self.assertIn("contract_version must be 1", public_envelope_errors(missing))

        unknown = deepcopy(version)
        unknown["contract_version"] = 2
        self.assertIn("contract_version must be 1", public_envelope_errors(unknown))

    def test_malformed_semver_is_rejected(self):
        version = load_json(PROPOSED / "version.json")
        for invalid in ("0.6", "v0.6.2", "0.06.2", "", None, 602):
            with self.subTest(value=invalid):
                candidate = deepcopy(version)
                candidate["engine_version"] = invalid
                self.assertIn(
                    "engine_version must be valid SemVer",
                    public_envelope_errors(candidate),
                )


class CompatibilityTests(unittest.TestCase):
    def test_no_released_structured_policy_exists_yet(self):
        self.assertIsNone(STRUCTURED_POLICY)
        self.assertFalse(is_compatible(load_json(PROPOSED / "version.json")))

    def test_released_versions_do_not_satisfy_hypothetical_json_range(self):
        hypothetical = CompatibilityPolicy(
            contract_version=1,
            minimum_engine=(0, 6, 2),
            maximum_engine_exclusive=(0, 7, 0),
        )
        version = load_json(PROPOSED / "version.json")
        for unsupported in ("0.6.0", "0.6.1"):
            with self.subTest(engine=unsupported):
                candidate = deepcopy(version)
                candidate["engine_version"] = unsupported
                self.assertFalse(is_compatible(candidate, hypothetical))

    def test_future_support_requires_both_contract_and_engine_range(self):
        hypothetical = CompatibilityPolicy(
            contract_version=1,
            minimum_engine=(0, 6, 2),
            maximum_engine_exclusive=(0, 7, 0),
        )
        version = load_json(PROPOSED / "version.json")
        self.assertTrue(is_compatible(version, hypothetical))

        wrong_contract = deepcopy(version)
        wrong_contract["contract_version"] = 2
        self.assertFalse(is_compatible(wrong_contract, hypothetical))

        too_new = deepcopy(version)
        too_new["engine_version"] = "0.7.0"
        self.assertFalse(is_compatible(too_new, hypothetical))

    def test_semver_parser_accepts_well_formed_release_metadata(self):
        self.assertEqual(parse_semver("0.6.2"), (0, 6, 2))
        self.assertEqual(parse_semver("0.6.2-rc.1+build.4"), (0, 6, 2))

    def test_private_protocol_payloads_are_never_public_envelopes(self):
        paths = sorted(OBSERVED.rglob("*.json"))
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(path=path.relative_to(OBSERVED)):
                payload = load_json(path)
                self.assertNotEqual(public_envelope_errors(payload), [])
                self.assertFalse(is_compatible(payload))

    def test_observed_patch_release_shape_change_is_recorded(self):
        v060_health = load_json(OBSERVED / "v0.6.0" / "health-response.json")
        v061_health = load_json(OBSERVED / "v0.6.1" / "health-response.json")
        self.assertNotIn("third_party_pad", v060_health["Health"])
        self.assertIn("third_party_pad", v061_health["Health"])

        v060_enrollment = load_json(
            OBSERVED / "v0.6.0" / "enrollment-response.json"
        )
        v061_enrollment = load_json(
            OBSERVED / "v0.6.1" / "enrollment-response.json"
        )
        self.assertNotIn("ir_depth_floored", v060_enrollment["Enrollment"])
        self.assertIn("ir_depth_floored", v061_enrollment["Enrollment"])


class EventContractTests(unittest.TestCase):
    def test_event_streams_are_ordered_and_have_one_terminal_event(self):
        for path in sorted(PROPOSED.glob("*.ndjson")):
            with self.subTest(path=path.name):
                events = load_ndjson(path)
                self.assertEqual(
                    [event["sequence"] for event in events],
                    list(range(len(events))),
                )
                operation_ids = {event["operation_id"] for event in events}
                self.assertEqual(len(operation_ids), 1)
                commands = {event["command"] for event in events}
                self.assertEqual(len(commands), 1)
                for event in events:
                    self.assertEqual(event["contract_version"], 1)
                    self.assertIsNotNone(parse_semver(event["engine_version"]))
                    self.assertIsInstance(event["terminal"], bool)

                terminal = [event for event in events if event["terminal"]]
                self.assertEqual(len(terminal), 1)
                self.assertIs(terminal[0], events[-1])
                self.assertIn(
                    terminal[0]["event"],
                    {"completed", "cancelled", "failed"},
                )

    def test_auth_test_never_releases_credentials_or_modifies_profiles(self):
        terminal = load_ndjson(PROPOSED / "auth-test.ndjson")[-1]
        self.assertEqual(terminal["event"], "completed")
        self.assertFalse(terminal["data"]["credential_released"])
        self.assertFalse(terminal["data"]["profile_modified"])

    def test_cancellation_is_a_typed_terminal_result(self):
        terminal = load_ndjson(PROPOSED / "enroll-cancelled.ndjson")[-1]
        self.assertEqual(terminal["event"], "cancelled")
        self.assertEqual(terminal["error"]["code"], "user-cancelled")

    def test_enrollment_merge_exposes_exact_cleanup_lineage(self):
        terminal = load_ndjson(PROPOSED / "enroll-merged.ndjson")[-1]
        self.assertFalse(terminal["data"]["created"])
        self.assertEqual(
            len(terminal["data"]["added_scan_ids"]),
            terminal["data"]["added_scans"],
        )
        self.assertEqual(len(set(terminal["data"]["added_scan_ids"])), 2)


class TransactionContractTests(unittest.TestCase):
    def setUp(self):
        self.plan = load_json(PROPOSED / "login-plan.json")
        self.apply = load_json(PROPOSED / "login-apply.json")
        self.verify = load_json(PROPOSED / "login-verify.json")
        self.rollback = load_json(PROPOSED / "login-rollback.json")

    def test_dry_run_is_explicitly_non_mutating(self):
        data = self.plan["data"]
        self.assertFalse(data["apply"])
        self.assertFalse(data["mutated"])
        self.assertTrue(data["plan_id"])
        self.assertTrue(data["password_fallback"]["preserved"])

    def test_plan_apply_verify_and_rollback_have_stable_lineage(self):
        plan_id = self.plan["data"]["plan_id"]
        self.assertEqual(self.apply["data"]["plan_id"], plan_id)

        transaction_id = self.apply["data"]["transaction_id"]
        self.assertEqual(self.verify["data"]["transaction_id"], transaction_id)
        self.assertEqual(
            self.rollback["data"]["transaction_id"],
            transaction_id,
        )
        self.assertEqual(self.apply["data"]["state"], "applied")
        self.assertEqual(self.verify["data"]["state"], "verified")
        self.assertEqual(self.rollback["data"]["state"], "rolled-back")
        self.assertTrue(self.rollback["data"]["restored"])

    def test_failed_post_apply_verification_has_typed_rollback(self):
        failed = load_json(PROPOSED / "login-apply-failed-rolled-back.json")
        self.assertFalse(failed["ok"])
        self.assertEqual(failed["data"]["state"], "verification-failed")
        self.assertFalse(failed["data"]["mutated"])
        self.assertEqual(failed["data"]["rollback"]["state"], "rolled-back")
        self.assertTrue(failed["data"]["rollback"]["restored"])
        self.assertEqual(
            failed["error"]["code"],
            "post-apply-verification-failed",
        )


class CameraContractTests(unittest.TestCase):
    def test_camera_operations_are_bounded_and_verifiable(self):
        listed = load_json(PROPOSED / "cameras-list.json")["data"]
        selected = load_json(PROPOSED / "cameras-select.json")["data"]
        probe = load_json(PROPOSED / "cameras-emitter-test.json")["data"]
        setup = load_json(PROPOSED / "cameras-emitter-setup.json")["data"]
        tune = load_json(PROPOSED / "cameras-tune.json")["data"]

        active = [pair for pair in listed["pairs"] if pair["active"]]
        self.assertEqual(len(active), 1)
        self.assertEqual(active[0]["pair_id"], selected["pair_id"])
        self.assertTrue(selected["selected"])
        self.assertTrue(selected["mutated"])
        self.assertTrue(probe["available"])
        self.assertFalse(probe["mutated"])
        self.assertGreaterEqual(probe["control_count"], 1)
        self.assertLessEqual(probe["control_count"], 256)
        self.assertTrue(setup["configured"])
        self.assertTrue(setup["mutated"])
        self.assertIn(tune["capture_mode"], {"concurrent", "sequential"})
        self.assertLessEqual(tune["retained_rgb"], 2.0)
        self.assertLessEqual(tune["retained_ir"], 2.0)
        self.assertLessEqual(tune["saved_ms"], 60_000.0)


class SanitizationTests(unittest.TestCase):
    FORBIDDEN_KEYS = {
        "password",
        "secret",
        "secret_bytes",
        "credential",
        "credentials",
        "embedding",
        "embeddings",
        "image",
        "images",
        "frame",
        "frames",
        "template_bytes",
        "username",
    }
    FORBIDDEN_VALUE_FRAGMENTS = {
        "/dev/video",
        "/home/",
        "loofi",
        "alice",
        "tester",
        "@example.",
    }

    def test_fixtures_contain_no_secret_or_biometric_payload_fields(self):
        for path in sorted(FIXTURES.rglob("*.json")):
            with self.subTest(path=path.relative_to(FIXTURES)):
                self._assert_sanitized(load_json(path))
        for path in sorted(FIXTURES.rglob("*.ndjson")):
            for index, event in enumerate(load_ndjson(path)):
                with self.subTest(path=path.relative_to(FIXTURES), event=index):
                    self._assert_sanitized(event)

    def _assert_sanitized(self, payload):
        for kind, value in walk_json(payload):
            if kind == "key":
                self.assertNotIn(str(value).casefold(), self.FORBIDDEN_KEYS)
            elif isinstance(value, str):
                lowered = value.casefold()
                for fragment in self.FORBIDDEN_VALUE_FRAGMENTS:
                    self.assertNotIn(fragment, lowered)


if __name__ == "__main__":
    unittest.main()
