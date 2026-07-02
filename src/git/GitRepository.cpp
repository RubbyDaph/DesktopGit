#include "GitRepository.h"

GitRepository::GitRepository(QObject *parent)
    : QObject(parent)
{
}

void GitRepository::SetPath(const QString &path)
{
    this->path = path;
}

QString GitRepository::Path() const
{
    return path;
}

bool GitRepository::IsValid() const
{
    if (path.isEmpty()) {
        return false;
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("rev-parse"),
        QStringLiteral("--is-inside-work-tree")
    }, path);

    return result.Success() && result.standardOutput.trimmed() == QStringLiteral("true");
}

QString GitRepository::CurrentBranch() const
{
    if (path.isEmpty()) {
        return {};
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("branch"),
        QStringLiteral("--show-current")
    }, path);

    if (!result.Success()) {
        return {};
    }

    return result.standardOutput.trimmed();
}
