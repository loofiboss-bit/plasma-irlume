// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Rectangle {
    id: root

    required property string text
    property int tone: 0

    readonly property color toneColor: {
        switch (tone) {
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

    implicitWidth: label.implicitWidth + Kirigami.Units.largeSpacing * 2
    implicitHeight: label.implicitHeight + Kirigami.Units.smallSpacing * 2
    radius: height / 2
    color: Qt.alpha(root.toneColor, 0.12)
    border.color: Qt.alpha(root.toneColor, 0.38)
    border.width: 1

    Accessible.role: Accessible.StaticText
    Accessible.name: text

    QQC2.Label {
        id: label

        anchors.centerIn: parent
        text: root.text
        color: root.toneColor
        font.weight: Font.DemiBold
    }
}
