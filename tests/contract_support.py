# SPDX-License-Identifier: GPL-3.0-or-later
"""Helpers for validating the proposed irlume integration contract.

This is Phase 0 test code, not the future production adapter.
"""

from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Any


SEMVER_RE = re.compile(
    r"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
    r"(?:-[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?"
    r"(?:\+[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?$"
)


def parse_semver(value: object) -> tuple[int, int, int] | None:
    """Return the SemVer core or None for a malformed value."""

    if not isinstance(value, str):
        return None
    match = SEMVER_RE.fullmatch(value)
    if match is None:
        return None
    return tuple(int(part) for part in match.groups())  # type: ignore[return-value]


@dataclass(frozen=True)
class CompatibilityPolicy:
    """A reviewed compatibility decision for the proposed JSON API."""

    contract_version: int
    minimum_engine: tuple[int, int, int]
    maximum_engine_exclusive: tuple[int, int, int]


# This concerns only the proposed JSON API. The production Phase 2 adapter has
# an independently tested irlume 0.6.x read-only CLI compatibility policy.
STRUCTURED_POLICY: CompatibilityPolicy | None = None


def public_envelope_errors(document: object) -> list[str]:
    """Validate common one-document JSON envelope requirements."""

    if not isinstance(document, dict):
        return ["document must be an object"]

    errors: list[str] = []
    if document.get("contract_version") != 1:
        errors.append("contract_version must be 1")
    if parse_semver(document.get("engine_version")) is None:
        errors.append("engine_version must be valid SemVer")
    if not isinstance(document.get("command"), str) or not document["command"]:
        errors.append("command must be a non-empty string")
    if not isinstance(document.get("ok"), bool):
        errors.append("ok must be a boolean")
    elif document["ok"] and not isinstance(document.get("data"), dict):
        errors.append("successful responses must contain an object data field")
    elif not document["ok"] and not isinstance(document.get("error"), dict):
        errors.append("failed responses must contain an object error field")
    return errors


def is_compatible(
    document: object,
    policy: CompatibilityPolicy | None = STRUCTURED_POLICY,
) -> bool:
    """Return true only for a valid envelope inside an explicit policy."""

    if policy is None or public_envelope_errors(document):
        return False
    assert isinstance(document, dict)
    if document["contract_version"] != policy.contract_version:
        return False
    engine = parse_semver(document["engine_version"])
    assert engine is not None
    return policy.minimum_engine <= engine < policy.maximum_engine_exclusive


def load_json(path: Path) -> Any:
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def load_ndjson(path: Path) -> list[dict[str, Any]]:
    events: list[dict[str, Any]] = []
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, start=1):
            if not line.strip():
                continue
            value = json.loads(line)
            if not isinstance(value, dict):
                raise ValueError(f"{path}:{line_number}: event must be an object")
            events.append(value)
    return events


def walk_json(value: Any):
    """Yield every key and scalar value in a JSON-compatible structure."""

    if isinstance(value, dict):
        for key, item in value.items():
            yield ("key", key)
            yield from walk_json(item)
    elif isinstance(value, list):
        for item in value:
            yield from walk_json(item)
    else:
        yield ("value", value)
