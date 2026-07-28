// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "components" as Components

ColumnLayout {
    id: root

    required property var systemState
    required property var authConfiguration
    required property var supportReport
    property var refresh: () => {}

    spacing: Kirigami.Units.largeSpacing

    Kirigami.Heading {
        Layout.fillWidth: true
        level: 1
        text: i18n("Support")
        wrapMode: Text.Wrap
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: true
        type: Kirigami.MessageType.Information
        text: i18n("Diagnostics are read-only. Refreshing runs fixed local probes and never modifies authentication.")
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: supportReport.hasIssue
        type: supportReport.issueCode === "rollback-failed" ? Kirigami.MessageType.Error : Kirigami.MessageType.Warning
        text: supportReport.issueTitle + "\n" + supportReport.recommendedAction
        Accessible.name: i18n("Recommended recovery action")
    }

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Emergency disable")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                text: i18n("Emergency disable")
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: i18n("Contract 1 reports current wiring but cannot disable it through the KCM. Use the documented TTY recovery command when a change is required.")
                color: Kirigami.Theme.disabledTextColor
                wrapMode: Text.Wrap
            }

            QQC2.Button {
                Layout.alignment: Qt.AlignRight
                text: authConfiguration.busy ? i18n("Disabling…") : i18n("Disable Face Login now")
                icon.name: "security-low"
                enabled: authConfiguration.canDisable && !authConfiguration.busy
                Accessible.name: text
                Accessible.description: i18n("Runs the fixed verified disable operation immediately")
                onClicked: authConfiguration.disableNow()
            }

            QQC2.Label {
                Layout.fillWidth: true
                visible: authConfiguration.statusText.length > 0
                text: authConfiguration.statusText
                color: authConfiguration.errorCode.length > 0 ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
                wrapMode: Text.Wrap
                Accessible.role: Accessible.StaticText
            }
        }
    }

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("TTY recovery")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.fillWidth: true
                text: i18n("TTY recovery")
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: i18n("Keep these instructions available before logging out or rebooting.")
                color: Kirigami.Theme.disabledTextColor
                wrapMode: Text.Wrap
            }

            QQC2.TextArea {
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 8
                text: supportReport.recoveryInstructions
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                Accessible.name: i18n("TTY recovery instructions")
            }

            QQC2.Button {
                Layout.alignment: Qt.AlignRight
                text: i18n("Copy recovery instructions")
                icon.name: "edit-copy"
                Accessible.name: text
                onClicked: supportReport.copyRecoveryInstructions()
            }
        }
    }

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Diagnostic source")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: i18n("Diagnostic source")
                font.weight: Font.DemiBold
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: i18n("Current data source: %1", systemState.dataSource)
                color: Kirigami.Theme.disabledTextColor
                wrapMode: Text.Wrap
            }

            QQC2.Button {
                text: i18n("Refresh diagnostics")
                icon.name: "view-refresh"
                Accessible.name: text
                onClicked: root.refresh()
            }
        }
    }

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Diagnostics")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: i18n("Diagnostics")
                font.weight: Font.DemiBold
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Security tier")
                value: systemState.securityTierLabel
                tone: systemState.securityTier === 0 ? 1 : (systemState.securityTier === 1 ? 2 : 3)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Camera")
                value: systemState.cameraLabel
                tone: systemState.cameraType === 0 ? 1 : (systemState.cameraType === 1 ? 2 : 3)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

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
                label: i18n("Background service")
                value: systemState.daemonStatusLabel
                tone: systemState.daemonStatus === 0 ? 1 : (systemState.daemonStatus === 3 ? 0 : 3)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Authentication configuration")
                value: systemState.pamStatusLabel
                tone: systemState.pamStatus === 0 ? 1 : (systemState.pamStatus === 1 ? 2 : 3)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("TPM 2.0 hardware")
                value: systemState.tpmStatusLabel
                tone: systemState.tpmStatus === 0 ? 1 : (systemState.tpmStatus === 1 ? 2 : 0)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Template protection")
                value: systemState.templateProtectionStatusLabel
                tone: systemState.templateProtectionStatus === 0 ? 1 : (systemState.templateProtectionStatus === 1 ? 2 : 0)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Secure Boot")
                value: systemState.secureBootStatusLabel
                tone: systemState.secureBootStatus === 0 ? 1 : (systemState.secureBootStatus === 1 ? 2 : 0)
            }
        }
    }

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Redacted support report preview")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: i18n("Redacted support report preview")
                font.weight: Font.DemiBold
            }

            QQC2.TextArea {
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 10
                text: supportReport.report
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                Accessible.name: i18n("Redacted support report")
            }

            Flow {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.Button {
                    text: i18n("Copy report")
                    icon.name: "edit-copy"
                    Accessible.name: text
                    onClicked: supportReport.copyReport()
                }

                QQC2.Button {
                    text: i18n("Export report")
                    icon.name: "document-save"
                    Accessible.name: text
                    Accessible.description: i18n("Saves a redacted Markdown report in Documents")
                    onClicked: supportReport.exportReport()
                }
            }

            QQC2.Label {
                Layout.fillWidth: true
                visible: supportReport.statusText.length > 0
                text: supportReport.statusText
                color: Kirigami.Theme.disabledTextColor
                wrapMode: Text.Wrap
                Accessible.role: Accessible.StaticText
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: supportReport.hasIssue
        type: Kirigami.MessageType.Warning
        text: i18n("Diagnostic code: %1", supportReport.issueCode)
    }
}
