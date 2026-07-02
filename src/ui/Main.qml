import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window

    width: 1200
    height: 760
    minimumWidth: 900
    minimumHeight: 560
    visible: true
    title: qsTr("DesktopGit")

    FolderDialog {
        id: repositoryFolderDialog

        title: qsTr("Open Git repository")

        onAccepted: appController.OpenRepository(selectedFolder)
    }

    header: ToolBar {
        id: mainToolBar

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
                color: palette.mid
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

            ColumnLayout {
                id: changesLayout

                anchors.fill: parent
                spacing: 8

                Label {
                    id: changesTitleLabel

                    text: qsTr("Changes")
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
                        width: ListView.view.width
                        text: model.displayStatus + "  " + model.path
                    }
                }
            }
        }

        Pane {
            id: diffPane

            SplitView.fillWidth: true

            ColumnLayout {
                id: diffLayout

                anchors.fill: parent
                spacing: 8

                Label {
                    id: diffTitleLabel

                    text: qsTr("Diff")
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                TextArea {
                    id: diffTextArea

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    readOnly: true
                    wrapMode: TextEdit.NoWrap
                    text: qsTr("Select a changed file to view its diff.")
                    font.family: "monospace"
                }
            }
        }

        Pane {
            id: commitPane

            SplitView.preferredWidth: 320
            SplitView.minimumWidth: 280

            ColumnLayout {
                id: commitLayout

                anchors.fill: parent
                spacing: 8

                Label {
                    id: commitTitleLabel

                    text: qsTr("Commit")
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
                    color: palette.mid
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Component.onCompleted: appController.CheckGitAvailable()
}
