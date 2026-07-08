#include "AppController.h"

#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QStringList>
#include <QtConcurrent>

namespace {

struct PushOperationResult
{
    GitChangeSummary summary;
    GitCommandResult commandResult;
};

struct CloneOperationResult
{
    QString targetPath;
    GitCommandResult commandResult;
};

}

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    SetStatusMessage(QStringLiteral("Git backend is not checked yet."));
}

bool AppController::GitAvailable() const
{
    return gitAvailable;
}

QString AppController::GitVersion() const
{
    return gitVersion;
}

QString AppController::StatusMessage() const
{
    return statusMessage;
}

QString AppController::RepositoryPath() const
{
    return repositoryPath;
}

QString AppController::CurrentBranch() const
{
    return currentBranch;
}

bool AppController::RepositoryInitialized() const
{
    return repositoryInitialized;
}

bool AppController::RemoteConnected() const
{
    return remoteConnected;
}

QString AppController::RemoteUrl() const
{
    return remoteUrl;
}

QString AppController::RepositoryConnectionStatusText() const
{
    return repositoryConnectionStatusText;
}

int AppController::AheadCount() const
{
    return aheadCount;
}

int AppController::BehindCount() const
{
    return behindCount;
}

bool AppController::HasUpstream() const
{
    return hasUpstream;
}

QString AppController::SyncStatusText() const
{
    return syncStatusText;
}

QString AppController::SelectedFilePath() const
{
    return selectedFilePath;
}

int AppController::SelectedFileCount() const
{
    return statusFileModel.SelectedCount();
}

int AppController::StagedFileCount() const
{
    return statusFileModel.StagedCount();
}

int AppController::LastPushFilesChanged() const
{
    return lastPushFilesChanged;
}

int AppController::LastPushLineChanges() const
{
    return lastPushLineChanges;
}

bool AppController::PushSummaryVisible() const
{
    return pushSummaryVisible;
}

bool AppController::PushInProgress() const
{
    return pushInProgress;
}

bool AppController::FetchInProgress() const
{
    return fetchInProgress;
}

bool AppController::PullInProgress() const
{
    return pullInProgress;
}

bool AppController::CloneInProgress() const
{
    return cloneInProgress;
}

bool AppController::HistoryVisible() const
{
    return historyVisible;
}

bool AppController::BranchesVisible() const
{
    return branchesVisible;
}

QString AppController::SelectedBranchName() const
{
    return selectedBranchName;
}

QString AppController::SelectedCommitHash() const
{
    return selectedCommitHash;
}

QString AppController::SelectedCommitShortHash() const
{
    return selectedCommitShortHash;
}

QString AppController::SelectedCommitSubject() const
{
    return selectedCommitSubject;
}

QString AppController::SelectedCommitBody() const
{
    return selectedCommitBody;
}

QString AppController::SelectedCommitAuthorName() const
{
    return selectedCommitAuthorName;
}

QString AppController::SelectedCommitAuthorEmail() const
{
    return selectedCommitAuthorEmail;
}

QString AppController::SelectedCommitDate() const
{
    return selectedCommitDate;
}

QString AppController::SelectedCommitFilePath() const
{
    return selectedCommitFilePath;
}

QString AppController::SelectedCommitDiff() const
{
    return selectedCommitDiff;
}

QString AppController::CurrentDiff() const
{
    return currentDiff;
}

StatusFileModel *AppController::StatusFiles()
{
    return &statusFileModel;
}

BranchModel *AppController::Branches()
{
    return &branchModel;
}

CommitHistoryModel *AppController::CommitHistory()
{
    return &commitHistoryModel;
}

CommitFileModel *AppController::CommitFiles()
{
    return &commitFileModel;
}

void AppController::CheckGitAvailable()
{
    const GitCommandResult result = gitCommandRunner.Run({QStringLiteral("--version")});

    if (result.Success()) {
        SetGitAvailable(true);
        SetGitVersion(result.standardOutput.trimmed());
        SetStatusMessage(gitVersion);
        return;
    }

    SetGitAvailable(false);
    SetGitVersion(QString());

    const QString error = result.standardError.trimmed();
    SetStatusMessage(error.isEmpty()
        ? QStringLiteral("Git executable was not found.")
        : error);
}

void AppController::OpenRepository(const QUrl &repositoryUrl)
{
    OpenRepositoryPath(repositoryUrl.toLocalFile());
}

bool AppController::IsRepositoryFolder(const QUrl &folderUrl) const
{
    const QString folderPath = folderUrl.toLocalFile();
    if (folderPath.isEmpty()) {
        return false;
    }

    return QFileInfo(QDir(folderPath).filePath(QStringLiteral(".git"))).exists();
}

