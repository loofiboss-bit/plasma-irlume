Name:           kfaceauth
Version:        4.0.0
Release:        1%{?dist}
Summary:        KDE development preview for native face authentication

License:        GPL-3.0-or-later
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.22
BuildRequires:  cmake-rpm-macros
BuildRequires:  clang-tools-extra
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
BuildRequires:  systemd-devel

Requires:       kf6-kcmutils >= 6.10.0
Requires:       kf6-kirigami >= 6.10.0
Requires:       plasma-systemsettings
Requires:       qt6-qtdeclarative >= 6.8.0
Requires:       qt6-qtmultimedia >= 6.8.0

%description
KFaceAuth Milestone 1 is a Plasma 6 System Settings development preview. It
provides bounded local system probes and an explicit, in-memory RGB or infrared
camera preview through a separate unprivileged worker.

The package contains no face-recognition model, biometric persistence, PAM
module, privileged helper, authentication decision path, or background service.
The native Rust engine skeleton remains source-only in this milestone.

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
%{_qt6_bindir}/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
find src tests/unit -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
    | xargs -0 clang-format --dry-run --Werror
desktop-file-validate %{buildroot}%{_datadir}/applications/kcm_kfaceauth.desktop

%files -f kcm_kfaceauth.lang
%license LICENSE
%doc CHANGELOG.md README.md
%doc docs/*.md
%{_qt6_plugindir}/plasma/kcms/systemsettings/kcm_kfaceauth.so
%{_libexecdir}/kfaceauth-camera-preview-worker
%{_datadir}/applications/kcm_kfaceauth.desktop

%changelog
* Tue Jul 28 2026 Loofi <noreply@example.invalid> - 4.0.0-1
- Start the standalone native architecture with fail-closed engine status
- Preserve the bounded unprivileged camera preview
