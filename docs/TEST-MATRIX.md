# Test matrix

| Area | Automated evidence |
|---|---|
| Native backend | queued completion, unavailable state, safe skeleton state, generation cancellation |
| Coordinator | stale generations ignored, non-blocking refresh, teardown cancellation |
| Unsupported operations | C++ capability snapshots and Rust dispatcher remain fail-closed |
| Rust protocol | version, round trip, malformed message, zero length, oversized length before allocation |
| Camera provider | bounded discovery and spectrum classification |
| Preview protocol | framing, session, sequence, dimensions, payload limits |
| Preview lifecycle | manual start, timeout, cancellation, frame clearing, backpressure |
| QML | all active pages instantiate offscreen at narrow and wide sizes |
| Privacy | reports exclude frames, device labels/tokens, paths, credentials, and biometric-like values |
| Localization | every active single-line user message has Swedish text |
| Packaging | central identity, no external engine requirement, no privileged scriptlet, reproducible archive shape |
| RPM lifecycle | isolated install, upgrade, removal, payload, privilege, PAM-sentinel, and user-data checks |

Real hardware remains a separate qualification task. Passing synthetic camera
tests is not evidence that recognition, liveness, enrollment, or authentication
works.
