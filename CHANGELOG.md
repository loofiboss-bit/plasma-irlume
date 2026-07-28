# Changelog

All notable changes to plasma-irlume are documented in this file.

## 3.0.0 - 2026-07-28

- Add an unprivileged `/usr/libexec` Qt Multimedia worker for local RGB, IR,
  and Unknown camera discovery and manually started ephemeral preview.
- Add the length-framed CBOR protocol v1 with private pipes, session IDs,
  monotonic sequences, fixed commands, bounded records, and latest-frame
  backpressure.
- Add `CameraPreviewSession` and the Camera Check page with explicit privacy
  limits, 60-second countdown, stable errors, and automatic stop/clear on
  hiding, application deactivation, timeout, stall, crash, or teardown.
- Limit discovery to 16 devices and preview to 640×480, 8 fps, and 128 KiB
  JPEG frames. Use reviewed udev properties without name-based IR claims.
- Remove the unused enrollment/landmark implementation and v2 camera and
  authentication mutation controls. Profiles and Access remain read-only.
- Add provider, protocol, worker, lifecycle, QML, localization, packaging,
  privilege-boundary, and RPM checks that require no physical camera in CI.
- Preserve irlume Contract 1 diagnostics and the `irlume >= 0.7.0` boundary.

The unpublished 2.1.0 and 2.2.0 development line is superseded by 3.0.0; no
separate 2.1.0 or 2.2.0 tags are planned.

## Unpublished development snapshots

## 2.2.0 - 2026-07-28

- Convert `FaceAuthBackend` to a typed asynchronous interface and add a
  generation-aware refresh coordinator with latest-request-wins cancellation.
- Convert the irlume adapter to a signal-driven `QProcess` state machine with
  fixed commands, a sanitized environment, per-command timeout, and separate
  256 KiB stdout/stderr limits.
- Run bounded local system probing away from the GUI thread and keep section
  results and errors operation-scoped.
- Treat the successful Contract 1 handshake separately from read capabilities
  and keep camera type separate from unknown security properties.
- Remove the former privileged helper, KAuth dependency, system D-Bus files,
  and Polkit policy; the standard package is entirely read-only.
- Vendor the exact irlume 0.7.0 Contract 1 schema for offline fixture
  validation and add an optional installed-engine checker.
- Strengthen asynchronous, generation, presentation, QML, localization, and
  RPM lifecycle coverage.

## 2.1.0 - 2026-07-28

- Add a backend-neutral `FaceAuthBackend` boundary and an irlume adapter.
- Negotiate released irlume 0.7 Machine API Contract 1 and capability-gate all
  fixed read-only commands.
- Replace human-readable output and engine-version parsing with typed JSON.
- Keep enrollment, profile, camera, and authentication mutations disabled and
  fail-closed without starting undocumented subprocesses.
- Require `irlume >= 0.7.0` without an upper version bound.
- Document the transitional dependency and future native-engine milestones.

## Historical releases

## 2.0.0 - 2026-07-26

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
