# Backend abstraction

`FaceAuthBackend` is an asynchronous `QObject` contract. Consumers call
`requestRefresh(generation)` or `cancelRefresh()` and receive generation-tagged
progress, completion, or cancellation signals.

The public types are:

- `EngineOperation`: handshake, status, doctor, profiles, and login status.
- `ResultState`: not advertised, pending, loading, available, or failed.
- `EngineFeature`: four known read features plus separate future mutation
  features that remain disabled for Contract 1.
- `OperationResult<T>`: typed data and at most one operation-scoped
  `EngineError`.

Unknown advertised capabilities are retained for diagnostics but never map to
a known feature. `contractAvailable` means only that Contract 1 negotiation
succeeded. It does not imply that any read command or mutation is available.

Production construction goes through `createProductionFaceAuthBackend()` and
always returns `IrlumeBackend` for `/usr/bin/irlume`. Tests inject an owned fake
backend through the KCM constructor.
