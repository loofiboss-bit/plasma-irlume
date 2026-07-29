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
    required property QtObject visionAnalysisSession

    title: i18n("Camera Check")
    padding: Kirigami.Units.largeSpacing

    onVisibleChanged: {
        if (visible && (cameraPreviewSession.state === 0 || cameraPreviewSession.state === 6)) {
            cameraPreviewSession.refreshDevices();
        } else if (!visible) {
            visionAnalysisSession.cancelAnalysis();
            cameraPreviewSession.stopPreview();
        }
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: i18n("Camera Check")
            wrapMode: Text.Wrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Preview starts only when you ask. Frames remain in memory, stop after 60 seconds, and are never used for enrollment or recognition.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Warning
            text: i18n("A visible RGB or infrared image does not verify liveness, identity, security level, or authentication readiness.")
        }

        RowLayout {
            Layout.fillWidth: true

            QQC2.ComboBox {
                id: deviceSelector
                objectName: "cameraDeviceSelector"

                Layout.fillWidth: true
                model: cameraPreviewSession
                textRole: "label"
                currentIndex: cameraPreviewSession.selectedDeviceIndex
                enabled: cameraPreviewSession.state === 2
                readonly property string accessibilityLabel: i18n("Local camera")
                Accessible.name: accessibilityLabel
                onActivated: index => cameraPreviewSession.selectedDeviceIndex = index
            }

            QQC2.Button {
                objectName: "cameraRefreshButton"
                text: i18n("Refresh")
                icon.name: "view-refresh"
                enabled: cameraPreviewSession.state !== 1
                    && cameraPreviewSession.state !== 3
                    && cameraPreviewSession.state !== 4
                    && cameraPreviewSession.state !== 5
                Accessible.name: text
                onClicked: cameraPreviewSession.refreshDevices()
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Private camera preview")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(Kirigami.Units.gridUnit * 12, width * 0.75)

                    CameraPreview {
                        anchors.fill: parent
                        session: root.cameraPreviewSession
                        mirrored: true
                        Accessible.name: cameraPreviewSession.spectrum === "ir"
                            ? i18n("Infrared camera preview")
                            : (cameraPreviewSession.spectrum === "rgb"
                                ? i18n("RGB camera preview")
                                : i18n("Camera preview"))
                    }

                    QQC2.Label {
                        anchors.centerIn: parent
                        visible: !cameraPreviewSession.frameAvailable
                        text: cameraPreviewSession.state === 3
                            ? i18n("Starting camera…")
                            : i18n("Preview is off")
                        color: "white"
                        font.weight: Font.DemiBold
                    }

                    QQC2.Label {
                        anchors {
                            top: parent.top
                            right: parent.right
                            margins: Kirigami.Units.smallSpacing
                        }
                        visible: cameraPreviewSession.state === 4
                        padding: Kirigami.Units.smallSpacing
                        text: i18n("%1 s", cameraPreviewSession.remainingSeconds)
                        color: "white"
                        background: Rectangle {
                            color: "#a0000000"
                            radius: Kirigami.Units.cornerRadius
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.BusyIndicator {
                        visible: cameraPreviewSession.state === 1
                            || cameraPreviewSession.state === 3
                            || cameraPreviewSession.state === 5
                        running: visible
                        Accessible.ignored: true
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: cameraPreviewSession.statusText
                        color: cameraPreviewSession.errorCode.length > 0
                            ? Kirigami.Theme.negativeTextColor
                            : Kirigami.Theme.disabledTextColor
                        wrapMode: Text.Wrap
                        Accessible.role: Accessible.StaticText
                    }

                    QQC2.Button {
                        objectName: "cameraPreviewAction"
                        text: cameraPreviewSession.state === 4 ? i18n("Stop preview") : i18n("Start preview")
                        icon.name: cameraPreviewSession.state === 4 ? "media-playback-stop" : "camera-photo"
                        enabled: cameraPreviewSession.state === 4
                            || (cameraPreviewSession.state === 2
                                && cameraPreviewSession.selectedDeviceIndex >= 0)
                        Accessible.name: text
                        onClicked: {
                            if (cameraPreviewSession.state === 4) {
                                cameraPreviewSession.stopPreview();
                            } else {
                                cameraPreviewSession.startPreview();
                            }
                        }
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    visible: cameraPreviewSession.droppedFrames > 0
                    text: i18n("Dropped preview frames: %1", cameraPreviewSession.droppedFrames)
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.Wrap
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: true
                    type: Kirigami.MessageType.Information
                    text: i18n("Analyze uses local YuNet face detection for one current frame. It does not identify a person, test liveness, or make an authentication decision.")
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.BusyIndicator {
                        visible: visionAnalysisSession.busy
                        running: visible
                        Accessible.ignored: true
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: visionAnalysisSession.statusText
                        color: visionAnalysisSession.errorCode.length > 0
                            ? Kirigami.Theme.negativeTextColor
                            : Kirigami.Theme.disabledTextColor
                        wrapMode: Text.Wrap
                        Accessible.role: Accessible.StaticText
                    }

                    QQC2.Button {
                        objectName: "visionAnalyzeAction"
                        text: i18n("Analyze current frame")
                        icon.name: "view-preview"
                        enabled: visionAnalysisSession.canAnalyze
                        Accessible.name: text
                        onClicked: visionAnalysisSession.analyzeCurrentFrame()
                    }
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: visionAnalysisSession.resultAvailable
                    type: Kirigami.MessageType.Information
                    text: visionAnalysisSession.faceFinding === 1
                        ? i18n("No face was found in this frame.")
                        : (visionAnalysisSession.faceFinding === 2
                            ? i18n("One face was found in this frame.")
                            : i18n("Multiple faces were found in this frame."))
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: visionAnalysisSession.resultAvailable
                        && visionAnalysisSession.faceFinding === 2
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.position === 2
                        text: i18n("Center your face in the frame.")
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.distance === 2
                        text: i18n("Move closer to the camera.")
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.distance === 3
                        text: i18n("Move farther from the camera.")
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.brightness === 1
                        text: i18n("Add more even light.")
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.brightness === 3
                        text: i18n("Reduce bright light on your face.")
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.contrast === 1
                        text: i18n("Use more even front lighting.")
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.sharpness === 1
                        text: i18n("Hold still and check that the camera lens is clear.")
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: visionAnalysisSession.position === 1
                            && visionAnalysisSession.distance === 1
                            && visionAnalysisSession.brightness === 2
                            && visionAnalysisSession.contrast === 2
                            && visionAnalysisSession.sharpness === 2
                        text: i18n("Framing and image quality are suitable for this development check.")
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }
}
