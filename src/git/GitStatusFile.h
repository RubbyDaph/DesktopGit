#pragma once

#include <QString>

struct GitStatusFile
{
    QString path;
    QString indexStatus;
    QString worktreeStatus;

    [[nodiscard]] bool IsStaged() const
    {
        return !indexStatus.isEmpty() && indexStatus != QStringLiteral("?");
    }

    [[nodiscard]] QString DisplayStatus() const
    {
        if (indexStatus == QStringLiteral("?") && worktreeStatus == QStringLiteral("?")) {
            return QStringLiteral("??");
        }

        return indexStatus + worktreeStatus;
    }
};
