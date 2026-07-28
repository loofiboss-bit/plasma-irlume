# User guide

The v2.2.0 KCM is a read-only status viewer. Refresh starts a bounded
asynchronous check and disables refresh buttons until the current request
finishes. Existing section data may remain visible with an **Updating…**
status. If a new section result fails, that section is cleared and shows its
own error and retry state.

The summary distinguishes:

- missing backend;
- unavailable or incompatible Contract 1 handshake;
- no compatible read capabilities;
- partial read-only diagnostics;
- complete read-only status.

Profiles come only from `profiles list`, camera type only from the status
camera section, authentication wiring only from `login status`, and readiness
only from status plus relevant doctor checks.

Infrared and RGB describe camera capability only. The KCM reports security
tier, liveness, password fallback, and authentication safety as unknown unless
an explicit supported result establishes them.

No button in this version captures a frame, enrolls a face, changes a profile,
opens an authorization prompt, edits PAM, or starts a helper.
