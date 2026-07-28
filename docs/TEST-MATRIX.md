# Test matrix

Automated release checks cover:

- Contract 1 handshake ranges, future engine-version acceptance, capabilities,
  envelope failures, output bounds, and structured errors;
- typed status, doctor, profile, and login parsing;
- unknown-versus-zero semantics;
- fail-closed profile, camera, enrollment, authentication, and KAuth entry
  points with no mutation subprocess;
- QML loading, localization, formatting, desktop metadata, source archives,
  RPM build, and non-mutating RPM removal.

Run:

```bash
rm -rf build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
find src tests/unit -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
  | xargs -0 clang-format --dry-run --Werror
desktop-file-validate data/kcm_irlume.desktop
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
```

These checks do not validate biometric accuracy, liveness, spoof resistance,
real camera compatibility, or live PAM/login behavior. No such hardware claim
is made by version 2.1.
