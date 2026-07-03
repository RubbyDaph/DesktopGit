#include "GitCommandRunner.h"

#include <QProcess>
#include <QProcessEnvironment>

bool GitCommandResult::Success() const
{
    return exitCode == 0 && !timedOut;
}

GitCommandRunner::GitCommandRunner(QObject *parent)
    : QObject(parent)
{
}

GitCommandResult GitCommandRunner::Run(
    const QStringList &arguments,
    const QString &workingDirectory,
    int timeoutMs,
    const QHash<QString, QString> &environmentOverrides) const
{
    QProcess process;
    GitCommandResult result;

    if (!environmentOverrides.isEmpty()) {
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        for (auto iterator = environmentOverrides.cbegin(); iterator != environmentOverrides.cend(); ++iterator) {
            environment.insert(iterator.key(), iterator.value());
        }

        process.setProcessEnvironment(environment);
    }

    if (!workingDirectory.isEmpty()) {
        process.setWorkingDirectory(workingDirectory);
    }

    process.start(QStringLiteral("git"), arguments);

    if (!process.waitForStarted()) {
        result.standardError = process.errorString();
        return result;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished();

        result.timedOut = true;
        result.standardOutput = QString::fromUtf8(process.readAllStandardOutput());
        result.standardError = QString::fromUtf8(process.readAllStandardError());
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromUtf8(process.readAllStandardOutput());
    result.standardError = QString::fromUtf8(process.readAllStandardError());

    return result;
}
