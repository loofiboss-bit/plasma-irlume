// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils

KCMUtils.SimpleKCM {
    id: root

    title: i18n("Face Login")
    KCMUtils.ConfigModule.buttons: KCMUtils.ConfigModule.NoAdditionalButton

    header: QQC2.TabBar {
        id: tabs

        width: parent.width
        Accessible.name: i18n("Face Login sections")
        KeyNavigation.down: currentIndex === 0 ? setupStatus
            : (currentIndex === 1 ? cameraCheck
            : (currentIndex === 2 ? enrollment
            : (currentIndex === 3 ? authentication : diagnostics)))

        QQC2.TabButton {
            text: i18n("Setup & Status")
            icon.name: "view-dashboard"
            display: tabs.width < 520 ? QQC2.AbstractButton.IconOnly : QQC2.AbstractButton.TextOnly
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Camera Check")
            icon.name: "camera-photo"
            display: tabs.width < 640 ? QQC2.AbstractButton.IconOnly : QQC2.AbstractButton.TextOnly
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Face Profiles")
            icon.name: "user-identity"
            display: tabs.width < 640 ? QQC2.AbstractButton.IconOnly : QQC2.AbstractButton.TextOnly
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Access")
            icon.name: "preferences-system-login"
            display: tabs.width < 640 ? QQC2.AbstractButton.IconOnly : QQC2.AbstractButton.TextOnly
            Accessible.name: text
        }

        QQC2.TabButton {
            text: i18n("Support")
            icon.name: "tools-report-bug"
            display: tabs.width < 640 ? QQC2.AbstractButton.IconOnly : QQC2.AbstractButton.TextOnly
            Accessible.name: text
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: 0

        SetupStatusPage {
            id: setupStatus

            Layout.fillWidth: true
            visible: tabs.currentIndex === 0
            enabled: visible
            systemState: kcm.systemState
            profileModel: kcm.profileModel
            authConfiguration: kcm.authConfiguration
            cameraPreviewSession: kcm.cameraPreviewSession
            refreshActive: kcm.refreshing
            partialDiagnostics: kcm.partialDiagnostics
            openCamera: () => tabs.currentIndex = 1
            openProfiles: () => tabs.currentIndex = 2
            openAccess: () => tabs.currentIndex = 3
            refresh: () => {
                kcm.refresh();
            }
        }

        CameraCheckPage {
            id: cameraCheck

            Layout.fillWidth: true
            visible: tabs.currentIndex === 1
            enabled: visible
            cameraPreviewSession: kcm.cameraPreviewSession
        }

        EnrollmentPage {
            id: enrollment

            Layout.fillWidth: true
            visible: tabs.currentIndex === 2
            enabled: visible
            profileModel: kcm.profileModel
        }

        AuthenticationPage {
            id: authentication

            Layout.fillWidth: true
            visible: tabs.currentIndex === 3
            enabled: visible
            systemState: kcm.systemState
            authConfiguration: kcm.authConfiguration
        }

        DiagnosticsPage {
            id: diagnostics

            Layout.fillWidth: true
            visible: tabs.currentIndex === 4
            enabled: visible
            systemState: kcm.systemState
            supportReport: kcm.supportReport
            refreshActive: kcm.refreshing
            partialDiagnostics: kcm.partialDiagnostics
            refresh: () => kcm.refresh()
        }
    }
}
