# Engine contract

Version 3.0.0 consumes irlume Machine API Contract 1 through exactly five
fixed read commands:

```text
/usr/bin/irlume version --json
/usr/bin/irlume status --json --contract 1
/usr/bin/irlume doctor --json --contract 1
/usr/bin/irlume profiles list --json --contract 1
/usr/bin/irlume login status --json --contract 1
```

The version response is always first. Later commands run only when their exact
capability is advertised. Unknown capabilities and schema-allowed properties
are ignored safely. Engine version is informational; compatibility is based on
the negotiated contract range and capabilities.

Each response must have a valid Contract 1 envelope, matching command, typed
payload, and exit-status semantics. Structured errors stay attached to the
operation that failed. A failed section clears that section only; other
successful results remain available.

The exact irlume v0.7.0 Draft 2020-12 schema is vendored under
`tests/schemas/irlume-0.7.0/` for offline tests only. No schema is downloaded
during build or test.

Contract 1 does not advertise supported camera, preview, enrollment, profile,
authentication, or PAM mutations. Every such feature remains false.
