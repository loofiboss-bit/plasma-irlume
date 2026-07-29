# User guide

KFaceAuth is an experimental local identity page in KDE System Settings. It
does not enable login or authentication.

## Camera Check

Refresh cameras, start preview explicitly, then choose **Analyze current
frame** for neutral YuNet face-presence and image-quality guidance. Preview
stops after 60 seconds and on page/app teardown.

## Face Profile

Face embeddings are sensitive biometric data. KFaceAuth intentionally stores
no captured image. It encrypts one current-user profile with AES-256-GCM and a
random key held only in your logged-in KWallet session.

1. Unlock KWallet when requested.
2. Start preview, then select **Start enrollment**.
3. Select **Capture sample** separately for each appearance. Exactly one
   centered, sufficiently large face is required.
4. Three samples are required, five are recommended, and eight is the hard
   storage maximum.
5. Choose **Retry sample** to discard the latest accepted sample, **Cancel** to
   discard the entire uncommitted session, or **Finish and save** for one
   atomic encrypted commit.

Enrollment expires after 120 seconds and is cancelled when its page, preview,
application, or KCM becomes inactive. Nothing is partially saved before Finish.

Status shows only absent/ready/unreadable/model-mismatch/unavailable and a
bounded sample count. **Delete face profile** authenticates and removes a valid
profile. **Reset unreadable data** destructively removes an unreadable file and
its KWallet key after confirmation; re-enrollment is then required. Deletion
cannot promise physical erasure from SSDs, snapshots, backups, journals, or
copy-on-write storage.

## Test Recognition

Start preview and choose **Test one current frame**. The current frame is
processed once, the encrypted current-user profile is opened, and the UI can
show Match, No match, Ambiguous, No profile, Vault locked, Model mismatch,
Unavailable, Cancelled, or Internal failure.

Scores are intentionally hidden. Requests are rate-limited. A Match changes
only this page: it cannot unlock, authenticate, authorize, call PAM/Polkit, or
alter the Linux session.

There is no liveness or spoof resistance. KWallet keys are unavailable before
login. FAR, FRR, bias, RGB/IR security, and authentication suitability remain
unqualified. Password fallback is irrelevant because KFaceAuth does not modify
authentication.
