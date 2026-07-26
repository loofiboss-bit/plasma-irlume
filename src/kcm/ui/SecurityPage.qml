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

    spacing: Kirigami.Units.largeSpacing

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Platform")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: i18n("Platform")
                font.weight: Font.DemiBold
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Fedora")
                value: systemState.fedoraVersion.length > 0 ? systemState.fedoraVersion : i18n("Unknown")
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Plasma")
                value: systemState.plasmaVersion.length > 0 ? systemState.plasmaVersion : i18n("Unknown")
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Display manager")
                value: systemState.activeDisplayManager
                tone: systemState.activeDisplayManager === "Plasma Login Manager"
                    || systemState.activeDisplayManager === "SDDM" ? 1 : 2
            }
        }
    }

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Security capabilities")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: i18n("Security capabilities")
                font.weight: Font.DemiBold
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("TPM 2.0")
                value: systemState.tpmStatusLabel
                tone: systemState.tpmStatus === 0 ? 1 : (systemState.tpmStatus === 1 ? 2 : 0)
            }

            QQC2.Label {
                Layout.fillWidth: true
                visible: systemState.tpmStatus !== 0
                text: i18n("Without TPM protection, face templates have reduced protection at rest.")
                color: Kirigami.Theme.disabledTextColor
                wrapMode: Text.Wrap
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

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("IR emitter")
                value: systemState.emitterStatusLabel
                tone: systemState.emitterStatus === 0 ? 1 : (systemState.emitterStatus === 1 ? 2 : 0)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Liveness checks")
                value: systemState.livenessStatusLabel
                tone: systemState.livenessStatus === 0 ? 1 : (systemState.livenessStatus === 1 ? 2 : 0)
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: systemState.securityTier === 1
        type: Kirigami.MessageType.Warning
        text: i18n("RGB-only hardware is a convenience feature. Login-screen activation remains unavailable.")
    }
}
