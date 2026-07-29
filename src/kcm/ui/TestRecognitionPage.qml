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
    required property QtObject localVerificationSession

    title: i18n("Test Recognition")
    padding: Kirigami.Units.largeSpacing

    onVisibleChanged: {
        localVerificationSession.setPageActive(visible)
        if (visible && (cameraPreviewSession.state === 0 || cameraPreviewSession.state === 6)) {
            cameraPreviewSession.refreshDevices()
        } else if (!visible) {
            cameraPreviewSession.stopPreview()
        }
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: i18n("Test Recognition")
            wrapMode: Text.Wrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Warning
            text: i18n("A Match is only an experimental in-session comparison. It cannot unlock, authenticate, authorize, call PAM, invoke Polkit, or change the Linux session.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("No liveness or presentation-attack detection exists. Similarity scores are intentionally hidden.")
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Private recognition preview")

            contentItem: ColumnLayout {
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(Kirigami.Units.gridUnit * 12, width * 0.75)

                    CameraPreview {
                        anchors.fill: parent
                        session: root.cameraPreviewSession
                        mirrored: true
                        Accessible.name: i18n("Private recognition camera preview")
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
                        KeyNavigation.right: verifyButton
                        onClicked: cameraPreviewSession.state === 4
                            ? cameraPreviewSession.stopPreview()
                            : cameraPreviewSession.startPreview()
                    }

                    QQC2.Button {
                        id: verifyButton
                        objectName: "verifyButton"
                        text: i18n("Test one current frame")
                        icon.name: "view-preview"
                        enabled: localVerificationSession.canVerify
                        Accessible.name: text
                        KeyNavigation.left: previewButton
                        KeyNavigation.right: clearButton
                        onClicked: localVerificationSession.verifyCurrentFrame()
                    }

                    QQC2.Button {
                        id: clearButton
                        objectName: "clearVerificationButton"
                        text: i18n("Clear result")
                        icon.name: "edit-clear"
                        enabled: !localVerificationSession.busy && localVerificationSession.result !== 0
                        Accessible.name: text
                        KeyNavigation.left: verifyButton
                        onClicked: localVerificationSession.clearResult()
                    }
                }

                QQC2.BusyIndicator {
                    visible: localVerificationSession.busy
                    running: visible
                    Accessible.ignored: true
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: localVerificationSession.result !== 0
                    type: localVerificationSession.result === 1
                        ? Kirigami.MessageType.Positive
                        : localVerificationSession.result === 3
                            ? Kirigami.MessageType.Warning
                            : Kirigami.MessageType.Information
                    text: localVerificationSession.statusText
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    visible: localVerificationSession.result === 0
                    text: localVerificationSession.statusText
                    wrapMode: Text.Wrap
                    Accessible.role: Accessible.StaticText
                }
            }
        }
    }
}
