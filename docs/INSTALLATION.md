# Installation

On Fedora 44:

```bash
sudo dnf copr enable archledger/irlume
sudo dnf copr enable loofitheboss/plasma-irlume
sudo dnf install plasma-irlume
```

Open **System Settings → Security & Privacy → Face Login** or run
`kcmshell6 kcm_irlume`.

plasma-irlume requires irlume 0.7.0 or newer and runtime Qt Multimedia. The
package installs the KCM, an ordinary unprivileged camera preview worker under
`/usr/libexec`, metadata, documentation, and translations. It installs no
helper, system D-Bus configuration, Polkit action, PAM file, daemon, engine
data, or user profile.

Installing, upgrading, or removing the package does not activate or rewrite
authentication. Removal leaves irlume profiles and user configuration
unchanged:

```bash
sudo dnf remove plasma-irlume
```
