# Observed private protocol evidence

These files are sanitized, source-derived examples of irlume's private Serde
wire shapes at the official v0.6.0 and v0.6.1 release commits.

They are **not** runtime captures, a public API, or supported integration
fixtures. `plasma-irlume` must never consume them in production.

Sources:

- v0.6.0 commit `76874ecf1411aa7aa815d1868b8d84f3a0aa1129`
- v0.6.1 commit `b5512867cdf8f5a5471d3a8099cd11c70d5b738d`
- `crates/irlume-common/src/lib.rs` in each release

Device paths, usernames, secrets, and host-specific values are omitted. The
v0.6.1 examples retain the additive `ir_depth_floored` and `third_party_pad`
fields to record the observed protocol change.
