// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "components" as Components

Kirigami.ScrollablePage {
    id: root

    required property QtObject systemState
    required property QtObject cameraPreviewSession
    required property bool refreshActive
    property var openCamera: () => {}
    property var refresh: () => {}

    title: i18n("Overview")
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Milestone 1 provides local camera discovery and a bounded in-memory preview. It does not detect or recognize faces, test liveness, enroll profiles, store biometric data, configure PAM, or authenticate users.")
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: systemState.headline

            contentItem: ColumnLayout {
                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 1
                    text: systemState.headline
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: systemState.summary
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Button {
                        text: root.refreshActive ? i18n("Updating…") : i18n("Refresh status")
                        icon.name: "view-refresh"
                        enabled: !root.refreshActive
                        onClicked: root.refresh()
                    }

                    QQC2.Button {
                        text: i18n("Open Camera Check")
                        icon.name: "camera-photo"
                        onClicked: root.openCamera()
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Milestone status")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Native engine")
                    value: systemState.engineStatusLabel
                    tone: systemState.engineStatus === 0 ? 1 : 2
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Local cameras")
                    value: i18np("%1 camera found", "%1 cameras found", cameraPreviewSession.deviceCount)
                    tone: cameraPreviewSession.deviceCount > 0 ? 1 : 0
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Vision processing")
                    value: systemState.visionStatusLabel
                    tone: 2
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Enrollment")
                    value: systemState.enrollmentStatusLabel
                    tone: 2
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Authentication and PAM")
                    value: systemState.authenticationStatusLabel
                    tone: 2
                }
            }
        }
    }
}
