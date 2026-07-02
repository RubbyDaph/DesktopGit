#include "GitDiffFormatter.h"

#include <QStringList>

QString GitDiffFormatter::FormatForDisplay(const QString &diff)
{
    QStringList formattedLines;
    const QStringList lines = diff.split(QLatin1Char('\n'));

    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("diff --git "))
            || line.startsWith(QStringLiteral("index "))
            || line.startsWith(QStringLiteral("--- "))
            || line.startsWith(QStringLiteral("+++ "))
            || line.startsWith(QStringLiteral("@@ "))) {
            continue;
        }

        formattedLines.append(line);
    }

    return formattedLines.join(QLatin1Char('\n')).trimmed();
}
