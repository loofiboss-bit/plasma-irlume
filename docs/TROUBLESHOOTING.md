# Troubleshooting

## Native engine unavailable

This is expected in Milestone 1. The engine workspace is source-only and the KCM
does not start it. Camera Check and diagnostics remain responsive. No biometric
or PAM feature should become enabled.

## Camera unavailable

- close other applications using the camera;
- verify the hardware privacy switch;
- select **Refresh**;
- reconnect the device if it is removable;
- inspect the stable preview error code in Diagnostics.

Camera errors do not alter native-engine status. Native-engine errors do not
weaken preview bounds or privacy.

## Face detector unavailable

If Camera Check reports that the verified model is unavailable, reinstall the
KFaceAuth RPM through the Fedora package manager. Do not copy or rename an ONNX
file manually: the worker requires the exact closed inventory, metadata, size,
and SHA-256.

If Camera Check reports invalid detector output, stop preview and retry once.
Persistent failure indicates an unsupported or damaged OpenCV/runtime
installation; record the stable error and verify the package with:

```bash
rpm -V kfaceauth
```

Neither error enables simulated inference.

## Preview stops

Automatic stop after 60 seconds is expected. Hiding the page, deactivating
System Settings, worker failure, or KCM teardown also stops capture and clears
the current frame.

## Build cannot find Qt Multimedia

Install `qt6-qtmultimedia-devel`. The runtime package alone does not contain the
CMake configuration and headers required to build the worker.

## Build cannot find OpenCV 4.13

Install `opencv-devel` from the Fedora 44 repositories. KFaceAuth deliberately
rejects a different OpenCV minor implementation until it has been reviewed.
