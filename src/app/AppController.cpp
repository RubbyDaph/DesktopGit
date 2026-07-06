#include "AppController.h"

#include <QFutureWatcher>
#include <QStringList>
#include <QtConcurrent>

namespace {

struct PushOperationResult
{
    GitChangeSummary summary;
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

QString AppController::CurrentDiff() const
{
    return currentDiff;
}

StatusFileModel *AppController::StatusFiles()
{
    return &statusFileModel;
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

void AppController::OpenRepositoryPath(const QString &path)
{
    gitRepository.SetPath(path);

    if (!gitRepository.IsValid()) {
        SetRepositoryPath(QString());
        SetCurrentBranch(QString());
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        statusFileModel.Clear();
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetStatusMessage(QStringLiteral("Selected directory is not a Git repository."));
        return;
    }

    SetRepositoryPath(path);
    SetCurrentBranch(gitRepository.CurrentBranch());
    RefreshRepository();

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
        emit SelectedFilesChanged();
        emit StagedFileCountChanged();
        SetSelectedFilePath(QString());
        SetCurrentDiff(QString());
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    const QList<GitStatusFile> files = gitRepository.Status();
    statusFileModel.SetFiles(files);
    emit SelectedFilesChanged();
    emit StagedFileCountChanged();

    SetCurrentBranch(gitRepository.CurrentBranch());
    SetStatusMessage(QStringLiteral("%1 changed file(s).").arg(files.size()));
}

void AppController::SelectStatusFile(const QString &path)
{
    if (repositoryPath.isEmpty()) {
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
    if (repositoryPath.isEmpty()) {
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
    SetSelectedFilePath(QString());
    SetCurrentDiff(QString());
    SetStatusMessage(QStringLiteral("Commit created."));
    emit commitCreated();
}

void AppController::PushRepository()
{
    if (repositoryPath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
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
    if (repositoryPath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
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
    if (repositoryPath.isEmpty()) {
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
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
