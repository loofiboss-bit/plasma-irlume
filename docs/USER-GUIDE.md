# plasma-irlume user guide

plasma-irlume presents local irlume readiness and authentication controls in
KDE System Settings. It does not perform face recognition itself and never
stores frames, images, embeddings, or face templates.

## Open Face Login

Open **System Settings → Security & Privacy → Face Login**, or run:

```bash
kcmshell6 kcm_irlume
```

The module has five pages.

## Overview

Overview summarizes the complete readiness path:

- camera capability;
- irlume engine and version;
- active display manager and PAM state;
- face-profile state;
- verified password fallback.

A healthy-looking camera alone does not mean that authentication is ready.
Treat an unknown, unsupported, or unverified value as unavailable.

## Face profile

When a reviewed irlume release advertises the required structured profile
contract, this page can:

- create a profile;
- test recognition without granting a credential;
- add an appearance scan;
- delete the selected current-account profile.

The current irlume 0.6.1 release does not advertise that contract, so the page
explains why profile actions are unavailable. Do not bypass the gate. Follow
the engine's own supported setup documentation until a compatible release has
been reviewed.

## Authentication

Authentication changes follow a preview, apply, verify, and rollback
transaction owned by irlume. Before the first enable:

1. Confirm password login from a TTY.
2. Read and acknowledge the displayed recovery command.
3. Preview the exact engine-owned targets.
4. Apply only if the display manager, security tier, profile, and password
   fallback checks are healthy.
5. Keep the desktop session open until verification succeeds.

RGB Convenience hardware is limited to lock-screen use. Login-screen
activation requires the Secure infrared tier. Face authentication for `sudo`,
`su`, SSH, Polkit, and package installation is outside this release.

Authentication controls remain unavailable on engines that do not advertise
the reviewed `login-transactions` contract.

## Security

Security reports:

- Fedora and Plasma versions;
- the active display manager;
- TPM availability and template protection;
- Secure Boot;
- IR-emitter and liveness status.

These are local status signals, not certification. RGB-only face recognition
is a convenience feature and is not permitted for greeter login by the KCM.

## Diagnostics

Refreshing Diagnostics runs fixed, read-only local probes. It does not edit PAM
or invoke an authentication mutation.

The page can:

- show the current diagnostic source and typed status values;
- copy the offline TTY recovery procedure;
- copy or export a redacted Markdown support report;
- request a fixed, verified emergency disable operation when the engine
  contract supports it.

Review every support report before sharing it. It excludes raw command output,
journals, usernames, profile names, device paths, biometric payloads, and
credentials by design.

## Safe disable and recovery

From an active desktop session, use **Diagnostics → Disable Face Login now**
and wait for verified success.

If graphical login or unlock is unreliable:

1. Press `Ctrl+Alt+F3`.
2. Sign in with the account password.
3. Run:

   ```bash
   sudo irlume login disable --apply
   authselect check
   systemctl status irlumed --no-pager
   ```

4. Return to the graphical session only after password authentication is
   confirmed.

Never manually edit `/etc/pam.d`, `/usr/lib/pam.d`, or `system-auth`. See
[RECOVERY.md](RECOVERY.md) for the complete offline procedure and
[TROUBLESHOOTING.md](TROUBLESHOOTING.md) for typed failure guidance.
