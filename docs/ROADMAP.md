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

## Milestone 2: bounded local vision

Implemented in this branch:

- explicit one-frame analysis separate from manual preview start;
- short-lived unprivileged Rust worker with versioned bounded pipe protocol;
- checked 640×480 RGB8/RGBA8/Gray8 frame parsing;
- typed zero, one, or multiple face-presence and image-quality results;
- monotonic generations, cancellation, timeouts, stale-result rejection, and
  transient cleanup;
- selected YuNet detector artifact with immutable source, explicit weight
  license, SHA-256 verification, offline packaging, and no runtime download;
- deterministic fake provider while the real inference runtime remains gated.

Milestone 2 still does not authorize enrollment, profile management, persistent
embeddings, matching, identity thresholds, liveness claims, PAM, services,
privileged helpers, SELinux policy, TPM work, networking, or telemetry.

## Blockers before encrypted native enrollment

In addition to real, measured detector inference, enrollment remains blocked on:

1. reviewed image normalization and presentation-attack resistance without
   treating quality guidance as liveness;
2. measured detector error behavior and CPU latency across the supported
   RGB/infrared hardware matrix;
3. an encrypted, versioned template format with per-user key ownership,
   deletion, migration, corruption, and rollback semantics;
4. peer-verified local IPC, authorization, rate limits, and recovery;
5. a separate privacy/threat review for embeddings and enrollment memory;
6. clean install/upgrade/removal and real-camera release qualification.

PAM remains a later, separately reviewed milestone after enrollment and
verification are independently safe.