void AppController::OpenRepositoryPath(const QString &path)
{
    const QFileInfo pathInfo(path);
    if (path.trimmed().isEmpty() || !pathInfo.exists() || !pathInfo.isDir()) {
        SetRepositoryPath(QString());
        SetCurrentBranch(QString());
        SetRepositoryConnectionState(false, false, QString(), QString());
        ClearBranchSyncStatus();
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        statusFileModel.Clear();
        branchModel.Clear();
        commitHistoryModel.Clear();
        commitFileModel.Clear();
        ClearCommitSelection();
        SetHistoryVisible(false);
        SetBranchesVisible(false);
        SetSelectedBranchName(QString());
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetStatusMessage(QStringLiteral("Selected path is not a directory."));
        return;
    }

    SetRepositoryPath(path);
    gitRepository.SetPath(path);
    branchModel.Clear();
    commitHistoryModel.Clear();
    commitFileModel.Clear();
    ClearCommitSelection();
    SetHistoryVisible(false);
    SetBranchesVisible(false);
    SetSelectedBranchName(QString());
    RefreshRepositoryConnectionState();

    if (!repositoryInitialized) {
        SetCurrentBranch(QString());
        ClearBranchSyncStatus();
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        statusFileModel.Clear();
        branchModel.Clear();
        commitHistoryModel.Clear();
        commitFileModel.Clear();
        ClearCommitSelection();
        SetHistoryVisible(false);
        SetBranchesVisible(false);
        SetSelectedBranchName(QString());
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetStatusMessage(QStringLiteral("Folder opened. Repository is not initialized."));
        return;
    }

    if (!gitRepository.IsValid()) {
        SetCurrentBranch(QString());
        ClearBranchSyncStatus();
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        statusFileModel.Clear();
        branchModel.Clear();
        commitHistoryModel.Clear();
        commitFileModel.Clear();
        ClearCommitSelection();
        SetHistoryVisible(false);
        SetBranchesVisible(false);
        SetSelectedBranchName(QString());
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetStatusMessage(QStringLiteral("Folder opened, but it is not a Git work tree."));
        return;
    }

    SetCurrentBranch(gitRepository.CurrentBranch());
    RefreshBranchSyncStatus();
    RefreshRepository();

    if (!remoteConnected) {
        SetStatusMessage(QStringLiteral("Repository opened. Remote origin is not connected."));
        return;
    }

    if (currentBranch.isEmpty()) {
        SetStatusMessage(QStringLiteral("Repository opened. Detached HEAD or no branch detected."));
        return;
    }

    SetStatusMessage(QStringLiteral("Repository opened."));
}

