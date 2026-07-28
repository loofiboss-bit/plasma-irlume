// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: root

    required property QtObject profileModel

    title: i18n("Face Profiles")
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: i18n("Face Profiles")
            wrapMode: Text.Wrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("Profiles come from irlume Contract 1 and are read-only. Camera Check does not enroll or change a face profile.")
        }

        RowLayout {
            Layout.fillWidth: true

            QQC2.BusyIndicator {
                visible: profileModel.busy
                running: visible
                Accessible.ignored: true
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: profileModel.statusText
                color: profileModel.errorCode.length > 0
                    ? Kirigami.Theme.negativeTextColor
                    : Kirigami.Theme.disabledTextColor
                wrapMode: Text.Wrap
            }

            QQC2.Button {
                text: i18n("Refresh")
                icon.name: "view-refresh"
                enabled: !profileModel.busy
                onClicked: profileModel.refresh()
            }
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: !profileModel.busy && profileModel.profileCount === 0
            text: profileModel.readOnlyAvailable
                ? i18n("No face profiles are enrolled.")
                : i18n("Profile information is unavailable.")
            icon.name: "user-identity"
        }

        Repeater {
            model: profileModel

            delegate: Kirigami.AbstractCard {
                required property string displayName
                required property int scanCount
                required property var scans

                Layout.fillWidth: true
                Accessible.role: Accessible.Grouping
                Accessible.name: displayName

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    RowLayout {
                        Layout.fillWidth: true

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                            Layout.preferredHeight: width
                            source: "user-identity"
                            Accessible.ignored: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: displayName
                                font.weight: Font.DemiBold
                                wrapMode: Text.Wrap
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: i18np("%1 appearance scan", "%1 appearance scans", scanCount)
                                color: Kirigami.Theme.disabledTextColor
                            }
                        }
                    }

                    Repeater {
                        model: scans

                        delegate: QQC2.Label {
                            required property var modelData

                            Layout.fillWidth: true
                            text: i18n("• %1", modelData.displayName)
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }
        }
    }
}
