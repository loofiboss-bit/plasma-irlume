# Changelog

All notable changes to plasma-irlume are documented in this file.

## 2.0.0-dev - unreleased

- Replace five technical tabs with Setup & Status, Face Profiles, Access, and
  Support.
- Add a version-gated, bounded in-memory IR/RGB enrollment preview with
  FaceMesh landmarks and typed positioning guidance.
- Add contract-v2 capability negotiation and engine-advertised profile limits.
- Add fail-closed profile/scan rename, guarded scan deletion, and exact
  enrollment-merge cleanup.
- Add reviewed camera-pair discovery, independently verified selection,
  read-only emitter probing, verified emitter setup, and bounded capture
  tuning through fixed machine commands and KAuth actions.
- Complete the Swedish UI and backend message catalog.
- Keep irlume 0.6.x read-only and block stable publication until the reviewed
  upstream contract and hardware matrix pass.

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
