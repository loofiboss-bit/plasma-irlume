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
    required property QtObject profileModel
    required property QtObject authConfiguration
    required property QtObject cameraPreviewSession
    required property bool refreshActive
    required property bool partialDiagnostics
    property var openCamera: () => {}
    property var openProfiles: () => {}
    property var openAccess: () => {}
    property var refresh: () => {}

    readonly property bool engineReady: systemState.engineStatus === 0
        && systemState.daemonStatus === 0
        && profileModel.readOnlyAvailable
    readonly property bool cameraFound: cameraPreviewSession.deviceCount > 0
    readonly property bool profileFound: profileModel.profileCount > 0
    readonly property int currentStep: !engineReady ? 0
        : (!cameraFound ? 1
        : (!profileFound ? 2 : 3))
    readonly property string nextAction: !engineReady
        ? i18n("Check the read-only irlume connection")
        : (!cameraFound
            ? i18n("Check the local camera")
            : (!profileFound
                ? i18n("Review face profiles")
                : i18n("Review Face Login wiring")))

    title: i18n("Setup & Status")
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: root.nextAction

            contentItem: GridLayout {
                columns: width < Kirigami.Units.gridUnit * 28 ? 1 : 2
                columnSpacing: Kirigami.Units.largeSpacing

                ColumnLayout {
                    Layout.fillWidth: true

                    QQC2.Label {
                        text: i18n("Next action")
                        color: Kirigami.Theme.disabledTextColor
                    }

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 1
                        text: root.nextAction
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: systemState.summary
                        wrapMode: Text.Wrap
                    }
                }

                QQC2.Button {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    text: root.currentStep === 0 ? i18n("Refresh status")
                        : (root.currentStep === 1 ? i18n("Open Camera Check")
                        : (root.currentStep === 2 ? i18n("Open Face Profiles") : i18n("Open Access")))
                    icon.name: root.currentStep === 0 ? "view-refresh" : "go-next"
                    enabled: root.currentStep !== 0 || !root.refreshActive
                    onClicked: {
                        if (root.currentStep === 0) {
                            root.refresh();
                        } else if (root.currentStep === 1) {
                            root.openCamera();
                        } else if (root.currentStep === 2) {
                            root.openProfiles();
                        } else {
                            root.openAccess();
                        }
                    }
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: root.partialDiagnostics
            type: Kirigami.MessageType.Warning
            text: i18n("Some read-only irlume sections are unavailable. Available sections remain independent.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Camera Check is local and separate from irlume. Finding a camera does not prove that Face Login is secure or enabled.")
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Readiness summary")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("irlume engine")
                    value: systemState.engineStatusLabel
                    tone: systemState.engineStatus === 0 ? 1 : 3
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Engine camera capability")
                    value: systemState.cameraStatusLabel
                    tone: systemState.cameraType === 0 || systemState.cameraType === 1 ? 1 : 0
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Local cameras")
                    value: i18np("%1 camera found", "%1 cameras found", cameraPreviewSession.deviceCount)
                    tone: cameraPreviewSession.deviceCount > 0 ? 1 : 0
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Face profiles")
                    value: i18np("%1 profile", "%1 profiles", profileModel.profileCount)
                    tone: profileModel.readOnlyAvailable ? 1 : 0
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Authentication wiring")
                    value: systemState.pamStatusLabel
                    tone: systemState.pamStatus === 0 ? 1 : 0
                }
            }
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: i18n("Setup path")
        }

        Repeater {
            model: [
                { title: i18n("Read-only engine diagnostics"), icon: "system-software-update", done: root.engineReady },
                { title: i18n("Local camera discovery"), icon: "camera-photo", done: root.cameraFound },
                { title: i18n("Read-only face profiles"), icon: "user-identity", done: root.profileFound },
                { title: i18n("Read-only Face Login wiring"), icon: "preferences-system-login",
                  done: root.authConfiguration.lockScreenEnabled || root.authConfiguration.loginScreenEnabled }
            ]

            delegate: Kirigami.AbstractCard {
                required property var modelData
                required property int index

                Layout.fillWidth: true
                Accessible.role: Accessible.ListItem
                Accessible.name: modelData.title

                contentItem: RowLayout {
                    Kirigami.Icon {
                        Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                        Layout.preferredHeight: width
                        source: modelData.done ? "emblem-checked" : modelData.icon
                        color: modelData.done ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.textColor
                        Accessible.ignored: true
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: i18n("%1. %2", index + 1, modelData.title)
                        font.weight: index === root.currentStep ? Font.DemiBold : Font.Normal
                        wrapMode: Text.Wrap
                    }

                    Components.StatusPill {
                        text: modelData.done ? i18n("Available")
                            : (index === root.currentStep ? i18n("Current") : i18n("Not established"))
                        tone: modelData.done ? 1 : (index === root.currentStep ? 2 : 0)
                    }
                }
            }
        }
    }
}
