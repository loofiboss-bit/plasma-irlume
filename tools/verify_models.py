#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

"""Verify the closed KFaceAuth model supply chain."""

from __future__ import annotations

import argparse
import hashlib
import os
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
import re
import sys

MANIFEST_NAME = "manifest.kfaceauth"
MAGIC = "KFACEAUTH_MODEL_MANIFEST\t1"
HEADER = "id\tpath\tsize\tsha256\trole\tbackend\tlicense\tprovenance"
MAX_MANIFEST_BYTES = 64 * 1024
MAX_ARTIFACT_BYTES = 64 * 1024 * 1024
TOKEN = re.compile(r"[A-Za-z0-9._+:-]{1,96}\Z")
DIGEST = re.compile(r"[0-9a-f]{64}\Z")

REQUIRED_ARTIFACTS = {
    "sface-2021dec": {
        "path": "files/face_recognition_sface_2021dec.onnx",
        "role": "embedding",
        "backend": "opencv-facerecognizersf",
        "license": "Apache-2.0",
        "provenance": "opencv-zoo-47534e27",
        "size": 38696353,
        "sha256": "0ba9fbfa01b5270c96627c4ef784da859931e02f04419c829e83484087c34e79",
    },
    "sface-apache-license": {
        "path": "licenses/sface-Apache-2.0.txt",
        "role": "license",
        "backend": "not-applicable",
        "license": "Apache-2.0",
        "provenance": "opencv-zoo-47534e27",
        "size": 11358,
        "sha256": "cfc7749b96f63bd31c3c42b5c471bf756814053e847c10f3eb003417bc523d30",
    },
    "sface-provenance": {
        "path": "provenance/sface-2021dec.txt",
        "role": "provenance",
        "backend": "not-applicable",
        "license": "GPL-3.0-or-later",
        "provenance": "repo-authored",
        "size": 1022,
        "sha256": "cf6afa6764396286257fa8cf69a4b49b0474ecec7841e0b6553e8cbe7211319d",
    },
    "yunet-2023mar": {
        "path": "files/face_detection_yunet_2023mar.onnx",
        "role": "detector",
        "backend": "opencv-facedetectoryn",
        "license": "MIT",
        "provenance": "opencv-zoo-47534e27",
        "size": 232589,
        "sha256": "8f2383e4dd3cfbb4553ea8718107fc0423210dc964f9f4280604804ed2552fa4",
    },
    "yunet-mit-license": {
        "path": "licenses/yunet-MIT.txt",
        "role": "license",
        "backend": "not-applicable",
        "license": "MIT",
        "provenance": "opencv-zoo-47534e27",
        "size": 1062,
        "sha256": "b3354fa804c2326e94cb8e3994ebd02a278ece481d1fae48549103c733843613",
    },
    "yunet-provenance": {
        "path": "provenance/yunet-2023mar.txt",
        "role": "provenance",
        "backend": "not-applicable",
        "license": "GPL-3.0-or-later",
        "provenance": "repo-authored",
        "size": 713,
        "sha256": "d54498523239a2d16810ffb1c87b0ad3d3a2697a9a8ce565a267ca3d9012db60",
    },
}


class VerificationError(ValueError):
    """A stable, user-safe model verification failure."""


@dataclass(frozen=True)
class Entry:
    artifact_id: str
    path: str
    size: int
    sha256: str
    role: str
    backend: str
    license: str
    provenance: str


def _safe_path(value: str) -> bool:
    path = PurePosixPath(value)
    return (
        bool(value)
        and "\\" not in value
        and not path.is_absolute()
        and all(component not in {"", ".", ".."} for component in path.parts)
        and str(path) == value
    )