void AppController::RefreshRepository()
{
    if (repositoryPath.isEmpty()) {
        statusFileModel.Clear();
        branchModel.Clear();
        commitHistoryModel.Clear();
        commitFileModel.Clear();
        ClearCommitSelection();
        SetHistoryVisible(false);
        SetBranchesVisible(false);
        SetSelectedBranchName(QString());
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    RefreshRepositoryConnectionState();
    if (!repositoryInitialized) {
        statusFileModel.Clear();
        branchModel.Clear();
        commitHistoryModel.Clear();
        commitFileModel.Clear();
        ClearCommitSelection();
        SetHistoryVisible(false);
        SetBranchesVisible(false);
        SetSelectedBranchName(QString());
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        ClearBranchSyncStatus();
        SetStatusMessage(QStringLiteral("Repository is not initialized."));
        return;
    }

    if (!gitRepository.IsValid()) {
        statusFileModel.Clear();
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        ClearBranchSyncStatus();
        SetStatusMessage(QStringLiteral("Opened folder is not a Git work tree."));
        return;
    }

    const QList<GitStatusFile> files = gitRepository.Status();
    statusFileModel.SetFiles(files);
    emit SelectedFilesChanged();
    emit StagedFileCountChanged();

    SetCurrentBranch(gitRepository.CurrentBranch());
    if (branchesVisible) {
        RefreshBranches();
    }
    RefreshBranchSyncStatus();
    SetStatusMessage(QStringLiteral("%1 changed file(s).").arg(files.size()));
}

void AppController::SelectStatusFile(const QString &path)
{
    if (repositoryPath.isEmpty() || !repositoryInitialized) {
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    SetSelectedFilePath(path);

    const QString diff = gitRepository.Diff(path);
    SetCurrentDiff(diff);

    if (diff.isEmpty()) {
        SetStatusMessage(QStringLiteral("No unstaged diff for %1.").arg(path));
        return;
    }

    SetStatusMessage(QStringLiteral("Showing diff for %1.").arg(path));
}

void AppController::ToggleFileSelection(const QString &path)
{
    statusFileModel.ToggleSelected(path);
    emit SelectedFilesChanged();
}

void AppController::SelectAllFiles()
{
    statusFileModel.SelectAll();
    emit SelectedFilesChanged();
}

void AppController::ClearFileSelection()
{
    statusFileModel.ClearSelection();
    emit SelectedFilesChanged();
}

void AppController::StageSelectedFile()
{
    if (!repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Initialize repository first."));
        return;
    }

    if (selectedFilePath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Select a file first."));
        return;
    }

    const QString filePath = selectedFilePath;
    if (!gitRepository.StageFile(filePath)) {
        SetStatusMessage(QStringLiteral("Failed to stage %1.").arg(filePath));
        return;
    }

    RefreshRepository();
    SelectStatusFile(filePath);
    SetStatusMessage(QStringLiteral("Staged %1.").arg(filePath));
}

void AppController::UnstageSelectedFile()
{
    if (!repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Initialize repository first."));
        return;
    }

    if (selectedFilePath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Select a file first."));
        return;
    }

    const QString filePath = selectedFilePath;
    if (!gitRepository.UnstageFile(filePath)) {
        SetStatusMessage(QStringLiteral("Failed to unstage %1.").arg(filePath));
        return;
    }

    RefreshRepository();
    SelectStatusFile(filePath);
    SetStatusMessage(QStringLiteral("Unstaged %1.").arg(filePath));
}

void AppController::StageSelectedFiles()
{
    if (!repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Initialize repository first."));
        return;
    }

    const QStringList filePaths = statusFileModel.SelectedPaths();
    if (filePaths.isEmpty()) {
        SetStatusMessage(QStringLiteral("Select at least one file first."));
        return;
    }

    int stagedCount = 0;
    QString failedFilePath;
    for (const QString &filePath : filePaths) {
        if (gitRepository.StageFile(filePath)) {
            ++stagedCount;
            continue;
        }

        failedFilePath = filePath;
        break;
    }

    const QString activeFilePath = selectedFilePath;
    RefreshRepository();
    if (!activeFilePath.isEmpty()) {
        SelectStatusFile(activeFilePath);
    }

    if (!failedFilePath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Staged %1 file(s), failed to stage %2.")
            .arg(stagedCount)
            .arg(failedFilePath));
        return;
    }

    SetStatusMessage(QStringLiteral("Staged %1 file(s).").arg(stagedCount));
}

void AppController::UnstageSelectedFiles()
{
    if (!repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Initialize repository first."));
        return;
    }

    const QStringList filePaths = statusFileModel.SelectedPaths();
    if (filePaths.isEmpty()) {
        SetStatusMessage(QStringLiteral("Select at least one file first."));
        return;
    }

    int unstagedCount = 0;
    QString failedFilePath;
    for (const QString &filePath : filePaths) {
        if (gitRepository.UnstageFile(filePath)) {
            ++unstagedCount;
            continue;
        }

        failedFilePath = filePath;
        break;
    }

    const QString activeFilePath = selectedFilePath;
    RefreshRepository();
    if (!activeFilePath.isEmpty()) {
        SelectStatusFile(activeFilePath);
    }

    if (!failedFilePath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Unstaged %1 file(s), failed to unstage %2.")
            .arg(unstagedCount)
            .arg(failedFilePath));
        return;
    }

    SetStatusMessage(QStringLiteral("Unstaged %1 file(s).").arg(unstagedCount));
}

