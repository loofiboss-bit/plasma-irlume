# Changelog

All notable changes to plasma-irlume are documented in this file.

## 1.0.0 - 2026-07-26

This is an experimental release. It must not be described as a
production release until the real-hardware and clean-install release matrix
passes.

### Added

- Native Plasma 6 System Settings integration for Fedora 44 KDE.
- Read-only, version-gated irlume diagnostics.
- Fail-closed profile workflows behind a structured engine contract.
- Transactional authentication preview, apply, verify, and rollback through a
  fixed-operation KAuth helper.
- One-click disable, TTY recovery guidance, and redacted support reports.
- Reproducible Fedora source archives and an RPM spec.
- Fedora 44 CI for formatting, QML lint, unit and fixture tests, RPM builds, and
  isolated install/uninstall lifecycle checks.
- Installation and user guides covering COPR setup, first run, updates,
  removal, recovery, diagnostics, and the current engine contract gate.

### Security boundary

- The package does not bundle irlume, camera code, biometric models, face
  profiles, or PAM modules.
- Installing, upgrading, or removing the GUI does not invoke irlume or mutate
  active authentication state.
- Engine releases outside the reviewed compatibility range fail closed.
