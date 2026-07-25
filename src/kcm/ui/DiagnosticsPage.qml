// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "components" as Components

ColumnLayout {
    id: root

    required property var systemState
    property var refresh: () => {}

    spacing: Kirigami.Units.largeSpacing

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: true
        type: Kirigami.MessageType.Information
        text: i18n("Diagnostics are read-only. Refreshing runs fixed local probes and never modifies authentication.")
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
                tone: systemState.templateProtectionStatus === 0 ? 1
                    : (systemState.templateProtectionStatus === 1 ? 2 : 0)
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
                text: systemState.supportReport
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                Accessible.name: i18n("Redacted support report")
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: systemState.issueCode.length > 0
        type: Kirigami.MessageType.Warning
        text: i18n("Diagnostic code: %1", systemState.issueCode)
    }
}
