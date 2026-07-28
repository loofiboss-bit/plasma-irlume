# Recovery

plasma-irlume 2.2 cannot mutate authentication. Its package lifecycle and KCM
do not activate, disable, repair, or rewrite PAM.

If face authentication was configured outside this KCM and must be disabled,
switch to a TTY, sign in with the existing password, and use the engine-owned
recovery command:

```bash
sudo irlume login disable --apply
```

This is guidance for a user-controlled recovery session; the KCM does not
execute it through Contract 1. Preserve password access and validate the
engine's own recovery documentation before changing a live authentication
stack.
