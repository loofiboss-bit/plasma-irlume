# Face Login Recovery

## Before enabling

Do not enable face authentication until password login works and a root TTY is
available. The KCM displays this recovery command and requires acknowledgement:

```bash
sudo irlume login disable --apply
```

The KCM enables the lock screen first. Login-screen activation is available
only for the Secure infrared tier. Face authentication for sudo and Polkit is
not part of version 1.0.

## Recover from a TTY

These instructions are intentionally self-contained. They do not require a web
browser, a graphical session, or access to this repository.

1. Press `Ctrl+Alt+F3` to open a text console.
2. Sign in with the existing password.
3. Run:

   ```bash
   sudo irlume login disable --apply
   ```

4. Verify the distribution authentication configuration:

   ```bash
   authselect check
   systemctl status irlumed --no-pager
   ```

5. Return to the graphical session with `Ctrl+Alt+F2` or reboot only after
   password authentication is confirmed.

Do not edit `/etc/pam.d`, `/usr/lib/pam.d`, `system-auth`, or display-manager PAM
files manually. The recovery command lets irlume restore only the state it owns.

## Recover from an active desktop session

Open **System Settings → Face Login → Diagnostics** and select **Disable Face
Login now**. The KCM sends no path, username, or shell command. Its fixed KAuth
disable operation asks irlume to regenerate a clean-state plan, applies it, and
verifies password fallback.

Keep the session open until the KCM confirms success. If it cannot confirm
rollback or disable, copy the TTY instructions from the same page and use them
before logging out or rebooting.

## Display-manager migration

After an upgrade from SDDM to Plasma Login Manager, or the reverse, refresh
Diagnostics before enabling Face Login. If stale wiring for the previous
display manager is detected, activation is blocked. Disable the old integration
first, refresh, and preview a new plan. The KCM does not migrate or rewrite PAM
state automatically.

## Automatic rollback

The privileged helper treats apply and verify as one transaction. It reports
success only after irlume confirms the desired state, daemon reachability, exact
PAM targets, and password fallback. Failed verification triggers an immediate
engine-owned rollback.

If the KCM says rollback could not be confirmed, keep the TTY open and run the
recovery command above before logging out or rebooting.

## Current engine gate

irlume 0.6.1 does not publish the required versioned `login-transactions`
contract. On that release the KCM fails closed before authorization or mutation.
Do not bypass this gate with human-output parsing or private daemon calls.
