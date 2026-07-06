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

bool IsGitHubSshPort22Failure(const GitCommandResult &result, const QString &remoteOutput)
{
    const QString output = result.standardOutput + QLatin1Char('\n') + result.standardError;
    return remoteOutput.contains(QStringLiteral("git@github.com:"), Qt::CaseInsensitive)
        && output.contains(QStringLiteral("port 22"), Qt::CaseInsensitive)
        && (output.contains(QStringLiteral("Connection closed"), Qt::CaseInsensitive)
            || output.contains(QStringLiteral("Could not read from remote repository"), Qt::CaseInsensitive));
}

GitCommandResult RunRemoteCommand(
    const GitCommandRunner &runner,
    const QString &path,
    const QStringList &arguments,
    int timeoutMs)
{
    GitCommandResult result = runner.Run(arguments, path, timeoutMs);
    if (result.Success()) {
        return result;
    }

    const GitCommandResult remoteResult = runner.Run({
        QStringLiteral("remote"),
        QStringLiteral("-v")
    }, path);

    if (!remoteResult.Success() || !IsGitHubSshPort22Failure(result, remoteResult.standardOutput)) {
        return result;
    }

    return runner.Run(
        arguments,
        path,
        timeoutMs,
        {{QStringLiteral("GIT_SSH_COMMAND"),
          QStringLiteral("ssh -o HostName=ssh.github.com -o Port=443")}});
}

}

GitRepository::GitRepository(QObject *parent)
    : QObject(parent)
{
}

int GitChangeSummary::LineChanges() const
{
    return additions + deletions;
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

GitChangeSummary GitRepository::OutgoingChangeSummary() const
{
    GitChangeSummary summary;
    if (path.isEmpty()) {
        return summary;
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("diff"),
        QStringLiteral("--numstat"),
        QStringLiteral("@{u}..HEAD")
    }, path);

    if (!result.Success()) {
        return summary;
    }

    const QStringList lines = result.standardOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 3) {
            continue;
        }

        ++summary.filesChanged;

        bool additionsOk = false;
        bool deletionsOk = false;
        const int additions = parts.at(0).toInt(&additionsOk);
        const int deletions = parts.at(1).toInt(&deletionsOk);

        if (additionsOk) {
            summary.additions += additions;
        }

        if (deletionsOk) {
            summary.deletions += deletions;
        }
    }

    return summary;
}

GitCommandResult GitRepository::Push() const
{
    if (path.isEmpty()) {
        return {};
    }

    constexpr int pushTimeoutMs = 120000;

    GitCommandResult result = runner.Run({QStringLiteral("push")}, path, pushTimeoutMs);
    if (result.Success()) {
        return result;
    }

    const GitCommandResult remoteResult = runner.Run({
        QStringLiteral("remote"),
        QStringLiteral("-v")
    }, path);

    if (remoteResult.Success() && IsGitHubSshPort22Failure(result, remoteResult.standardOutput)) {
        const GitCommandResult retryResult = runner.Run(
            {QStringLiteral("push")},
            path,
            pushTimeoutMs,
            {{QStringLiteral("GIT_SSH_COMMAND"),
              QStringLiteral("ssh -o HostName=ssh.github.com -o Port=443")}});

        if (retryResult.Success()) {
            return retryResult;
        }

        return retryResult;
    }

    const QString output = result.standardOutput + QLatin1Char('\n') + result.standardError;
    const bool missingUpstream = output.contains(QStringLiteral("no upstream branch"))
        || output.contains(QStringLiteral("--set-upstream"))
        || output.contains(QStringLiteral("set the remote as upstream"));

    if (!missingUpstream) {
        return result;
    }

    const QString branch = CurrentBranch();
    if (branch.isEmpty()) {
        return result;
    }

    return runner.Run({
        QStringLiteral("push"),
        QStringLiteral("-u"),
        QStringLiteral("origin"),
        branch
    }, path, pushTimeoutMs);
}

GitCommandResult GitRepository::Fetch() const
{
    if (path.isEmpty()) {
        return {};
    }

    constexpr int remoteOperationTimeoutMs = 120000;
    return RunRemoteCommand(
        runner,
        path,
        {QStringLiteral("fetch"), QStringLiteral("--prune")},
        remoteOperationTimeoutMs);
}

GitCommandResult GitRepository::Pull() const
{
    if (path.isEmpty()) {
        return {};
    }

    constexpr int remoteOperationTimeoutMs = 120000;
    return RunRemoteCommand(
        runner,
        path,
        {QStringLiteral("pull"), QStringLiteral("--ff-only")},
        remoteOperationTimeoutMs);
}

GitBranchSyncStatus GitRepository::BranchSyncStatus() const
{
    GitBranchSyncStatus status;
    if (path.isEmpty()) {
        return status;
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("rev-list"),
        QStringLiteral("--left-right"),
        QStringLiteral("--count"),
        QStringLiteral("@{u}...HEAD")
    }, path);

    if (!result.Success()) {
        return status;
    }

    const QStringList parts = result.standardOutput.simplified().split(QLatin1Char(' '));
    if (parts.size() != 2) {
        return status;
    }

    bool behindOk = false;
    bool aheadOk = false;
    const int behind = parts.at(0).toInt(&behindOk);
    const int ahead = parts.at(1).toInt(&aheadOk);

    if (!behindOk || !aheadOk) {
        return status;
    }

    status.hasUpstream = true;
    status.behind = behind;
    status.ahead = ahead;
    return status;
}
