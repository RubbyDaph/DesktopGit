#pragma once

#include "GitCommandRunner.h"
#include "GitStatusFile.h"

#include <QList>
#include <QObject>
#include <QString>

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

private:
    QString path;
    GitCommandRunner runner;
};
