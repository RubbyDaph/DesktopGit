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
    property color addedTextColor: "#8fdda8"

    ColumnLayout {
        id: branchesLayout

        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        enabled: root.controller !== null && root.controller !== undefined
        visible: enabled

        RowLayout {
            id: branchesToolbarLayout

            Layout.fillWidth: true
            spacing: 8

            AppButton {
                id: closeBranchesButton

                text: qsTr("Back")
                onClicked: root.controller.CloseBranches()
            }

            Label {
                id: branchesTitleLabel

                text: qsTr("Branches")
                color: root.textColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Label {
                id: branchesRepositoryLabel

                text: root.controller ? root.controller.repositoryPath : ""
                color: root.mutedTextColor
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            AppButton {
                id: refreshBranchesButton

                text: qsTr("Refresh")
                onClicked: root.controller.RefreshBranches()
            }
        }

        SplitView {
            id: branchesSplitView

            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Pane {
                id: branchListPane

                SplitView.fillWidth: true
                SplitView.minimumWidth: 360

                background: Rectangle {
                    color: root.panelColor
                    border.color: root.borderColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        id: branchListTitleLabel

                        text: qsTr("Local Branches")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    ListView {
                        id: branchListView

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.controller ? root.controller.branchModel : null
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: ItemDelegate {
                            id: branchDelegate

                            required property string name
                            required property bool current
                            required property string upstream

                            width: branchListView.width
                            height: 64
                            highlighted: root.controller ? root.controller.selectedBranchName === name : false
                            onClicked: root.controller.SelectBranch(name)

                            background: Rectangle {
                                color: branchDelegate.highlighted
                                    ? "#33425f"
                                    : branchDelegate.hovered
                                    ? root.panelRaisedColor
                                    : "transparent"
                                border.color: branchDelegate.current ? root.accentColor : "transparent"
                                border.width: branchDelegate.current ? 1 : 0
                                radius: 6
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 10

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    Label {
                                        text: branchDelegate.name
                                        color: branchDelegate.current ? root.accentColor : root.textColor
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: branchDelegate.upstream.length > 0
                                            ? branchDelegate.upstream
                                            : qsTr("no upstream")
                                        color: root.mutedTextColor
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                Rectangle {
                                    id: currentBranchBadge

                                    visible: branchDelegate.current
                                    color: "#243528"
                                    border.color: "#3d6f49"
                                    border.width: 1
                                    radius: 6
                                    Layout.preferredWidth: currentBranchBadgeLabel.implicitWidth + 18
                                    Layout.preferredHeight: 26

                                    Label {
                                        id: currentBranchBadgeLabel

                                        anchors.centerIn: parent
                                        text: qsTr("current")
                                        color: root.addedTextColor
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }
                        }

                        Label {
                            id: emptyBranchListLabel

                            anchors.centerIn: parent
                            text: qsTr("No local branches")
                            color: root.mutedTextColor
                            visible: branchListView.count === 0
                        }
                    }
                }
            }

            Pane {
                id: branchActionsPane

                SplitView.preferredWidth: 340
                SplitView.minimumWidth: 300

                background: Rectangle {
                    color: root.panelColor
                    border.color: root.borderColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 14

                    Label {
                        id: branchActionsTitleLabel

                        text: qsTr("Branch Actions")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    Rectangle {
                        id: selectedBranchPanel

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
                                text: qsTr("Selected branch")
                                color: root.mutedTextColor
                                font.pixelSize: 12
                            }

                            Label {
                                text: root.controller && root.controller.selectedBranchName.length > 0
                                    ? root.controller.selectedBranchName
                                    : qsTr("None")
                                color: root.textColor
                                font.pixelSize: 15
                                font.weight: Font.DemiBold
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }
                    }

                    AppButton {
                        id: checkoutBranchButton

                        text: qsTr("Checkout")
                        primary: true
                        enabled: root.controller
                            && root.controller.selectedBranchName.length > 0
                            && root.controller.selectedBranchName !== root.controller.currentBranch
                        onClicked: root.controller.CheckoutSelectedBranch()
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.borderColor
                    }

                    Label {
                        id: createBranchTitleLabel

                        text: qsTr("Create Branch")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    TextField {
                        id: newBranchNameField

                        Layout.fillWidth: true
                        placeholderText: qsTr("new-branch-name")
                        color: root.textColor
                        placeholderTextColor: root.mutedTextColor
                        selectedTextColor: root.textColor
                        selectionColor: "#3f5f91"

                        background: Rectangle {
                            color: root.panelRaisedColor
                            border.color: newBranchNameField.activeFocus ? root.accentColor : root.borderColor
                            border.width: 1
                            radius: 6
                        }

                        Keys.onReturnPressed: createBranchButton.clicked()
                        Keys.onEnterPressed: createBranchButton.clicked()
                    }

                    AppButton {
                        id: createBranchButton

                        text: qsTr("Create")
                        enabled: newBranchNameField.text.trim().length > 0
                        onClicked: {
                            root.controller.CreateBranch(newBranchNameField.text)
                            if (root.controller.selectedBranchName === newBranchNameField.text.trim()) {
                                newBranchNameField.clear()
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }
}
