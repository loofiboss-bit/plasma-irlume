# Test matrix

| Area | Automated evidence |
|---|---|
| Native backend | queued completion, unavailable state, safe skeleton state, generation cancellation |
| Coordinator | stale generations ignored, non-blocking refresh, teardown cancellation |
| Unsupported operations | C++ capability snapshots and Rust dispatcher remain fail-closed |
| Rust protocol | version, round trip, malformed message, zero length, oversized length before allocation |
| Model supply chain | strict manifest, exact names/sizes/SHA-256, missing/renamed/modified/unlisted rejection |
| Vision parser | dimensions, formats, checked stride arithmetic, truncation, overflow, cancellation, deadline |
| Vision provider | deterministic zero, one, and multiple face results plus bounded quality |
| Vision worker | one request, version/size/sequence, startup, timeout, crash, malformed response, cleanup |
| Vision KCM bridge | second explicit action, stale generations, cancellation, page/application/preview teardown |
| Camera provider | bounded discovery and spectrum classification |
| Preview protocol | framing, session, sequence, dimensions, payload limits |
| Preview lifecycle | manual start, timeout, cancellation, frame clearing, backpressure |
| QML | all active pages instantiate offscreen at narrow and wide sizes |
| Privacy | reports exclude frames, device labels/tokens, paths, credentials, and biometric-like values |
| Localization | every active single-line user message has Swedish text |
| Packaging | central identity, no external engine requirement, no privileged scriptlet, reproducible archive shape |
| RPM lifecycle | isolated install, upgrade, removal, payload, privilege, PAM-sentinel, and user-data checks |

Real camera and real YuNet inference remain separate qualification tasks.
Passing deterministic synthetic tests is not evidence that recognition,
liveness, enrollment, or authentication works.
