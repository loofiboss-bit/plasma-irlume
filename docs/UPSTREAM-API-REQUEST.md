# Upstream request: stable machine interface for desktop integrations

## Suggested title

Add a versioned JSON/NDJSON integration contract for status, enrollment, auth
testing, and transactional login wiring

## Context

`plasma-irlume` is a KDE System Settings integration for irlume. It currently
uses a narrow, version-gated parser for documented read-only 0.6.x CLI
diagnostics. A supported machine interface would reduce parser maintenance and
is required before richer streaming or transactional workflows.

irlume v0.6.1 already has useful typed internal responses and safe human-facing
dry runs, but the public CLI does not expose a complete versioned machine
contract. The current CLI is sufficient for conservative read-only
diagnostics, not for enrollment streams or authenticated mutations.

This request keeps irlume responsible for hardware, inference, templates, TPM,
PAM planning, writes, verification, and rollback. The desktop integration owns
only presentation, workflow, cancellation, and version gating.

## Common output rules

Please add a public contract version and document its compatibility policy.
Every JSON object should contain at least:

```json
{
  "contract_version": 1,
  "engine_version": "0.6.x",
  "command": "status",
  "ok": true,
  "data": {}
}
```

Requirements:

- machine-mode stdout contains only one JSON document or the declared NDJSON
  event stream;
- incidental logs and human diagnostics go to stderr;
- errors use stable codes and typed fields, not message matching;
- unknown flags, unsupported contracts, authorization failures, cancellation,
  timeouts, precondition failures, drift, apply failures, verification
  failures, and rollback failures are distinguishable;
- exit 0 means the requested operation completed successfully; documented
  nonzero codes group usage/compatibility, unavailable/degraded state,
  authorization, cancellation, and operation failure;
- optional fields may be added compatibly, while removal or semantic changes
  require a new `contract_version`;
- output contains no frames, images, embeddings, templates, passwords,
  credential material, TPM secrets, or unnecessary host identifiers.

## Read operations

Please provide:

```text
irlume version --json
irlume status --json
irlume doctor --json
irlume profiles list --json
irlume login status --json
```

Expected typed content:

- `version`: engine version, contract version, and advertised capabilities;
- `status`: daemon state, configured method, security tier, camera capability,
  enrollment summary, template/keyring protection state, and integration
  summary;
- `doctor`: an array of stable check IDs with `pass`, `warn`, `fail`, or
  `unknown`, severity, and optional structured details;
- `profiles list`: opaque profile and scan IDs, display names, counts, and
  relevant per-user policy flags;
- `login status`: active display manager, supported PAM service mapping,
  desired state, actual state, drift, and password-fallback state.

Human explanations can remain present in normal CLI mode. Machine consumers
should localize from stable IDs and codes.

## Enrollment and authentication-test events

Please expose a safe event stream, for example:

```text
irlume enroll --events=jsonl
irlume auth test --events=jsonl
```

`auth test` should perform claimed-user 1:1 verification only. It must not
identify arbitrary users, release a credential, modify thresholds, or change
profiles.

Every event should include the contract version, engine version, command,
operation ID, monotonically increasing sequence number, event type, and a
terminal flag. The stream should support:

- operation started;
- camera/capture/liveness/matching stage changes;
- bounded progress where meaningful;
- exactly one terminal `completed`, `cancelled`, or `failed` event.

Cancellation must release the camera and leave no partial profile or temporary
image. Events must never contain frame data, embeddings, or credentials.

## Transactional login operations

Please provide a machine contract for:

```text
irlume login enable --scope lock-screen --json
irlume login enable --scope login-screen --json
irlume login disable --json
irlume login enable --scope <lock-screen|login-screen> --apply --plan-id <id> --json
irlume login disable --apply --plan-id <id> --json
irlume login verify --transaction-id <id> --json
irlume login rollback --transaction-id <id> --apply --json
```

The exact command spelling can differ; the required semantics are:

### Plan

- no mutation;
- opaque plan ID bound to the observed system state;
- operation, requested scopes, detected display manager, security tier, and
  typed preconditions;
- abstract targets and actions, with before/after digests where applicable;
- explicit result for password-fallback preservation;
- refusal when the display manager or PAM layout is unsupported.

### Apply

- requires the plan ID;
- rejects state drift without writing;
- returns an opaque transaction ID;
- returns a typed result for every planned operation;
- records enough irlume-owned state for exact rollback;
- never accepts arbitrary paths or shell arguments from the caller.

### Verify

- compares the transaction's desired state to current daemon, PAM, display
  manager, SELinux, and password-fallback state;
- returns typed checks rather than prose;
- does not report success merely because files exist.

### Rollback

- restores the exact pre-transaction state without overwriting unrelated
  changes;
- reports every restored/skipped/failed operation;
- is idempotent or returns a stable “already rolled back” result;
- remains available through a documented TTY recovery command.

If apply-time verification fails, the result should state whether automatic
rollback completed, partially completed, or failed, using the same typed
rollback result shape.

## Acceptance criteria

- Published schema and compatibility documentation are part of an official
  irlume release.
- Upstream tests freeze success, degraded, error, cancellation, drift,
  verification-failure, and rollback outputs.
- A consumer can implement all listed workflows without parsing prose or
  accessing `/run/irlume.sock`.
- Password fallback is explicitly verified for every successful login
  transaction.

Representative synthetic payloads are available in
`tests/fixtures/irlume/proposed-v1/` in the `plasma-irlume` repository. They
illustrate required semantics and are open to upstream naming adjustments.
