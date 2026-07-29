# Hardware qualification

Automated and build-host measurements are not camera, demographic, FAR/FRR,
liveness, spoof, or authentication qualification. Milestone 4 may pass code
gates without hardware evidence, but missing hardware coverage is a release
blocker and must be reported.

## Non-installed evaluator

Use only images whose participants explicitly consented to this evaluation and
whose storage/license terms permit local processing. Do not add the dataset,
manifest, results with identities, screenshots, or camera frames to the
repository.

The manifest is tab-separated: an opaque transient group number followed by an
absolute path to a binary PPM (`P6`) image. Group numbers are used only in
memory for same/different aggregate comparisons and are never emitted.

```bash
cargo run --manifest-path engine/Cargo.toml --release --offline \
  --bin kfaceauth-identity-evaluate -- \
  --model-root "$PWD/models" \
  --dataset-manifest /private/consented-evaluation.tsv
```

JSON output contains only environment, sample/error counts, model
initialization, fresh-provider and reused-provider pipeline
median/p95/worst, first and subsequent fresh-worker-process latency, parent
and worker peak resident memory, aggregate repeated-sample consistency,
aggregate same/different comparisons, and aggregate threshold sweep counts.
The first worker observation is the closest available cold-start measurement;
subsequent fresh processes normally benefit from the operating-system page
cache. Repeated truly cold measurements require an externally controlled
qualification host. The tool never emits an image, embedding, landmark,
participant/group label, path, persistent camera identifier, or per-image
score.

FAR/FRR fields are `unqualified` unless the caller explicitly adds
`--labelled-evaluation` and supplies at least two identities with at least two
accepted samples each. Even then, results describe only that supplied set and
are not product qualification.

## Consent-based manual procedure

1. Use a clean, normally dependency-resolved Fedora 44 installation. Record
   Fedora/kernel/CPU/OpenCV/KFaceAuth versions and RGB/IR class without serial
   number or stable device node.
2. Unlock KWallet, start preview explicitly, start enrollment explicitly, and
   capture every sample with a separate click. Confirm 3 samples enable Finish,
   5 are recommended, 8 is the hard bound, and no continuous capture occurs.
3. Cancel at each stage; hide the page; deactivate System Settings; stop
   preview; replace an active request; wait beyond 120 seconds; and close the
   KCM. Confirm no partial profile, lingering worker, frame, or result.
4. Complete enrollment and repeat explicit one-frame verification under
   ordinary glasses/appearance changes, modest pose, and varied ordinary
   lighting. Record only result categories and aggregate timing.
5. With separately consenting participants, run deliberate wrong-person
   checks. Never retain names, images, per-image scores, or claim FAR/FRR from
   a small convenience sample.
6. Lock KWallet and cancel its access prompt. Confirm stable unavailable
   results and no key-file fallback. Unlock and retry.
7. On an isolated disposable profile, modify/truncate the vault and lose the
   KWallet key. Confirm fail-closed unreadable state, corruption preservation,
   explicit destructive reset, and required re-enrollment.
8. Delete the valid profile, confirm status becomes absent, and re-enroll.
   Confirm deletion makes no physical-erasure claim.
9. Repeat preview/enrollment/test teardown at least 20 times. Confirm workers
   exit and the UI remains responsive.
10. Record CPU, model initialization, cold/warm median/p95/worst latency, peak
    RSS, fan/power behavior, and UI responsiveness. Inspect journal and
    redacted support report for images, keys, embeddings, scores, rectangles,
    identifiers, and user paths.

## Result record and release boundary

Record date, tester, environment, consent scope, camera class, conditions,
aggregate results, cancellation/teardown behavior, performance, failures, and
untested coverage. Do not record biometric material or stable participant
identity.

Missing representative FAR/FRR, bias, broad RGB/IR hardware, accessibility,
and spoof testing remains explicit release-blocker evidence. Passing this
procedure does not authorize liveness or PAM work.
