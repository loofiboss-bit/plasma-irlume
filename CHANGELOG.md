# Changelog

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
