# User guide

## Camera Check

Open **Camera Check** to discover local cameras. Discovery does not start
capture. Select a device and press **Start preview** to grant access for one
preview session. The preview stops after 60 seconds or immediately when you
press Stop, leave the tab, switch away from System Settings, or close the KCM.

RGB, Infrared, and Unknown describe only the locally observed camera node.
A visible image does not verify liveness, security level, identity matching,
or Face Login readiness. Preview does not enroll or modify a profile.

No approval is remembered. Frames and opaque device tokens remain in memory
and are cleared on stop or failure.

## Read-only engine pages

**Face Profiles** shows the Contract 1 profile list. **Access** shows observed
login and lock-screen wiring. Neither page can change the engine, profiles, or
PAM. **Support** contains redacted diagnostic state but no image, device
identifier, or preview status.

Unknown state is not treated as zero or success. If one irlume section fails,
other valid diagnostic sections and Camera Check remain independent.
