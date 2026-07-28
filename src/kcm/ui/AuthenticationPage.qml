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

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Authentication wiring is read-only. v3 never edits PAM or starts a privileged helper.")
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Authentication status")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Lock screen")
                    value: authConfiguration.lockScreenEnabled ? i18n("Wired") : i18n("Not wired")
                    tone: authConfiguration.lockScreenEnabled ? 1 : 0
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Login screen")
                    value: authConfiguration.loginScreenEnabled ? i18n("Wired") : i18n("Not wired")
                    tone: authConfiguration.loginScreenEnabled ? 1 : 0
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Password fallback")
                    value: systemState.passwordFallbackStatus === 0
                        ? i18n("Verified")
                        : (systemState.passwordFallbackStatus === 1
                            ? i18n("Not verified")
                            : i18n("Unknown"))
                    tone: systemState.passwordFallbackPreserved ? 1 : 0
                }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: authConfiguration.statusText
            color: authConfiguration.errorCode.length > 0
                ? Kirigami.Theme.negativeTextColor
                : Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
        }
    }
}
