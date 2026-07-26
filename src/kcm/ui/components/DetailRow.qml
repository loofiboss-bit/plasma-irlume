// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

RowLayout {
    id: root

    required property string label
    required property string value
    property int tone: 0

    spacing: Kirigami.Units.largeSpacing

    QQC2.Label {
        Layout.fillWidth: true
        text: root.label
        color: Kirigami.Theme.textColor
        wrapMode: Text.Wrap
    }

    QQC2.Label {
        Layout.maximumWidth: Math.max(root.width * 0.52, Kirigami.Units.gridUnit * 8)
        text: root.value
        color: {
            switch (root.tone) {
            case 1:
                return Kirigami.Theme.positiveTextColor;
            case 2:
                return Kirigami.Theme.neutralTextColor;
            case 3:
                return Kirigami.Theme.negativeTextColor;
            default:
                return Kirigami.Theme.textColor;
            }
        }
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignRight
        wrapMode: Text.Wrap
    }
}
