# Test matrix

The v2.2.0 acceptance suite covers:

- Contract 1 command construction, capability gating, future engine versions,
  unknown properties and capabilities, malformed documents, duplicate fields,
  contradictory values, structured errors, start failure, timeout, and output
  limits.
- Immediate KCM construction, responsive event delivery, latest-generation
  wins, stale signals, cancellation, teardown during refresh, partial results,
  and not-advertised versus failed sections.
- Section-specific profile, camera, authentication, readiness, and support
  presentation, including old-data retention only while updating.
- QML creation, accessibility contracts, 320/480/960 px layouts, fixed busy
  indicator space, Swedish localization, formatting, and desktop metadata.
- Exact offline validation of all released Contract 1 fixtures against the
  vendored irlume v0.7.0 schema.
- Source archive, SRPM, RPM, rpmlint, payload/dependency inspection, and an
  isolated install/remove lifecycle that hashes PAM sentinels and preserves
  engine and user data.

The final static gate rejects blocking process waits in `src` and rejects any
privileged helper, system D-Bus service/policy, or Polkit action in the RPM.
