#pragma once

#include "GitCommandRunner.h"
#include "GitStatusFile.h"

#include <QList>
#include <QObject>
#include <QString>

struct GitChangeSummary
{
    int filesChanged = 0;
    int additions = 0;
    int deletions = 0;

    [[nodiscard]] int LineChanges() const;
};

struct GitBranchSyncStatus
{
    int ahead = 0;
    int behind = 0;
    bool hasUpstream = false;
};

struct GitBranchInfo
{
    QString name;
    QString upstream;
    bool isCurrent = false;
};

struct GitStashInfo
{
    int index = -1;
    QString name;
    QString branch;
    QString message;
};

struct GitCommitInfo
{
    QString hash;
    QString shortHash;
    QString subject;
    QString body;
    QString authorName;
    QString authorEmail;
    QString date;
};

struct GitCommitFile
{
    QString path;
    QString status;
    int additions = 0;
    int deletions = 0;

    [[nodiscard]] int Changes() const;
};

class GitRepository : public QObject
{
    Q_OBJECT

public:
    explicit GitRepository(QObject *parent = nullptr);

    void SetPath(const QString &path);
    QString Path() const;

    static QString NormalizeRemoteUrl(const QString &url);
    static QString DefaultCloneFolderName(const QString &url);

    bool IsInitialized() const;
    bool IsValid() const;
    bool HasRemote(const QString &name) const;
    QString RemoteUrl(const QString &name) const;
    GitCommandResult InitializeRepository() const;
    GitCommandResult AddRemote(const QString &name, const QString &url) const;
    QString CurrentBranch() const;
    QList<GitStatusFile> Status() const;
    QString Diff(const QString &filePath) const;
    bool StageFile(const QString &filePath) const;
    bool UnstageFile(const QString &filePath) const;
    GitCommandResult Commit(const QString &message) const;
    GitChangeSummary OutgoingChangeSummary() const;
    GitCommandResult Push() const;
    GitCommandResult Fetch() const;
    GitCommandResult Pull() const;
    GitCommandResult CloneRepository(
        const QString &remoteUrl,
        const QString &parentDirectory,
        const QString &folderName) const;
    QList<GitBranchInfo> LocalBranches() const;
    bool ValidateBranchName(const QString &branchName) const;
    GitCommandResult CheckoutBranch(const QString &branchName) const;
    GitCommandResult CreateBranch(const QString &branchName) const;
    GitCommandResult StashPush(const QString &message) const;
    QList<GitStashInfo> Stashes() const;
    GitCommandResult StashApply(const QString &stashName) const;
    GitCommandResult StashPop(const QString &stashName) const;
    GitCommandResult StashDrop(const QString &stashName) const;
    GitBranchSyncStatus BranchSyncStatus() const;
    QList<GitCommitInfo> CommitHistory(int limit = 100) const;
    QList<GitCommitFile> CommitFiles(const QString &commitHash) const;
    QString CommitFileDiff(const QString &commitHash, const QString &filePath) const;

private:
    QString path;
    GitCommandRunner runner;
};
