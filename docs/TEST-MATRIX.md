# Test matrix

| Area | Automated evidence |
|---|---|
| Native backend | queued completion, unavailable state, safe skeleton state, generation cancellation |
| Coordinator | stale generations ignored, non-blocking refresh, teardown cancellation |
| Unsupported operations | C++ capability snapshots and Rust dispatcher remain fail-closed |
| Rust protocol | version, round trip, malformed message, zero length, oversized length before allocation |
| Model supply chain | strict manifest, exact names/sizes/SHA-256, missing/renamed/modified/unlisted rejection |
| Vision parser | dimensions, formats, checked stride arithmetic, truncation, overflow, cancellation, deadline |
| Vision provider | verified real initialization, RGB/RGBA/Gray conversion, padded rows, zero-face inference, adversarial native output, bounded quality |
| Vision worker | one request, version/size/sequence, startup, timeout, crash, malformed response, cleanup |
| Vision KCM bridge | second explicit action, stale generations, cancellation, page/application/preview teardown |
| Camera provider | bounded discovery and spectrum classification |
| Preview protocol | framing, session, sequence, dimensions, payload limits |
| Preview lifecycle | manual start, timeout, cancellation, frame clearing, backpressure |
| QML | all active pages instantiate offscreen at narrow and wide sizes |
| Privacy | reports exclude frames, device labels/tokens, paths, credentials, and biometric-like values |
| Localization | every active single-line user message has Swedish text |
| Native FFI | exact OpenCV gate, malformed tensor/result rejection, input immutability, core-dump hardening, ASan/UBSan build |
| Fake isolation | deterministic provider appears only in explicit test code and never in the manifest, worker CLI, installed payload, or RPM |
| Packaging | central identity, Fedora OpenCV dependencies, no privileged scriptlet, reproducible archive/SRPM/RPM |
| RPM lifecycle | isolated install, upgrade, removal, payload, privilege, PAM-sentinel, and user-data checks |

Real camera and licensed positive-fixture inference remain separate
qualification tasks. Passing synthetic tests is not evidence that recognition,
liveness, enrollment, or authentication works.
