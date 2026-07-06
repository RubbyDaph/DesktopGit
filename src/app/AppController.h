#pragma once

#include "GitCommandRunner.h"
#include "GitRepository.h"
#include "StatusFileModel.h"

#include <QObject>
#include <QString>
#include <QUrl>

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool gitAvailable READ GitAvailable NOTIFY GitAvailableChanged)
    Q_PROPERTY(QString gitVersion READ GitVersion NOTIFY GitVersionChanged)
    Q_PROPERTY(QString statusMessage READ StatusMessage NOTIFY StatusMessageChanged)
    Q_PROPERTY(QString repositoryPath READ RepositoryPath NOTIFY RepositoryPathChanged)
    Q_PROPERTY(QString currentBranch READ CurrentBranch NOTIFY CurrentBranchChanged)
    Q_PROPERTY(int aheadCount READ AheadCount NOTIFY BranchSyncStatusChanged)
    Q_PROPERTY(int behindCount READ BehindCount NOTIFY BranchSyncStatusChanged)
    Q_PROPERTY(bool hasUpstream READ HasUpstream NOTIFY BranchSyncStatusChanged)
    Q_PROPERTY(QString syncStatusText READ SyncStatusText NOTIFY BranchSyncStatusChanged)
    Q_PROPERTY(QString selectedFilePath READ SelectedFilePath NOTIFY SelectedFilePathChanged)
    Q_PROPERTY(int selectedFileCount READ SelectedFileCount NOTIFY SelectedFilesChanged)
    Q_PROPERTY(int stagedFileCount READ StagedFileCount NOTIFY StagedFileCountChanged)
    Q_PROPERTY(int lastPushFilesChanged READ LastPushFilesChanged NOTIFY LastPushSummaryChanged)
    Q_PROPERTY(int lastPushLineChanges READ LastPushLineChanges NOTIFY LastPushSummaryChanged)
    Q_PROPERTY(bool pushSummaryVisible READ PushSummaryVisible NOTIFY PushSummaryVisibleChanged)
    Q_PROPERTY(bool pushInProgress READ PushInProgress NOTIFY PushInProgressChanged)
    Q_PROPERTY(bool fetchInProgress READ FetchInProgress NOTIFY FetchInProgressChanged)
    Q_PROPERTY(bool pullInProgress READ PullInProgress NOTIFY PullInProgressChanged)
    Q_PROPERTY(QString currentDiff READ CurrentDiff NOTIFY CurrentDiffChanged)
    Q_PROPERTY(StatusFileModel* statusFileModel READ StatusFiles CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);

    [[nodiscard]] bool GitAvailable() const;
    [[nodiscard]] QString GitVersion() const;
    [[nodiscard]] QString StatusMessage() const;
    [[nodiscard]] QString RepositoryPath() const;
    [[nodiscard]] QString CurrentBranch() const;
    [[nodiscard]] int AheadCount() const;
    [[nodiscard]] int BehindCount() const;
    [[nodiscard]] bool HasUpstream() const;
    [[nodiscard]] QString SyncStatusText() const;
    [[nodiscard]] QString SelectedFilePath() const;
    [[nodiscard]] int SelectedFileCount() const;
    [[nodiscard]] int StagedFileCount() const;
    [[nodiscard]] int LastPushFilesChanged() const;
    [[nodiscard]] int LastPushLineChanges() const;
    [[nodiscard]] bool PushSummaryVisible() const;
    [[nodiscard]] bool PushInProgress() const;
    [[nodiscard]] bool FetchInProgress() const;
    [[nodiscard]] bool PullInProgress() const;
    [[nodiscard]] QString CurrentDiff() const;
    [[nodiscard]] class StatusFileModel *StatusFiles();

    Q_INVOKABLE void CheckGitAvailable();
    Q_INVOKABLE void OpenRepository(const QUrl &repositoryUrl);
    Q_INVOKABLE void OpenRepositoryPath(const QString &path);
    Q_INVOKABLE void RefreshRepository();
    Q_INVOKABLE void SelectStatusFile(const QString &path);
    Q_INVOKABLE void ToggleFileSelection(const QString &path);
    Q_INVOKABLE void SelectAllFiles();
    Q_INVOKABLE void ClearFileSelection();
    Q_INVOKABLE void StageSelectedFile();
    Q_INVOKABLE void UnstageSelectedFile();
    Q_INVOKABLE void StageSelectedFiles();
    Q_INVOKABLE void UnstageSelectedFiles();
    Q_INVOKABLE void CommitStagedFiles(const QString &message);
    Q_INVOKABLE void PushRepository();
    Q_INVOKABLE void FetchRepository();
    Q_INVOKABLE void PullRepository();
    Q_INVOKABLE void ClosePushSummary();

signals:
    void GitAvailableChanged();
    void GitVersionChanged();
    void StatusMessageChanged();
    void RepositoryPathChanged();
    void CurrentBranchChanged();
    void BranchSyncStatusChanged();
    void SelectedFilePathChanged();
    void SelectedFilesChanged();
    void StagedFileCountChanged();
    void LastPushSummaryChanged();
    void PushSummaryVisibleChanged();
    void PushInProgressChanged();
    void FetchInProgressChanged();
    void PullInProgressChanged();
    void CurrentDiffChanged();
    void commitCreated();
    void pushCompleted();
    void fetchCompleted();
    void pullCompleted();

private:
    void SetGitAvailable(bool value);
    void SetGitVersion(const QString &value);
    void SetStatusMessage(const QString &value);
    void SetRepositoryPath(const QString &value);
    void SetCurrentBranch(const QString &value);
    void SetBranchSyncStatus(const GitBranchSyncStatus &status);
    void ClearBranchSyncStatus();
    void RefreshBranchSyncStatus();
    void SetSelectedFilePath(const QString &value);
    void SetCurrentDiff(const QString &value);
    void SetPushSummaryVisible(bool value);
    void SetPushInProgress(bool value);
    void SetFetchInProgress(bool value);
    void SetPullInProgress(bool value);

    GitCommandRunner gitCommandRunner;
    GitRepository gitRepository;
    class StatusFileModel statusFileModel;
    bool gitAvailable = false;
    QString gitVersion;
    QString statusMessage;
    QString repositoryPath;
    QString currentBranch;
    int aheadCount = 0;
    int behindCount = 0;
    bool hasUpstream = false;
    QString syncStatusText;
    QString selectedFilePath;
    QString currentDiff;
    int lastPushFilesChanged = 0;
    int lastPushLineChanges = 0;
    bool pushSummaryVisible = false;
    bool pushInProgress = false;
    bool fetchInProgress = false;
    bool pullInProgress = false;
};
