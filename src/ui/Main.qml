import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.folderlistmodel

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

    Dialog {
        id: repositoryPickerDialog

        property string currentFolder: ""
        property string selectedFolder: currentFolder

        function normalizeFolder(folderPath) {
            const path = String(folderPath)
            return path.startsWith("file://") ? path : "file://" + path
        }

        function displayPath(folderUrl) {
            const path = decodeURIComponent(String(folderUrl))
            return path.startsWith("file://") ? path.substring(7) : path
        }

        function parentFolder(folderUrl) {
            let path = String(folderUrl)

            if (path.endsWith("/") && path.length > "file:///".length) {
                path = path.substring(0, path.length - 1)
            }

            const lastSlash = path.lastIndexOf("/")
            if (lastSlash <= "file://".length) {
                return "file:///"
            }

            return path.substring(0, lastSlash)
        }

        function openFolder(folderUrl) {
            currentFolder = String(folderUrl)
            selectedFolder = currentFolder
        }

        function prepareOpen() {
            if (appController.repositoryPath.length > 0) {
                openFolder(normalizeFolder(appController.repositoryPath))
            }

            open()
        }

        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        width: Math.min(window.width - 48, 760)
        height: Math.min(window.height - 64, 560)
        modal: true
        focus: true
        title: qsTr("Open Git repository")
        standardButtons: Dialog.NoButton

        background: Rectangle {
            color: window.panelColor
            border.color: window.borderColor
            border.width: 1
        }

        FolderListModel {
            id: repositoryFolderModel

            folder: repositoryPickerDialog.currentFolder
            showDirs: true
            showFiles: false
            showDotAndDotDot: false
            sortField: FolderListModel.Name
        }

        contentItem: ColumnLayout {
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: qsTr("Open repository")
                    color: window.textColor
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }

                AppButton {
                    text: qsTr("Up")
                    enabled: repositoryPickerDialog.currentFolder !== "file:///"
                    onClicked: repositoryPickerDialog.openFolder(
                        repositoryPickerDialog.parentFolder(repositoryPickerDialog.currentFolder))
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 34
                color: window.panelRaisedColor
                border.color: window.borderColor
                border.width: 1

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    verticalAlignment: Text.AlignVCenter
                    text: repositoryPickerDialog.displayPath(repositoryPickerDialog.currentFolder)
                    color: window.mutedTextColor
                    elide: Text.ElideMiddle
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: window.backgroundColor
                border.color: window.borderColor
                border.width: 1
                clip: true

                ListView {
                    id: repositoryFolderListView

                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: repositoryFolderModel
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: ItemDelegate {
                        id: repositoryFolderDelegate

                        required property string fileName
                        required property url fileUrl
                        readonly property bool gitFolder: appController.IsRepositoryFolder(fileUrl)

                        width: repositoryFolderListView.width
                        height: 42
                        highlighted: repositoryPickerDialog.selectedFolder === String(fileUrl)
                        onClicked: repositoryPickerDialog.selectedFolder = String(fileUrl)
                        onDoubleClicked: repositoryPickerDialog.openFolder(fileUrl)

                        background: Rectangle {
                            color: repositoryFolderDelegate.highlighted
                                ? "#333842"
                                : (repositoryFolderDelegate.hovered ? window.panelRaisedColor : "transparent")
                            border.color: repositoryFolderDelegate.highlighted ? window.accentColor : "transparent"
                            border.width: 1
                        }

                        contentItem: RowLayout {
                            spacing: 8

                            Label {
                                text: "dir"
                                color: window.mutedTextColor
                                font.family: "monospace"
                                Layout.preferredWidth: 34
                            }

                            Label {
                                text: repositoryFolderDelegate.fileName
                                color: window.textColor
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                visible: repositoryFolderDelegate.gitFolder
                                color: "#243528"
                                border.color: "#3d6f49"
                                border.width: 1
                                radius: 5
                                Layout.preferredWidth: gitFolderLabel.implicitWidth + 12
                                Layout.preferredHeight: 22

                                Label {
                                    id: gitFolderLabel

                                    anchors.centerIn: parent
                                    text: qsTr("Git")
                                    color: window.addedTextColor
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: repositoryFolderModel.count === 0
                        text: qsTr("No folders")
                        color: window.mutedTextColor
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: qsTr("Selected: ") + repositoryPickerDialog.displayPath(repositoryPickerDialog.selectedFolder)
                    color: window.mutedTextColor
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                AppButton {
                    text: qsTr("Use Current")
                    onClicked: repositoryPickerDialog.selectedFolder = repositoryPickerDialog.currentFolder
                }

                AppButton {
                    text: qsTr("Cancel")
                    onClicked: repositoryPickerDialog.close()
                }

                AppButton {
                    text: qsTr("Open")
                    primary: true
                    onClicked: {
                        appController.OpenRepository(repositoryPickerDialog.selectedFolder)
                        repositoryPickerDialog.close()
                    }
                }
            }
        }

        Component.onCompleted: openFolder(normalizeFolder(StandardPaths.writableLocation(StandardPaths.HomeLocation)))
    }

    Dialog {
        id: connectRepositoryDialog

        function prepareOpen() {
            remoteUrlTextField.text = appController.remoteUrl
            open()
            remoteUrlTextField.forceActiveFocus()
        }

        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        width: Math.min(window.width - 48, 460)
        modal: true
        focus: true
        title: qsTr("Connect repository")
        standardButtons: Dialog.NoButton

        background: Rectangle {
            color: window.panelColor
            border.color: window.borderColor
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            spacing: 12

            Label {
                text: qsTr("Connect repository")
                color: window.textColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }

            Label {
                text: appController.repositoryInitialized
                    ? qsTr("Add an origin remote to this local repository.")
                    : qsTr("Initialize this folder and add an origin remote.")
                color: window.mutedTextColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            TextField {
                id: remoteUrlTextField

                Layout.fillWidth: true
                placeholderText: qsTr("https://github.com/user/repository")
                color: window.textColor
                placeholderTextColor: window.mutedTextColor
                selectByMouse: true

                background: Rectangle {
                    color: window.panelRaisedColor
                    border.color: remoteUrlTextField.activeFocus ? window.accentColor : window.borderColor
                    border.width: 1
                    radius: 5
                }

                onAccepted: {
                    if (appController.ConnectRepository(text)) {
                        connectRepositoryDialog.close()
                    }
                }
            }

            Label {
                text: appController.statusMessage
                color: window.mutedTextColor
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
                    onClicked: connectRepositoryDialog.close()
                }

                AppButton {
                    text: qsTr("Connect")
                    primary: true
                    enabled: remoteUrlTextField.text.trim().length > 0
                    onClicked: {
                        if (appController.ConnectRepository(remoteUrlTextField.text)) {
                            connectRepositoryDialog.close()
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: pushSummaryDialog

        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        width: Math.min(window.width - 48, 360)
        visible: appController.pushSummaryVisible
        modal: true
        focus: true
        title: qsTr("Push completed")
        standardButtons: Dialog.NoButton
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: window.panelColor
            border.color: window.borderColor
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            spacing: 14

            Label {
                text: qsTr("Push completed")
                color: window.textColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Changes were pushed to the remote repository.")
                color: window.mutedTextColor
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: window.borderColor
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Files pushed")
                    color: window.mutedTextColor
                    Layout.fillWidth: true
                }

                Label {
                    text: appController.lastPushFilesChanged
                    color: window.textColor
                    font.family: "monospace"
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Label {
                    text: qsTr("Line changes")
                    color: window.mutedTextColor
                    Layout.fillWidth: true
                }

                Label {
                    text: appController.lastPushLineChanges
                    color: window.textColor
                    font.family: "monospace"
                }
            }

            AppButton {
                text: qsTr("Ok")
                primary: true
                Layout.alignment: Qt.AlignRight
                onClicked: appController.ClosePushSummary()
            }
        }
    }

    header: ToolBar {
        id: mainToolBar

        implicitHeight: 50

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
            anchors.topMargin: 7
            anchors.bottomMargin: 7
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

            Rectangle {
                id: branchSyncStatusBadge

                visible: appController.repositoryPath.length > 0
                    && (appController.syncStatusText.length > 0
                        || appController.repositoryConnectionStatusText.length > 0)
                color: !appController.repositoryInitialized || !appController.remoteConnected
                    ? "#34272a"
                    : appController.hasUpstream
                    ? (appController.aheadCount === 0 && appController.behindCount === 0
                        ? "#243528"
                        : "#353222")
                    : "#34272a"
                border.color: !appController.repositoryInitialized || !appController.remoteConnected
                    ? "#6b3c45"
                    : appController.hasUpstream
                    ? (appController.aheadCount === 0 && appController.behindCount === 0
                        ? "#3d6f49"
                        : "#776a32")
                    : "#6b3c45"
                border.width: 1
                radius: 6
                Layout.preferredHeight: 28
                Layout.preferredWidth: branchSyncStatusLabel.implicitWidth + 20

                Label {
                    id: branchSyncStatusLabel

                    anchors.centerIn: parent
                    text: appController.repositoryInitialized && appController.remoteConnected
                        ? (appController.syncStatusText.length > 0
                            ? appController.syncStatusText
                            : appController.repositoryConnectionStatusText)
                        : appController.repositoryConnectionStatusText
                    color: !appController.repositoryInitialized || !appController.remoteConnected
                        ? window.removedTextColor
                        : appController.hasUpstream
                        ? (appController.aheadCount === 0 && appController.behindCount === 0
                            ? window.addedTextColor
                            : "#e4d58a")
                        : window.removedTextColor
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
            }

            AppButton {
                id: connectRepositoryButton

                text: qsTr("Connect repository")
                visible: appController.repositoryPath.length > 0
                    && (!appController.repositoryInitialized || !appController.remoteConnected)
                enabled: appController.gitAvailable
                    && !appController.fetchInProgress
                    && !appController.pullInProgress
                    && !appController.pushInProgress
                onClicked: connectRepositoryDialog.prepareOpen()
            }

            AppButton {
                id: openRepositoryButton

                text: qsTr("Open")
                primary: true
                enabled: appController.gitAvailable
                onClicked: repositoryPickerDialog.prepareOpen()
            }

            AppButton {
                id: refreshRepositoryButton

                text: appController.repositoryPath.length > 0 ? qsTr("Refresh") : qsTr("Check Git")
                enabled: !appController.fetchInProgress
                    && !appController.pullInProgress
                    && !appController.pushInProgress
                onClicked: appController.repositoryPath.length > 0
                    ? appController.RefreshRepository()
                    : appController.CheckGitAvailable()
            }

            AppButton {
                id: fetchRepositoryButton

                text: qsTr("Fetch")
                enabled: appController.repositoryPath.length > 0
                    && appController.repositoryInitialized
                    && appController.remoteConnected
                    && !appController.fetchInProgress
                    && !appController.pullInProgress
                    && !appController.pushInProgress
                onClicked: appController.FetchRepository()
            }

            AppButton {
                id: pullRepositoryButton

                text: qsTr("Pull")
                enabled: appController.repositoryPath.length > 0
                    && appController.repositoryInitialized
                    && appController.remoteConnected
                    && !appController.fetchInProgress
                    && !appController.pullInProgress
                    && !appController.pushInProgress
                onClicked: appController.PullRepository()
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

                RowLayout {
                    id: changesHeaderLayout

                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        id: changesTitleLabel

                        text: qsTr("Changes")
                        color: window.textColor
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }

                    AppButton {
                        id: selectAllFilesButton

                        text: qsTr("Select all")
                        enabled: changedFilesListView.count > 0
                        onClicked: appController.SelectAllFiles()
                    }

                    AppButton {
                        id: clearFileSelectionButton

                        text: qsTr("Clear")
                        enabled: appController.selectedFileCount > 0
                        onClicked: appController.ClearFileSelection()
                    }
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

                            CheckBox {
                                id: statusFileSelectionCheckBox

                                checked: model.selected
                                onClicked: appController.ToggleFileSelection(model.path)

                                indicator: Rectangle {
                                    implicitWidth: 16
                                    implicitHeight: 16
                                    x: statusFileSelectionCheckBox.leftPadding
                                    y: parent.height / 2 - height / 2
                                    radius: 3
                                    color: statusFileSelectionCheckBox.checked
                                        ? window.accentColor
                                        : window.panelRaisedColor
                                    border.color: statusFileSelectionCheckBox.checked
                                        ? window.accentColor
                                        : window.borderColor
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

                Label {
                    id: selectedFileLabel

                    Layout.fillWidth: true
                    text: appController.selectedFileCount > 0
                        ? qsTr("%1 selected file(s)").arg(appController.selectedFileCount)
                        : (appController.selectedFilePath.length > 0
                            ? appController.selectedFilePath
                            : qsTr("No file selected"))
                    color: window.mutedTextColor
                    elide: Text.ElideMiddle
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    AppButton {
                        id: stageSelectedFileButton

                        Layout.fillWidth: true
                        text: qsTr("Stage")
                        enabled: appController.selectedFileCount > 0
                        onClicked: appController.StageSelectedFiles()
                    }

                    AppButton {
                        id: unstageSelectedFileButton

                        Layout.fillWidth: true
                        text: qsTr("Unstage")
                        enabled: appController.selectedFileCount > 0
                        onClicked: appController.UnstageSelectedFiles()
                    }
                }

                TextArea {
                    id: commitMessageTextArea

                    Layout.fillWidth: true
                    Layout.preferredHeight: 160
                    placeholderText: qsTr("Commit message")
                    wrapMode: TextEdit.Wrap
                    enabled: appController.repositoryPath.length > 0
                        && appController.repositoryInitialized
                    color: window.textColor
                    placeholderTextColor: window.mutedTextColor

                    background: Rectangle {
                        color: window.panelRaisedColor
                        border.color: window.borderColor
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
                        enabled: !appController.pushInProgress
                            && appController.repositoryInitialized
                            && appController.stagedFileCount > 0
                            && commitMessageTextArea.text.trim().length > 0
                        onClicked: appController.CommitStagedFiles(commitMessageTextArea.text)
                    }

                    AppButton {
                        id: pushRepositoryButton

                        Layout.fillWidth: true
                        text: qsTr("Push")
                        enabled: appController.repositoryPath.length > 0
                            && appController.repositoryInitialized
                            && appController.remoteConnected
                            && !appController.pushInProgress
                        onClicked: appController.PushRepository()
                    }
                }

                ProgressBar {
                    id: pushProgressBar

                    Layout.fillWidth: true
                    visible: appController.pushInProgress
                    indeterminate: true

                    background: Rectangle {
                        implicitHeight: 4
                        color: window.panelRaisedColor
                        radius: 2
                    }

                    contentItem: Item {
                        implicitHeight: 4

                        Rectangle {
                            id: pushProgressIndicator

                            width: parent.width * 0.35
                            height: parent.height
                            radius: 2
                            color: window.accentColor

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
                    text: appController.statusMessage
                    color: window.mutedTextColor
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    Connections {
        target: appController

        function onCommitCreated() {
            commitMessageTextArea.clear()
        }

        function onPushCompleted() {
            commitMessageTextArea.clear()
        }
    }

    Component.onCompleted: appController.CheckGitAvailable()
}
