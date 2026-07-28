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

    required property var systemState
    required property var supportReport
    required property bool refreshActive
    property var refresh: () => {}

    title: i18n("Diagnostics")
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Diagnostics are read-only. Refreshing runs bounded local probes and never performs biometric or PAM operations.")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: supportReport.hasIssue
            type: Kirigami.MessageType.Warning
            text: supportReport.issueTitle + "\n" + supportReport.recommendedAction
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Native status")

            contentItem: ColumnLayout {
                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Native engine")
                    value: systemState.engineStatusLabel
                    tone: systemState.engineStatus === 0 ? 1 : 2
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
                    label: i18n("Authentication decisions")
                    value: systemState.authenticationStatusLabel
                    tone: 2
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("PAM configuration")
                    value: systemState.pamStatusLabel
                    tone: 2
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Template persistence")
                    value: systemState.templatePersistenceStatusLabel
                    tone: 2
                }

                Kirigami.Separator { Layout.fillWidth: true }

                Components.DetailRow {
                    Layout.fillWidth: true
                    label: i18n("Secure Boot")
                    value: systemState.secureBootStatusLabel
                    tone: 0
                }

                QQC2.Button {
                    Layout.alignment: Qt.AlignRight
                    text: root.refreshActive ? i18n("Updating…") : i18n("Refresh diagnostics")
                    icon.name: "view-refresh"
                    enabled: !root.refreshActive
                    onClicked: root.refresh()
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Redacted support report preview")

            contentItem: ColumnLayout {
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
                        onClicked: supportReport.copyReport()
                    }

                    QQC2.Button {
                        text: i18n("Export report")
                        icon.name: "document-save"
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
                }
            }
        }
    }
}
