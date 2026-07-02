#pragma once

#include "GitCommandRunner.h"

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

private:
    QString path;
    GitCommandRunner runner;
};
