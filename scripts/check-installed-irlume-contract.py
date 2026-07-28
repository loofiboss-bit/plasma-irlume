#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Offline qualification check for the installed irlume Contract 1 reader."""

from __future__ import annotations

import json
import os
from pathlib import Path
import selectors
import subprocess
import sys
import time

try:
    from jsonschema import Draft202012Validator
except ImportError:
    print("error: python3-jsonschema is required", file=sys.stderr)
    raise SystemExit(2)


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "tests/schemas/irlume-0.7.0/machine-api-v1.schema.json"
IRLUME = Path("/usr/bin/irlume")
MAX_BYTES = 256 * 1024
TIMEOUT_SECONDS = 3.0
COMMANDS = (
    ("version", ("version", "--json")),
    ("status", ("status", "--json", "--contract", "1")),
    ("doctor", ("doctor", "--json", "--contract", "1")),
    ("profiles.list", ("profiles", "list", "--json", "--contract", "1")),
    ("login.status", ("login", "status", "--json", "--contract", "1")),
)


def run_bounded(arguments: tuple[str, ...]) -> tuple[int, bytes, bytes]:
    environment = {
        "HOME": os.environ.get("HOME", "/"),
        "LANG": "C",
        "LC_ALL": "C",
        "LOGNAME": os.environ.get("LOGNAME", ""),
        "USER": os.environ.get("USER", ""),
        "XDG_RUNTIME_DIR": os.environ.get("XDG_RUNTIME_DIR", ""),
    }
    process = subprocess.Popen(
        (str(IRLUME), *arguments),
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=environment,
        shell=False,
    )
    assert process.stdout is not None and process.stderr is not None
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ, "stdout")
    selector.register(process.stderr, selectors.EVENT_READ, "stderr")
    output = {"stdout": bytearray(), "stderr": bytearray()}
    deadline = time.monotonic() + TIMEOUT_SECONDS
    try:
        while selector.get_map():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError
            for key, _ in selector.select(remaining):
                chunk = os.read(key.fileobj.fileno(), 65536)
                if not chunk:
                    selector.unregister(key.fileobj)
                    continue
                channel = output[key.data]
                if len(channel) + len(chunk) > MAX_BYTES:
                    raise BufferError(key.data)
                channel.extend(chunk)
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError
        return process.wait(timeout=remaining), bytes(output["stdout"]), bytes(output["stderr"])
    except (BufferError, TimeoutError, subprocess.TimeoutExpired) as error:
        process.kill()
        process.wait()
        detail = "timed out" if not isinstance(error, BufferError) else f"{error.args[0]} exceeded 256 KiB"
        raise RuntimeError(detail) from error
    finally:
        selector.close()


def main() -> int:
    if not IRLUME.is_file() or not os.access(IRLUME, os.X_OK):
        print("error: /usr/bin/irlume is missing or not executable", file=sys.stderr)
        return 2
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    Draft202012Validator.check_schema(schema)
    validator = Draft202012Validator(schema)

    for expected_command, arguments in COMMANDS:
        try:
            exit_code, stdout, _stderr = run_bounded(arguments)
            document = json.loads(stdout)
            validator.validate(document)
        except (json.JSONDecodeError, RuntimeError) as error:
            print(f"error: {expected_command}: {error}", file=sys.stderr)
            return 1
        except Exception as error:  # jsonschema reports a useful stable message here.
            print(f"error: {expected_command}: schema validation failed: {error}", file=sys.stderr)
            return 1
        if document.get("command") != expected_command:
            print(f"error: {expected_command}: response command mismatch", file=sys.stderr)
            return 1
        expected_exit = 0 if document.get("ok") is True else 1
        if exit_code != expected_exit:
            print(f"error: {expected_command}: exit status and response disagree", file=sys.stderr)
            return 1

    print("Installed irlume Contract 1 read commands passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
