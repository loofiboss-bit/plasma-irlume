# Threat boundary

Milestone 1 is architecture groundwork, not an authentication system.

## Trusted in this milestone

- fixed, bounded local platform reads;
- the KCM's generation-aware asynchronous coordinator;
- the reviewed camera worker protocol and its strict bounds;
- the Rust protocol codec's closed message types and length checks.

## Explicitly outside the trusted computing base

- face detection, embeddings, matching, and liveness;
- enrollment and deletion workflows;
- biometric templates, encryption keys, and persistence;
- PAM decisions or configuration;
- privileged helpers, system services, and policy installation;
- SELinux policy;
- network access, telemetry, model download, and remote inference.

These areas are represented as `Unsupported` or `Not implemented`. They have no
mutation API, command path, fallback heuristic, or permissive default.

## Fail-closed rules

- Missing native-engine status keeps the KCM responsive but never enables a
  biometric or PAM operation.
- Unsupported protocol versions and malformed or oversized frames are rejected.
- The Rust dispatcher recognizes only `Capabilities` and `Status`.
- Camera presence and visible preview pixels never become a security,
  liveness, identity, or readiness claim.
- Stale backend or system-probe generations cannot replace current state.
- Package installation, upgrade, and removal contain no authentication
  scriptlets and do not touch PAM.

## Preview privacy

Preview capture is manual, time-limited, bounded, and ephemeral. Frames remain
in memory, are dropped under backpressure, and are cleared when capture stops.
Support reports contain only aggregate device counts, spectrum counts, stable
error codes, and dropped-frame counts. They exclude frames, labels, tokens, and
device nodes.
