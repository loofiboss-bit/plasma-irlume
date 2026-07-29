# User guide

KFaceAuth currently appears as a development preview in System Settings.

## Overview

Overview shows whether the native engine skeleton is available and explicitly
marks vision, enrollment, authentication, PAM, and template persistence as not
implemented. The normal Milestone 1 state is `Native engine unavailable`.

## Camera Check

Select **Refresh** to discover local cameras. Select a camera and choose
**Start preview** to begin a private preview. It stops after 60 seconds; you can
also stop it manually or leave the page.

The preview is only a camera and privacy diagnostic. To run detection, choose
**Analyze current frame** separately. This starts one short-lived local YuNet
worker for the current frame; it is not continuous. A new analysis replaces
and cancels an active one.

The result can report zero, one, or multiple detected faces plus neutral image
or framing guidance. It does not recognize a person, test liveness, enroll
anything, or prove authentication readiness. If the verified model or OpenCV
runtime is unavailable, Camera Check shows an error and never substitutes a
simulated result.

## Diagnostics

Diagnostics refreshes bounded local status and can copy or export a redacted
Markdown report. Review the report before sharing it. It intentionally excludes
frames, device identifiers, biometric data, credentials, and local paths.