void AppController::CommitStagedFiles(const QString &message)
{
    const QString trimmedMessage = message.trimmed();
    if (repositoryPath.isEmpty() || !repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    if (statusFileModel.StagedCount() == 0) {
        SetStatusMessage(QStringLiteral("Stage at least one file before committing."));
        return;
    }

    if (trimmedMessage.isEmpty()) {
        SetStatusMessage(QStringLiteral("Commit message cannot be empty."));
        return;
    }

    const GitCommandResult result = gitRepository.Commit(trimmedMessage);
    if (!result.Success()) {
        const QString error = result.standardError.trimmed();
        SetStatusMessage(error.isEmpty()
            ? QStringLiteral("Failed to create commit.")
            : error);
        return;
    }

    RefreshRepository();
    ClearFileSelection();
    commitHistoryModel.Clear();
    commitFileModel.Clear();
    ClearCommitSelection();
    SetSelectedFilePath(QString());
    SetCurrentDiff(QString());
    SetStatusMessage(QStringLiteral("Commit created."));
    emit commitCreated();
}

void AppController::PushRepository()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    if (!remoteConnected) {
        SetStatusMessage(QStringLiteral("Connect remote origin before pushing."));
        return;
    }

    if (pushInProgress) {
        return;
    }

    SetPushInProgress(true);
    SetStatusMessage(QStringLiteral("Pushing changes..."));

    const QString pushedRepositoryPath = repositoryPath;
    auto *watcher = new QFutureWatcher<PushOperationResult>(this);
    connect(watcher, &QFutureWatcher<PushOperationResult>::finished, this, [this, watcher, pushedRepositoryPath]() {
        const PushOperationResult pushResult = watcher->result();
        watcher->deleteLater();

        SetPushInProgress(false);

        if (!pushResult.commandResult.Success()) {
            const QString output = (pushResult.commandResult.standardError.trimmed().isEmpty()
                ? pushResult.commandResult.standardOutput
                : pushResult.commandResult.standardError).trimmed();
            if (pushResult.commandResult.timedOut) {
                SetStatusMessage(QStringLiteral("Push timed out. Check your network connection or Git credentials."));
                return;
            }

            SetStatusMessage(output.isEmpty()
                ? QStringLiteral("Failed to push repository.")
                : output);
            return;
        }

        lastPushFilesChanged = pushResult.summary.filesChanged;
        lastPushLineChanges = pushResult.summary.LineChanges();
        emit LastPushSummaryChanged();

        if (repositoryPath == pushedRepositoryPath) {
            RefreshRepository();
            RefreshBranchSyncStatus();
        }
        SetStatusMessage(QStringLiteral("Push completed."));
        SetPushSummaryVisible(true);
        emit pushCompleted();
    });

    watcher->setFuture(QtConcurrent::run([pushedRepositoryPath]() {
        GitRepository repository;
        repository.SetPath(pushedRepositoryPath);

        PushOperationResult result;
        result.summary = repository.OutgoingChangeSummary();
        result.commandResult = repository.Push();
        return result;
    }));
}

void AppController::ClosePushSummary()
{
    SetPushSummaryVisible(false);
}

void AppController::FetchRepository()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    if (!remoteConnected) {
        SetStatusMessage(QStringLiteral("Connect remote origin before fetching."));
        return;
    }

    if (fetchInProgress || pullInProgress || pushInProgress) {
        return;
    }

    SetFetchInProgress(true);
    SetStatusMessage(QStringLiteral("Fetching changes..."));

    const QString fetchedRepositoryPath = repositoryPath;
    auto *watcher = new QFutureWatcher<GitCommandResult>(this);
    connect(watcher, &QFutureWatcher<GitCommandResult>::finished, this, [this, watcher, fetchedRepositoryPath]() {
        const GitCommandResult result = watcher->result();
        watcher->deleteLater();

        SetFetchInProgress(false);

        if (!result.Success()) {
            const QString output = (result.standardError.trimmed().isEmpty()
                ? result.standardOutput
                : result.standardError).trimmed();
            if (result.timedOut) {
                SetStatusMessage(QStringLiteral("Fetch timed out. Check your network connection or Git credentials."));
                return;
            }

            SetStatusMessage(output.isEmpty()
                ? QStringLiteral("Failed to fetch repository.")
                : output);
            return;
        }

        if (repositoryPath == fetchedRepositoryPath) {
            RefreshRepository();
            RefreshBranchSyncStatus();
        }

        SetStatusMessage(QStringLiteral("Fetch completed."));
        emit fetchCompleted();
    });

    watcher->setFuture(QtConcurrent::run([fetchedRepositoryPath]() {
        GitRepository repository;
        repository.SetPath(fetchedRepositoryPath);
        return repository.Fetch();
    }));
}

void AppController::PullRepository()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized) {
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    if (!remoteConnected) {
        SetStatusMessage(QStringLiteral("Connect remote origin before pulling."));
        return;
    }

    if (fetchInProgress || pullInProgress || pushInProgress) {
        return;
    }

    SetPullInProgress(true);
    SetStatusMessage(QStringLiteral("Pulling changes..."));

    const QString pulledRepositoryPath = repositoryPath;
    auto *watcher = new QFutureWatcher<GitCommandResult>(this);
    connect(watcher, &QFutureWatcher<GitCommandResult>::finished, this, [this, watcher, pulledRepositoryPath]() {
        const GitCommandResult result = watcher->result();
        watcher->deleteLater();

        SetPullInProgress(false);

        if (!result.Success()) {
            const QString output = (result.standardError.trimmed().isEmpty()
                ? result.standardOutput
                : result.standardError).trimmed();
            if (result.timedOut) {
                SetStatusMessage(QStringLiteral("Pull timed out. Check your network connection or Git credentials."));
                return;
            }

            SetStatusMessage(output.isEmpty()
                ? QStringLiteral("Failed to pull repository.")
                : output);
            return;
        }

        if (repositoryPath == pulledRepositoryPath) {
            RefreshRepository();
            RefreshBranchSyncStatus();
        }

        SetStatusMessage(QStringLiteral("Pull completed."));
        emit pullCompleted();
    });

    watcher->setFuture(QtConcurrent::run([pulledRepositoryPath]() {
        GitRepository repository;
        repository.SetPath(pulledRepositoryPath);
        return repository.Pull();
    }));
}

