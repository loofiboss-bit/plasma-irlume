#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 PATH_TO_BINARY_RPM" >&2
    exit 2
fi

rpm_path="$(realpath "$1")"
if [[ ! -f "${rpm_path}" ]]; then
    echo "RPM not found: ${rpm_path}" >&2
    exit 2
fi

if [[ "${EUID}" -ne 0 ]]; then
    if ! command -v podman >/dev/null 2>&1; then
        echo "Run as root or install Podman for the isolated RPM transaction" >&2
        exit 2
    fi

    script_path="$(realpath "${BASH_SOURCE[0]}")"
    exec podman run --rm \
        --security-opt label=disable \
        --volume "${rpm_path}:/tmp/plasma-irlume.rpm:ro" \
        --volume "${script_path}:/tmp/rpm-smoke-test.sh:ro" \
        fedora:44 \
        bash /tmp/rpm-smoke-test.sh /tmp/plasma-irlume.rpm
fi

package_name="$(rpm -qp --queryformat '%{NAME}' "${rpm_path}")"
if [[ "${package_name}" != "plasma-irlume" ]]; then
    echo "Expected plasma-irlume RPM, got ${package_name}" >&2
    exit 1
fi

if rpm -qp --scripts "${rpm_path}" | grep -Eiq 'irlume|pam|authselect'; then
    echo "Package scriptlets must not mutate engine or authentication state" >&2
    exit 1
fi

smoke_root="$(mktemp -d)"
trap 'rm -rf -- "${smoke_root}"' EXIT

mkdir -p \
    "${smoke_root}/usr/lib/sysimage/rpm" \
    "${smoke_root}/var/lib/irlume/profiles" \
    "${smoke_root}/home/test/.config"
printf 'profile-state\n' >"${smoke_root}/var/lib/irlume/profiles/preserve-me"
printf 'user-setting\n' >"${smoke_root}/home/test/.config/plasma-irlume-preserve-me"

rpm --root "${smoke_root}" --initdb
rpm --root "${smoke_root}" --nodeps --install "${rpm_path}"

required_paths=(
    "/usr/libexec/kf6/kauth/plasma-irlume-auth-helper"
    "/usr/share/applications/kcm_irlume.desktop"
    "/usr/share/dbus-1/system-services/io.github.loofibossbit.plasmairlume.helper.service"
    "/usr/share/dbus-1/system.d/io.github.loofibossbit.plasmairlume.helper.conf"
    "/usr/share/polkit-1/actions/io.github.loofibossbit.plasmairlume.helper.policy"
)

for path in "${required_paths[@]}"; do
    if [[ ! -f "${smoke_root}${path}" ]]; then
        echo "Installed payload is missing ${path}" >&2
        exit 1
    fi
done

plugin_path="$(
    rpm --root "${smoke_root}" -ql plasma-irlume \
        | grep -E '/plasma/kcms/systemsettings/kcm_irlume\.so$'
)"
if [[ -z "${plugin_path}" || ! -f "${smoke_root}${plugin_path}" ]]; then
    echo "Installed payload is missing the KCM plugin" >&2
    exit 1
fi

mapfile -t packaged_paths < <(
    rpm --root "${smoke_root}" -ql plasma-irlume \
        | while IFS= read -r path; do
              [[ -f "${smoke_root}${path}" || -L "${smoke_root}${path}" ]] && printf '%s\n' "${path}"
          done
)

rpm --root "${smoke_root}" --erase plasma-irlume

for path in "${packaged_paths[@]}"; do
    if [[ -e "${smoke_root}${path}" || -L "${smoke_root}${path}" ]]; then
        echo "Package-owned file remains after uninstall: ${path}" >&2
        exit 1
    fi
done

test -f "${smoke_root}/var/lib/irlume/profiles/preserve-me"
test -f "${smoke_root}/home/test/.config/plasma-irlume-preserve-me"

echo "RPM install/uninstall lifecycle smoke test passed"
