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
        id: cloneRepositoryDialog

        property string currentFolder: ""
        property string selectedParentFolder: currentFolder
        property bool folderNameEdited: false

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
            selectedParentFolder = currentFolder
        }

        function prepareOpen() {
            cloneRemoteUrlTextField.text = ""
            cloneFolderNameTextField.text = ""
            folderNameEdited = false
            if (appController.repositoryPath.length > 0) {
                openFolder(parentFolder(normalizeFolder(appController.repositoryPath)))
            }
            open()
            cloneRemoteUrlTextField.forceActiveFocus()
        }

        x: Math.round((window.width - width) / 2)
        y: Math.round((window.height - height) / 2)
        width: Math.min(window.width - 48, 760)
        height: Math.min(window.height - 64, 620)
        modal: true
        focus: true
        title: qsTr("Clone repository")
        standardButtons: Dialog.NoButton
        closePolicy: appController.cloneInProgress ? Popup.NoAutoClose : Popup.CloseOnEscape

        background: Rectangle {
            color: window.panelColor
            border.color: window.borderColor
            border.width: 1
            radius: 6
        }

        FolderListModel {
            id: cloneParentFolderModel

            folder: cloneRepositoryDialog.currentFolder
            showDirs: true
            showFiles: false
            showDotAndDotDot: false
            sortField: FolderListModel.Name
        }

        contentItem: ColumnLayout {
            spacing: 10

            Label {
                text: qsTr("Clone repository")
                color: window.textColor
                font.pixelSize: 18
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }

            TextField {
                id: cloneRemoteUrlTextField

                Layout.fillWidth: true
                placeholderText: qsTr("https://github.com/user/repository")
                color: window.textColor
                placeholderTextColor: window.mutedTextColor
                selectByMouse: true
                enabled: !appController.cloneInProgress

                background: Rectangle {
                    color: window.panelRaisedColor
                    border.color: cloneRemoteUrlTextField.activeFocus ? window.accentColor : window.borderColor
                    border.width: 1
                    radius: 5
                }

                onTextChanged: {
                    if (!cloneRepositoryDialog.folderNameEdited) {
                        cloneFolderNameTextField.text = appController.DefaultCloneFolderName(text)
                    }
                }
            }

            TextField {
                id: cloneFolderNameTextField

                Layout.fillWidth: true
                placeholderText: qsTr("folder-name")
                color: window.textColor
                placeholderTextColor: window.mutedTextColor
                selectByMouse: true
                enabled: !appController.cloneInProgress

                background: Rectangle {
                    color: window.panelRaisedColor
                    border.color: cloneFolderNameTextField.activeFocus ? window.accentColor : window.borderColor
                    border.width: 1
                    radius: 5
                }

                onTextEdited: cloneRepositoryDialog.folderNameEdited = true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: qsTr("Parent folder")
                    color: window.textColor
                    font.weight: Font.DemiBold
                }

                Label {
                    text: cloneRepositoryDialog.displayPath(cloneRepositoryDialog.selectedParentFolder)
                    color: window.mutedTextColor
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                AppButton {
                    text: qsTr("Up")
                    enabled: !appController.cloneInProgress
                        && cloneRepositoryDialog.currentFolder !== "file:///"
                    onClicked: cloneRepositoryDialog.openFolder(
                        cloneRepositoryDialog.parentFolder(cloneRepositoryDialog.currentFolder))
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
                    id: cloneParentFolderListView

                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    model: cloneParentFolderModel
                    enabled: !appController.cloneInProgress
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: ItemDelegate {
                        id: cloneParentFolderDelegate

                        required property string fileName
                        required property url fileUrl

                        width: cloneParentFolderListView.width
                        height: 42
                        highlighted: cloneRepositoryDialog.selectedParentFolder === String(fileUrl)
                        onClicked: cloneRepositoryDialog.selectedParentFolder = String(fileUrl)
                        onDoubleClicked: cloneRepositoryDialog.openFolder(fileUrl)

                        background: Rectangle {
                            color: cloneParentFolderDelegate.highlighted
                                ? "#333842"
                                : (cloneParentFolderDelegate.hovered ? window.panelRaisedColor : "transparent")
                            border.color: cloneParentFolderDelegate.highlighted ? window.accentColor : "transparent"
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
                                text: cloneParentFolderDelegate.fileName
                                color: window.textColor
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        visible: cloneParentFolderModel.count === 0
                        text: qsTr("No folders")
                        color: window.mutedTextColor
                    }
                }
            }

            ProgressBar {
                Layout.fillWidth: true
                visible: appController.cloneInProgress
                indeterminate: visible
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
                    enabled: !appController.cloneInProgress
                    onClicked: cloneRepositoryDialog.close()
                }

                AppButton {
                    text: qsTr("Clone")
                    primary: true
                    enabled: !appController.cloneInProgress
                        && cloneRemoteUrlTextField.text.trim().length > 0
                        && cloneFolderNameTextField.text.trim().length > 0
                    onClicked: appController.CloneRepository(
                        cloneRemoteUrlTextField.text,
                        cloneRepositoryDialog.displayPath(cloneRepositoryDialog.selectedParentFolder),
                        cloneFolderNameTextField.text)
                }
            }
        }

        Connections {
            target: appController

            function onCloneCompleted() {
                cloneRepositoryDialog.close()
            }
        }

        Component.onCompleted: openFolder(normalizeFolder(StandardPaths.writableLocation(StandardPaths.HomeLocation)))
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
                id: fetchRepositoryButton

                text: qsTr("Fetch")
                enabled: appController.repositoryPath.length > 0
                    && appController.repositoryInitialized
                    && appController.remoteConnected
                    && !appController.cloneInProgress
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
                    && !appController.cloneInProgress
                    && !appController.fetchInProgress
                    && !appController.pullInProgress
                    && !appController.pushInProgress
                onClicked: appController.PullRepository()
            }

            AppButton {
                id: repositoryMenuButton

                text: qsTr("Repository")
                primary: appController.repositoryPath.length === 0
                enabled: appController.gitAvailable
                    && !appController.cloneInProgress
                onClicked: repositoryMenu.open()

                Menu {
                    id: repositoryMenu

                    y: repositoryMenuButton.height + 4
                    width: 210

                    background: Rectangle {
                        color: window.panelColor
                        border.color: window.borderColor
                        border.width: 1
                        radius: 6
                    }

                    MenuItem {
                        text: qsTr("Open repository")
                        enabled: appController.gitAvailable
                            && !appController.cloneInProgress
                        onTriggered: repositoryPickerDialog.prepareOpen()
                    }

                    MenuItem {
                        text: qsTr("Clone repository")
                        enabled: appController.gitAvailable
                            && !appController.cloneInProgress
                            && !appController.fetchInProgress
                            && !appController.pullInProgress
                            && !appController.pushInProgress
                        onTriggered: cloneRepositoryDialog.prepareOpen()
                    }

                    MenuSeparator {}

                    MenuItem {
                        text: qsTr("Refresh")
                        enabled: appController.repositoryPath.length > 0
                            && !appController.cloneInProgress
                            && !appController.fetchInProgress
                            && !appController.pullInProgress
                            && !appController.pushInProgress
                        onTriggered: appController.RefreshRepository()
                    }

                    MenuItem {
                        text: qsTr("Connect remote")
                        visible: appController.repositoryPath.length > 0
                            && (!appController.repositoryInitialized || !appController.remoteConnected)
                        enabled: appController.gitAvailable
                            && !appController.cloneInProgress
                            && !appController.fetchInProgress
                            && !appController.pullInProgress
                            && !appController.pushInProgress
                        onTriggered: connectRepositoryDialog.prepareOpen()
                    }
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            id: sectionSidebar

            Layout.fillHeight: true
            Layout.preferredWidth: 142
            color: window.panelColor
            border.color: window.borderColor
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                AppButton {
                    id: changesNavigationButton

                    Layout.fillWidth: true
                    text: qsTr("Changes")
                    primary: !appController.historyVisible
                        && !appController.branchesVisible
                        && !appController.stashVisible
                    enabled: appController.repositoryPath.length > 0
                        && appController.repositoryInitialized
                    onClicked: appController.OpenWorkingTree()
                }

                AppButton {
                    id: historyNavigationButton

                    Layout.fillWidth: true
                    text: qsTr("History")
                    primary: appController.historyVisible
                    enabled: appController.repositoryPath.length > 0
                        && appController.repositoryInitialized
                        && !appController.cloneInProgress
                        && !appController.fetchInProgress
                        && !appController.pullInProgress
                        && !appController.pushInProgress
                    onClicked: appController.OpenHistory()
                }

                AppButton {
                    id: branchesNavigationButton

                    Layout.fillWidth: true
                    text: qsTr("Branches")
                    primary: appController.branchesVisible
                    enabled: appController.repositoryPath.length > 0
                        && appController.repositoryInitialized
                        && !appController.cloneInProgress
                        && !appController.fetchInProgress
                        && !appController.pullInProgress
                        && !appController.pushInProgress
                    onClicked: appController.OpenBranches()
                }

                AppButton {
                    id: stashNavigationButton

                    Layout.fillWidth: true
                    text: qsTr("Stash")
                    primary: appController.stashVisible
                    enabled: appController.repositoryPath.length > 0
                        && appController.repositoryInitialized
                        && !appController.cloneInProgress
                        && !appController.fetchInProgress
                        && !appController.pullInProgress
                        && !appController.pushInProgress
                    onClicked: appController.OpenStash()
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }

        StackLayout {
            id: mainContentStack

            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: appController.stashVisible ? 3
                : appController.branchesVisible ? 2
                : appController.historyVisible ? 1
                : 0

            WorkingTreeView {
                id: workingTreePage

                controller: appController
                panelColor: window.panelColor
                panelRaisedColor: window.panelRaisedColor
                borderColor: window.borderColor
                textColor: window.textColor
                mutedTextColor: window.mutedTextColor
                addedLineColor: window.addedLineColor
                removedLineColor: window.removedLineColor
                addedTextColor: window.addedTextColor
                removedTextColor: window.removedTextColor
                accentColor: window.accentColor
            }

            CommitHistoryView {
                id: historyPage

                controller: appController
                panelColor: window.panelColor
                panelRaisedColor: window.panelRaisedColor
                borderColor: window.borderColor
                textColor: window.textColor
                mutedTextColor: window.mutedTextColor
                addedLineColor: window.addedLineColor
                removedLineColor: window.removedLineColor
                addedTextColor: window.addedTextColor
                removedTextColor: window.removedTextColor
                accentColor: window.accentColor
            }

            BranchesView {
                id: branchesPage

                controller: appController
                panelColor: window.panelColor
                panelRaisedColor: window.panelRaisedColor
                borderColor: window.borderColor
                textColor: window.textColor
                mutedTextColor: window.mutedTextColor
                addedTextColor: window.addedTextColor
                accentColor: window.accentColor
            }

            StashView {
                id: stashPage

                controller: appController
                panelColor: window.panelColor
                panelRaisedColor: window.panelRaisedColor
                borderColor: window.borderColor
                textColor: window.textColor
                mutedTextColor: window.mutedTextColor
                removedTextColor: window.removedTextColor
                accentColor: window.accentColor
            }
        }
    }

    Component.onCompleted: appController.CheckGitAvailable()
}
