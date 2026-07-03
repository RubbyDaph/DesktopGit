#include "GitRepository.h"

#include "GitDiffFormatter.h"
#include "GitStatusParser.h"

#include <QHash>
#include <QStringList>

namespace {

struct DiffStat
{
    int additions = 0;
    int deletions = 0;
};

QHash<QString, DiffStat> ParseNumstat(const QString &output)
{
    QHash<QString, DiffStat> stats;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 3) {
            continue;
        }

        bool additionsOk = false;
        bool deletionsOk = false;
        const int additions = parts.at(0).toInt(&additionsOk);
        const int deletions = parts.at(1).toInt(&deletionsOk);

        if (!additionsOk || !deletionsOk) {
            continue;
        }

        stats.insert(parts.at(2), DiffStat{additions, deletions});
    }

    return stats;
}

void AddStats(QHash<QString, DiffStat> *target, const QHash<QString, DiffStat> &source)
{
    for (auto iterator = source.cbegin(); iterator != source.cend(); ++iterator) {
        DiffStat stat = target->value(iterator.key());
        stat.additions += iterator.value().additions;
        stat.deletions += iterator.value().deletions;
        target->insert(iterator.key(), stat);
    }
}

}

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

QList<GitStatusFile> GitRepository::Status() const
{
    if (path.isEmpty()) {
        return {};
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("status"),
        QStringLiteral("--porcelain=v1")
    }, path);

    if (!result.Success()) {
        return {};
    }

    QList<GitStatusFile> files = GitStatusParser::Parse(result.standardOutput);

    const GitCommandResult unstagedNumstatResult = runner.Run({
        QStringLiteral("diff"),
        QStringLiteral("--numstat")
    }, path);

    QHash<QString, DiffStat> stats;
    if (unstagedNumstatResult.Success()) {
        AddStats(&stats, ParseNumstat(unstagedNumstatResult.standardOutput));
    }

    const GitCommandResult stagedNumstatResult = runner.Run({
        QStringLiteral("diff"),
        QStringLiteral("--cached"),
        QStringLiteral("--numstat")
    }, path);

    if (stagedNumstatResult.Success()) {
        AddStats(&stats, ParseNumstat(stagedNumstatResult.standardOutput));
    }

    for (GitStatusFile &file : files) {
        const DiffStat stat = stats.value(file.path);
        file.additions = stat.additions;
        file.deletions = stat.deletions;
    }

    return files;
}

QString GitRepository::Diff(const QString &filePath) const
{
    if (path.isEmpty() || filePath.isEmpty()) {
        return {};
    }

    const GitCommandResult unstagedResult = runner.Run({
        QStringLiteral("diff"),
        QStringLiteral("--"),
        filePath
    }, path);

    if (!unstagedResult.Success()) {
        return {};
    }

    if (!unstagedResult.standardOutput.isEmpty()) {
        return GitDiffFormatter::FormatForDisplay(unstagedResult.standardOutput);
    }

    const GitCommandResult stagedResult = runner.Run({
        QStringLiteral("diff"),
        QStringLiteral("--cached"),
        QStringLiteral("--"),
        filePath
    }, path);

    if (!stagedResult.Success()) {
        return {};
    }

    return GitDiffFormatter::FormatForDisplay(stagedResult.standardOutput);
}

bool GitRepository::StageFile(const QString &filePath) const
{
    if (path.isEmpty() || filePath.isEmpty()) {
        return false;
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("add"),
        QStringLiteral("--"),
        filePath
    }, path);

    return result.Success();
}

bool GitRepository::UnstageFile(const QString &filePath) const
{
    if (path.isEmpty() || filePath.isEmpty()) {
        return false;
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("restore"),
        QStringLiteral("--staged"),
        QStringLiteral("--"),
        filePath
    }, path);

    return result.Success();
}

GitCommandResult GitRepository::Commit(const QString &message) const
{
    if (path.isEmpty() || message.trimmed().isEmpty()) {
        return {};
    }

    return runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        message
    }, path);
}
