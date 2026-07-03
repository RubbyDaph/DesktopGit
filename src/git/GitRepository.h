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

class GitRepository : public QObject
{
    Q_OBJECT

public:
    explicit GitRepository(QObject *parent = nullptr);

    void SetPath(const QString &path);
    QString Path() const;

    bool IsValid() const;
    QString CurrentBranch() const;
    QList<GitStatusFile> Status() const;
    QString Diff(const QString &filePath) const;
    bool StageFile(const QString &filePath) const;
    bool UnstageFile(const QString &filePath) const;
    GitCommandResult Commit(const QString &message) const;
    GitChangeSummary OutgoingChangeSummary() const;
    GitCommandResult Push() const;

private:
    QString path;
    GitCommandRunner runner;
};
