# Native engine roadmap

## Current milestone: v2.2.0 read-only hardening

The KCM provides asynchronous Contract 1 diagnostics only. It does not access
a camera, capture frames, process biometrics, modify profiles, configure PAM,
or start a privileged component.

## Next milestone

The next milestone is exactly a separate, unprivileged native camera discovery
and bounded ephemeral preview-process without face detection, embeddings,
storage, daemon, PAM, or authentication decisions.

That milestone requires a new explicit implementation plan and is not partly
implemented by v2.2.0.
