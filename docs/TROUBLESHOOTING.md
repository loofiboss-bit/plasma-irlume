# Face Login troubleshooting

Use password authentication whenever Face Login is unavailable. Do not edit PAM
files manually, reset the TPM, or bypass the structured-contract gate.

| Diagnostic code | Safe next action |
| --- | --- |
| `camera-busy` | Close applications using the camera, then retry. |
| `camera-unavailable` | Reconnect or re-enable the camera. If it disappeared after a kernel update, boot the previous kernel and refresh diagnostics. |
| `ir-emitter-failed` / `emitter-unavailable` | Check privacy controls and camera cabling, use password login, then rerun diagnostics. |
| `tpm-unseal-failed` | Use password login. Restore the previous boot state or re-arm protection through irlume; do not reset the TPM. |
| `secure-boot-pcr-changed` | Restore the expected Secure Boot or firmware state before re-arming TPM protection. |
| `engine-version-unsupported` | Install a reviewed compatible irlume release. |
| `structured-contract-unavailable` | Keep mutations disabled until irlume publishes the reviewed contract. |
| `pam-drift` | Disable Face Login, run `authselect check`, then refresh before enabling again. |
| `display-manager-migration` | Disable the previous display-manager integration, refresh, and preview a new plan. |
| `kwallet-password-mismatch` | Unlock KWallet with the account password and re-arm wallet integration after a password change. |
| `rollback-failed` | Keep the session open and follow the TTY recovery instructions immediately. |

## Support report

The Diagnostics page can copy or export a Markdown support report. It is built
from typed status values and excludes raw command output, journals, usernames,
profile names, paths, frames, images, embeddings, templates, and credentials.
Review the report before sharing it. The default export location is Documents.

## TTY recovery

The full offline procedure is in [RECOVERY.md](RECOVERY.md). The essential
disable command is:

```bash
sudo irlume login disable --apply
```