QString AppController::DefaultCloneFolderName(const QString &remoteUrl) const
{
    return GitRepository::DefaultCloneFolderName(remoteUrl);
}

void AppController::CloneRepository(
    const QString &remoteUrl,
    const QString &parentDirectory,
    const QString &folderName)
{
    if (cloneInProgress || fetchInProgress || pullInProgress || pushInProgress) {
        return;
    }

    const QString normalizedRemoteUrl = GitRepository::NormalizeRemoteUrl(remoteUrl);
    if (normalizedRemoteUrl.isEmpty()) {
        SetStatusMessage(QStringLiteral("Remote URL is invalid."));
        return;
    }

    const QString trimmedParentDirectory = parentDirectory.trimmed();
    if (trimmedParentDirectory.isEmpty() || !QFileInfo(trimmedParentDirectory).isDir()) {
        SetStatusMessage(QStringLiteral("Clone parent folder is invalid."));
        return;
    }

    const QString trimmedFolderName = folderName.trimmed();
    if (trimmedFolderName.isEmpty()
        || trimmedFolderName.contains(QLatin1Char('/'))
        || trimmedFolderName.contains(QLatin1Char('\\'))) {
        SetStatusMessage(QStringLiteral("Clone folder name is invalid."));
        return;
    }

    const QString targetPath = QDir(trimmedParentDirectory).filePath(trimmedFolderName);
    if (QFileInfo(targetPath).exists()) {
        SetStatusMessage(QStringLiteral("Clone target folder already exists."));
        return;
    }

    SetCloneInProgress(true);
    SetStatusMessage(QStringLiteral("Cloning repository..."));

    auto *watcher = new QFutureWatcher<CloneOperationResult>(this);
    connect(watcher, &QFutureWatcher<CloneOperationResult>::finished, this, [this, watcher]() {
        const CloneOperationResult cloneResult = watcher->result();
        watcher->deleteLater();

        SetCloneInProgress(false);

        if (!cloneResult.commandResult.Success()) {
            const QString output = (cloneResult.commandResult.standardError.trimmed().isEmpty()
                ? cloneResult.commandResult.standardOutput
                : cloneResult.commandResult.standardError).trimmed();
            if (cloneResult.commandResult.timedOut) {
                SetStatusMessage(QStringLiteral("Clone timed out. Check your network connection or Git credentials."));
                return;
            }

            SetStatusMessage(output.isEmpty()
                ? QStringLiteral("Failed to clone repository.")
                : output);
            return;
        }

        OpenRepositoryPath(cloneResult.targetPath);
        SetStatusMessage(QStringLiteral("Repository cloned."));
        emit cloneCompleted();
    });

    watcher->setFuture(QtConcurrent::run([normalizedRemoteUrl, trimmedParentDirectory, trimmedFolderName, targetPath]() {
        GitRepository repository;
        CloneOperationResult result;
        result.targetPath = targetPath;
        result.commandResult = repository.CloneRepository(
            normalizedRemoteUrl,
            trimmedParentDirectory,
            trimmedFolderName);
        return result;
    }));
}

bool AppController::ConnectRepository(const QString &remoteUrl)
{
    if (repositoryPath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Open a project folder first."));
        return false;
    }

    const QString normalizedRemoteUrl = GitRepository::NormalizeRemoteUrl(remoteUrl);
    if (normalizedRemoteUrl.isEmpty()) {
        SetStatusMessage(QStringLiteral("Remote URL is invalid."));
        return false;
    }

    if (!repositoryInitialized) {
        const GitCommandResult initResult = gitRepository.InitializeRepository();
        if (!initResult.Success()) {
            const QString error = initResult.standardError.trimmed();
            SetStatusMessage(error.isEmpty()
                ? QStringLiteral("Failed to initialize repository.")
                : error);
            RefreshRepositoryConnectionState();
            return false;
        }
    }

    RefreshRepositoryConnectionState();
    if (!remoteConnected) {
        const GitCommandResult remoteResult = gitRepository.AddRemote(QStringLiteral("origin"), normalizedRemoteUrl);
        if (!remoteResult.Success()) {
            const QString error = remoteResult.standardError.trimmed();
            SetStatusMessage(error.isEmpty()
                ? QStringLiteral("Failed to connect remote origin.")
                : error);
            RefreshRepositoryConnectionState();
            return false;
        }
    }

    RefreshRepositoryConnectionState();
    RefreshRepository();
    SetStatusMessage(QStringLiteral("Repository connected."));
    return true;
}

