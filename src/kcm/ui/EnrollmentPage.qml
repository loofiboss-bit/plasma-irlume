// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: root

    required property QtObject systemState
    required property QtObject profileModel

    title: i18n("Face profile")
    padding: Kirigami.Units.largeSpacing

    ColumnLayout {
        width: root.availableWidth
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 1
            text: i18n("Face profile")
            wrapMode: Text.Wrap
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Create and test a face profile without displaying or saving camera frames.")
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: !profileModel.contractAvailable && !profileModel.busy
            type: Kirigami.MessageType.Information
            text: profileModel.statusText.length > 0
                ? profileModel.statusText
                : i18n("Profile management requires an irlume release with the structured integration contract.")
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
            visible: profileModel.contractAvailable && profileModel.profileCount === 0
            Accessible.role: Accessible.Grouping
            Accessible.name: i18n("Create a face profile")

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    level: 2
                    text: i18n("Set up face recognition")
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
                    enabled: !profileModel.busy
                        && systemState.engineStatus === 0
                        && systemState.daemonStatus === 0
                        && systemState.cameraType !== 2
                        && systemState.cameraType !== 3
                    onClicked: profileModel.enroll()
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

                    Flow {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing

                        QQC2.Button {
                            text: i18n("Test recognition")
                            icon.name: "security-high"
                            enabled: !profileModel.busy
                            onClicked: profileModel.testRecognition(profileCard.profileId)
                        }

                        QQC2.Button {
                            text: i18n("Add appearance scan")
                            icon.name: "list-add"
                            enabled: !profileModel.busy
                            onClicked: profileModel.addAppearanceScan(profileCard.profileId)
                        }

                        QQC2.Button {
                            text: i18n("Delete profile")
                            icon.name: "edit-delete"
                            enabled: !profileModel.busy
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
