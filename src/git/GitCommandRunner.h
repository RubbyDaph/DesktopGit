#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>

struct GitCommandResult
{
    int exitCode = -1;
    QString standardOutput;
    QString standardError;
    bool timedOut = false;

    [[nodiscard]] bool Success() const;
};

class GitCommandRunner : public QObject
{
    Q_OBJECT

public:
    explicit GitCommandRunner(QObject *parent = nullptr);

    [[nodiscard]] GitCommandResult Run(
        const QStringList &arguments,
        const QString &workingDirectory = QString(),
        int timeoutMs = 10000,
        const QHash<QString, QString> &environmentOverrides = {}) const;
};
