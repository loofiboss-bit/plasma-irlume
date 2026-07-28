# Troubleshooting

| Error | Meaning |
| --- | --- |
| `engine-not-installed` | `/usr/bin/irlume` is not executable. Install `irlume >= 0.7.0`. |
| `unsupported-contract` | The advertised contract range does not include Contract 1. |
| `invalid-handshake` | Required contract, capability, or public-limit data is malformed or missing. |
| `invalid-json-document` | The engine did not return one valid JSON object. |
| `engine-timeout` | A fixed read-only command exceeded the bounded timeout. |
| `engine-output-too-large` | A response exceeded the accepted output bound. |
| `capability-unavailable` | The requested read section or mutation operation is not advertised. |

Unknown state is not treated as zero or as success. Refresh after correcting
the engine installation. Support reports contain typed, redacted state rather
than raw command output.
