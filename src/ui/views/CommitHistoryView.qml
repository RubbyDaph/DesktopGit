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
    property color addedLineColor: "#173d28"
    property color removedLineColor: "#462323"
    property color addedTextColor: "#8fdda8"
    property color removedTextColor: "#f0a0a0"
    property color accentColor: "#7aa2f7"

    ColumnLayout {
        id: historyLayout

        anchors.fill: parent
        anchors.margins: 10
        spacing: 10
        enabled: root.controller !== null && root.controller !== undefined
        visible: enabled

        RowLayout {
            id: historyToolbarLayout

            Layout.fillWidth: true
            spacing: 8

            AppButton {
                id: closeHistoryButton

                text: qsTr("Back")
                onClicked: root.controller.CloseHistory()
            }

            Label {
                id: historyTitleLabel

                text: qsTr("Commit History")
                color: root.textColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Label {
                id: historyRepositoryLabel

                text: root.controller ? root.controller.repositoryPath : ""
                color: root.mutedTextColor
                elide: Text.ElideMiddle
                Layout.fillWidth: true
            }

            AppButton {
                id: refreshHistoryButton

                text: qsTr("Refresh")
                onClicked: root.controller.RefreshCommitHistory()
            }
        }

        Rectangle {
            id: commitDetailsPanel

            Layout.fillWidth: true
            Layout.maximumHeight: 132
            implicitHeight: commitDetailsContent.implicitHeight + 18
            color: root.panelColor
            border.color: root.borderColor
            border.width: 1
            radius: 6

            ColumnLayout {
                id: commitDetailsContent

                anchors.fill: parent
                anchors.margins: 9
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: root.controller && root.controller.selectedCommitSubject.length > 0
                            ? root.controller.selectedCommitSubject
                            : qsTr("Select a commit")
                        color: root.textColor
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: root.controller ? root.controller.selectedCommitShortHash : ""
                        color: root.accentColor
                        font.family: "monospace"
                        visible: text.length > 0
                    }

                    AppButton {
                        text: qsTr("Copy hash")
                        enabled: root.controller && root.controller.selectedCommitHash.length > 0
                        onClicked: root.controller.CopySelectedCommitHash()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.controller ? root.controller.selectedCommitBody : ""
                    visible: text.length > 0
                    color: root.textColor
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: root.controller && root.controller.selectedCommitAuthorName.length > 0
                            ? root.controller.selectedCommitAuthorName
                            : qsTr("No author")
                        color: root.mutedTextColor
                        elide: Text.ElideRight
                    }

                    Label {
                        text: root.controller && root.controller.selectedCommitAuthorEmail.length > 0
                            ? "<" + root.controller.selectedCommitAuthorEmail + ">"
                            : ""
                        visible: text.length > 0
                        color: root.mutedTextColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: root.controller ? root.controller.selectedCommitDate : ""
                        visible: text.length > 0
                        color: root.mutedTextColor
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideRight
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.controller ? root.controller.selectedCommitHash : ""
                    visible: text.length > 0
                    color: root.mutedTextColor
                    font.family: "monospace"
                    elide: Text.ElideMiddle
                }
            }
        }

        SplitView {
            id: historySplitView

            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Pane {
                id: commitHistoryPane

                SplitView.preferredWidth: 420
                SplitView.minimumWidth: 320

                background: Rectangle {
                    color: root.panelColor
                    border.color: root.borderColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: qsTr("Commits")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    ListView {
                        id: commitHistoryListView

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.controller ? root.controller.commitHistoryModel : null
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: ItemDelegate {
                            id: commitHistoryDelegate

                            required property string hash
                            required property string shortHash
                            required property string subject
                            required property string authorName
                            required property string date

                            width: commitHistoryListView.width
                            height: Math.max(78, commitHistoryContent.implicitHeight + 12)
                            highlighted: root.controller ? root.controller.selectedCommitHash === hash : false
                            onClicked: root.controller.SelectCommit(hash)
                            ToolTip.visible: hovered && subject.length > 0
                            ToolTip.delay: 450
                            ToolTip.text: subject

                            background: Rectangle {
                                color: commitHistoryDelegate.highlighted
                                    ? "#333842"
                                    : (commitHistoryDelegate.hovered ? root.panelRaisedColor : "transparent")
                                border.color: commitHistoryDelegate.highlighted ? root.accentColor : "transparent"
                                border.width: 1
                            }

                            contentItem: ColumnLayout {
                                id: commitHistoryContent

                                spacing: 3

                                RowLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: shortHash
                                        color: root.accentColor
                                        font.family: "monospace"
                                        font.pixelSize: 12
                                    }

                                    Label {
                                        text: date
                                        color: root.mutedTextColor
                                        elide: Text.ElideRight
                                        horizontalAlignment: Text.AlignRight
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                }

                                Label {
                                    text: subject.length > 0 ? subject : qsTr("(no subject)")
                                    color: root.textColor
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: authorName
                                    color: root.mutedTextColor
                                    elide: Text.ElideRight
                                    font.pixelSize: 12
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: commitHistoryListView.count === 0
                            text: qsTr("No commits yet")
                            color: root.mutedTextColor
                        }
                    }
                }
            }

            Pane {
                id: commitFilesPane

                SplitView.preferredWidth: 320
                SplitView.minimumWidth: 260

                background: Rectangle {
                    color: root.panelColor
                    border.color: root.borderColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: qsTr("Files")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }

                    ListView {
                        id: commitFilesListView

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.controller ? root.controller.commitFileModel : null
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: ItemDelegate {
                            id: commitFileDelegate

                            required property string path
                            required property string status
                            required property int additions
                            required property int deletions
                            required property int changes

                            width: commitFilesListView.width
                            highlighted: root.controller ? root.controller.selectedCommitFilePath === path : false
                            onClicked: root.controller.SelectCommitFile(path)

                            background: Rectangle {
                                color: commitFileDelegate.highlighted
                                    ? "#333842"
                                    : (commitFileDelegate.hovered ? root.panelRaisedColor : "transparent")
                                border.color: commitFileDelegate.highlighted ? root.accentColor : "transparent"
                                border.width: 1
                            }

                            contentItem: RowLayout {
                                spacing: 8

                                Label {
                                    text: status.length > 0 ? status : "?"
                                    color: root.textColor
                                    font.family: "monospace"
                                    Layout.preferredWidth: 26
                                }

                                Label {
                                    text: path
                                    color: root.textColor
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }

                                Label {
                                    text: "+" + additions
                                    color: root.addedTextColor
                                    visible: additions > 0
                                    font.family: "monospace"
                                }

                                Label {
                                    text: "-" + deletions
                                    color: root.removedTextColor
                                    visible: deletions > 0
                                    font.family: "monospace"
                                }

                                Label {
                                    text: changes
                                    color: root.mutedTextColor
                                    visible: changes > 0
                                    font.family: "monospace"
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: commitFilesListView.count === 0
                            text: root.controller && root.controller.selectedCommitHash.length > 0
                                ? qsTr("No files")
                                : qsTr("Select a commit")
                            color: root.mutedTextColor
                        }
                    }
                }
            }

            Pane {
                id: commitDiffPane

                SplitView.fillWidth: true

                background: Rectangle {
                    color: root.panelColor
                    border.color: root.borderColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: root.controller && root.controller.selectedCommitFilePath.length > 0
                            ? root.controller.selectedCommitFilePath
                            : qsTr("Commit Diff")
                        color: root.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: root.panelRaisedColor
                        border.color: root.borderColor
                        border.width: 1
                        clip: true

                        Label {
                            anchors.centerIn: parent
                            visible: !root.controller || root.controller.selectedCommitDiff.length === 0
                            text: root.controller && root.controller.selectedCommitFilePath.length > 0
                                ? qsTr("No diff for this file")
                                : qsTr("Select a file from this commit")
                            color: root.mutedTextColor
                        }

                        ListView {
                            id: commitDiffLinesListView

                            anchors.fill: parent
                            anchors.margins: 1
                            visible: root.controller && root.controller.selectedCommitDiff.length > 0
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            model: root.controller && root.controller.selectedCommitDiff.length > 0
                                ? root.controller.selectedCommitDiff.split("\n")
                                : []

                            delegate: Rectangle {
                                required property string modelData

                                width: commitDiffLinesListView.width
                                height: commitDiffLineText.implicitHeight + 6
                                color: modelData.startsWith("+")
                                    ? root.addedLineColor
                                    : (modelData.startsWith("-")
                                        ? root.removedLineColor
                                        : "transparent")

                                Text {
                                    id: commitDiffLineText

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
        }
    }
}
