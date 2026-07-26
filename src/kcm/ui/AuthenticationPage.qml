// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: root

    required property QtObject systemState
    required property QtObject authConfiguration

    title: i18n("Access")
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: i18n("Access")
            wrapMode: Text.Wrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Review and enable Face Login for each supported scope. irlume plans every change; the KCM never edits PAM files directly.")
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: authConfiguration.errorCode.length > 0
            type: authConfiguration.rollbackRestored
                ? Kirigami.MessageType.Positive
                : Kirigami.MessageType.Error
            text: authConfiguration.statusText
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Recovery before enabling")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 2
                    text: i18n("Recovery before enabling")
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("If graphical login or unlock stops working, switch to a TTY, sign in with your password, and run:")
                    wrapMode: Text.Wrap
                }

                QQC2.TextField {
                    Layout.fillWidth: true
                    text: authConfiguration.recoveryCommand
                    readOnly: true
                    selectByMouse: true
                    Accessible.name: i18n("TTY recovery command")
                }

                QQC2.CheckBox {
                    Layout.fillWidth: true
                    text: i18n("I have read the TTY recovery command")
                    checked: authConfiguration.recoveryAcknowledged
                    onToggled: authConfiguration.recoveryAcknowledged = checked
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("Password authentication remains enabled and is verified after every successful change.")
                    color: Kirigami.Theme.positiveTextColor
                    wrapMode: Text.Wrap
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Lock screen")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 2
                    text: i18n("Lock screen")
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("Available for Secure infrared and RGB Convenience hardware after a healthy profile check.")
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: authConfiguration.lockScreenEnabled ? i18n("Enabled") : i18n("No verified change in this session")
                        color: authConfiguration.lockScreenEnabled
                            ? Kirigami.Theme.positiveTextColor
                            : Kirigami.Theme.disabledTextColor
                        wrapMode: Text.Wrap
                    }

                    QQC2.Button {
                        text: i18n("Review plan")
                        icon.name: "document-preview"
                        enabled: authConfiguration.canEnableLockScreen
                        onClicked: authConfiguration.previewLockScreen()
                    }

                    QQC2.Button {
                        text: i18n("Review and enable")
                        icon.name: "security-high"
                        enabled: authConfiguration.canApplyLockScreen
                        onClicked: authConfiguration.enableLockScreen()
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Login screen")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 2
                    text: i18n("Login screen")
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: systemState.securityTier === 0
                        ? i18n("Secure infrared hardware and liveness checks are ready.")
                        : i18n("Login-screen activation requires the Secure infrared tier.")
                    wrapMode: Text.Wrap
                }

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: authConfiguration.loginScreenEnabled ? i18n("Enabled") : i18n("No verified change in this session")
                        color: authConfiguration.loginScreenEnabled
                            ? Kirigami.Theme.positiveTextColor
                            : Kirigami.Theme.disabledTextColor
                        wrapMode: Text.Wrap
                    }

                    QQC2.Button {
                        text: i18n("Review plan")
                        icon.name: "document-preview"
                        enabled: authConfiguration.canEnableLoginScreen
                        onClicked: authConfiguration.previewLoginScreen()
                    }

                    QQC2.Button {
                        text: i18n("Review and enable")
                        icon.name: "security-high"
                        enabled: authConfiguration.canApplyLoginScreen
                        onClicked: authConfiguration.enableLoginScreen()
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            visible: authConfiguration.previewAvailable
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Authentication change preview")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 2
                    text: authConfiguration.previewTitle
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("Engine-owned targets in the dry-run plan:")
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: authConfiguration.previewChanges

                    delegate: QQC2.Label {
                        required property string modelData

                        Layout.fillWidth: true
                        text: i18n("• %1", modelData)
                        font.family: "monospace"
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Disable face authentication")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true
                    level: 2
                    text: i18n("Disable integration")
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("Disable only the targets reported by irlume for the active display manager.")
                    wrapMode: Text.Wrap
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Button {
                        text: i18n("Preview disable")
                        icon.name: "document-preview"
                        enabled: authConfiguration.canApplyDisable
                        onClicked: authConfiguration.previewDisable()
                    }

                    QQC2.Button {
                        text: i18n("Disable")
                        icon.name: "security-low"
                        enabled: authConfiguration.canDisable
                        onClicked: authConfiguration.disable()
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: authConfiguration.busy || authConfiguration.statusText.length > 0

            QQC2.BusyIndicator {
                visible: authConfiguration.busy
                running: visible
                Accessible.ignored: true
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: authConfiguration.statusText
                color: authConfiguration.errorCode.length > 0
                    ? Kirigami.Theme.negativeTextColor
                    : Kirigami.Theme.textColor
                wrapMode: Text.Wrap
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Face authentication for sudo, su, SSH, and Polkit is outside V2 and cannot be enabled here.")
        }
    }
}
