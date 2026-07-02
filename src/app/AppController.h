#pragma once

#include "GitCommandRunner.h"
#include "GitRepository.h"

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

public:
    explicit AppController(QObject *parent = nullptr);

    [[nodiscard]] bool GitAvailable() const;
    [[nodiscard]] QString GitVersion() const;
    [[nodiscard]] QString StatusMessage() const;
    [[nodiscard]] QString RepositoryPath() const;
    [[nodiscard]] QString CurrentBranch() const;

    Q_INVOKABLE void CheckGitAvailable();
    Q_INVOKABLE void OpenRepository(const QUrl &repositoryUrl);
    Q_INVOKABLE void OpenRepositoryPath(const QString &path);

signals:
    void GitAvailableChanged();
    void GitVersionChanged();
    void StatusMessageChanged();
    void RepositoryPathChanged();
    void CurrentBranchChanged();

private:
    void SetGitAvailable(bool value);
    void SetGitVersion(const QString &value);
    void SetStatusMessage(const QString &value);
    void SetRepositoryPath(const QString &value);
    void SetCurrentBranch(const QString &value);

    GitCommandRunner gitCommandRunner;
    GitRepository gitRepository;
    bool gitAvailable = false;
    QString gitVersion;
    QString statusMessage;
    QString repositoryPath;
    QString currentBranch;
};
