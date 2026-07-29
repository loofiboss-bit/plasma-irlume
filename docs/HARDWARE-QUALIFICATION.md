# Hardware qualification

Automated, container, and CI measurements are build-environment evidence only.
They do not qualify a camera, a workstation, general detector accuracy,
demographic behavior, FAR, FRR, liveness, or authentication suitability.

## Developer benchmark

Build the non-installed benchmark and run it against the source model:

```bash
cargo run --manifest-path engine/Cargo.toml --release --offline \
  --bin kfaceauth-yunet-benchmark -- \
  --model-root "$PWD/models" \
  --width 640 --height 480 --format rgb8 \
  --warm-up 10 --iterations 100
```

The command uses a synthetic, non-personal frame and emits JSON containing
provider initialization, first inference, median, p95, worst, dimensions,
format, architecture, OpenCV version, exact model hash, iteration counts, and
peak resident memory where the platform exposes it. It never emits pixels,
rectangles, landmarks, scores, or camera identifiers. It does not tune any
runtime parameter.

### Recorded build-environment result

On 2026-07-29, the release benchmark ran in the Fedora 44 container image on an
x86_64 host exposing an 11th Gen Intel Core i5-1145G7 (4 cores/8 threads).
The container used OpenCV 4.13.0-1.fc44, Rust 1.97.1, GCC 16.1.1, a 640x480
RGB8 synthetic frame, 10 warm-up iterations, and 100 measured iterations:

| Metric | Result |
|---|---:|
| provider initialization | 15.151 ms |
| first inference | 41.384 ms |
| cold start | 56.535 ms |
| median inference | 26.958 ms |
| p95 inference | 44.163 ms |
| worst inference | 65.248 ms |
| peak resident memory | 53,208 KiB |

Model SHA-256:
`8f2383e4dd3cfbb4553ea8718107fc0423210dc964f9f4280604804ed2552fa4`.
This is a container/build result with a synthetic no-face frame, not camera or
accuracy qualification.

Record whether a result came from native hardware, a virtual machine, a
container, or CI. Record CPU model, core count, Fedora release, kernel,
OpenCV/KFaceAuth build, power mode, and whether the system was otherwise idle.

## Manual Fedora 44 procedure

Use only consenting participants and do not retain screenshots or camera
frames.

1. Install the normally dependency-resolved RPM on a Fedora 44 test system.
2. Open System Settings, select Camera Check, and record the hardware and
   software environment without recording a stable device node or serial.
3. For an RGB camera, start preview explicitly, analyze one frame explicitly,
   stop preview, and verify that no continuous inference occurs.
4. Repeat with an IR camera if available. Record that IR coverage was absent
   when it was not tested; do not infer it from RGB behavior.
5. Exercise normal and low room lighting with zero, one, and multiple
   consenting faces. Record only the result category and failure code, never
   rectangles, scores, or images.
6. With one participant, exercise ordinary glasses, small pose changes,
   ordinary appearance variation, near-edge framing, and motion. Treat results
   as detector observations only.
7. During active inference, issue another Analyze request. Confirm that the old
   worker is cancelled and only the newest generation can update the UI.
8. Repeat preview start/stop, page hiding, application deactivation, and KCM
   teardown at least 20 times. Confirm the worker exits, the last result clears,
   and no camera process remains.
9. Observe CPU use, peak memory, interaction latency, fan/power behavior, and
   UI responsiveness. Run the developer benchmark separately for numeric
   latency; do not derive thresholds from it.
10. Inspect the journal and a redacted support report. Confirm there are no
    pixels, coordinates, landmarks, scores, per-user results, raw camera
    identifiers, model tensors, or user paths.

## Result record

For each tested combination, record:

- date and tester;
- Fedora, kernel, architecture, CPU, OpenCV, and KFaceAuth versions;
- RGB or IR class without a persistent device identifier;
- lighting category and participant count;
- explicit action/result/error and cancellation outcome;
- median/p95/worst benchmark latency and peak memory;
- UI responsiveness observations;
- untested conditions and failures.

Passing this procedure permits only a narrowly scoped detector qualification
for the recorded system. It does not authorize embeddings, enrollment,
persistence, liveness claims, PAM, or authentication.