void AppController::OpenHistory()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized || !gitRepository.IsValid()) {
        SetStatusMessage(QStringLiteral("Open an initialized Git repository first."));
        return;
    }

    SetBranchesVisible(false);
    SetHistoryVisible(true);
    RefreshCommitHistory();
}

void AppController::CloseHistory()
{
    SetHistoryVisible(false);
}

void AppController::OpenBranches()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized || !gitRepository.IsValid()) {
        SetStatusMessage(QStringLiteral("Open an initialized Git repository first."));
        return;
    }

    SetHistoryVisible(false);
    SetBranchesVisible(true);
    RefreshBranches();
}

void AppController::CloseBranches()
{
    SetBranchesVisible(false);
}

void AppController::RefreshBranches()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized || !gitRepository.IsValid()) {
        branchModel.Clear();
        SetSelectedBranchName(QString());
        SetStatusMessage(QStringLiteral("Open an initialized Git repository first."));
        return;
    }

    const QList<GitBranchInfo> branches = gitRepository.LocalBranches();
    branchModel.SetBranches(branches);

    if (branches.isEmpty()) {
        SetSelectedBranchName(QString());
        SetStatusMessage(QStringLiteral("No local branches yet."));
        return;
    }

    if (!branchModel.ContainsBranch(selectedBranchName)) {
        const QString currentBranchName = gitRepository.CurrentBranch();
        if (branchModel.ContainsBranch(currentBranchName)) {
            SetSelectedBranchName(currentBranchName);
        } else {
            SetSelectedBranchName(branches.first().name);
        }
    }

    SetStatusMessage(QStringLiteral("%1 local branch(es) loaded.").arg(branches.size()));
}

void AppController::SelectBranch(const QString &name)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty() || !branchModel.ContainsBranch(trimmedName)) {
        SetSelectedBranchName(QString());
        return;
    }

    SetSelectedBranchName(trimmedName);
}

void AppController::CheckoutSelectedBranch()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized || !gitRepository.IsValid()) {
        SetStatusMessage(QStringLiteral("Open an initialized Git repository first."));
        return;
    }

    if (selectedBranchName.isEmpty()) {
        SetStatusMessage(QStringLiteral("Select a branch first."));
        return;
    }

    if (selectedBranchName == currentBranch) {
        SetStatusMessage(QStringLiteral("Branch %1 is already checked out.").arg(selectedBranchName));
        return;
    }

    const GitCommandResult result = gitRepository.CheckoutBranch(selectedBranchName);
    if (!result.Success()) {
        const QString output = (result.standardError.trimmed().isEmpty()
            ? result.standardOutput
            : result.standardError).trimmed();
        SetStatusMessage(output.isEmpty()
            ? QStringLiteral("Failed to checkout branch.")
            : output);
        return;
    }

    SetCurrentBranch(gitRepository.CurrentBranch());
    RefreshRepository();
    RefreshBranches();
    SetStatusMessage(QStringLiteral("Checked out %1.").arg(currentBranch));
}

void AppController::CreateBranch(const QString &name)
{
    if (repositoryPath.isEmpty() || !repositoryInitialized || !gitRepository.IsValid()) {
        SetStatusMessage(QStringLiteral("Open an initialized Git repository first."));
        return;
    }

    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        SetStatusMessage(QStringLiteral("Branch name cannot be empty."));
        return;
    }

    if (!gitRepository.ValidateBranchName(trimmedName)) {
        SetStatusMessage(QStringLiteral("Branch name is invalid."));
        return;
    }

    if (branchModel.ContainsBranch(trimmedName)) {
        SetStatusMessage(QStringLiteral("Branch %1 already exists.").arg(trimmedName));
        return;
    }

    const GitCommandResult result = gitRepository.CreateBranch(trimmedName);
    if (!result.Success()) {
        const QString output = (result.standardError.trimmed().isEmpty()
            ? result.standardOutput
            : result.standardError).trimmed();
        SetStatusMessage(output.isEmpty()
            ? QStringLiteral("Failed to create branch.")
            : output);
        return;
    }

    SetCurrentBranch(gitRepository.CurrentBranch());
    SetSelectedBranchName(currentBranch);
    RefreshRepository();
    RefreshBranches();
    SetStatusMessage(QStringLiteral("Created and checked out %1.").arg(currentBranch));
}

