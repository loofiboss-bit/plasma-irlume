# Installation

On Fedora 44:

```bash
sudo dnf copr enable archledger/irlume
sudo dnf copr enable loofitheboss/plasma-irlume
sudo dnf install plasma-irlume
```

Open **System Settings → Security & Privacy → Face Login** or run
`kcmshell6 kcm_irlume`.

plasma-irlume requires irlume 0.7.0 or newer, but uses Contract 1 negotiation
instead of an upper engine-version bound. The package installs only the KCM,
desktop metadata, documentation, and translations. It installs no helper,
system D-Bus configuration, Polkit action, PAM file, engine data, or user
profile.

Removing the package leaves irlume profiles and user configuration unchanged:

```bash
sudo dnf remove plasma-irlume
```

For an offline diagnostic check of an installed engine from a source checkout:

```bash
scripts/check-installed-irlume-contract.py
```
