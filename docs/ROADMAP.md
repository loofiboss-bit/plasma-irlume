# Roadmap

## Milestone 1: native foundation

Complete in this branch:

- standalone neutral identity;
- asynchronous backend and generation-aware cancellation;
- explicit unavailable/unsupported states;
- bounded versioned Rust protocol;
- typed status and capabilities;
- source-only `vision`, `templates`, `daemon`, and `cli` skeletons;
- preserved camera preview, system probe, localization, packaging, and CI.

## Blockers before native enrollment

Enrollment must remain unavailable until all of the following exist and are
reviewed:

1. A bounded camera-to-vision interface that never reuses preview frames as
   biometric input implicitly.
2. A selected, redistributable, reproducibly packaged face model with fixed
   hashes and no runtime download.
3. Documented image normalization, quality gates, presentation-attack
   resistance, and measured false-accept/false-reject behavior.
4. An encrypted template format with versioning, key ownership, deletion,
   migration, corruption handling, and rollback semantics.
5. Per-user authorization and a reviewed local IPC transport with peer identity
   checks, timeouts, rate limits, and cancellation.
6. Hardware qualification for RGB/infrared pairs, privacy switches, suspend,
   hotplug, contention, and firmware changes.
7. A threat model and privacy review covering capture, memory lifetime,
   diagnostics, and recovery.

Only after enrollment and verification are independently safe may a later
milestone design PAM integration. That separate gate requires password fallback,
transactional configuration, recovery, lockout/rate limiting, privileged
boundary review, SELinux design, and real login-manager testing.

Milestone 1 does not authorize any of those implementations.