void AppController::RefreshCommitHistory()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized || !gitRepository.IsValid()) {
        commitHistoryModel.Clear();
        commitFileModel.Clear();
        ClearCommitSelection();
        SetStatusMessage(QStringLiteral("Open an initialized Git repository first."));
        return;
    }

    const QList<GitCommitInfo> commits = gitRepository.CommitHistory(100);
    commitHistoryModel.SetCommits(commits);

    if (commits.isEmpty()) {
        commitFileModel.Clear();
        ClearCommitSelection();
        SetStatusMessage(QStringLiteral("No commits yet."));
        return;
    }

    if (!commitHistoryModel.ContainsHash(selectedCommitHash)) {
        SelectCommit(commits.first().hash);
    } else {
        SelectCommit(selectedCommitHash);
    }

    SetStatusMessage(QStringLiteral("%1 commit(s) loaded.").arg(commits.size()));
}

void AppController::SelectCommit(const QString &hash)
{
    if (hash.trimmed().isEmpty() || !commitHistoryModel.ContainsHash(hash)) {
        commitFileModel.Clear();
        ClearCommitSelection();
        return;
    }

    SetSelectedCommitHash(hash);
    SetSelectedCommitDetails(commitHistoryModel.CommitByHash(hash));
    SetSelectedCommitFilePath(QString());
    SetSelectedCommitDiff(QString());

    const QList<GitCommitFile> files = gitRepository.CommitFiles(hash);
    commitFileModel.SetFiles(files);

    SetStatusMessage(QStringLiteral("%1 file(s) in selected commit.").arg(files.size()));
}

void AppController::SelectCommitFile(const QString &path)
{
    if (selectedCommitHash.isEmpty() || path.isEmpty() || !commitFileModel.ContainsPath(path)) {
        SetSelectedCommitFilePath(QString());
        SetSelectedCommitDiff(QString());
        return;
    }

    SetSelectedCommitFilePath(path);
    SetSelectedCommitDiff(gitRepository.CommitFileDiff(selectedCommitHash, path));
    SetStatusMessage(QStringLiteral("Showing commit diff for %1.").arg(path));
}

void AppController::CopySelectedCommitHash()
{
    if (selectedCommitHash.isEmpty()) {
        SetStatusMessage(QStringLiteral("Select a commit first."));
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        SetStatusMessage(QStringLiteral("Clipboard is not available."));
        return;
    }

    clipboard->setText(selectedCommitHash);
    SetStatusMessage(QStringLiteral("Commit hash copied."));
}

void AppController::SetGitAvailable(bool value)
{
    if (gitAvailable == value) {
        return;
    }

    gitAvailable = value;
    emit GitAvailableChanged();
}

void AppController::SetGitVersion(const QString &value)
{
    if (gitVersion == value) {
        return;
    }

    gitVersion = value;
    emit GitVersionChanged();
}

void AppController::SetStatusMessage(const QString &value)
{
    if (statusMessage == value) {
        return;
    }

    statusMessage = value;
    emit StatusMessageChanged();
}

void AppController::SetRepositoryPath(const QString &value)
{
    if (repositoryPath == value) {
        return;
    }

    repositoryPath = value;
    emit RepositoryPathChanged();
}

void AppController::SetCurrentBranch(const QString &value)
{
    if (currentBranch == value) {
        return;
    }

    currentBranch = value;
    emit CurrentBranchChanged();
}

void AppController::SetRepositoryConnectionState(
    bool initialized,
    bool connected,
    const QString &url,
    const QString &statusText)
{
    if (repositoryInitialized == initialized
        && remoteConnected == connected
        && remoteUrl == url
        && repositoryConnectionStatusText == statusText) {
        return;
    }

    repositoryInitialized = initialized;
    remoteConnected = connected;
    remoteUrl = url;
    repositoryConnectionStatusText = statusText;
    emit RepositoryConnectionChanged();
}

void AppController::SetBranchSyncStatus(const GitBranchSyncStatus &status)
{
    QString nextSyncStatusText;
    if (!status.hasUpstream) {
        nextSyncStatusText = QStringLiteral("no upstream");
    } else if (status.ahead > 0 && status.behind > 0) {
        nextSyncStatusText = QStringLiteral("ahead %1 / behind %2").arg(status.ahead).arg(status.behind);
    } else if (status.ahead > 0) {
        nextSyncStatusText = QStringLiteral("ahead %1").arg(status.ahead);
    } else if (status.behind > 0) {
        nextSyncStatusText = QStringLiteral("behind %1").arg(status.behind);
    } else {
        nextSyncStatusText = QStringLiteral("synced");
    }

    if (aheadCount == status.ahead
        && behindCount == status.behind
        && hasUpstream == status.hasUpstream
        && syncStatusText == nextSyncStatusText) {
        return;
    }

    aheadCount = status.ahead;
    behindCount = status.behind;
    hasUpstream = status.hasUpstream;
    syncStatusText = nextSyncStatusText;
    emit BranchSyncStatusChanged();
}

