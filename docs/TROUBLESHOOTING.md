# Troubleshooting

## Native Camera Check

| Error | Meaning and action |
| --- | --- |
| `permission-denied` | Camera access was denied. Grant access in the desktop prompt and refresh. |
| `no-camera` | No compatible local capture node remains available. Reconnect the camera and refresh. |
| `camera-busy` | Another application owns the selected camera. Stop that application and retry. |
| `camera-unavailable` | The selected local node could not be opened. Refresh discovery. |
| `format-unavailable` | The device advertises no usable preview format. Try another device. |
| `startup-timeout` | Capture did not start within five seconds. |
| `stream-stalled` | An active preview delivered no frame for three seconds. |
| `protocol-error` | The private worker returned malformed, oversized, or out-of-sequence data. |
| `worker-crashed` | The worker exited or did not acknowledge stop within one second. |

Every preview error clears the frame. Refresh starts a new private worker when
the previous worker was terminated.

## irlume diagnostics

| Error | Meaning |
| --- | --- |
| `engine-not-installed` | `/usr/bin/irlume` is not executable. Install `irlume >= 0.7.0`. |
| `unsupported-contract` | The advertised contract range does not include Contract 1. |
| `invalid-handshake` | Required contract, capability, or public-limit data is malformed or missing. |
| `invalid-json-document` | The engine did not return one valid JSON object. |
| `engine-timeout` | A fixed read-only command exceeded its timeout. |
| `engine-output-too-large` | A response exceeded the accepted output bound. |

Native preview errors do not erase otherwise valid irlume diagnostic data.
