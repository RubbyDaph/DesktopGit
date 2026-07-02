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

private:
    QString path;
    GitCommandRunner runner;
};
