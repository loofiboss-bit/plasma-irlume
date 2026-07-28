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
        --volume "${rpm_path}:/tmp/kfaceauth.rpm:ro" \
        --volume "${script_path}:/tmp/rpm-smoke-test.sh:ro" \
        fedora:44 \
        bash /tmp/rpm-smoke-test.sh /tmp/kfaceauth.rpm
fi

package_name="$(rpm -qp --queryformat '%{NAME}' "${rpm_path}")"
if [[ "${package_name}" != "kfaceauth" ]]; then
    echo "Expected kfaceauth RPM, got ${package_name}" >&2
    exit 1
fi

if rpm -qp --scripts "${rpm_path}" | grep -Eiq 'pam|authselect|biometric'; then
    echo "Package scriptlets must not mutate authentication state" >&2
    exit 1
fi

smoke_root="$(mktemp -d)"
trap 'rm -rf -- "${smoke_root}"' EXIT

mkdir -p \
    "${smoke_root}/usr/lib/sysimage/rpm" \
    "${smoke_root}/home/test/.config" \
    "${smoke_root}/etc/pam.d"
printf 'user-setting\n' >"${smoke_root}/home/test/.config/kfaceauth-preserve-me"
printf 'auth required pam_unix.so\n' >"${smoke_root}/etc/pam.d/kfaceauth-sentinel"

pam_hash_before="$(sha256sum "${smoke_root}/etc/pam.d/kfaceauth-sentinel")"
user_hash_before="$(sha256sum "${smoke_root}/home/test/.config/kfaceauth-preserve-me")"

rpm --root "${smoke_root}" --initdb
rpm --root "${smoke_root}" --nodeps --install "${rpm_path}"
rpm --root "${smoke_root}" --nodeps --upgrade --replacepkgs "${rpm_path}"

required_paths=(
    "/usr/libexec/kfaceauth-camera-preview-worker"
    "/usr/share/applications/kcm_kfaceauth.desktop"
)

for path in "${required_paths[@]}"; do
    if [[ ! -f "${smoke_root}${path}" ]]; then
        echo "Installed payload is missing ${path}" >&2
        exit 1
    fi
done

if find "${smoke_root}/usr/libexec/kfaceauth-camera-preview-worker" -perm /6000 -print -quit | grep -q .; then
    echo "Camera preview worker must not be setuid or setgid" >&2
    exit 1
fi
if command -v getcap >/dev/null 2>&1 &&
    getcap "${smoke_root}/usr/libexec/kfaceauth-camera-preview-worker" | grep -q .; then
    echo "Camera preview worker must not have file capabilities" >&2
    exit 1
fi

payload="$(rpm -qlp "${rpm_path}")"
for pattern in \
    '/kauth/' \
    '/dbus-1/system-services/' \
    '/dbus-1/system.d/' \
    '/polkit-1/actions/' \
    'kfaceauth-auth-helper' \
    'pam_kfaceauth'; do
    if grep -Fq "${pattern}" <<<"${payload}"; then
        echo "Installed payload contains forbidden privileged path: ${pattern}" >&2
        exit 1
    fi
done

plugin_path="$(
    rpm --root "${smoke_root}" -ql kfaceauth \
        | grep -E '/plasma/kcms/systemsettings/kcm_kfaceauth\.so$'
)"
if [[ -z "${plugin_path}" || ! -f "${smoke_root}${plugin_path}" ]]; then
    echo "Installed payload is missing the KCM plugin" >&2
    exit 1
fi

if [[ "$(sha256sum "${smoke_root}/etc/pam.d/kfaceauth-sentinel")" != "${pam_hash_before}" ]]; then
    echo "PAM sentinel changed during installation" >&2
    exit 1
fi
[[ "$(sha256sum "${smoke_root}/home/test/.config/kfaceauth-preserve-me")" == "${user_hash_before}" ]]

mapfile -t packaged_paths < <(
    rpm --root "${smoke_root}" -ql kfaceauth \
        | while IFS= read -r path; do
              [[ -f "${smoke_root}${path}" || -L "${smoke_root}${path}" ]] && printf '%s\n' "${path}"
          done
)

rpm --root "${smoke_root}" --erase kfaceauth

if [[ "$(sha256sum "${smoke_root}/etc/pam.d/kfaceauth-sentinel")" != "${pam_hash_before}" ]]; then
    echo "PAM sentinel changed during removal" >&2
    exit 1
fi
for path in "${packaged_paths[@]}"; do
    if [[ -e "${smoke_root}${path}" || -L "${smoke_root}${path}" ]]; then
        echo "Package-owned file remains after uninstall: ${path}" >&2
        exit 1
    fi
done
[[ "$(sha256sum "${smoke_root}/home/test/.config/kfaceauth-preserve-me")" == "${user_hash_before}" ]]

echo "RPM install/upgrade/remove lifecycle smoke test passed"
