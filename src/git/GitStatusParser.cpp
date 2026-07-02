#include "GitStatusParser.h"

#include <QStringList>

QList<GitStatusFile> GitStatusParser::Parse(const QString &output)
{
    QList<GitStatusFile> files;

    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith(QLatin1Char('#'))) {
            continue;
        }

        if (line.size() < 4) {
            continue;
        }

        const QString status = line.left(2);
        const QString path = line.mid(3).trimmed();

        if (path.isEmpty()) {
            continue;
        }

        GitStatusFile file;
        file.indexStatus = status.left(1).trimmed();
        file.worktreeStatus = status.mid(1, 1).trimmed();
        file.path = path;

        if (status == QStringLiteral("??")) {
            file.indexStatus = QStringLiteral("?");
            file.worktreeStatus = QStringLiteral("?");
        }

        files.append(file);
    }

    return files;
}
