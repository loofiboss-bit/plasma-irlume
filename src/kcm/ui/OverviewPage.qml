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
        Accessible.name: systemState.headline + ", " + systemState.securityTierLabel

        contentItem: GridLayout {
            columns: width < Kirigami.Units.gridUnit * 28 ? 1 : 2
            columnSpacing: Kirigami.Units.largeSpacing
            rowSpacing: Kirigami.Units.smallSpacing

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    Layout.fillWidth: true
                    text: systemState.headline
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.35
                    font.weight: Font.DemiBold
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: systemState.summary
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.Wrap
                }
            }

            Components.StatusPill {
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                text: systemState.securityTierLabel
                tone: systemState.securityTier === 0 ? 1
                    : (systemState.securityTier === 1 ? 2
                    : (systemState.securityTier === 3 ? 0 : 3))
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        QQC2.Label {
            text: i18n("Readiness path")
            font.weight: Font.DemiBold
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Every link must be healthy before face authentication can be considered ready.")
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width < Kirigami.Units.gridUnit * 38 ? 1 : 3
            columnSpacing: Kirigami.Units.smallSpacing
            rowSpacing: Kirigami.Units.smallSpacing

            Components.TrustNode {
                Layout.fillWidth: true
                title: i18n("Camera")
                description: systemState.cameraLabel
                statusText: systemState.cameraStatusLabel
                iconName: "camera-photo"
                tone: systemState.cameraType === 0 ? 1 : (systemState.cameraType === 1 ? 2 : 3)
            }

            Components.TrustNode {
                Layout.fillWidth: true
                title: i18n("irlume")
                description: systemState.engineVersion.length > 0
                    ? i18n("Engine %1", systemState.engineVersion)
                    : i18n("No engine version reported")
                statusText: systemState.engineStatusLabel
                iconName: "security-high"
                tone: systemState.engineStatus === 0 ? 1 : 3
            }

            Components.TrustNode {
                Layout.fillWidth: true
                title: i18n("Desktop authentication")
                description: systemState.activeDisplayManager
                statusText: systemState.pamStatusLabel
                iconName: "preferences-system-login"
                tone: systemState.pamStatus === 0 ? 1 : (systemState.pamStatus === 1 ? 2 : 3)
            }
        }
    }

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Current status")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: i18n("Current status")
                font.weight: Font.DemiBold
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Face profile")
                value: systemState.profileStatusLabel
                tone: systemState.profileStatus === 0 ? 1 : (systemState.profileStatus === 1 ? 2 : 0)
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }

            Components.DetailRow {
                Layout.fillWidth: true
                label: i18n("Password fallback")
                value: systemState.passwordFallbackPreserved ? i18n("Always available") : i18n("Not verified")
                tone: systemState.passwordFallbackPreserved ? 1 : 3
            }
        }
    }
}
