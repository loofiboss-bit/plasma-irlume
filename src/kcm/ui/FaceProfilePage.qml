// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property
// qmllint disable import
// qmllint disable unresolved-type

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kfaceauth 4.0

Kirigami.ScrollablePage {
    id: root

    required property QtObject cameraPreviewSession
    required property QtObject enrollmentSession

    title: i18n("Face Profile")
    padding: Kirigami.Units.largeSpacing

    onVisibleChanged: {
        enrollmentSession.setPageActive(visible)
        if (visible && (cameraPreviewSession.state === 0 || cameraPreviewSession.state === 6)) {
            cameraPreviewSession.refreshDevices()
        } else if (!visible) {
            cameraPreviewSession.stopPreview()
        }
    }

    QQC2.Dialog {
        id: deleteConfirmation
        objectName: "deleteProfileConfirmation"
        parent: QQC2.Overlay.overlay
        modal: true
        title: i18n("Delete face profile?")
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
        onAccepted: enrollmentSession.deleteProfile()

        QQC2.Label {
            width: Math.min(Kirigami.Units.gridUnit * 28, root.width)
            text: i18n("This deletes the encrypted local profile. Storage hardware, snapshots, backups, SSDs, and copy-on-write filesystems may retain physical copies.")
            wrapMode: Text.Wrap
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
    }

    QQC2.Dialog {
        id: resetConfirmation
        objectName: "resetProfileConfirmation"
        parent: QQC2.Overlay.overlay
        modal: true
        title: i18n("Reset unreadable profile data?")
        standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
        onAccepted: enrollmentSession.resetUnreadable()

        QQC2.Label {
            width: Math.min(Kirigami.Units.gridUnit * 28, root.width)
            text: i18n("The unreadable vault and its KWallet key will be removed. This cannot recover the profile; you must enroll again.")
            wrapMode: Text.Wrap
            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: i18n("Face Profile")
            wrapMode: Text.Wrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Warning
            text: i18n("Face embeddings are sensitive biometric data. KFaceAuth encrypts them with a user-session KWallet key. No captured image is intentionally saved.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Enrollment enables only an experimental comparison inside this page. It does not enable login, unlock, PAM, sudo, Polkit, liveness, or spoof resistance.")
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Enrollment profile status")

            contentItem: ColumnLayout {
                QQC2.Label {
                    Layout.fillWidth: true
                    text: enrollmentSession.profileState === 2
                        ? i18n("No face profile is enrolled.")
                        : enrollmentSession.profileState === 3
                            ? i18np("%1 encrypted sample is enrolled.", "%1 encrypted samples are enrolled.", enrollmentSession.storedSampleCount)
                            : enrollmentSession.profileState === 4
                                ? i18n("The encrypted profile is unreadable or corrupt.")
                                : enrollmentSession.profileState === 5
                                    ? i18n("The profile model version does not match this runtime.")
                                    : enrollmentSession.profileState === 6
                                        ? i18n("KWallet is locked or access was cancelled.")
                                        : enrollmentSession.profileState === 7
                                            ? i18n("Profile status is unavailable.")
                                            : i18n("Checking profile status…")
                    wrapMode: Text.Wrap
                    Accessible.role: Accessible.StaticText
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Button {
                        id: refreshStatusButton
                        objectName: "refreshStatusButton"
                        text: i18n("Refresh status")
                        icon.name: "view-refresh"
                        enabled: !enrollmentSession.busy
                        Accessible.name: text
                        KeyNavigation.right: deleteButton
                        onClicked: enrollmentSession.refreshProfileStatus()
                    }

                    QQC2.Button {
                        id: deleteButton
                        objectName: "deleteProfileButton"
                        text: i18n("Delete face profile")
                        icon.name: "edit-delete"
                        enabled: enrollmentSession.profileState === 3 && !enrollmentSession.busy
                        Accessible.name: text
                        KeyNavigation.left: refreshStatusButton
                        KeyNavigation.right: resetButton
                        onClicked: deleteConfirmation.open()
                    }

                    QQC2.Button {
                        id: resetButton
                        objectName: "resetProfileButton"
                        text: i18n("Reset unreadable data")
                        icon.name: "edit-clear-all"
                        enabled: (enrollmentSession.profileState === 4
                                  || enrollmentSession.profileState === 5
                                  || enrollmentSession.profileState === 7)
                            && !enrollmentSession.busy
                        Accessible.name: text
                        KeyNavigation.left: deleteButton
                        onClicked: resetConfirmation.open()
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Private enrollment preview")

            contentItem: ColumnLayout {
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(Kirigami.Units.gridUnit * 12, width * 0.75)

                    CameraPreview {
                        anchors.fill: parent
                        session: root.cameraPreviewSession
                        mirrored: true
                        Accessible.name: i18n("Private enrollment camera preview")
                    }

                    QQC2.Label {
                        anchors.centerIn: parent
                        visible: !cameraPreviewSession.frameAvailable
                        text: i18n("Preview is off")
                        color: "white"
                        font.weight: Font.DemiBold
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Button {
                        id: previewButton
                        objectName: "previewButton"
                        text: cameraPreviewSession.state === 4 ? i18n("Stop preview") : i18n("Start preview")
                        icon.name: cameraPreviewSession.state === 4 ? "media-playback-stop" : "camera-photo"
                        enabled: cameraPreviewSession.state === 4
                            || (cameraPreviewSession.state === 2 && cameraPreviewSession.selectedDeviceIndex >= 0)
                        Accessible.name: text
                        KeyNavigation.right: startEnrollmentButton
                        onClicked: cameraPreviewSession.state === 4
                            ? cameraPreviewSession.stopPreview()
                            : cameraPreviewSession.startPreview()
                    }

                    QQC2.Button {
                        id: startEnrollmentButton
                        objectName: "startEnrollmentButton"
                        text: i18n("Start enrollment")
                        icon.name: "list-add-user"
                        enabled: cameraPreviewSession.state === 4
                            && !enrollmentSession.busy
                            && enrollmentSession.state !== 2
                            && enrollmentSession.state !== 3
                            && enrollmentSession.state !== 4
                        Accessible.name: text
                        KeyNavigation.left: previewButton
                        KeyNavigation.right: captureButton
                        onClicked: enrollmentSession.startEnrollment()
                    }

                    QQC2.Button {
                        id: captureButton
                        objectName: "captureButton"
                        text: i18n("Capture sample")
                        icon.name: "camera-photo"
                        enabled: enrollmentSession.canCapture
                        Accessible.name: text
                        KeyNavigation.left: startEnrollmentButton
                        onClicked: enrollmentSession.captureSample()
                    }
                }

                QQC2.ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: enrollmentSession.maximumSamples
                    value: enrollmentSession.sampleCount
                    Accessible.name: i18n("Enrollment sample progress")
                    Accessible.description: i18n("%1 of %2 maximum samples; five are recommended", enrollmentSession.sampleCount, enrollmentSession.maximumSamples)
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18np("%1 sample accepted", "%1 samples accepted", enrollmentSession.sampleCount)
                    wrapMode: Text.Wrap
                    Accessible.role: Accessible.StaticText
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    visible: enrollmentSession.remainingSeconds > 0
                    text: i18n("Enrollment time remaining: %1 seconds", enrollmentSession.remainingSeconds)
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: enrollmentSession.statusText
                    color: enrollmentSession.errorCode.length > 0
                        ? Kirigami.Theme.negativeTextColor
                        : Kirigami.Theme.textColor
                    wrapMode: Text.Wrap
                    Accessible.role: Accessible.StaticText
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("Guidance: keep exactly one face visible, use even light, center the face, and vary ordinary pose or appearance between captures.")
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Button {
                        id: retryButton
                        objectName: "retrySampleButton"
                        text: i18n("Retry sample")
                        icon.name: "edit-undo"
                        enabled: enrollmentSession.sampleCount > 0 && !enrollmentSession.busy
                        Accessible.name: text
                        KeyNavigation.right: cancelButton
                        onClicked: enrollmentSession.discardLastSample()
                    }

                    QQC2.Button {
                        id: cancelButton
                        objectName: "cancelEnrollmentButton"
                        text: i18n("Cancel")
                        icon.name: "dialog-cancel"
                        enabled: enrollmentSession.state >= 1 && enrollmentSession.state <= 5
                        Accessible.name: text
                        KeyNavigation.left: retryButton
                        KeyNavigation.right: finishButton
                        onClicked: enrollmentSession.cancel()
                    }

                    Item { Layout.fillWidth: true }

                    QQC2.Button {
                        id: finishButton
                        objectName: "finishEnrollmentButton"
                        text: i18n("Finish and save")
                        icon.name: "document-save"
                        enabled: enrollmentSession.canFinish
                        Accessible.name: text
                        KeyNavigation.left: cancelButton
                        onClicked: enrollmentSession.finishAndSave()
                    }
                }
            }
        }
    }
}
