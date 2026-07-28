# User guide

The KCM refreshes one typed snapshot from irlume Machine API Contract 1.

- **Setup & Status** shows Fedora, display manager, daemon, camera, TPM, and
  template-protection observations. Unavailable data is shown as unknown.
- **Face Profiles** shows sanitized profile and scan display summaries when
  the engine advertises `profiles-list-json`.
- **Access** shows current login and lock-screen wiring when
  `login-status-json` is advertised.
- **Support** provides recovery guidance and a redacted support report derived
  only from typed state.

All controls that would enroll, test authentication, edit profiles or scans,
select/tune a camera, or alter login wiring are intentionally disabled. Contract
1 is read-only. The UI does not fall back to human-readable output, private
daemon calls, or direct PAM edits.

Password fallback is shown as unknown unless a future reviewed backend can
verify it. Version 2.1 does not claim biometric accuracy, liveness, or hardware
qualification.
