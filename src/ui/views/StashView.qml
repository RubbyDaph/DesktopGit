import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var controller
    property color panelColor: "#26282c"
    property color panelRaisedColor: "#2b2d31"
    property color borderColor: "#3a3d42"
    property color textColor: "#e7e9ee"
    property color mutedTextColor: "#9aa0aa"
    property color accentColor: "#7aa2f7"
    property color removedTextColor: "#f0a0a0"

    Dialog {
        id: dropStashDialog

        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        width: Math.min(root.width - 48, 360)
        modal: true
        focus: true
        title: qsTr("Drop stash")
        standardButtons: Dialog.NoButton

        background: Rectangle {
            color: root.panelColor
            border.color: root.borderColor
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            spacing: 14

            Label {
                text: qsTr("Drop selected stash?")
                color: root.textColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }

            Label {
                text: root.controller ? root.controller.selectedStashName : ""
                color: root.mutedTextColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Item {
                    Layout.fillWidth: true
                }

                AppButton {
                    text: qsTr("Cancel")
                    onClicked: dropStashDialog.close()
                }

                AppButton {
                    text: qsTr("Drop")
                    primary: true
                    onClicked: {
                        root.controller.DropSelectedStash()
                        dropStashDialog.close()
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: stashLayout

        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        enabled: root.controller !== null && root.controller !== undefined
        visible: enabled

        RowLayout {
            id: stashToolbarLayout

            Layout.fillWidth: true
            spacing: 8

            AppButton {
                id: closeStashButton

                text: qsTr("Back")
                onClicked: root.controller.CloseStash()
            }

            Label {
                id: stashTitleLabel

                text: qsTr("Stash")
                color: root.textColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Label {
                id: stashRepositoryLabel

                text: root.controller ? root.controller.repositoryPath : ""
                color: root.mutedTextColor
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            AppButton {
                id: refreshStashButton

                text: qsTr("Refresh")
                onClicked: root.controller.RefreshStashes()
            }
        }

        SplitView {
            id: stashSplitView

            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Pane {
                id: stashListPane

                SplitView.fillWidth: true
                SplitView.minimumWidth: 380

                background: Rectangle {
                    color: root.panelColor
                    border.color: root.borderColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        id: stashListTitleLabel

                        text: qsTr("Stash Entries")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    ListView {
                        id: stashListView

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.controller ? root.controller.stashModel : null
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: ItemDelegate {
                            id: stashDelegate

                            required property int index
                            required property string name
                            required property string branch
                            required property string message

                            width: stashListView.width
                            height: 72
                            highlighted: root.controller ? root.controller.selectedStashName === name : false
                            onClicked: root.controller.SelectStash(name)

                            background: Rectangle {
                                color: stashDelegate.highlighted
                                    ? "#33425f"
                                    : stashDelegate.hovered
                                    ? root.panelRaisedColor
                                    : "transparent"
                                border.color: stashDelegate.highlighted ? root.accentColor : "transparent"
                                border.width: 1
                                radius: 6
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                anchors.topMargin: 8
                                anchors.bottomMargin: 8
                                spacing: 3

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Label {
                                        text: stashDelegate.name
                                        color: root.accentColor
                                        font.family: "monospace"
                                        font.pixelSize: 13
                                    }

                                    Label {
                                        text: stashDelegate.branch.length > 0
                                            ? stashDelegate.branch
                                            : qsTr("unknown branch")
                                        color: root.mutedTextColor
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                Label {
                                    text: stashDelegate.message.length > 0
                                        ? stashDelegate.message
                                        : qsTr("No message")
                                    color: root.textColor
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        Label {
                            id: emptyStashListLabel

                            anchors.centerIn: parent
                            text: qsTr("No stash entries")
                            color: root.mutedTextColor
                            visible: stashListView.count === 0
                        }
                    }
                }
            }

            Pane {
                id: stashActionsPane

                SplitView.preferredWidth: 360
                SplitView.minimumWidth: 320

                background: Rectangle {
                    color: root.panelColor
                    border.color: root.borderColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 14

                    Label {
                        id: createStashTitleLabel

                        text: qsTr("Create Stash")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    TextField {
                        id: stashMessageField

                        Layout.fillWidth: true
                        placeholderText: qsTr("stash message")
                        color: root.textColor
                        placeholderTextColor: root.mutedTextColor
                        selectByMouse: true

                        background: Rectangle {
                            color: root.panelRaisedColor
                            border.color: stashMessageField.activeFocus ? root.accentColor : root.borderColor
                            border.width: 1
                            radius: 6
                        }

                        Keys.onReturnPressed: pushStashButton.clicked()
                        Keys.onEnterPressed: pushStashButton.clicked()
                    }

                    AppButton {
                        id: pushStashButton

                        text: qsTr("Stash push")
                        primary: true
                        onClicked: {
                            root.controller.StashPush(stashMessageField.text)
                            stashMessageField.clear()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.borderColor
                    }

                    Label {
                        id: selectedStashTitleLabel

                        text: qsTr("Selected Stash")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Rectangle {
                        id: selectedStashPanel

                        Layout.fillWidth: true
                        implicitHeight: 82
                        color: root.panelRaisedColor
                        border.color: root.borderColor
                        border.width: 1
                        radius: 6

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Label {
                                text: qsTr("Selected")
                                color: root.mutedTextColor
                                font.pixelSize: 12
                            }

                            Label {
                                text: root.controller && root.controller.selectedStashName.length > 0
                                    ? root.controller.selectedStashName
                                    : qsTr("None")
                                color: root.textColor
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        AppButton {
                            id: applyStashButton

                            text: qsTr("Apply")
                            enabled: root.controller && root.controller.selectedStashName.length > 0
                            onClicked: root.controller.ApplySelectedStash()
                        }

                        AppButton {
                            id: popStashButton

                            text: qsTr("Pop")
                            enabled: root.controller && root.controller.selectedStashName.length > 0
                            onClicked: root.controller.PopSelectedStash()
                        }

                        AppButton {
                            id: dropStashButton

                            text: qsTr("Drop")
                            enabled: root.controller && root.controller.selectedStashName.length > 0
                            onClicked: dropStashDialog.open()
                        }
                    }

                    Label {
                        id: stashStatusLabel

                        text: root.controller ? root.controller.statusMessage : ""
                        color: root.mutedTextColor
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}
