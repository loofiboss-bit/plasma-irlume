%global source_date_epoch_from_changelog 1
%global use_source_date_epoch_as_buildtime 1
%global clamp_mtime_to_source_date_epoch 1

Name:           kfaceauth
Version:        4.0.0
Release:        1%{?dist}
Summary:        KDE development preview for native face authentication

License:        GPL-3.0-or-later AND MIT
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.22
BuildRequires:  cmake-rpm-macros
BuildRequires:  cargo
BuildRequires:  clang-tools-extra
BuildRequires:  clippy
BuildRequires:  desktop-file-utils
BuildRequires:  extra-cmake-modules >= 6.10.0
BuildRequires:  gcc-c++
BuildRequires:  kf6-kcmutils-devel >= 6.10.0
BuildRequires:  kf6-kcoreaddons-devel >= 6.10.0
BuildRequires:  kf6-ki18n-devel >= 6.10.0
BuildRequires:  kf6-kirigami-devel >= 6.10.0
BuildRequires:  ninja-build
BuildRequires:  python3
BuildRequires:  qt6-qtbase-devel >= 6.8.0
BuildRequires:  qt6-qtdeclarative-devel >= 6.8.0
BuildRequires:  qt6-qtmultimedia-devel >= 6.8.0
BuildRequires:  rust
BuildRequires:  rustfmt
BuildRequires:  systemd-devel

Requires:       kf6-kcmutils >= 6.10.0
Requires:       kf6-kirigami >= 6.10.0
Requires:       plasma-systemsettings
Requires:       qt6-qtdeclarative >= 6.8.0
Requires:       qt6-qtmultimedia >= 6.8.0

%description
KFaceAuth Milestone 2 is a Plasma 6 System Settings development preview. It
provides bounded local probes, an explicit in-memory camera preview, and a
second explicit one-frame vision-analysis path through separate unprivileged
workers.

The package contains a hash-pinned MIT-licensed YuNet detector for future
evaluation, but real inference remains disabled behind an explicit deterministic
provider. It contains no biometric templates, matching, biometric
persistence, PAM
module, privileged helper, authentication decision path, or background service.

%prep
%autosetup -p1

%build
%cmake \
    -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install
if find %{buildroot}%{_datadir}/locale -type f -name 'kcm_kfaceauth.mo' -print -quit \
    | grep -q .; then
    %find_lang kcm_kfaceauth
else
    : >kcm_kfaceauth.lang
fi

%check
export QT_QPA_PLATFORM=offscreen
%ctest
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/verify_models.py --root models
%{_qt6_bindir}/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
find src tests/unit -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
    | xargs -0 clang-format --dry-run --Werror
cargo fmt --manifest-path engine/Cargo.toml --all -- --check
cargo clippy --manifest-path engine/Cargo.toml \
    --workspace --all-targets --locked --offline -- -D warnings
cargo test --manifest-path engine/Cargo.toml \
    --workspace --all-targets --locked --offline
desktop-file-validate %{buildroot}%{_datadir}/applications/kcm_kfaceauth.desktop

%files -f kcm_kfaceauth.lang
%license LICENSE
%license models/licenses/yunet-MIT.txt
%doc CHANGELOG.md README.md
%doc docs/*.md
%{_qt6_plugindir}/plasma/kcms/systemsettings/kcm_kfaceauth.so
%{_libexecdir}/kfaceauth-camera-preview-worker
%{_libexecdir}/kfaceauth-vision-worker
%{_datadir}/kfaceauth/models/manifest.kfaceauth
%{_datadir}/kfaceauth/models/files/fake-provider-v1.cfg
%{_datadir}/kfaceauth/models/files/face_detection_yunet_2023mar.onnx
%{_datadir}/kfaceauth/models/licenses/yunet-MIT.txt
%{_datadir}/kfaceauth/models/provenance/yunet-2023mar.txt
%{_datadir}/applications/kcm_kfaceauth.desktop

%changelog
* Tue Jul 28 2026 Loofi <noreply@example.invalid> - 4.0.0-1
- Start the standalone native architecture with fail-closed engine status
- Preserve the bounded unprivileged camera preview
- Add bounded one-frame vision analysis and a verified model supply chain
- Package YuNet for evaluation while keeping real inference disabled
