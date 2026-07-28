// SPDX-License-Identifier: GPL-3.0-or-later
// qmllint disable unqualified
// qmllint disable missing-property
// qmllint disable import
// qmllint disable unresolved-type

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.irlume 2.0

Kirigami.ScrollablePage {
    id: root

    required property QtObject systemState
    required property QtObject profileModel
    required property QtObject enrollmentSession

    title: i18n("Face Profiles")
    padding: Kirigami.Units.largeSpacing
    onVisibleChanged: {
        if (!visible && enrollmentSession.active) {
            profileModel.cancel();
        }
        if (!visible) {
            enrollmentSession.clearFrame();
        }
    }

    Connections {
        target: profileModel
        function onStateChanged() {
            if (profileModel.mergeConfirmationRequired && !mergeDialog.visible) {
                mergeDialog.open();
            }
        }
    }

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: i18n("Face Profiles")
            wrapMode: Text.Wrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Register, test, and improve recognition. Preview frames stay in memory and are cleared when the session ends.")
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: !profileModel.mutationSupported && !profileModel.busy
            type: Kirigami.MessageType.Information
            text: profileModel.statusText.length > 0
                ? profileModel.statusText
                : i18n("Profiles are read-only because the installed backend does not expose mutation capabilities.")
            actions: [
                Kirigami.Action {
                    text: i18n("Check again")
                    icon.name: "view-refresh"
                    onTriggered: profileModel.refresh()
                }
            ]
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            visible: profileModel.busy || profileModel.statusText.length > 0
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Profile operation status")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

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
                        font.weight: Font.DemiBold
                        wrapMode: Text.Wrap
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    visible: profileModel.stageLabel.length > 0
                    text: profileModel.stageLabel
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.Wrap
                }

                QQC2.ProgressBar {
                    Layout.fillWidth: true
                    visible: profileModel.totalScans > 0
                    from: 0
                    to: profileModel.totalScans
                    value: profileModel.capturedScans
                    Accessible.name: i18n("%1 of %2 scans captured",
                                          profileModel.capturedScans,
                                          profileModel.totalScans)
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: profileModel.busy || profileModel.canRetry

                    Item {
                        Layout.fillWidth: true
                    }

                    QQC2.Button {
                        visible: profileModel.canRetry && !profileModel.busy
                        text: i18n("Try again")
                        icon.name: "view-refresh"
                        onClicked: profileModel.retry()
                    }

                    QQC2.Button {
                        visible: profileModel.cancellable
                        text: i18n("Cancel")
                        icon.name: "dialog-cancel"
                        onClicked: profileModel.cancel()
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            visible: enrollmentSession.active || enrollmentSession.frameAvailable
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Live camera guidance")

            contentItem: GridLayout {
                columns: width < Kirigami.Units.gridUnit * 30 ? 1 : 2
                columnSpacing: Kirigami.Units.largeSpacing
                rowSpacing: Kirigami.Units.smallSpacing

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 20

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(Kirigami.Units.gridUnit * 12, width * 0.75)

                        EnrollmentPreview {
                            anchors.fill: parent
                            session: root.enrollmentSession
                            mirrored: true
                            Accessible.name: enrollmentSession.spectrum === "rgb"
                                ? i18n("RGB camera preview with face landmarks")
                                : i18n("Infrared camera preview with face landmarks")
                            Accessible.description: enrollmentSession.guidance
                        }

                        QQC2.Label {
                            anchors {
                                left: parent.left
                                top: parent.top
                                margins: Kirigami.Units.smallSpacing
                            }
                            padding: Kirigami.Units.smallSpacing
                            text: enrollmentSession.spectrum === "rgb" ? i18n("RGB · Convenience") : i18n("IR · Secure")
                            color: "white"
                            background: Rectangle {
                                color: "#a0000000"
                                radius: Kirigami.Units.cornerRadius
                            }
                        }

                        QQC2.Label {
                            anchors.centerIn: parent
                            visible: enrollmentSession.countdown > 0
                            text: enrollmentSession.countdown
                            color: "white"
                            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 3
                            font.weight: Font.Bold
                            Accessible.name: i18n("Capture in %1", enrollmentSession.countdown)
                        }
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: enrollmentSession.guidance.length > 0
                            ? enrollmentSession.guidance : i18n("Position your face inside the guide.")
                        font.weight: Font.DemiBold
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 2
                        text: i18n("Camera guidance")
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: [
                            { label: i18n("Face detected"), ready: enrollmentSession.faceDetected },
                            { label: i18n("Centered"), ready: enrollmentSession.centered },
                            { label: i18n("Distance"), ready: enrollmentSession.wellFramed },
                            { label: i18n("Looking at camera"), ready: enrollmentSession.facingCamera },
                            { label: i18n("Lighting"), ready: enrollmentSession.wellLit },
                            { label: i18n("Infrared ready"), ready: enrollmentSession.irReady }
                        ]

                        delegate: RowLayout {
                            required property var modelData

                            Kirigami.Icon {
                                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                                Layout.preferredHeight: width
                                source: modelData.ready ? "emblem-checked" : "emblem-unavailable"
                                color: modelData.ready ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.disabledTextColor
                                Accessible.ignored: true
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: modelData.label
                                color: modelData.ready ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    QQC2.ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: enrollmentSession.quality
                        Accessible.name: i18n("Capture quality: %1 percent", enrollmentSession.quality)
                    }

                    QQC2.Button {
                        Layout.alignment: Qt.AlignRight
                        text: i18n("Cancel preview")
                        icon.name: "dialog-cancel"
                        enabled: profileModel.cancellable
                        onClicked: profileModel.cancel()
                    }
                }
            }
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            visible: profileModel.profileCount < profileModel.maxProfiles
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Create a face profile")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    level: 2
                    text: profileModel.profileCount === 0
                        ? i18n("Set up face recognition") : i18n("Add another face profile")
                    wrapMode: Text.Wrap
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("Look toward the camera and slowly vary your angle while irlume captures encrypted recognition scans.")
                    wrapMode: Text.Wrap
                }

                QQC2.Button {
                    Layout.alignment: Qt.AlignRight
                    text: i18n("Start enrollment")
                    icon.name: "camera-photo"
                    enabled: profileModel.mutationSupported
                        && !profileModel.busy
                        && systemState.engineStatus === 0
                        && systemState.daemonStatus === 0
                        && systemState.cameraType !== 2
                        && systemState.cameraType !== 3
                    onClicked: profileModel.enroll()
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("%1 of %2 profiles used", profileModel.profileCount, profileModel.maxProfiles)
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.Wrap
                }
            }
        }

        Repeater {
            model: profileModel

            delegate: Kirigami.AbstractCard {
                id: profileCard

                required property string profileId
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
                                text: profileCard.displayName
                                font.weight: Font.DemiBold
                                wrapMode: Text.Wrap
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: i18np("%1 appearance scan", "%1 appearance scans", profileCard.scanCount)
                                color: Kirigami.Theme.disabledTextColor
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: i18n("Add a scan for glasses, a beard change, or different lighting. This is usually better than deleting and recreating the profile.")
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: profileCard.scans

                        delegate: RowLayout {
                            required property var modelData

                            Layout.fillWidth: true

                            Kirigami.Icon {
                                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                                Layout.preferredHeight: width
                                source: "view-list-details"
                                Accessible.ignored: true
                            }

                            QQC2.Label {
                                Layout.fillWidth: true
                                text: modelData.displayName
                                elide: Text.ElideRight
                            }

                            QQC2.ToolButton {
                                text: i18n("Rename scan")
                                icon.name: "edit-rename"
                                display: QQC2.AbstractButton.IconOnly
                                enabled: profileModel.mutationSupported
                                    && !profileModel.busy && !profileModel.mergeConfirmationRequired
                                Accessible.name: i18n("Rename scan “%1”", modelData.displayName)
                                onClicked: {
                                    renameDialog.profileId = profileCard.profileId;
                                    renameDialog.scanId = modelData.scanId;
                                    renameDialog.currentName = modelData.displayName;
                                    renameDialog.scanRecord = true;
                                    renameDialog.open();
                                }
                            }

                            QQC2.ToolButton {
                                text: i18n("Delete scan")
                                icon.name: "edit-delete"
                                display: QQC2.AbstractButton.IconOnly
                                enabled: profileModel.mutationSupported
                                    && !profileModel.busy
                                    && !profileModel.mergeConfirmationRequired
                                    && profileCard.scanCount > 1
                                Accessible.name: i18n("Delete scan “%1”", modelData.displayName)
                                onClicked: {
                                    deleteScanDialog.profileId = profileCard.profileId;
                                    deleteScanDialog.scanId = modelData.scanId;
                                    deleteScanDialog.scanName = modelData.displayName;
                                    deleteScanDialog.open();
                                }
                            }
                        }
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Button {
                            text: i18n("Test recognition")
                            icon.name: "security-high"
                            enabled: profileModel.mutationSupported
                                && !profileModel.busy && !profileModel.mergeConfirmationRequired
                            onClicked: profileModel.testRecognition(profileCard.profileId)
                        }

                        QQC2.Button {
                            text: i18n("Add appearance scan")
                            icon.name: "list-add"
                            enabled: profileModel.mutationSupported
                                && !profileModel.busy && !profileModel.mergeConfirmationRequired
                            onClicked: profileModel.addAppearanceScan(profileCard.profileId)
                        }

                        QQC2.Button {
                            text: i18n("Rename profile")
                            icon.name: "edit-rename"
                            enabled: profileModel.mutationSupported
                                && !profileModel.busy && !profileModel.mergeConfirmationRequired
                            onClicked: {
                                renameDialog.profileId = profileCard.profileId;
                                renameDialog.scanId = "";
                                renameDialog.currentName = profileCard.displayName;
                                renameDialog.scanRecord = false;
                                renameDialog.open();
                            }
                        }

                        QQC2.Button {
                            text: i18n("Delete profile")
                            icon.name: "edit-delete"
                            enabled: profileModel.mutationSupported
                                && !profileModel.busy && !profileModel.mergeConfirmationRequired
                            onClicked: {
                                deleteDialog.profileId = profileCard.profileId;
                                deleteDialog.profileName = profileCard.displayName;
                                deleteDialog.open();
                            }
                        }
                    }
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: systemState.cameraType === 2 || systemState.cameraType === 3
            type: Kirigami.MessageType.Warning
            text: i18n("A usable camera must be available before enrollment or recognition testing.")
        }
    }

    Kirigami.PromptDialog {
        id: mergeDialog

        title: i18n("Keep scans in the existing profile?")
        subtitle: i18np(
            "irlume recognized this face as “%2” and added %1 scan. Choose OK to keep it and run a recognition test, or Cancel to remove only the new scan.",
            "irlume recognized this face as “%2” and added %1 scans. Choose OK to keep them and run a recognition test, or Cancel to remove only the new scans.",
            profileModel.pendingMergeScanCount,
            profileModel.pendingMergeProfileName)
        standardButtons: QQC2.Dialog.Cancel | QQC2.Dialog.Ok
        onAccepted: profileModel.confirmIdentityMerge(true)
        onRejected: profileModel.confirmIdentityMerge(false)
    }

    Kirigami.PromptDialog {
        id: renameDialog

        property string profileId
        property string scanId
        property string currentName
        property bool scanRecord: false

        title: scanRecord ? i18n("Rename appearance scan") : i18n("Rename face profile")
        standardButtons: Kirigami.Dialog.NoButton
        onOpened: nameField.forceActiveFocus()
        onClosed: {
            profileId = "";
            scanId = "";
            currentName = "";
            scanRecord = false;
        }
        customFooterActions: [
            Kirigami.Action {
                text: i18n("Rename")
                icon.name: "dialog-ok"
                enabled: nameField.text.length > 0
                    && nameField.text.length <= 80
                    && nameField.text === nameField.text.trim()
                    && nameField.text !== renameDialog.currentName
                onTriggered: {
                    if (renameDialog.scanRecord) {
                        profileModel.renameScan(renameDialog.profileId, renameDialog.scanId, nameField.text);
                    } else {
                        profileModel.renameProfile(renameDialog.profileId, nameField.text);
                    }
                    renameDialog.close();
                }
            },
            Kirigami.Action {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onTriggered: renameDialog.close()
            }
        ]

        QQC2.TextField {
            id: nameField

            text: renameDialog.currentName
            maximumLength: 80
            placeholderText: i18n("Display name")
            Accessible.name: renameDialog.title
            onAccepted: {
                if (renameDialog.customFooterActions[0].enabled) {
                    renameDialog.customFooterActions[0].trigger();
                }
            }
        }
    }

    Kirigami.PromptDialog {
        id: deleteScanDialog

        property string profileId
        property string scanId
        property string scanName

        title: i18n("Delete appearance scan?")
        subtitle: i18n("Delete “%1” only? The profile and its other appearance scans will be kept.", scanName)
        standardButtons: QQC2.Dialog.Cancel | QQC2.Dialog.Ok
        onAccepted: profileModel.deleteScan(profileId, scanId)
        onClosed: {
            profileId = "";
            scanId = "";
            scanName = "";
        }
    }

    Kirigami.PromptDialog {
        id: deleteDialog

        property string profileId
        property string profileName

        title: i18n("Delete face profile?")
        subtitle: i18n("Delete “%1” and all of its appearance scans for the current account? Face recognition will stop working until a new profile is enrolled.", profileName)
        standardButtons: QQC2.Dialog.Cancel | QQC2.Dialog.Ok
        onAccepted: {
            profileModel.deleteProfile(profileId);
            profileId = "";
            profileName = "";
        }
        onRejected: {
            profileId = "";
            profileName = "";
        }
    }
}
