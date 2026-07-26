# V2 Implementation Status

## Product goal

V2 turns Face Login into a guided KDE workflow with four task-oriented areas:
**Setup & Status**, **Face Profiles**, **Access**, and **Support**. Live state
always comes from the engine and system probes; the KCM does not persist an
optimistic completion flag.

The visual direction is Plasma-native biometric readiness: a compact
next-action dashboard, a six-step readiness path, and an in-memory camera
preview with landmarks and redundant text guidance. System colors, typography,
focus handling, and controls remain authoritative.

## Implemented locally

- contract-v2 capability negotiation for `profiles-json`, `events-jsonl`,
  `position-report`, and `preview-ir-jpeg`;
- a dedicated `EnrollmentSession` process boundary for enrollment,
  improve-recognition, and non-authenticating recognition tests;
- bounded JPEG validation (640 by 480, 128 KiB), 8 fps backpressure, monotonic
  event sequences, session IDs, typed positioning, cancellation, timeout, and
  memory clearing;
- a deliberately separate painted preview item; the normal diagnostic adapter
  still rejects image and biometric fields;
- IR/RGB labeling, face box, exactly 478 normalized landmarks, countdown,
  quality, and text checklist;
- preview cancellation and buffer clearing when the profile page is hidden;
- engine-advertised profile-count limits;
- four task-oriented areas and a live-state setup path;
- existing fixed KAuth plan, apply, verify, rollback, and emergency-disable
  boundary retained for Access;
- QML creation tests at 320, 480, and 960 logical pixels and a warning-free
  repository lint command.

## Deliberately gated

No released irlume engine currently publishes the reviewed V2 contract.
Consequently, irlume 0.6.x remains read-only and production enrollment,
preview, profile mutation, and login mutation stay unavailable. Synthetic
fixtures and unit tests prove the consumer boundary, not upstream support.

The following items cannot be marked release-complete in this repository:

- upstream schemas, implementation, tests, and release-derived fixtures;
- camera-pair selection, emitter testing, and capture tuning through public
  engine operations;
- profile rename, individual scan deletion, and identity-merge results;
- daemon-restart reconnection against a real V2 engine;
- real Secure IR and RGB Convenience hardware;
- Fedora 44 clean install, V1-to-V2 upgrade, password fallback, and terminal
  successful public COPR installation.

Stable `2.0.0` publication is blocked until every item above has release
evidence. The private daemon protocol, human CLI parsing for mutation, and
parallel camera access are never acceptable substitutes.

## Qualification evidence

The following evidence was refreshed on 2026-07-26:

- the complete local build, CTest, Python fixture, QML lint, C++ formatting,
  source archive, RPM build, rpmlint, and isolated package lifecycle gates
  passed;
- a host upgrade from `1.0.0-1.fc44` to `2.0.0-0.1.dev.fc44` preserved the
  engine status and every PAM file digest;
- official irlume `0.6.1` and upstream `main` at
  `82165049f95440b281b78f3152dcd8901e4effbf` do not publish the required
  machine contract; upstream tracking is
  [archledger/irlume#108](https://github.com/archledger/irlume/issues/108);
- COPR build
  [`10774932`](https://copr.fedorainfracloud.org/coprs/build/10774932) reports
  terminal `succeeded` for the V1 package after the earlier Pulp outage, but
  its result URL returns 404 and the public repository metadata remains empty;
  clean installation therefore still fails. The repository-publication defect
  is tracked in
  [fedora-copr/copr#4401](https://github.com/fedora-copr/copr/issues/4401#issuecomment-5084418359).

Neither terminal COPR API state without package availability nor synthetic
contract fixtures satisfy the stable V2 release gate.

## Upstream handoff

The exact required transport is specified in
[`UPSTREAM-API-REQUEST.md`](UPSTREAM-API-REQUEST.md). Once an official release
exists:

1. pin the accepted engine range and replace synthetic payloads with sanitized
   release-derived fixtures;
2. execute failure injection for cancellation, oversized and stale frames,
   daemon restart, verification failure, and rollback;
3. implement and gate the remaining camera/profile operations advertised by
   that release;
4. run the complete hardware and Fedora package matrix in
   [`TEST-MATRIX.md`](TEST-MATRIX.md);
5. only then change the Fedora dependency to the reviewed minimum engine and
   publish stable `2.0.0`.
