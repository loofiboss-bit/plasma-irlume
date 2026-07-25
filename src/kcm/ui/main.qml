// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami

KCMUtils.SimpleKCM {
    id: root

    title: i18n("Face Login")
    KCMUtils.ConfigModule.buttons: KCMUtils.ConfigModule.NoAdditionalButton

    header: QQC2.TabBar {
        id: tabs

        width: parent.width
        Accessible.name: i18n("Face Login sections")
        KeyNavigation.down: currentIndex === 0 ? overview : (currentIndex === 1 ? security : diagnostics)

        QQC2.TabButton {
            text: i18n("Overview")
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Security")
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Diagnostics")
            Accessible.name: text
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 0

        OverviewPage {
            id: overview

            Layout.fillWidth: true
            visible: tabs.currentIndex === 0
            enabled: visible
            systemState: kcm.systemState
        }

        SecurityPage {
            id: security

            Layout.fillWidth: true
            visible: tabs.currentIndex === 1
            enabled: visible
            systemState: kcm.systemState
        }

        DiagnosticsPage {
            id: diagnostics

            Layout.fillWidth: true
            visible: tabs.currentIndex === 2
            enabled: visible
            systemState: kcm.systemState
            refresh: () => kcm.refresh()
        }
    }
}
