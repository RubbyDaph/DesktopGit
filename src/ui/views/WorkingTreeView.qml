import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SplitView {
    id: root

    property var controller
    property color panelColor: "#26282c"
    property color panelRaisedColor: "#2b2d31"
    property color borderColor: "#3a3d42"
    property color textColor: "#e7e9ee"
    property color mutedTextColor: "#9aa0aa"
    property color addedLineColor: "#173d28"
    property color removedLineColor: "#462323"
    property color addedTextColor: "#8fdda8"
    property color removedTextColor: "#f0a0a0"
    property color accentColor: "#7aa2f7"

    orientation: Qt.Horizontal

    Pane {
        id: changesPane

        SplitView.preferredWidth: 300
        SplitView.minimumWidth: 240

        background: Rectangle {
            color: root.panelColor
            border.color: root.borderColor
            border.width: 1
        }

        ColumnLayout {
            id: changesLayout

            anchors.fill: parent
            spacing: 8

            RowLayout {
                id: changesHeaderLayout

                Layout.fillWidth: true
                spacing: 8

                Label {
                    id: changesTitleLabel

                    text: qsTr("Changes")
                    color: root.textColor
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }

                AppButton {
                    id: selectAllFilesButton

                    text: qsTr("Select all")
                    enabled: changedFilesListView.count > 0
                    onClicked: root.controller.SelectAllFiles()
                }

                AppButton {
                    id: clearFileSelectionButton

                    text: qsTr("Clear")
                    enabled: root.controller && root.controller.selectedFileCount > 0
                    onClicked: root.controller.ClearFileSelection()
                }
            }

            ListView {
                id: changedFilesListView

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.controller ? root.controller.statusFileModel : null

                delegate: ItemDelegate {
                    id: statusFileDelegate

                    width: ListView.view.width
                    highlighted: root.controller ? root.controller.selectedFilePath === model.path : false
                    onClicked: root.controller.SelectStatusFile(model.path)

                    background: Rectangle {
                        color: statusFileDelegate.highlighted
                            ? "#333842"
                            : (statusFileDelegate.hovered ? root.panelRaisedColor : "transparent")
                        border.color: statusFileDelegate.highlighted ? root.accentColor : "transparent"
                        border.width: 1
                    }

                    contentItem: RowLayout {
                        spacing: 8

                        CheckBox {
                            id: statusFileSelectionCheckBox

                            checked: model.selected
                            onClicked: root.controller.ToggleFileSelection(model.path)

                            indicator: Rectangle {
                                implicitWidth: 16
                                implicitHeight: 16
                                x: statusFileSelectionCheckBox.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: 3
                                color: statusFileSelectionCheckBox.checked
                                    ? root.accentColor
                                    : root.panelRaisedColor
                                border.color: statusFileSelectionCheckBox.checked
                                    ? root.accentColor
                                    : root.borderColor
                                border.width: 1

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 2
                                    visible: statusFileSelectionCheckBox.checked
                                    color: "#ffffff"
                                }
                            }
                        }

                        AppIcon {
                            name: model.displayStatus === "??"
                                ? "plus"
                                : model.displayStatus.indexOf("D") !== -1
                                ? "trash-2"
                                : model.displayStatus.indexOf("A") !== -1
                                ? "plus"
                                : model.displayStatus.indexOf("M") !== -1
                                ? "files"
                                : model.staged
                                ? "check"
                                : "files"
                            size: 16
                            Layout.preferredWidth: 28
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Label {
                            text: model.path
                            color: root.textColor
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "+" + model.additions
                            color: root.addedTextColor
                            visible: model.additions > 0
                            font.family: "monospace"
                        }

                        Label {
                            text: "-" + model.deletions
                            color: root.removedTextColor
                            visible: model.deletions > 0
                            font.family: "monospace"
                        }

                        Label {
                            text: model.changes
                            color: root.mutedTextColor
                            visible: model.changes > 0
                            font.family: "monospace"
                        }
                    }
                }
            }
        }
    }

    Pane {
        id: diffPane

        SplitView.fillWidth: true

        background: Rectangle {
            color: root.panelColor
            border.color: root.borderColor
            border.width: 1
        }

        ColumnLayout {
            id: diffLayout

            anchors.fill: parent
            spacing: 8

            Label {
                id: diffTitleLabel

                text: qsTr("Diff")
                color: root.textColor
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Rectangle {
                id: diffViewer

                Layout.fillWidth: true
                Layout.fillHeight: true
                color: root.panelRaisedColor
                border.color: root.borderColor
                border.width: 1
                clip: true

                Label {
                    id: emptyDiffLabel

                    anchors.centerIn: parent
                    visible: !root.controller || root.controller.currentDiff.length === 0
                    text: qsTr("Select a changed file to view its diff.")
                    color: root.mutedTextColor
                }

                ListView {
                    id: diffLinesListView

                    anchors.fill: parent
                    anchors.margins: 1
                    visible: root.controller && root.controller.currentDiff.length > 0
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: root.controller && root.controller.currentDiff.length > 0
                        ? root.controller.currentDiff.split("\n")
                        : []

                    delegate: Rectangle {
                        required property string modelData

                        width: diffLinesListView.width
                        height: diffLineText.implicitHeight + 6
                        color: modelData.startsWith("+")
                            ? root.addedLineColor
                            : (modelData.startsWith("-")
                                ? root.removedLineColor
                                : "transparent")

                        Text {
                            id: diffLineText

                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            text: modelData.length > 0 ? modelData : " "
                            color: modelData.startsWith("+")
                                ? root.addedTextColor
                                : (modelData.startsWith("-")
                                    ? root.removedTextColor
                                    : root.textColor)
                            font.family: "monospace"
                            font.pixelSize: 13
                            elide: Text.ElideNone
                        }
                    }
                }
            }
        }
    }

    Pane {
        id: commitPane

        SplitView.preferredWidth: 320
        SplitView.minimumWidth: 280

        background: Rectangle {
            color: root.panelColor
            border.color: root.borderColor
            border.width: 1
        }

        ColumnLayout {
            id: commitLayout

            anchors.fill: parent
            spacing: 8

            Label {
                id: commitTitleLabel

                text: qsTr("Commit")
                color: root.textColor
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }

            Label {
                id: selectedFileLabel

                Layout.fillWidth: true
                text: root.controller && root.controller.selectedFileCount > 0
                    ? qsTr("%1 selected file(s)").arg(root.controller.selectedFileCount)
                    : (root.controller && root.controller.selectedFilePath.length > 0
                        ? root.controller.selectedFilePath
                        : qsTr("No file selected"))
                color: root.mutedTextColor
                elide: Text.ElideMiddle
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                AppButton {
                    id: stageSelectedFileButton

                    Layout.fillWidth: true
                    text: qsTr("Stage")
                    enabled: root.controller && root.controller.selectedFileCount > 0
                    onClicked: root.controller.StageSelectedFiles()
                }

                AppButton {
                    id: unstageSelectedFileButton

                    Layout.fillWidth: true
                    text: qsTr("Unstage")
                    enabled: root.controller && root.controller.selectedFileCount > 0
                    onClicked: root.controller.UnstageSelectedFiles()
                }
            }

            TextArea {
                id: commitMessageTextArea

                Layout.fillWidth: true
                Layout.preferredHeight: 160
                placeholderText: qsTr("Commit message")
                wrapMode: TextEdit.Wrap
                enabled: root.controller
                    && root.controller.repositoryPath.length > 0
                    && root.controller.repositoryInitialized
                color: root.textColor
                placeholderTextColor: root.mutedTextColor

                background: Rectangle {
                    color: root.panelRaisedColor
                    border.color: root.borderColor
                    border.width: 1
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                AppButton {
                    id: createCommitButton

                    Layout.fillWidth: true
                    text: qsTr("Commit")
                    primary: true
                    enabled: root.controller
                        && !root.controller.pushInProgress
                        && root.controller.repositoryInitialized
                        && root.controller.stagedFileCount > 0
                        && commitMessageTextArea.text.trim().length > 0
                    onClicked: root.controller.CommitStagedFiles(commitMessageTextArea.text)
                }

                AppButton {
                    id: pushRepositoryButton

                    Layout.fillWidth: true
                    text: qsTr("Push")
                    enabled: root.controller
                        && root.controller.repositoryPath.length > 0
                        && root.controller.repositoryInitialized
                        && root.controller.remoteConnected
                        && !root.controller.pushInProgress
                    onClicked: root.controller.PushRepository()
                }
            }

            ProgressBar {
                id: pushProgressBar

                Layout.fillWidth: true
                visible: root.controller && root.controller.pushInProgress
                indeterminate: true

                background: Rectangle {
                    implicitHeight: 4
                    color: root.panelRaisedColor
                    radius: 2
                }

                contentItem: Item {
                    implicitHeight: 4

                    Rectangle {
                        id: pushProgressIndicator

                        width: parent.width * 0.35
                        height: parent.height
                        radius: 2
                        color: root.accentColor

                        NumberAnimation on x {
                            from: 0
                            to: Math.max(0, pushProgressIndicator.parent.width - pushProgressIndicator.width)
                            duration: 900
                            loops: Animation.Infinite
                            running: pushProgressBar.visible
                        }
                    }
                }
            }

            Item {
                id: commitPanelSpacer

                Layout.fillHeight: true
            }

            Label {
                id: backendStatusLabel

                Layout.fillWidth: true
                text: root.controller ? root.controller.statusMessage : ""
                color: root.mutedTextColor
                wrapMode: Text.WordWrap
            }
        }
    }

    Connections {
        target: root.controller
        enabled: root.controller !== null && root.controller !== undefined

        function onCommitCreated() {
            commitMessageTextArea.clear()
        }

        function onPushCompleted() {
            commitMessageTextArea.clear()
        }
    }
}
