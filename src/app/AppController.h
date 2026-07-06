#pragma once

#include "CommitFileModel.h"
#include "CommitHistoryModel.h"
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
    Q_PROPERTY(bool repositoryInitialized READ RepositoryInitialized NOTIFY RepositoryConnectionChanged)
    Q_PROPERTY(bool remoteConnected READ RemoteConnected NOTIFY RepositoryConnectionChanged)
    Q_PROPERTY(QString remoteUrl READ RemoteUrl NOTIFY RepositoryConnectionChanged)
    Q_PROPERTY(QString repositoryConnectionStatusText READ RepositoryConnectionStatusText NOTIFY RepositoryConnectionChanged)
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
    Q_PROPERTY(bool historyVisible READ HistoryVisible NOTIFY HistoryVisibleChanged)
    Q_PROPERTY(QString selectedCommitHash READ SelectedCommitHash NOTIFY SelectedCommitChanged)
    Q_PROPERTY(QString selectedCommitFilePath READ SelectedCommitFilePath NOTIFY SelectedCommitFileChanged)
    Q_PROPERTY(QString selectedCommitDiff READ SelectedCommitDiff NOTIFY SelectedCommitDiffChanged)
    Q_PROPERTY(QString currentDiff READ CurrentDiff NOTIFY CurrentDiffChanged)
    Q_PROPERTY(StatusFileModel* statusFileModel READ StatusFiles CONSTANT)
    Q_PROPERTY(CommitHistoryModel* commitHistoryModel READ CommitHistory CONSTANT)
    Q_PROPERTY(CommitFileModel* commitFileModel READ CommitFiles CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);

    [[nodiscard]] bool GitAvailable() const;
    [[nodiscard]] QString GitVersion() const;
    [[nodiscard]] QString StatusMessage() const;
    [[nodiscard]] QString RepositoryPath() const;
    [[nodiscard]] QString CurrentBranch() const;
    [[nodiscard]] bool RepositoryInitialized() const;
    [[nodiscard]] bool RemoteConnected() const;
    [[nodiscard]] QString RemoteUrl() const;
    [[nodiscard]] QString RepositoryConnectionStatusText() const;
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
    [[nodiscard]] bool HistoryVisible() const;
    [[nodiscard]] QString SelectedCommitHash() const;
    [[nodiscard]] QString SelectedCommitFilePath() const;
    [[nodiscard]] QString SelectedCommitDiff() const;
    [[nodiscard]] QString CurrentDiff() const;
    [[nodiscard]] class StatusFileModel *StatusFiles();
    [[nodiscard]] class CommitHistoryModel *CommitHistory();
    [[nodiscard]] class CommitFileModel *CommitFiles();

    Q_INVOKABLE void CheckGitAvailable();
    Q_INVOKABLE void OpenRepository(const QUrl &repositoryUrl);
    Q_INVOKABLE void OpenRepositoryPath(const QString &path);
    Q_INVOKABLE bool IsRepositoryFolder(const QUrl &folderUrl) const;
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
    Q_INVOKABLE bool ConnectRepository(const QString &remoteUrl);
    Q_INVOKABLE void OpenHistory();
    Q_INVOKABLE void CloseHistory();
    Q_INVOKABLE void RefreshCommitHistory();
    Q_INVOKABLE void SelectCommit(const QString &hash);
    Q_INVOKABLE void SelectCommitFile(const QString &path);
    Q_INVOKABLE void ClosePushSummary();

signals:
    void GitAvailableChanged();
    void GitVersionChanged();
    void StatusMessageChanged();
    void RepositoryPathChanged();
    void CurrentBranchChanged();
    void RepositoryConnectionChanged();
    void BranchSyncStatusChanged();
    void SelectedFilePathChanged();
    void SelectedFilesChanged();
    void StagedFileCountChanged();
    void LastPushSummaryChanged();
    void PushSummaryVisibleChanged();
    void PushInProgressChanged();
    void FetchInProgressChanged();
    void PullInProgressChanged();
    void HistoryVisibleChanged();
    void SelectedCommitChanged();
    void SelectedCommitFileChanged();
    void SelectedCommitDiffChanged();
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
    void SetRepositoryConnectionState(bool initialized, bool connected, const QString &url, const QString &statusText);
    void SetBranchSyncStatus(const GitBranchSyncStatus &status);
    void ClearBranchSyncStatus();
    void RefreshRepositoryConnectionState();
    void RefreshBranchSyncStatus();
    void SetSelectedFilePath(const QString &value);
    void SetCurrentDiff(const QString &value);
    void SetPushSummaryVisible(bool value);
    void SetPushInProgress(bool value);
    void SetFetchInProgress(bool value);
    void SetPullInProgress(bool value);
    void SetHistoryVisible(bool value);
    void SetSelectedCommitHash(const QString &value);
    void SetSelectedCommitFilePath(const QString &value);
    void SetSelectedCommitDiff(const QString &value);
    void ClearCommitSelection();

    GitCommandRunner gitCommandRunner;
    GitRepository gitRepository;
    class StatusFileModel statusFileModel;
    class CommitHistoryModel commitHistoryModel;
    class CommitFileModel commitFileModel;
    bool gitAvailable = false;
    QString gitVersion;
    QString statusMessage;
    QString repositoryPath;
    QString currentBranch;
    bool repositoryInitialized = false;
    bool remoteConnected = false;
    QString remoteUrl;
    QString repositoryConnectionStatusText;
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
    bool historyVisible = false;
    QString selectedCommitHash;
    QString selectedCommitFilePath;
    QString selectedCommitDiff;
};
