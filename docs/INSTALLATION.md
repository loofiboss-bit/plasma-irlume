# Installing plasma-irlume

plasma-irlume 1.0.0 is an experimental KDE System Settings integration for
Fedora 44. It requires the separate irlume engine and does not install or
enable face authentication by itself.

## Supported system

- Fedora 44 KDE Plasma Desktop on a mutable, DNF-managed installation
- x86-64
- Plasma Login Manager or SDDM
- Plasma 6.6 or newer
- irlume 0.6.x for read-only diagnostics

Fedora Kinoite, rpm-ostree systems, other distributions, and other display
managers are not supported by this release.

## Install from COPR

Enable the upstream engine repository first, then the plasma-irlume repository:

```bash
sudo dnf copr enable archledger/irlume
sudo dnf copr enable loofitheboss/plasma-irlume
sudo dnf install plasma-irlume
```

Installing these packages does not wire face authentication into PAM. Review
the transaction before accepting it and make sure the packages come from the
two expected COPR projects.

Verify the installed versions:

```bash
rpm -q plasma-irlume irlume
irlume --version
```

Open the module:

```bash
kcmshell6 kcm_irlume
```

It is also available in **System Settings → Security & Privacy → Face Login**.

## First-run safety check

Before enabling any authentication integration:

1. Confirm that normal password login and screen unlock work.
2. Open a TTY with `Ctrl+Alt+F3` and confirm that password login works there.
3. Keep this recovery command available:

   ```bash
   sudo irlume login disable --apply
   ```

4. Open **Face Login → Diagnostics** and refresh the local checks.
5. Read [the recovery guide](RECOVERY.md) before logging out or rebooting.

Do not manually edit PAM files or bypass a disabled control in the KCM.

## Current irlume compatibility

The current irlume 0.6.1 release supports plasma-irlume's conservative,
read-only diagnostics. It does not publish the versioned structured contracts
required by the KCM for profile or authentication mutations. With that engine,
the corresponding controls intentionally remain unavailable.

This is a safety boundary, not an installation failure. Use irlume's own
documented interface for engine-owned setup until a reviewed upstream release
advertises the contracts in [ENGINE-CONTRACT.md](ENGINE-CONTRACT.md). Do not
work around the gate with private daemon calls or human-output parsing.

## Update

Update both packages through the normal Fedora transaction:

```bash
sudo dnf upgrade --refresh plasma-irlume irlume
```

After an update, open Diagnostics and refresh. The KCM rechecks the engine
version and capabilities instead of carrying compatibility assumptions across
upgrades.

## Remove

Removing the GUI does not disable an active irlume integration or delete face
profiles. Disable authentication first, verify password login, and then remove
the KCM:

```bash
sudo irlume login disable --apply
authselect check
sudo dnf remove plasma-irlume
```

If face authentication was never enabled, only the final command is needed.
The separately installed `irlume` package remains available unless it is
removed explicitly.

## Verify package origin

Show the enabled repositories and the installed package source:

```bash
dnf repolist --enabled
dnf repoquery --installed --qf '%{name} %{version}-%{release} %{repoid}' \
  plasma-irlume irlume
```

Release checksums and RPM artifacts are attached to the matching GitHub
release. See [the test matrix](TEST-MATRIX.md) for the exact distinction
between automated package evidence and unverified real-hardware behavior.
