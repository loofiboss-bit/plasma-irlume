# Test matrix

The v3.0.0 automated suite requires no physical camera and covers:

- provider classification for RGB, IR, and Unknown without name heuristics;
- bounded format selection, 640×480 scaling, and the 128 KiB JPEG ceiling;
- CBOR fragmentation, invalid and oversized records, session and monotonic
  sequence validation, fixed command shape, and free-argument rejection;
- discovery, start, frame delivery, stop, repeated start, worker crash,
  startup timeout, stream stall, missing stop acknowledgement, simulated
  60-second limit, hot-unplug, busy device, frame clearing, and stable errors;
- latest-frame backpressure and bounded device/control queues;
- QML creation at 320/480/960 px, keyboard focus, accessible camera names,
  Swedish localization, and automatic stop when Camera Check is hidden;
- unchanged Contract 1 command, generation, timeout, schema, redaction, and
  read-only presentation coverage;
- source archive, SRPM/RPM, rpmlint, dependency/payload inspection, absence of
  privileged scriptlets, setuid bits and file capabilities, and isolated
  install/upgrade/remove checks with PAM and user-data sentinels.

Release qualification additionally requires text-only verification on the
local Fedora 44 HP RGB/IR pair: both capture nodes must be classified, preview,
and release cleanly. Public release gates require exact tag/CI lineage,
checksums, RPM/SRPM inspection, COPR API `succeeded`, published 3.0.0 metadata,
a clean Fedora 44 install, and an upgrade from public v2.0.0. These public
gates are not satisfied by a local build.

Run the local text-only camera check after building:

```bash
QT_QPA_PLATFORM=offscreen build/bin/camera_hardware_check
```

Its output contains only counts, RGB/IR/Unknown, stable errors, and pass/fail
release results—never device identifiers, labels, or images.
