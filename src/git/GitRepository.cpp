#include "GitRepository.h"

#include "GitDiffFormatter.h"
#include "GitStatusParser.h"

#include <QDir>
#include <QFileInfo>
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

int GitCommitFile::Changes() const
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

QString GitRepository::NormalizeRemoteUrl(const QString &url)
{
    QString normalizedUrl = url.trimmed();
    while (normalizedUrl.endsWith(QLatin1Char('/'))) {
        normalizedUrl.chop(1);
    }

    if (normalizedUrl.isEmpty() || normalizedUrl.contains(QLatin1Char(' '))) {
        return {};
    }

    const bool isGitHubWebUrl = normalizedUrl.startsWith(QStringLiteral("https://github.com/"), Qt::CaseInsensitive)
        || normalizedUrl.startsWith(QStringLiteral("http://github.com/"), Qt::CaseInsensitive);
    const bool isGitHubSshUrl = normalizedUrl.startsWith(QStringLiteral("git@github.com:"), Qt::CaseInsensitive);

    if ((isGitHubWebUrl || isGitHubSshUrl) && !normalizedUrl.endsWith(QStringLiteral(".git"), Qt::CaseInsensitive)) {
        normalizedUrl += QStringLiteral(".git");
    }

    return normalizedUrl;
}

bool GitRepository::IsInitialized() const
{
    if (path.isEmpty()) {
        return false;
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("rev-parse"),
        QStringLiteral("--git-dir")
    }, path);

    if (result.Success()) {
        return true;
    }

    const QFileInfo gitEntry(QDir(path).filePath(QStringLiteral(".git")));
    return gitEntry.exists();
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

bool GitRepository::HasRemote(const QString &name) const
{
    return !RemoteUrl(name).isEmpty();
}

QString GitRepository::RemoteUrl(const QString &name) const
{
    if (path.isEmpty() || name.trimmed().isEmpty()) {
        return {};
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("remote"),
        QStringLiteral("get-url"),
        name.trimmed()
    }, path);

    if (!result.Success()) {
        return {};
    }

    return result.standardOutput.trimmed();
}

GitCommandResult GitRepository::InitializeRepository() const
{
    if (path.isEmpty()) {
        return {};
    }

    return runner.Run({QStringLiteral("init")}, path);
}

GitCommandResult GitRepository::AddRemote(const QString &name, const QString &url) const
{
    const QString remoteName = name.trimmed();
    const QString remoteUrl = NormalizeRemoteUrl(url);
    if (path.isEmpty() || remoteName.isEmpty() || remoteUrl.isEmpty()) {
        return {};
    }

    return runner.Run({
        QStringLiteral("remote"),
        QStringLiteral("add"),
        remoteName,
        remoteUrl
    }, path);
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

QList<GitBranchInfo> GitRepository::LocalBranches() const
{
    if (path.isEmpty()) {
        return {};
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("for-each-ref"),
        QStringLiteral("--format=%(refname:short)%09%(upstream:short)%09%(HEAD)"),
        QStringLiteral("refs/heads")
    }, path);

    if (!result.Success()) {
        return {};
    }

    QList<GitBranchInfo> branches;
    const QStringList lines = result.standardOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    branches.reserve(lines.size());

    for (const QString &line : lines) {
        const QStringList fields = line.split(QLatin1Char('\t'));
        if (fields.size() != 3) {
            continue;
        }

        GitBranchInfo branch;
        branch.name = fields.at(0).trimmed();
        branch.upstream = fields.at(1).trimmed();
        branch.isCurrent = fields.at(2).trimmed() == QStringLiteral("*");

        if (!branch.name.isEmpty()) {
            branches.append(branch);
        }
    }

    return branches;
}

bool GitRepository::ValidateBranchName(const QString &branchName) const
{
    const QString trimmedBranchName = branchName.trimmed();
    if (trimmedBranchName.isEmpty()) {
        return false;
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("check-ref-format"),
        QStringLiteral("--branch"),
        trimmedBranchName
    }, path);

    return result.Success();
}

