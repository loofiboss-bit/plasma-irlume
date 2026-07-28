# Changelog

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