void AppController::ClearBranchSyncStatus()
{
    if (aheadCount == 0 && behindCount == 0 && !hasUpstream && syncStatusText.isEmpty()) {
        return;
    }

    aheadCount = 0;
    behindCount = 0;
    hasUpstream = false;
    syncStatusText = QString();
    emit BranchSyncStatusChanged();
}

void AppController::RefreshRepositoryConnectionState()
{
    if (repositoryPath.isEmpty()) {
        SetRepositoryConnectionState(false, false, QString(), QString());
        return;
    }

    gitRepository.SetPath(repositoryPath);

    const bool initialized = gitRepository.IsInitialized();
    if (!initialized) {
        SetRepositoryConnectionState(false, false, QString(), QStringLiteral("not initialized"));
        return;
    }

    const QString originUrl = gitRepository.RemoteUrl(QStringLiteral("origin"));
    const bool connected = !originUrl.isEmpty();
    SetRepositoryConnectionState(
        true,
        connected,
        originUrl,
        connected ? QStringLiteral("remote connected") : QStringLiteral("remote not connected"));
}

void AppController::RefreshBranchSyncStatus()
{
    if (repositoryPath.isEmpty() || !repositoryInitialized || currentBranch.isEmpty()) {
        ClearBranchSyncStatus();
        return;
    }

    SetBranchSyncStatus(gitRepository.BranchSyncStatus());
}

void AppController::SetSelectedFilePath(const QString &value)
{
    if (selectedFilePath == value) {
        return;
    }

    selectedFilePath = value;
    emit SelectedFilePathChanged();
}

void AppController::SetCurrentDiff(const QString &value)
{
    if (currentDiff == value) {
        return;
    }

    currentDiff = value;
    emit CurrentDiffChanged();
}

void AppController::SetPushSummaryVisible(bool value)
{
    if (pushSummaryVisible == value) {
        return;
    }

    pushSummaryVisible = value;
    emit PushSummaryVisibleChanged();
}

void AppController::SetPushInProgress(bool value)
{
    if (pushInProgress == value) {
        return;
    }

    pushInProgress = value;
    emit PushInProgressChanged();
}

void AppController::SetFetchInProgress(bool value)
{
    if (fetchInProgress == value) {
        return;
    }

    fetchInProgress = value;
    emit FetchInProgressChanged();
}

void AppController::SetPullInProgress(bool value)
{
    if (pullInProgress == value) {
        return;
    }

    pullInProgress = value;
    emit PullInProgressChanged();
}

void AppController::SetCloneInProgress(bool value)
{
    if (cloneInProgress == value) {
        return;
    }

    cloneInProgress = value;
    emit CloneInProgressChanged();
}

void AppController::SetHistoryVisible(bool value)
{
    if (historyVisible == value) {
        return;
    }

    historyVisible = value;
    emit HistoryVisibleChanged();
}

void AppController::SetBranchesVisible(bool value)
{
    if (branchesVisible == value) {
        return;
    }

    branchesVisible = value;
    emit BranchesVisibleChanged();
}

void AppController::SetSelectedBranchName(const QString &value)
{
    if (selectedBranchName == value) {
        return;
    }

    selectedBranchName = value;
    emit SelectedBranchChanged();
}

void AppController::SetSelectedCommitHash(const QString &value)
{
    if (selectedCommitHash == value) {
        return;
    }

    selectedCommitHash = value;
    emit SelectedCommitChanged();
}

void AppController::SetSelectedCommitDetails(const GitCommitInfo &commit)
{
    if (selectedCommitShortHash == commit.shortHash
        && selectedCommitSubject == commit.subject
        && selectedCommitBody == commit.body
        && selectedCommitAuthorName == commit.authorName
        && selectedCommitAuthorEmail == commit.authorEmail
        && selectedCommitDate == commit.date) {
        return;
    }

    selectedCommitShortHash = commit.shortHash;
    selectedCommitSubject = commit.subject;
    selectedCommitBody = commit.body;
    selectedCommitAuthorName = commit.authorName;
    selectedCommitAuthorEmail = commit.authorEmail;
    selectedCommitDate = commit.date;
    emit SelectedCommitChanged();
}

void AppController::SetSelectedCommitFilePath(const QString &value)
{
    if (selectedCommitFilePath == value) {
        return;
    }

    selectedCommitFilePath = value;
    emit SelectedCommitFileChanged();
}

void AppController::SetSelectedCommitDiff(const QString &value)
{
    if (selectedCommitDiff == value) {
        return;
    }

    selectedCommitDiff = value;
    emit SelectedCommitDiffChanged();
}

void AppController::ClearCommitSelection()
{
    SetSelectedCommitHash(QString());
    SetSelectedCommitDetails({});
    SetSelectedCommitFilePath(QString());
    SetSelectedCommitDiff(QString());
}
