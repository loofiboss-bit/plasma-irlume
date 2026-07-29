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
            text: i18n("KFaceAuth 4.0.0 provides explicit local enrollment and in-session comparison. It does not test liveness, configure PAM, authenticate users, unlock the session, or authorize any system action.")
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

                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Button {
                        objectName: "overviewRefreshButton"
                        text: root.refreshActive ? i18n("Updating…") : i18n("Refresh status")
                        icon.name: "view-refresh"
                        enabled: !root.refreshActive
                        Accessible.name: text
                        onClicked: root.refresh()
                    }

                    QQC2.Button {
                        objectName: "openCameraButton"
                        text: i18n("Open Camera Check")
                        icon.name: "camera-photo"
                        Accessible.name: text
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
                    value: i18n("Explicit local actions only")
                    tone: 1
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Enrollment")
                    value: systemState.enrollmentStatusLabel
                    tone: systemState.enrollmentStatus === 0 ? 1 : 2
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