def parse_manifest(data: bytes) -> list[Entry]:
    if not data or len(data) > MAX_MANIFEST_BYTES:
        raise VerificationError("manifest-size")
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError as error:
        raise VerificationError("manifest-encoding") from error
    if "\r" in text or not text.endswith("\n"):
        raise VerificationError("manifest-newline")
    lines = text.splitlines()
    if len(lines) < 3 or lines[0] != MAGIC or lines[1] != HEADER:
        raise VerificationError("manifest-schema")

    entries: list[Entry] = []
    ids: set[str] = set()
    paths: set[str] = set()
    previous_path: str | None = None
    for line in lines[2:]:
        fields = line.split("\t")
        if len(fields) != 8:
            raise VerificationError("manifest-columns")
        artifact_id, path, size_text, digest, role, backend, license_id, provenance = fields
        if not all(
            TOKEN.fullmatch(value)
            for value in (artifact_id, role, backend, license_id, provenance)
        ):
            raise VerificationError("manifest-token")
        if not _safe_path(path):
            raise VerificationError("manifest-path")
        if not DIGEST.fullmatch(digest):
            raise VerificationError("manifest-digest")
        if not size_text.isascii() or not size_text.isdecimal():
            raise VerificationError("manifest-size-field")
        size = int(size_text)
        if not 0 < size <= MAX_ARTIFACT_BYTES:
            raise VerificationError("manifest-size-field")
        if artifact_id in ids or path in paths:
            raise VerificationError("manifest-duplicate")
        if previous_path is not None and path <= previous_path:
            raise VerificationError("manifest-order")
        ids.add(artifact_id)
        paths.add(path)
        previous_path = path
        entries.append(
            Entry(
                artifact_id,
                path,
                size,
                digest,
                role,
                backend,
                license_id,
                provenance,
            )
        )
    if not entries:
        raise VerificationError("manifest-empty")
    return entries


def _reject_symlink_components(root: Path, relative: PurePosixPath) -> Path:
    current = root
    for component in relative.parts:
        current = current / component
        if current.is_symlink():
            raise VerificationError("unsafe-symlink")
    return current


def _regular_files(root: Path) -> set[str]:
    files: set[str] = set()
    for directory, directory_names, file_names in os.walk(root, followlinks=False):
        base = Path(directory)
        for name in directory_names:
            if (base / name).is_symlink():
                raise VerificationError("unsafe-symlink")
        for name in file_names:
            path = base / name
            if path.is_symlink() or not path.is_file():
                raise VerificationError("unsafe-filesystem-entry")
            files.add(path.relative_to(root).as_posix())
    return files


def verify(root: Path) -> list[Entry]:
    if root.is_symlink():
        raise VerificationError("model-root-symlink")
    root = root.resolve(strict=True)
    if not root.is_dir():
        raise VerificationError("model-root")
    manifest_path = root / MANIFEST_NAME
    if manifest_path.is_symlink() or not manifest_path.is_file():
        raise VerificationError("manifest-file")
    entries = parse_manifest(manifest_path.read_bytes())

    by_id = {entry.artifact_id: entry for entry in entries}
    if set(by_id) != set(REQUIRED_ARTIFACTS):
        raise VerificationError("required-artifacts")
    for artifact_id, expected in REQUIRED_ARTIFACTS.items():
        actual = by_id[artifact_id]
        for field, value in expected.items():
            if getattr(actual, field) != value:
                raise VerificationError("required-metadata")

    listed = {entry.path for entry in entries}
    present = _regular_files(root)
    present.discard(MANIFEST_NAME)
    if present != listed:
        raise VerificationError("file-inventory")

    for entry in entries:
        path = _reject_symlink_components(root, PurePosixPath(entry.path))
        stat = path.stat()
        if not path.is_file() or stat.st_size != entry.size:
            raise VerificationError("artifact-size")
        hasher = hashlib.sha256()
        with path.open("rb") as artifact:
            while chunk := artifact.read(64 * 1024):
                hasher.update(chunk)
        if hasher.hexdigest() != entry.sha256:
            raise VerificationError("artifact-digest")
    return entries


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "models",
        help="model directory containing manifest.kfaceauth",
    )
    arguments = parser.parse_args(argv)
    try:
        entries = verify(arguments.root)
    except (OSError, VerificationError) as error:
        print(f"model-verification=failed reason={error}", file=sys.stderr)
        return 1
    print(f"model-verification=ok artifacts={len(entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
