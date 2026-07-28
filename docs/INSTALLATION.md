# Installation

plasma-irlume 2.1 targets Fedora 44 KDE and requires the separately packaged
`irlume >= 0.7.0`.

```bash
sudo dnf copr enable archledger/irlume
sudo dnf copr enable loofitheboss/plasma-irlume
sudo dnf install plasma-irlume
irlume version --json
```

The reported contract range must include `1`. The engine version itself is
informational and has no upper bound in the RPM.

Open **System Settings → Security & Privacy → Face Login**. Version 2.1 shows
read-only state. It cannot enroll, configure cameras, or change PAM/login
wiring because Contract 1 has no mutation capability.

Installing, upgrading, and removing plasma-irlume has no package script that
runs irlume, edits PAM, or changes profiles. Removing the GUI therefore leaves
engine-owned data and existing authentication state untouched.
