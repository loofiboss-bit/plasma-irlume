# Changelog

## Milestone 3 development

- Enable real local YuNet face detection through Fedora OpenCV 4.13 and a
  narrow reviewed C ABI bridge.
- Make production Camera Check use the verified real provider with stable
  fail-closed model/runtime errors and replacement cancellation.
- Add explicit preprocessing/postprocessing contracts, native sanitizers,
  adversarial tests, a machine-readable benchmark, and hardware qualification
  procedure.
- Keep deterministic inference test-only and retain all embedding, identity,
  enrollment, persistence, liveness, PAM, service, networking, and
  authentication non-goals.

## Milestone 2 development

- Add explicit bounded one-frame Camera Check analysis through a separate
  unprivileged Rust worker.
- Add backend-neutral typed face-presence and image-quality results with strict
  frame/protocol/cancellation bounds.
- Select and package the MIT-licensed YuNet detector through an offline,
  SHA-256-verified model manifest.
- Keep real inference disabled behind an explicit deterministic provider and
  retain all enrollment, persistence, identity, PAM, and authentication
  non-goals.

## 4.0.0

- Establish the temporary KFaceAuth identity from one central CMake file.
- Replace the external-engine adapter with an asynchronous, fail-closed native
  backend that never starts a face-authentication executable.
- Add source-only Rust `protocol`, `vision`, `templates`, `daemon`, and `cli`
  crates with bounded typed status/capability handling.
- Preserve the unprivileged bounded camera preview and system probing.
- Remove external engine schemas, fixtures, scripts, package requirements,
  identifiers, and compatibility surfaces.
- Keep recognition, liveness, enrollment, template persistence, PAM, and
  authentication decisions explicitly unsupported.
