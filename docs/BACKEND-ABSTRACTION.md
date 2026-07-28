# Backend abstraction

`FaceAuthBackend` is the small ownership boundary between the Plasma frontend
and a face-authentication implementation. Its single refresh operation returns
a typed `EngineSnapshot`; it does not expose transport details or raw backend
documents.

The current `IrlumeBackend` implementation performs the Contract 1 handshake,
capability-gates four read-only calls, validates their common envelopes, and
maps accepted data into backend-neutral types. Compatibility follows the
advertised contract range and capabilities. `engine_version` is informational,
so a future version is not rejected merely because its SemVer changed.

Mutation support is an explicit capability in the neutral model and is always
false for Contract 1. Unknown, similarly named advertised capabilities cannot
change it. The existing presentation models therefore remain usable while all
unsupported operations fail closed.

A future backend can implement the same interface without changing QML. That
does not imply that camera capture, recognition, template protection, a daemon,
or PAM integration exists today.
