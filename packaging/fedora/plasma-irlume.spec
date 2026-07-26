Name:           plasma-irlume
Version:        2.0.0
Release:        0.1.dev%{?dist}
Summary:        Experimental KDE integration for the irlume face-authentication engine

License:        GPL-3.0-or-later
URL:            https://github.com/loofiboss-bit/plasma-irlume
Source0:        %{url}/archive/refs/tags/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.22
BuildRequires:  cmake-rpm-macros
BuildRequires:  clang-tools-extra
BuildRequires:  desktop-file-utils
BuildRequires:  extra-cmake-modules >= 6.10.0
BuildRequires:  gcc-c++
BuildRequires:  kf6-kauth-devel >= 6.10.0
BuildRequires:  kf6-kcmutils-devel >= 6.10.0
BuildRequires:  kf6-kcoreaddons-devel >= 6.10.0
BuildRequires:  kf6-ki18n-devel >= 6.10.0
BuildRequires:  kf6-kirigami-devel >= 6.10.0
BuildRequires:  ninja-build
BuildRequires:  python3
BuildRequires:  qt6-qtbase-devel >= 6.8.0
BuildRequires:  qt6-qtdeclarative-devel >= 6.8.0

Requires:       irlume >= 0.6.0
Requires:       kf6-kcmutils >= 6.10.0
Requires:       kf6-kirigami >= 6.10.0
Requires:       plasma-systemsettings
Requires:       qt6-qtdeclarative >= 6.8.0

%description
plasma-irlume is an experimental Plasma 6 System Settings module for the
separately packaged irlume face-authentication engine. It presents hardware
and security readiness, profile workflows, transactional authentication
controls, recovery guidance, and redacted diagnostics.

The package does not contain biometric models, profiles, PAM modules, or the
authentication engine. Removing it does not remove enrolled profiles or change
active PAM state.

%prep
%autosetup -p1

%build
%cmake \
    -DBUILD_TESTING=ON
%cmake_build

%install
%cmake_install
if find %{buildroot}%{_datadir}/locale -type f -name 'kcm_irlume.mo' -print -quit \
    | grep -q .; then
    %find_lang kcm_irlume
else
    : >kcm_irlume.lang
fi

%check
export QT_QPA_PLATFORM=offscreen
%ctest
python3 -m unittest discover -s tests -p 'test_*.py'
%{_qt6_bindir}/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
find src tests/unit -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
    | xargs -0 clang-format --dry-run --Werror
desktop-file-validate data/kcm_irlume.desktop

%files -f kcm_irlume.lang
%license LICENSE
%doc CHANGELOG.md README.md
%doc docs/*.md
%{_qt6_plugindir}/plasma/kcms/systemsettings/kcm_irlume.so
%{_kf6_libexecdir}/kauth/plasma-irlume-auth-helper
%{_datadir}/applications/kcm_irlume.desktop
%{_datadir}/dbus-1/system-services/io.github.loofibossbit.plasmairlume.helper.service
%{_datadir}/dbus-1/system.d/io.github.loofibossbit.plasmairlume.helper.conf
%{_datadir}/polkit-1/actions/io.github.loofibossbit.plasmairlume.helper.policy

%changelog
* Sun Jul 26 2026 Loofi <noreply@example.invalid> - 2.0.0-0.1.dev
- Add the gated V2 workflow and bounded in-memory enrollment preview

* Sun Jul 26 2026 Loofi <noreply@example.invalid> - 1.0.0-1
- Publish the experimental Fedora 44 release