GitCommandResult GitRepository::CheckoutBranch(const QString &branchName) const
{
    const QString trimmedBranchName = branchName.trimmed();
    if (path.isEmpty() || trimmedBranchName.isEmpty()) {
        return {};
    }

    return runner.Run({
        QStringLiteral("checkout"),
        trimmedBranchName
    }, path);
}

GitCommandResult GitRepository::CreateBranch(const QString &branchName) const
{
    const QString trimmedBranchName = branchName.trimmed();
    if (path.isEmpty() || trimmedBranchName.isEmpty()) {
        return {};
    }

    return runner.Run({
        QStringLiteral("checkout"),
        QStringLiteral("-b"),
        trimmedBranchName
    }, path);
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

QList<GitCommitInfo> GitRepository::CommitHistory(int limit) const
{
    if (path.isEmpty() || limit <= 0) {
        return {};
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("log"),
        QStringLiteral("-n"),
        QString::number(limit),
        QStringLiteral("--date=iso-strict"),
        QStringLiteral("--pretty=format:%H%x1f%h%x1f%s%x1f%b%x1f%an%x1f%ae%x1f%ad%x1e")
    }, path);

    if (!result.Success()) {
        return {};
    }

    QList<GitCommitInfo> commits;
    const QStringList records = result.standardOutput.split(QChar(0x1e), Qt::SkipEmptyParts);
    commits.reserve(records.size());

    for (const QString &record : records) {
        const QStringList fields = record.split(QChar(0x1f));
        if (fields.size() != 7) {
            continue;
        }

        GitCommitInfo commit;
        commit.hash = fields.at(0).trimmed();
        commit.shortHash = fields.at(1).trimmed();
        commit.subject = fields.at(2).trimmed();
        commit.body = fields.at(3).trimmed();
        commit.authorName = fields.at(4).trimmed();
        commit.authorEmail = fields.at(5).trimmed();
        commit.date = fields.at(6).trimmed();
        commits.append(commit);
    }

    return commits;
}

QList<GitCommitFile> GitRepository::CommitFiles(const QString &commitHash) const
{
    if (path.isEmpty() || commitHash.trimmed().isEmpty()) {
        return {};
    }

    const QString hash = commitHash.trimmed();
    const GitCommandResult statusResult = runner.Run({
        QStringLiteral("show"),
        QStringLiteral("--format="),
        QStringLiteral("--name-status"),
        QStringLiteral("--no-renames"),
        hash
    }, path);

    const GitCommandResult numstatResult = runner.Run({
        QStringLiteral("show"),
        QStringLiteral("--format="),
        QStringLiteral("--numstat"),
        QStringLiteral("--no-renames"),
        hash
    }, path);

    if (!statusResult.Success() || !numstatResult.Success()) {
        return {};
    }

    QHash<QString, QString> statuses;
    const QStringList statusLines = statusResult.standardOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : statusLines) {
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 2) {
            continue;
        }

        statuses.insert(parts.at(1), parts.at(0));
    }

    QList<GitCommitFile> files;
    const QStringList numstatLines = numstatResult.standardOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    files.reserve(numstatLines.size());

    for (const QString &line : numstatLines) {
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 3) {
            continue;
        }

        GitCommitFile file;
        file.path = parts.at(2);
        file.status = statuses.value(file.path);

        bool additionsOk = false;
        bool deletionsOk = false;
        const int additions = parts.at(0).toInt(&additionsOk);
        const int deletions = parts.at(1).toInt(&deletionsOk);

        if (additionsOk) {
            file.additions = additions;
        }

        if (deletionsOk) {
            file.deletions = deletions;
        }

        files.append(file);
    }

    return files;
}

QString GitRepository::CommitFileDiff(const QString &commitHash, const QString &filePath) const
{
    if (path.isEmpty() || commitHash.trimmed().isEmpty() || filePath.isEmpty()) {
        return {};
    }

    const GitCommandResult result = runner.Run({
        QStringLiteral("show"),
        QStringLiteral("--format="),
        QStringLiteral("--no-ext-diff"),
        commitHash.trimmed(),
        QStringLiteral("--"),
        filePath
    }, path);

    if (!result.Success()) {
        return {};
    }

    return GitDiffFormatter::FormatForDisplay(result.standardOutput);
}
