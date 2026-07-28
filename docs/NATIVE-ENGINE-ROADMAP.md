# Native engine roadmap

## Current milestone: v3.0.0 Native Camera Check

The current native boundary is limited to local device discovery and an
explicitly started, bounded, ephemeral preview in a separate unprivileged
process. It does not use the irlume socket and does not perform face
detection, landmarking, liveness analysis, embeddings, recognition,
enrollment, profile mutation, PAM configuration, or authentication decisions.

The v2 enrollment/landmark preview and dead camera/authentication mutation
controls have been removed. Contract 1 diagnostics, Face Profiles, and Access
remain read-only.

## Future work

Enrollment, biometric analysis, profile mutation, and PAM activation require
a separate reviewed plan and machine contract. They must remain absent until
that plan defines transactional mutation, verification, recovery, hardware
qualification, privacy limits, and public release gates. Native Camera Check
must not be treated as evidence that such functionality is safe or ready.
