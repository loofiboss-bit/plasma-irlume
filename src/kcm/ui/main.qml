// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils

KCMUtils.SimpleKCM {
    id: root

    title: i18n("KFaceAuth (Development Preview)")
    KCMUtils.ConfigModule.buttons: KCMUtils.ConfigModule.NoAdditionalButton

    header: QQC2.TabBar {
        id: tabs

        width: parent.width
        Accessible.name: i18n("KFaceAuth sections")
        KeyNavigation.down: currentIndex === 0 ? overview
            : (currentIndex === 1 ? cameraCheck : diagnostics)

        QQC2.TabButton {
            text: i18n("Overview")
            icon.name: "view-dashboard"
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Camera Check")
            icon.name: "camera-photo"
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Diagnostics")
            icon.name: "tools-report-bug"
            Accessible.name: text
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 0

        SetupStatusPage {
            id: overview

            Layout.fillWidth: true
            visible: tabs.currentIndex === 0
            enabled: visible
            systemState: kcm.systemState
            cameraPreviewSession: kcm.cameraPreviewSession
            refreshActive: kcm.refreshing
            openCamera: () => tabs.currentIndex = 1
            refresh: () => kcm.refresh()
        }

        CameraCheckPage {
            id: cameraCheck

            Layout.fillWidth: true
            visible: tabs.currentIndex === 1
            enabled: visible
            cameraPreviewSession: kcm.cameraPreviewSession
            visionAnalysisSession: kcm.visionAnalysisSession
        }

        DiagnosticsPage {
            id: diagnostics

            Layout.fillWidth: true
            visible: tabs.currentIndex === 2
            enabled: visible
            systemState: kcm.systemState
            supportReport: kcm.supportReport
            refreshActive: kcm.refreshing
            refresh: () => kcm.refresh()
        }
    }
}
