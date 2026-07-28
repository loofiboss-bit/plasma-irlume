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

## Preview stops

Automatic stop after 60 seconds is expected. Hiding the page, deactivating
System Settings, worker failure, or KCM teardown also stops capture and clears
the current frame.

## Build cannot find Qt Multimedia

Install `qt6-qtmultimedia-devel`. The runtime package alone does not contain the
CMake configuration and headers required to build the worker.
