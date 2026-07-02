#include "AppController.h"

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
        SetStatusMessage(QStringLiteral("Open a Git repository first."));
        return;
    }

    const QList<GitStatusFile> files = gitRepository.Status();
    statusFileModel.SetFiles(files);

    SetCurrentBranch(gitRepository.CurrentBranch());
    SetStatusMessage(QStringLiteral("%1 changed file(s).").arg(files.size()));
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
