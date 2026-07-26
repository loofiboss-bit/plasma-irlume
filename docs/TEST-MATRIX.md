# V2 test matrix

## Automated contract coverage

Run:

```bash
cmake --fresh -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
/usr/lib64/qt6/bin/qmllint src/kcm/ui/*.qml src/kcm/ui/components/*.qml
find src tests/unit -type f \( -name '*.cpp' -o -name '*.h' \) -print0 \
  | xargs -0 clang-format --dry-run --Werror
packaging/fedora/create-source-archive.sh
rpmbuild -ba packaging/fedora/plasma-irlume.spec \
  --define "_sourcedir $PWD"
rpmlint packaging/fedora/plasma-irlume.spec
packaging/fedora/rpm-smoke-test.sh \
  "$HOME/rpmbuild/RPMS/$(uname -m)/plasma-irlume-2.0.0-0.1.dev.fc44.$(uname -m).rpm"
```

The automated suite covers:

- fixed KAuth action IDs and fixed irlume argument construction;
- rejection of arbitrary scopes, executable paths, shell strings, PAM paths,
  users, and unsafe opaque IDs;
- Fedora 44, supported display-manager, profile, password-fallback, and
  Secure-tier preflight;
- rejection of an SDDM plan on Plasma Login Manager and vice versa;
- plan/apply/verify lineage and required daemon, PAM, desired-state, and
  password-fallback checks;
- automatic rollback after an injected post-apply verification failure;
- engine-reported rollback without a duplicate rollback attempt;
- RGB Convenience gating to lock-screen only;
- a successful matching preview before enable or disable;
- recovery-command acknowledgement before the first enable;
- fixed machine-mode commands with no shell, username, path, or free-form QML
  arguments;
- contract version, command, engine version, operation ID, sequence, terminal
  state, output bounds, and sensitive-field rejection;
- preview session ID, bounded JPEG, dimensions, spectrum, normalized landmarks,
  typed positioning, frame dropping, cancellation, and memory clearing;
- profile-list loading and safe opaque IDs;
- enrollment followed by a non-mutating recognition test;
- deletion of an unverified new profile after a failed test;
- standalone recognition failure without profile mutation;
- camera-busy retry and cancellation forwarding;
- selected-profile deletion and rejection of cross-profile mutation results;
- QML creation at 320, 480, and 960 pixel widths for every diagnostic state.
- one-click disable routing through the fixed verified KAuth disable action;
- actionable recovery guidance for every Phase 5 error code;
- support-report removal of home/device/config paths, email addresses, secrets,
  credentials, and biometric payload labels;
- atomic Markdown export;
- copyable TTY recovery instructions;
- SDDM-to-Plasma Login Manager and reverse-migration drift detection before
  mutation.

Synthetic fixtures prove consumer behavior only. They are not evidence that a
released irlume build implements the proposed machine contract.

## Accessibility and localization gate

Run QML lint and formatting checks, then inspect every page at 320, 480, and
960 logical pixels. Interactive recovery controls have accessible names and
descriptions, status is expressed in text as well as color, long labels wrap,
and all user-visible strings use KDE translation functions.

Manual release checks remain required with keyboard-only navigation, a screen
reader, 100%, 125%, 150%, and 200% scaling, and both light and dark Plasma
color schemes.

The suite does not invoke Polkit, write PAM files, or mutate the host.

## Fedora package lifecycle gate

The source archive is normalized by path order, ownership, permissions, and
timestamp. The RPM build runs the C++ unit suite, Python fixture suite, QML
lint, and C++ formatting check in a Fedora 44 buildroot.

The lifecycle smoke test installs the RPM without dependencies into an isolated
RPM root, verifies the KCM plugin, helper, desktop metadata, D-Bus policy, and
Polkit action, then erases the package. It proves that:

- every security-boundary file is owned by the RPM;
- uninstall removes those package-owned files;
- no package scriptlet invokes `irlume` or mutates authentication;
- simulated engine profile and user-setting sentinels survive uninstall.

This isolated test does not prove that a live engine, PAM stack, display
manager, camera, TPM, or Polkit prompt works. Those remain manual release gates.

An upgrade does not own or rewrite user settings, engine profiles, or PAM
state. Opening the upgraded KCM performs a fresh version-gated engine probe, so
compatibility is re-evaluated instead of being copied from package state.

## V2 release status

The V1 repository is packaging-complete for the experimental 1.0.0 release.
V2 is a gated development implementation and is not approved for publication.
GitHub and COPR publication do not satisfy or replace the following live
checks, which are not claimed by this repository state:

- Fedora 44 KDE fresh install with Plasma Login Manager;
- Fedora 44 upgrade retaining SDDM;
- Secure IR and RGB Convenience real-hardware paths;
- password login and unlock before and after activation;
- suspend/resume, TPM present/absent, and camera-busy recovery;
- live install, upgrade, disable, and uninstall with the reviewed irlume
  dependency.

## Live contract gate

Before enabling a new irlume release:

1. Confirm that its official documentation publishes the reviewed V2 contract and a
   compatibility policy.
2. Capture sanitized outputs for every command in `docs/ENGINE-CONTRACT.md`.
3. Replace the synthetic event fixtures with release-derived sanitized
   fixtures.
4. Verify exact capability names, command spelling, exit codes, cancellation,
   atomic cleanup, and selected-profile deletion against upstream tests.
5. Review the release and explicitly update the accepted engine version range.
6. Run plan/apply/verify/rollback failure injection in a disposable Fedora 44
   VM before enabling the release in production.

Human-oriented 0.6.x output, private daemon responses, and the presence of
similarly named CLI commands do not pass this gate.

## Real-hardware matrix

Run each enrollment and recognition scenario with normal journal logging and
confirm that no image file is created:

| Scenario | Expected result |
| --- | --- |
| Glasses off, then on | Recognition succeeds or recommends one additional scan |
| Normal indoor light | Enrollment and test complete |
| Dark room with IR hardware | Secure-tier enrollment and test complete |
| Bright backlight | Typed rejection or retry guidance; no profile drift |
| Camera open in another application | `camera-busy`, camera-app guidance, then successful retry |
| Cancel during capture | Terminal cancellation, camera released, no partial profile |
| Suspend/resume before test | Test recovers or fails with a typed retryable error |
| Failed post-enrollment test | New profile is deleted before failure is shown |
| Delete selected profile | Only that current-account profile is removed |

After each run:

```bash
journalctl -u irlumed --since "10 minutes ago" --no-pager
find "${XDG_RUNTIME_DIR:-/run/user/$(id -u)}" /tmp -maxdepth 2 -type f \
  \( -iname '*irlume*' -o -iname '*face*' \) -print
```

Review journal output locally for camera release and typed operation results.
Do not attach unsanitized logs, profile names, usernames, device paths, or
biometric material to an issue.
