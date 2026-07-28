# Native engine roadmap

No native Loofi-owned biometric engine is implemented in version 2.1. The
future work is deliberately separated into security-reviewable milestones:

1. Backend-neutral frontend — implemented by the current transitional release.
2. Unprivileged preview and camera discovery.
3. Local enrollment and non-authentication test matching.
4. Secure daemon and template store.
5. Lock-screen-only PAM integration.
6. Login-screen integration.
7. IR liveness and replay resistance.
8. TPM-backed template protection.
9. Optional higher-risk authentication surfaces.

Each future milestone requires its own threat model, contracts, deterministic
tests, failure recovery, and hardware qualification. Later milestones must not
be inferred from completion of an earlier one.
