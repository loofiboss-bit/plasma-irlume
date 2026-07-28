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
    required property QtObject cameraConfiguration
    property var openProfiles: () => {}
    property var openAccess: () => {}
    property var refresh: () => {}

    readonly property bool engineReady: systemState.engineStatus === 0
        && systemState.daemonStatus === 0
        && profileModel.readOnlyAvailable
    readonly property bool cameraReady: cameraConfiguration.ready
        && cameraConfiguration.emitterTested
        && cameraConfiguration.emitterAvailable
    readonly property bool profileReady: profileModel.profileCount > 0
    readonly property bool recoveryReady: authConfiguration.recoveryAcknowledged
    readonly property int currentStep: !engineReady ? 0
        : (!cameraReady ? 1
        : (!profileReady ? 2
        : (!recoveryReady ? 4 : 5)))
    readonly property string nextAction: !engineReady
        ? i18n("Install a Contract 1 compatible backend")
        : (!cameraReady
        ? i18n("Connect and test a supported camera")
        : (!profileReady
        ? i18n("Register your first face profile")
        : (!recoveryReady
        ? i18n("Review recovery before enabling")
        : i18n("Review and enable Face Login"))))

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
                rowSpacing: Kirigami.Units.smallSpacing

                ColumnLayout {
                    Layout.fillWidth: true

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: i18n("Next action")
                        color: Kirigami.Theme.disabledTextColor
                        wrapMode: Text.Wrap
                    }

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 1
                        text: root.nextAction
                        wrapMode: Text.Wrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: root.systemState.summary
                        wrapMode: Text.Wrap
                    }
                }

                QQC2.Button {
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    text: root.currentStep < 2 ? i18n("Check again")
                        : (root.currentStep === 2 ? i18n("Open Face Profiles") : i18n("Open Access"))
                    icon.name: root.currentStep < 2 ? "view-refresh" : "go-next"
                    onClicked: {
                        if (root.currentStep < 2) {
                            root.refresh();
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
            visible: !root.profileModel.mutationSupported && !root.profileModel.busy
            type: Kirigami.MessageType.Warning
            text: i18n("irlume Contract 1 is read-only. Enrollment and authentication changes remain disabled.")
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Camera configuration")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true

                        QQC2.Label {
                            text: i18n("Secure camera pair")
                            font.weight: Font.DemiBold
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: root.cameraConfiguration.statusText
                            color: root.cameraConfiguration.errorCode.length > 0
                                ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
                            wrapMode: Text.Wrap
                        }
                    }

                    QQC2.BusyIndicator {
                        visible: running
                        running: root.cameraConfiguration.busy
                    }
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: !root.cameraConfiguration.mutationSupported
                        && !root.cameraConfiguration.busy
                    type: Kirigami.MessageType.Warning
                    text: i18n("Contract 1 reports camera capability but does not support camera configuration.")
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: true

                    QQC2.ComboBox {
                        id: cameraPair

                        Layout.fillWidth: true
                        model: root.cameraConfiguration.pairLabels
                        currentIndex: root.cameraConfiguration.selectedPairIndex
                        enabled: root.cameraConfiguration.mutationSupported
                            && !root.cameraConfiguration.busy
                            && root.cameraConfiguration.pairLabels.length > 0
                        Accessible.name: i18n("Secure camera pair")
                        onActivated: index => root.cameraConfiguration.selectedPairIndex = index
                    }

                    QQC2.Button {
                        text: i18n("Use pair")
                        icon.name: "dialog-ok-apply"
                        enabled: root.cameraConfiguration.mutationSupported
                            && !root.cameraConfiguration.busy
                            && root.cameraConfiguration.selectedPairIndex >= 0
                            && root.cameraConfiguration.selectedPairIndex
                                !== root.cameraConfiguration.activePairIndex
                        onClicked: root.cameraConfiguration.selectPair()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: true

                    QQC2.Button {
                        text: i18n("Check again")
                        icon.name: "view-refresh"
                        enabled: !root.cameraConfiguration.busy
                        onClicked: root.cameraConfiguration.refresh()
                    }

                    QQC2.Button {
                        text: i18n("Set up emitter")
                        icon.name: "preferences-system-power-management"
                        enabled: root.cameraConfiguration.mutationSupported
                            && !root.cameraConfiguration.busy && root.cameraConfiguration.ready
                        onClicked: root.cameraConfiguration.setupEmitter()
                    }

                    QQC2.Button {
                        text: i18n("Tune capture")
                        icon.name: "configure"
                        enabled: root.cameraConfiguration.mutationSupported
                            && !root.cameraConfiguration.busy && root.cameraConfiguration.ready
                        onClicked: root.cameraConfiguration.tuneCamera()
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    visible: true
                    label: i18n("Active pair")
                    value: root.cameraConfiguration.activePairIndex >= 0
                        ? i18n("Verified") : i18n("Not selected")
                    tone: root.cameraConfiguration.activePairIndex >= 0 ? 1 : 2
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                    visible: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    visible: true
                    label: i18n("Infrared emitter")
                    value: !root.cameraConfiguration.emitterTested ? i18n("Not tested")
                        : (root.cameraConfiguration.emitterAvailable
                        ? i18n("Available") : i18n("Unavailable"))
                    tone: root.cameraConfiguration.emitterAvailable ? 1
                        : (root.cameraConfiguration.emitterTested ? 3 : 0)
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                    visible: root.cameraConfiguration.captureMode.length > 0
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    visible: root.cameraConfiguration.captureMode.length > 0
                    label: i18n("Capture mode")
                    value: root.cameraConfiguration.captureMode
                    tone: root.cameraConfiguration.tuneConclusive ? 1 : 2
                }
            }
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 2
            text: i18n("Setup path")
            wrapMode: Text.Wrap
        }

        Repeater {
            model: [
                { title: i18n("Engine compatibility"), icon: "system-software-update", done: root.engineReady },
                { title: i18n("Camera and infrared test"), icon: "camera-photo", done: root.cameraReady },
                { title: i18n("Face profile registration"), icon: "user-identity", done: root.profileReady },
                { title: i18n("Recognition test"), icon: "security-high", done: root.profileReady },
                { title: i18n("Recovery readiness"), icon: "tools-report-bug", done: root.recoveryReady },
                { title: i18n("Review and enable"), icon: "preferences-system-login",
                  done: root.authConfiguration.lockScreenEnabled || root.authConfiguration.loginScreenEnabled }
            ]

            delegate: Kirigami.AbstractCard {
                required property var modelData
                required property int index

                Layout.fillWidth: true
                Accessible.role: Accessible.ListItem
                Accessible.name: modelData.title + ", "
                    + (modelData.done ? i18n("Complete") : (index === root.currentStep ? i18n("Current") : i18n("Pending")))

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
                        text: modelData.done ? i18n("Complete")
                            : (index === root.currentStep ? i18n("Current") : i18n("Pending"))
                        tone: modelData.done ? 1 : (index === root.currentStep ? 2 : 0)
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Current protection")

            contentItem: ColumnLayout {
                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Security level")
                    value: root.systemState.securityTierLabel
                    tone: root.systemState.securityTier === 0 ? 1 : (root.systemState.securityTier === 1 ? 2 : 3)
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Password fallback")
                    value: root.systemState.passwordFallbackPreserved ? i18n("Available") : i18n("Not verified")
                    tone: root.systemState.passwordFallbackPreserved ? 1 : 3
                }
            }
        }
    }
}
