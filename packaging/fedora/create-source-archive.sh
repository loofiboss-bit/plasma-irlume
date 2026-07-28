#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
version="$(
    sed -nE 's/^project\(plasma-irlume VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES CXX\)$/\1/p' \
        "${repo_root}/CMakeLists.txt"
)"

if [[ -z "${version}" ]]; then
    echo "Unable to read the project version from CMakeLists.txt" >&2
    exit 1
fi

output="${1:-${repo_root}/plasma-irlume-${version}.tar.gz}"
source_date_epoch="${SOURCE_DATE_EPOCH:-$(git -C "${repo_root}" log -1 --format=%ct 2>/dev/null || printf '0')}"
temporary_archive="$(mktemp --tmpdir plasma-irlume-archive.XXXXXX.tar.gz)"
trap 'rm -f -- "${temporary_archive}"' EXIT

tar \
    --sort=name \
    --mtime="@${source_date_epoch}" \
    --owner=0 \
    --group=0 \
    --numeric-owner \
    --mode='u+rwX,go+rX,go-w' \
    --transform="s,^\.,plasma-irlume-${version}," \
    --exclude='./.git' \
    --exclude='./.cache' \
    --exclude='./.directory' \
    --exclude='./.mypy_cache' \
    --exclude='./.pytest_cache' \
    --exclude='./.vscode' \
    --exclude='*/__pycache__' \
    --exclude='./build*' \
    --exclude='./rpmbuild' \
    --exclude='./*.rpm' \
    --exclude='./*.src.rpm' \
    --exclude='./plasma-irlume-*.tar.gz' \
    -C "${repo_root}" \
    -czf "${temporary_archive}" \
    .

mv -- "${temporary_archive}" "${output}"
chmod 0644 "${output}"
trap - EXIT

echo "${output}"
