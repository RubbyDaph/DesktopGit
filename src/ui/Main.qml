import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window

    readonly property color backgroundColor: "#1f2023"
    readonly property color panelColor: "#26282c"
    readonly property color panelRaisedColor: "#2b2d31"
    readonly property color borderColor: "#3a3d42"
    readonly property color textColor: "#e7e9ee"
    readonly property color mutedTextColor: "#9aa0aa"
    readonly property color addedLineColor: "#173d28"
    readonly property color removedLineColor: "#462323"
    readonly property color addedTextColor: "#8fdda8"
    readonly property color removedTextColor: "#f0a0a0"
    readonly property color accentColor: "#7aa2f7"

    width: 1200
    height: 760
    minimumWidth: 900
    minimumHeight: 560
    visible: true
    title: qsTr("DesktopGit")
    color: backgroundColor

    palette.window: backgroundColor
    palette.windowText: textColor
    palette.base: panelRaisedColor
    palette.alternateBase: panelColor
    palette.text: textColor
    palette.button: panelRaisedColor
    palette.buttonText: textColor
    palette.mid: mutedTextColor
    palette.highlight: accentColor
    palette.highlightedText: "#ffffff"

    FolderDialog {
        id: repositoryFolderDialog

        title: qsTr("Open Git repository")

        onAccepted: appController.OpenRepository(selectedFolder)
    }

    header: ToolBar {
        id: mainToolBar

        background: Rectangle {
            color: window.panelColor
            border.color: window.borderColor
            border.width: 1
        }

        RowLayout {
            id: mainToolBarLayout

            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            Label {
                id: appTitleLabel

                text: qsTr("DesktopGit")
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Label {
                id: repositoryPathLabel

                text: appController.repositoryPath.length > 0
                    ? appController.repositoryPath + (appController.currentBranch.length > 0
                        ? "  |  " + appController.currentBranch
                        : "")
                    : qsTr("No repository opened")
                color: window.mutedTextColor
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            Button {
                id: openRepositoryButton

                text: qsTr("Open")
                enabled: appController.gitAvailable
                onClicked: repositoryFolderDialog.open()
            }

            Button {
                id: refreshRepositoryButton

                text: appController.repositoryPath.length > 0 ? qsTr("Refresh") : qsTr("Check Git")
                onClicked: appController.repositoryPath.length > 0
                    ? appController.RefreshRepository()
                    : appController.CheckGitAvailable()
            }
        }
    }

    SplitView {
        id: mainSplitView

        anchors.fill: parent
        orientation: Qt.Horizontal

        Pane {
            id: changesPane

            SplitView.preferredWidth: 300
            SplitView.minimumWidth: 240

            background: Rectangle {
                color: window.panelColor
                border.color: window.borderColor
                border.width: 1
            }

            ColumnLayout {
                id: changesLayout

                anchors.fill: parent
                spacing: 8

                Label {
                    id: changesTitleLabel

                    text: qsTr("Changes")
                    color: window.textColor
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                ListView {
                    id: changedFilesListView

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    model: appController.statusFileModel

                    delegate: ItemDelegate {
                        id: statusFileDelegate

                        width: ListView.view.width
                        highlighted: appController.selectedFilePath === model.path
                        onClicked: appController.SelectStatusFile(model.path)

                        background: Rectangle {
                            color: appController.selectedFilePath === model.path
                                ? "#333842"
                                : (statusFileDelegate.hovered ? window.panelRaisedColor : "transparent")
                            border.color: appController.selectedFilePath === model.path
                                ? window.accentColor
                                : "transparent"
                            border.width: 1
                        }

                        contentItem: RowLayout {
                            spacing: 8

                            Label {
                                text: model.displayStatus
                                color: window.textColor
                                font.family: "monospace"
                                Layout.preferredWidth: 28
                            }

                            Label {
                                text: model.path
                                color: window.textColor
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            Label {
                                text: "+" + model.additions
                                color: window.addedTextColor
                                visible: model.additions > 0
                                font.family: "monospace"
                            }

                            Label {
                                text: "-" + model.deletions
                                color: window.removedTextColor
                                visible: model.deletions > 0
                                font.family: "monospace"
                            }

                            Label {
                                text: model.changes
                                color: window.mutedTextColor
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
                color: window.panelColor
                border.color: window.borderColor
                border.width: 1
            }

            ColumnLayout {
                id: diffLayout

                anchors.fill: parent
                spacing: 8

                Label {
                    id: diffTitleLabel

                    text: qsTr("Diff")
                    color: window.textColor
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                Rectangle {
                    id: diffViewer

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: window.panelRaisedColor
                    border.color: window.borderColor
                    border.width: 1
                    clip: true

                    Label {
                        id: emptyDiffLabel

                        anchors.centerIn: parent
                        visible: appController.currentDiff.length === 0
                        text: qsTr("Select a changed file to view its diff.")
                        color: window.mutedTextColor
                    }

                    ListView {
                        id: diffLinesListView

                        anchors.fill: parent
                        anchors.margins: 1
                        visible: appController.currentDiff.length > 0
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        model: appController.currentDiff.length > 0
                            ? appController.currentDiff.split("\n")
                            : []

                        delegate: Rectangle {
                            required property string modelData

                            width: diffLinesListView.width
                            height: diffLineText.implicitHeight + 6
                            color: modelData.startsWith("+")
                                ? window.addedLineColor
                                : (modelData.startsWith("-")
                                    ? window.removedLineColor
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
                                    ? window.addedTextColor
                                    : (modelData.startsWith("-")
                                        ? window.removedTextColor
                                        : window.textColor)
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
                color: window.panelColor
                border.color: window.borderColor
                border.width: 1
            }

            ColumnLayout {
                id: commitLayout

                anchors.fill: parent
                spacing: 8

                Label {
                    id: commitTitleLabel

                    text: qsTr("Commit")
                    color: window.textColor
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                TextArea {
                    id: commitMessageTextArea

                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
                    placeholderText: qsTr("Commit message")
                    wrapMode: TextEdit.Wrap
                    enabled: false
                    color: window.textColor
                    placeholderTextColor: window.mutedTextColor

                    background: Rectangle {
                        color: window.panelRaisedColor
                        border.color: window.borderColor
                        border.width: 1
                    }
                }

                Button {
                    id: createCommitButton

                    Layout.fillWidth: true
                    text: qsTr("Commit")
                    enabled: false
                }

                Item {
                    id: commitPanelSpacer

                    Layout.fillHeight: true
                }

                Label {
                    id: backendStatusLabel

                    Layout.fillWidth: true
                    text: appController.statusMessage
                    color: window.mutedTextColor
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Component.onCompleted: appController.CheckGitAvailable()
}
