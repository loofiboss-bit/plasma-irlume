Name:           plasma-irlume
Version:        3.0.0
Release:        1
Summary:        Payload-only legacy KCM transition fixture
License:        GPL-3.0-or-later

%description
An isolated payload-only fixture for testing the package-name transition to
kfaceauth. This package is never a release artifact.

%install
mkdir -p \
    %{buildroot}%{_libexecdir} \
    %{buildroot}%{_libdir}/qt6/plugins/plasma/kcms/systemsettings \
    %{buildroot}%{_datadir}/applications
touch %{buildroot}%{_libexecdir}/plasma-irlume-camera-preview-worker
touch %{buildroot}%{_libdir}/qt6/plugins/plasma/kcms/systemsettings/kcm_irlume.so
touch %{buildroot}%{_datadir}/applications/kcm_irlume.desktop

%files
%{_libexecdir}/plasma-irlume-camera-preview-worker
%{_libdir}/qt6/plugins/plasma/kcms/systemsettings/kcm_irlume.so
%{_datadir}/applications/kcm_irlume.desktop

%changelog
* Wed Jul 29 2026 KFaceAuth Tests <noreply@example.invalid> - 3.0.0-1
- Add a payload-only package transition fixture
