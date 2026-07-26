// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.AbstractCard {
    id: root

    required property string title
    required property string description
    required property string statusText
    required property string iconName
    property int tone: 0

    Accessible.role: Accessible.Grouping
    Accessible.name: title + ", " + statusText

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                Layout.preferredHeight: width
                source: root.iconName
                color: Kirigami.Theme.textColor
                Accessible.ignored: true
            }

            QQC2.Label {
                Layout.fillWidth: true
                text: root.title
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
            }
        }

        StatusPill {
            text: root.statusText
            tone: root.tone
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: root.description
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
        }
    }
}
