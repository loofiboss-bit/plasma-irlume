# Test matrix

| Area | Automated evidence |
|---|---|
| Model supply chain | exact YuNet/SFace names, sizes, hashes, licenses, provenance; missing/renamed/modified/duplicate/unlisted rejection |
| OpenCV bridge | owned input copies, five-landmark alignment, 112×112 BGR crop, 1×128 FP32 feature, exception containment, malformed/non-finite rejection |
| Embeddings | zero norm, non-finite/range failure, deterministic L2 normalization and cosine, model/version binding |
| Identity protocol | closed operations, positive generations, exact bounds, malformed/trailing/oversized frames, fixed response types, no scores |
| Vault crypto | AES-GCM round trip, wrong key, tag/ciphertext/AAD tamper, nonce uniqueness, plaintext/key absence |
| Vault format | schema/UID/model/hash/format/dimension/normalization/sample validation, truncation/oversize, corruption preservation |
| Vault filesystem | owner/mode/type/symlink/hard-link checks, bounded locking, verified atomic write, rollback, rotation, deletion/reset |
| Enrollment | 3–8 bounds, duplicate rejection, one-frame actions, 120-second lifecycle, cancellation, no partial commit |
| Verification | match/no-match/ambiguous thresholds, median aggregation, no-profile/mismatch errors, cancellation, timeout, stale generations, rate limiting |
| KCM/QML | all pages create offscreen, keyboard focus and accessibility names, destructive dialogs, page/app/preview teardown |
| Status/privacy | separate local identity capabilities, PAM/system states unsupported, aggregate-only support reports, no embeddings/scores in QML |
| Security | no PAM/authselect writes, service, privilege, socket/network, runtime download, production fake selector, arbitrary production vault root |
| Packaging | exact dependencies/files/licenses/workers, ordinary permissions, no auth scriptlets, reproducible archive/SRPM/RPM and isolated lifecycle |
| Localization | all active user-visible messages have checked Swedish translations |

Hardware, representative FAR/FRR, demographic/bias behavior, liveness, and
spoof resistance are deliberately separate qualification evidence.
