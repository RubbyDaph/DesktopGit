#include "AppController.h"
#include "BranchModel.h"
#include "CommitFileModel.h"
#include "CommitHistoryModel.h"
#include "GitCommandRunner.h"
#include "GitDiffFormatter.h"
#include "GitRepository.h"
#include "GitStatusParser.h"
#include "StashModel.h"
#include "StatusFileModel.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

namespace {

bool WriteTextFile(const QString &path, const QString &text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << text;
    return true;
}

QString ReadTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

}

class TestDesktopGitCore : public QObject
{
    Q_OBJECT

private slots:
    void ParseStatusOutput();
    void IgnoreInvalidStatusLines();
    void PopulateStatusFileModel();
    void ClearStatusFileModel();
    void SelectStatusFilesInModel();
    void PopulateBranchModel();
    void PopulateStashModel();
    void RunGitVersionCommand();
    void RunGitCommandWithEnvironmentOverride();
    void NormalizeRemoteUrls();
    void GenerateDefaultCloneFolderNames();
    void CloneRepositoryChanges();
    void InitializeAndConnectRepository();
    void OpenPlainFolderAndConnectFromController();
    void OpenRepositoryWithExistingOriginFromController();
    void OpenWorkingTreeFromController();
    void ReadCommitHistoryAndCommitDiff();
    void PopulateCommitHistoryModel();
    void PopulateCommitFileModel();
    void ReadCommitHistoryFromController();
    void OpenHistoryForEmptyAndPlainRepositories();
    void FormatDiffForDisplay();
    void ReadStatusAndDiffFromRepository();
    void StageAndUnstageRepositoryFile();
    void StageAndUnstageSelectedFilesFromController();
    void CommitRepositoryChanges();
    void CommitStagedFilesFromController();
    void PushRepositoryChanges();
    void PushRepositorySetsUpstreamWhenMissing();
    void PushRepositoryFromController();
    void FetchRepositoryChanges();
    void PullRepositoryChanges();
    void FetchAndPullRepositoryFromController();
    void CloneRepositoryFromController();
    void ReadBranchSyncStatus();
    void ReadAndChangeLocalBranches();
    void ManageBranchesFromController();
    void ManageStashes();
    void ManageStashesFromController();
};

void TestDesktopGitCore::ParseStatusOutput()
{
    const QList<GitStatusFile> files = GitStatusParser::Parse(
        QStringLiteral(" M src/main.cpp\nA  README.md\n?? notes.txt\n"));

    QCOMPARE(files.size(), 3);

    QCOMPARE(files.at(0).path, QStringLiteral("src/main.cpp"));
    QCOMPARE(files.at(0).indexStatus, QString());
    QCOMPARE(files.at(0).worktreeStatus, QStringLiteral("M"));
    QCOMPARE(files.at(0).DisplayStatus(), QStringLiteral("M"));
    QCOMPARE(files.at(0).IsStaged(), false);

    QCOMPARE(files.at(1).path, QStringLiteral("README.md"));
    QCOMPARE(files.at(1).indexStatus, QStringLiteral("A"));
    QCOMPARE(files.at(1).worktreeStatus, QString());
    QCOMPARE(files.at(1).DisplayStatus(), QStringLiteral("A"));
    QCOMPARE(files.at(1).IsStaged(), true);

    QCOMPARE(files.at(2).path, QStringLiteral("notes.txt"));
    QCOMPARE(files.at(2).indexStatus, QStringLiteral("?"));
    QCOMPARE(files.at(2).worktreeStatus, QStringLiteral("?"));
    QCOMPARE(files.at(2).DisplayStatus(), QStringLiteral("??"));
    QCOMPARE(files.at(2).IsStaged(), false);
}

void TestDesktopGitCore::IgnoreInvalidStatusLines()
{
    const QList<GitStatusFile> files = GitStatusParser::Parse(
        QStringLiteral("\n# branch.oid abc\nM\n M valid.cpp\n"));

    QCOMPARE(files.size(), 1);
    QCOMPARE(files.first().path, QStringLiteral("valid.cpp"));
}

void TestDesktopGitCore::PopulateStatusFileModel()
{
    StatusFileModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    GitStatusFile file;
    file.path = QStringLiteral("src/main.cpp");
    file.indexStatus = QStringLiteral("M");
    file.additions = 3;
    file.deletions = 1;

    model.SetFiles({file});

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 1);

    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, StatusFileModel::PathRole).toString(), QStringLiteral("src/main.cpp"));
    QCOMPARE(model.data(index, StatusFileModel::IndexStatusRole).toString(), QStringLiteral("M"));
    QCOMPARE(model.data(index, StatusFileModel::DisplayStatusRole).toString(), QStringLiteral("M"));
    QCOMPARE(model.data(index, StatusFileModel::StagedRole).toBool(), true);
    QCOMPARE(model.data(index, StatusFileModel::AdditionsRole).toInt(), 3);
    QCOMPARE(model.data(index, StatusFileModel::DeletionsRole).toInt(), 1);
    QCOMPARE(model.data(index, StatusFileModel::ChangesRole).toInt(), 4);
    QCOMPARE(model.StagedCount(), 1);
    QVERIFY(model.data(model.index(2, 0), StatusFileModel::PathRole).isNull());
}

void TestDesktopGitCore::ClearStatusFileModel()
{
    StatusFileModel model;

    GitStatusFile file;
    file.path = QStringLiteral("README.md");
    file.worktreeStatus = QStringLiteral("M");

    model.SetFiles({file});
    QCOMPARE(model.rowCount(), 1);

    model.Clear();
    QCOMPARE(model.rowCount(), 0);
}

void TestDesktopGitCore::SelectStatusFilesInModel()
{
    StatusFileModel model;

    GitStatusFile firstFile;
    firstFile.path = QStringLiteral("first.txt");

    GitStatusFile secondFile;
    secondFile.path = QStringLiteral("second.txt");

    model.SetFiles({firstFile, secondFile});
    QCOMPARE(model.SelectedCount(), 0);

    model.ToggleSelected(QStringLiteral("first.txt"));
    QCOMPARE(model.SelectedCount(), 1);
    QCOMPARE(model.SelectedPaths(), QStringList({QStringLiteral("first.txt")}));
    QCOMPARE(model.data(model.index(0, 0), StatusFileModel::SelectedRole).toBool(), true);
    QCOMPARE(model.data(model.index(1, 0), StatusFileModel::SelectedRole).toBool(), false);

    model.SelectAll();
    QCOMPARE(model.SelectedCount(), 2);

    model.ClearSelection();
    QCOMPARE(model.SelectedCount(), 0);
    QCOMPARE(model.data(model.index(0, 0), StatusFileModel::SelectedRole).toBool(), false);

    model.ToggleSelected(QStringLiteral("first.txt"));
    model.SetFiles({secondFile});
    QCOMPARE(model.SelectedCount(), 0);
}

void TestDesktopGitCore::PopulateBranchModel()
{
    BranchModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    GitBranchInfo mainBranch;
    mainBranch.name = QStringLiteral("main");
    mainBranch.upstream = QStringLiteral("origin/main");
    mainBranch.isCurrent = true;

    GitBranchInfo featureBranch;
    featureBranch.name = QStringLiteral("feature/test");

    model.SetBranches({mainBranch, featureBranch});

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 2);
    QVERIFY(model.ContainsBranch(QStringLiteral("main")));
    QVERIFY(model.ContainsBranch(QStringLiteral("feature/test")));
    QVERIFY(!model.ContainsBranch(QStringLiteral("missing")));

    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, BranchModel::NameRole).toString(), QStringLiteral("main"));
    QCOMPARE(model.data(index, BranchModel::CurrentRole).toBool(), true);
    QCOMPARE(model.data(index, BranchModel::UpstreamRole).toString(), QStringLiteral("origin/main"));

    model.Clear();
    QCOMPARE(resetSpy.count(), 2);
    QCOMPARE(model.rowCount(), 0);
}

void TestDesktopGitCore::PopulateStashModel()
{
    StashModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    GitStashInfo stash;
    stash.index = 0;
    stash.name = QStringLiteral("stash@{0}");
    stash.branch = QStringLiteral("main");
    stash.message = QStringLiteral("Work in progress");

    model.SetStashes({stash});

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.ContainsStash(QStringLiteral("stash@{0}")));
    QVERIFY(!model.ContainsStash(QStringLiteral("stash@{1}")));

    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, StashModel::IndexRole).toInt(), 0);
    QCOMPARE(model.data(index, StashModel::NameRole).toString(), QStringLiteral("stash@{0}"));
    QCOMPARE(model.data(index, StashModel::BranchRole).toString(), QStringLiteral("main"));
    QCOMPARE(model.data(index, StashModel::MessageRole).toString(), QStringLiteral("Work in progress"));

    model.Clear();
    QCOMPARE(resetSpy.count(), 2);
    QCOMPARE(model.rowCount(), 0);
}

void TestDesktopGitCore::RunGitVersionCommand()
{
    GitCommandRunner runner;
    const GitCommandResult result = runner.Run({QStringLiteral("--version")});

    QVERIFY2(result.Success(), qPrintable(result.standardError));
    QVERIFY(result.standardOutput.startsWith(QStringLiteral("git version")));
}

void TestDesktopGitCore::RunGitCommandWithEnvironmentOverride()
{
    GitCommandRunner runner;
    const GitCommandResult result = runner.Run({
        QStringLiteral("-c"),
        QStringLiteral("alias.print-env=!sh -c 'printf %s \"$DESKTOPGIT_TEST_ENV\"'"),
        QStringLiteral("print-env")
    }, QString(), 10000, {{QStringLiteral("DESKTOPGIT_TEST_ENV"), QStringLiteral("from-env")}});

    QVERIFY2(result.Success(), qPrintable(result.standardError));
    QCOMPARE(result.standardOutput, QStringLiteral("from-env"));
}

void TestDesktopGitCore::NormalizeRemoteUrls()
{
    QCOMPARE(
        GitRepository::NormalizeRemoteUrl(QStringLiteral("https://github.com/user/repository")),
        QStringLiteral("https://github.com/user/repository.git"));
    QCOMPARE(
        GitRepository::NormalizeRemoteUrl(QStringLiteral("https://github.com/user/repository.git")),
        QStringLiteral("https://github.com/user/repository.git"));
    QCOMPARE(
        GitRepository::NormalizeRemoteUrl(QStringLiteral("git@github.com:user/repository")),
        QStringLiteral("git@github.com:user/repository.git"));
    QCOMPARE(
        GitRepository::NormalizeRemoteUrl(QStringLiteral("git@github.com:user/repository.git")),
        QStringLiteral("git@github.com:user/repository.git"));
    QCOMPARE(
        GitRepository::NormalizeRemoteUrl(QStringLiteral("  https://example.com/repository.git/  ")),
        QStringLiteral("https://example.com/repository.git"));
    QCOMPARE(GitRepository::NormalizeRemoteUrl(QStringLiteral("  ")), QString());
    QCOMPARE(GitRepository::NormalizeRemoteUrl(QStringLiteral("https://github.com/user/repo with space")), QString());
}

void TestDesktopGitCore::GenerateDefaultCloneFolderNames()
{
    QCOMPARE(
        GitRepository::DefaultCloneFolderName(QStringLiteral("https://github.com/user/repository")),
        QStringLiteral("repository"));
    QCOMPARE(
        GitRepository::DefaultCloneFolderName(QStringLiteral("https://github.com/user/repository.git")),
        QStringLiteral("repository"));
    QCOMPARE(
        GitRepository::DefaultCloneFolderName(QStringLiteral("git@github.com:user/repository.git")),
        QStringLiteral("repository"));
    QCOMPARE(
        GitRepository::DefaultCloneFolderName(QStringLiteral("/tmp/repository.git")),
        QStringLiteral("repository"));
    QCOMPARE(GitRepository::DefaultCloneFolderName(QStringLiteral("  ")), QString());
}

void TestDesktopGitCore::CloneRepositoryChanges()
{
    QTemporaryDir sourceDirectory;
    QVERIFY(sourceDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    QTemporaryDir cloneParentDirectory;
    QVERIFY(cloneParentDirectory.isValid());

    const QString sourcePath = sourceDirectory.path();
    const QString remotePath = remoteDirectory.path();
    const QString cloneParentPath = cloneParentDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, sourcePath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, sourcePath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, sourcePath).Success(), "git config user.name failed");

    QVERIFY(WriteTextFile(QDir(sourcePath).filePath(QStringLiteral("file.txt")), QStringLiteral("source line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, sourcePath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, sourcePath).Success(), "git commit failed");
    QVERIFY2(runner.Run({QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remotePath}, sourcePath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("HEAD")}, sourcePath).Success(), "git push failed");
    const GitCommandResult branchResult = runner.Run({QStringLiteral("branch"), QStringLiteral("--show-current")}, sourcePath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());
    QVERIFY2(runner.Run({QStringLiteral("--git-dir"), remotePath, QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/") + branchName}, sourcePath).Success(), "git symbolic-ref failed");

    GitRepository repository;
    const GitCommandResult cloneResult = repository.CloneRepository(
        remotePath,
        cloneParentPath,
        QStringLiteral("cloned-repository"));
    QVERIFY2(cloneResult.Success(), qPrintable(cloneResult.standardError));

    const QString clonedRepositoryPath = QDir(cloneParentPath).filePath(QStringLiteral("cloned-repository"));
    QVERIFY(QFileInfo(QDir(clonedRepositoryPath).filePath(QStringLiteral(".git"))).exists());
    QCOMPARE(ReadTextFile(QDir(clonedRepositoryPath).filePath(QStringLiteral("file.txt"))), QStringLiteral("source line\n"));

    const GitCommandResult duplicateCloneResult = repository.CloneRepository(
        remotePath,
        cloneParentPath,
        QStringLiteral("cloned-repository"));
    QVERIFY(!duplicateCloneResult.Success());
    QCOMPARE(duplicateCloneResult.standardError, QStringLiteral("Clone target folder already exists."));

    const GitCommandResult invalidFolderResult = repository.CloneRepository(
        remotePath,
        cloneParentPath,
        QStringLiteral("bad/name"));
    QVERIFY(!invalidFolderResult.Success());
    QCOMPARE(invalidFolderResult.standardError, QStringLiteral("Clone folder name is invalid."));
}

void TestDesktopGitCore::InitializeAndConnectRepository()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();

    GitRepository repository;
    repository.SetPath(repositoryPath);

    QCOMPARE(repository.IsInitialized(), false);
    QCOMPARE(repository.IsValid(), false);

    const GitCommandResult initResult = repository.InitializeRepository();
    QVERIFY2(initResult.Success(), qPrintable(initResult.standardError));
    QCOMPARE(repository.IsInitialized(), true);
    QCOMPARE(repository.IsValid(), true);
    QCOMPARE(repository.HasRemote(QStringLiteral("origin")), false);

    const GitCommandResult remoteResult = repository.AddRemote(
        QStringLiteral("origin"),
        QStringLiteral("https://github.com/user/repository"));
    QVERIFY2(remoteResult.Success(), qPrintable(remoteResult.standardError));
    QCOMPARE(repository.HasRemote(QStringLiteral("origin")), true);
    QCOMPARE(repository.RemoteUrl(QStringLiteral("origin")), QStringLiteral("https://github.com/user/repository.git"));

    const GitCommandResult duplicateRemoteResult = repository.AddRemote(
        QStringLiteral("origin"),
        QStringLiteral("https://github.com/user/other"));
    QCOMPARE(duplicateRemoteResult.Success(), false);
    QCOMPARE(repository.RemoteUrl(QStringLiteral("origin")), QStringLiteral("https://github.com/user/repository.git"));
}

void TestDesktopGitCore::OpenPlainFolderAndConnectFromController()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    GitCommandRunner runner;
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");

    AppController controller;
    controller.OpenRepositoryPath(repositoryPath);

    QCOMPARE(controller.RepositoryPath(), repositoryPath);
    QCOMPARE(controller.RepositoryInitialized(), false);
    QCOMPARE(controller.RemoteConnected(), false);
    QCOMPARE(controller.RepositoryConnectionStatusText(), QStringLiteral("not initialized"));
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Folder opened. Repository is not initialized."));

    QVERIFY(controller.ConnectRepository(remotePath));
    QCOMPARE(controller.RepositoryInitialized(), true);
    QCOMPARE(controller.RemoteConnected(), true);
    QCOMPARE(controller.RemoteUrl(), remotePath);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Repository connected."));
}

void TestDesktopGitCore::OpenRepositoryWithExistingOriginFromController()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    GitCommandRunner runner;
    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remotePath}, repositoryPath).Success(), "git remote add failed");

    AppController controller;
    controller.OpenRepositoryPath(repositoryPath);

    QCOMPARE(controller.RepositoryPath(), repositoryPath);
    QCOMPARE(controller.RepositoryInitialized(), true);
    QCOMPARE(controller.RemoteConnected(), true);
    QCOMPARE(controller.RemoteUrl(), remotePath);
    QCOMPARE(controller.RepositoryConnectionStatusText(), QStringLiteral("remote connected"));

    QVERIFY(controller.ConnectRepository(QStringLiteral("https://github.com/user/ignored")));
    QCOMPARE(controller.RemoteUrl(), remotePath);
}

void TestDesktopGitCore::OpenWorkingTreeFromController()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");
    QVERIFY(WriteTextFile(QDir(repositoryPath).filePath(QStringLiteral("file.txt")), QStringLiteral("base line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    AppController controller;
    controller.OpenRepositoryPath(repositoryPath);

    controller.OpenHistory();
    QCOMPARE(controller.HistoryVisible(), true);
    controller.OpenWorkingTree();
    QCOMPARE(controller.HistoryVisible(), false);
    QCOMPARE(controller.BranchesVisible(), false);
    QCOMPARE(controller.StashVisible(), false);

    controller.OpenBranches();
    QCOMPARE(controller.BranchesVisible(), true);
    controller.OpenWorkingTree();
    QCOMPARE(controller.HistoryVisible(), false);
    QCOMPARE(controller.BranchesVisible(), false);
    QCOMPARE(controller.StashVisible(), false);

    controller.OpenStash();
    QCOMPARE(controller.StashVisible(), true);

    controller.OpenWorkingTree();
    QCOMPARE(controller.HistoryVisible(), false);
    QCOMPARE(controller.BranchesVisible(), false);
    QCOMPARE(controller.StashVisible(), false);
}

void TestDesktopGitCore::ReadCommitHistoryAndCommitDiff()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString repositoryPath = temporaryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    const QString firstFilePath = QDir(repositoryPath).filePath(QStringLiteral("first.txt"));
    QVERIFY(WriteTextFile(firstFilePath, QStringLiteral("first line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("first.txt")}, repositoryPath).Success(), "git add first failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit first failed");

    const QString secondFilePath = QDir(repositoryPath).filePath(QStringLiteral("second.txt"));
    QVERIFY(WriteTextFile(secondFilePath, QStringLiteral("second line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("second.txt")}, repositoryPath).Success(), "git add second failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Add second file"),
        QStringLiteral("-m"),
        QStringLiteral("Second file commit body")
    }, repositoryPath).Success(), "git commit second failed");

    GitRepository repository;
    repository.SetPath(repositoryPath);

    const QList<GitCommitInfo> commits = repository.CommitHistory(1);
    QCOMPARE(commits.size(), 1);
    QCOMPARE(commits.first().subject, QStringLiteral("Add second file"));
    QCOMPARE(commits.first().body, QStringLiteral("Second file commit body"));
    QCOMPARE(commits.first().authorName, QStringLiteral("DesktopGit Test"));
    QCOMPARE(commits.first().authorEmail, QStringLiteral("test@example.local"));
    QVERIFY(!commits.first().hash.isEmpty());
    QVERIFY(!commits.first().shortHash.isEmpty());
    QVERIFY(!commits.first().date.isEmpty());

    const QList<GitCommitFile> files = repository.CommitFiles(commits.first().hash);
    QCOMPARE(files.size(), 1);
    QCOMPARE(files.first().path, QStringLiteral("second.txt"));
    QCOMPARE(files.first().status, QStringLiteral("A"));
    QCOMPARE(files.first().additions, 1);
    QCOMPARE(files.first().deletions, 0);
    QCOMPARE(files.first().Changes(), 1);

    const QString diff = repository.CommitFileDiff(commits.first().hash, QStringLiteral("second.txt"));
    QVERIFY(diff.contains(QStringLiteral("+second line")));
    QVERIFY(!diff.contains(QStringLiteral("diff --git")));
}

void TestDesktopGitCore::PopulateCommitHistoryModel()
{
    CommitHistoryModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    GitCommitInfo commit;
    commit.hash = QStringLiteral("abcdef");
    commit.shortHash = QStringLiteral("abcdef");
    commit.subject = QStringLiteral("Subject");
    commit.body = QStringLiteral("Body");
    commit.authorName = QStringLiteral("Author");
    commit.authorEmail = QStringLiteral("author@example.local");
    commit.date = QStringLiteral("2026-07-07T12:00:00+03:00");

    model.SetCommits({commit});

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.ContainsHash(QStringLiteral("abcdef")));
    QVERIFY(!model.ContainsHash(QStringLiteral("missing")));

    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, CommitHistoryModel::HashRole).toString(), QStringLiteral("abcdef"));
    QCOMPARE(model.data(index, CommitHistoryModel::ShortHashRole).toString(), QStringLiteral("abcdef"));
    QCOMPARE(model.data(index, CommitHistoryModel::SubjectRole).toString(), QStringLiteral("Subject"));
    QCOMPARE(model.data(index, CommitHistoryModel::BodyRole).toString(), QStringLiteral("Body"));
    QCOMPARE(model.data(index, CommitHistoryModel::AuthorNameRole).toString(), QStringLiteral("Author"));
    QCOMPARE(model.data(index, CommitHistoryModel::AuthorEmailRole).toString(), QStringLiteral("author@example.local"));
    QCOMPARE(model.data(index, CommitHistoryModel::DateRole).toString(), QStringLiteral("2026-07-07T12:00:00+03:00"));

    model.Clear();
    QCOMPARE(model.rowCount(), 0);
}

void TestDesktopGitCore::PopulateCommitFileModel()
{
    CommitFileModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);

    GitCommitFile file;
    file.path = QStringLiteral("src/main.cpp");
    file.status = QStringLiteral("M");
    file.additions = 3;
    file.deletions = 2;

    model.SetFiles({file});

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.ContainsPath(QStringLiteral("src/main.cpp")));
    QVERIFY(!model.ContainsPath(QStringLiteral("missing.cpp")));

    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, CommitFileModel::PathRole).toString(), QStringLiteral("src/main.cpp"));
    QCOMPARE(model.data(index, CommitFileModel::StatusRole).toString(), QStringLiteral("M"));
    QCOMPARE(model.data(index, CommitFileModel::AdditionsRole).toInt(), 3);
    QCOMPARE(model.data(index, CommitFileModel::DeletionsRole).toInt(), 2);
    QCOMPARE(model.data(index, CommitFileModel::ChangesRole).toInt(), 5);

    model.Clear();
    QCOMPARE(model.rowCount(), 0);
}

void TestDesktopGitCore::ReadCommitHistoryFromController()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString repositoryPath = temporaryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("new line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add update failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Update file"),
        QStringLiteral("-m"),
        QStringLiteral("Update file body")
    }, repositoryPath).Success(), "git commit update failed");

    AppController controller;
    controller.OpenRepositoryPath(repositoryPath);
    controller.OpenHistory();

    QCOMPARE(controller.HistoryVisible(), true);
    QCOMPARE(controller.CommitHistory()->rowCount(), 2);
    QVERIFY(!controller.SelectedCommitHash().isEmpty());
    QVERIFY(!controller.SelectedCommitShortHash().isEmpty());
    QCOMPARE(controller.SelectedCommitSubject(), QStringLiteral("Update file"));
    QCOMPARE(controller.SelectedCommitBody(), QStringLiteral("Update file body"));
    QCOMPARE(controller.SelectedCommitAuthorName(), QStringLiteral("DesktopGit Test"));
    QCOMPARE(controller.SelectedCommitAuthorEmail(), QStringLiteral("test@example.local"));
    QVERIFY(!controller.SelectedCommitDate().isEmpty());
    QCOMPARE(controller.CommitFiles()->rowCount(), 1);

    const QModelIndex commitIndex = controller.CommitHistory()->index(0, 0);
    QCOMPARE(controller.CommitHistory()->data(commitIndex, CommitHistoryModel::SubjectRole).toString(), QStringLiteral("Update file"));

    controller.SelectCommitFile(QStringLiteral("file.txt"));
    QCOMPARE(controller.SelectedCommitFilePath(), QStringLiteral("file.txt"));
    QVERIFY(controller.SelectedCommitDiff().contains(QStringLiteral("+new line")));

    controller.SelectCommit(QStringLiteral("missing"));
    QCOMPARE(controller.SelectedCommitHash(), QString());
    QCOMPARE(controller.SelectedCommitSubject(), QString());
    QCOMPARE(controller.SelectedCommitBody(), QString());
    QCOMPARE(controller.SelectedCommitAuthorName(), QString());
    QCOMPARE(controller.SelectedCommitAuthorEmail(), QString());
    QCOMPARE(controller.SelectedCommitDate(), QString());
    QCOMPARE(controller.CommitFiles()->rowCount(), 0);

    controller.CloseHistory();
    QCOMPARE(controller.HistoryVisible(), false);
}

void TestDesktopGitCore::OpenHistoryForEmptyAndPlainRepositories()
{
    QTemporaryDir plainDirectory;
    QVERIFY(plainDirectory.isValid());

    AppController plainController;
    plainController.OpenRepositoryPath(plainDirectory.path());
    plainController.OpenHistory();

    QCOMPARE(plainController.HistoryVisible(), false);
    QCOMPARE(plainController.CommitHistory()->rowCount(), 0);

    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    GitCommandRunner runner;
    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryDirectory.path()).Success(), "git init failed");

    AppController emptyRepositoryController;
    emptyRepositoryController.OpenRepositoryPath(repositoryDirectory.path());
    emptyRepositoryController.OpenHistory();

    QCOMPARE(emptyRepositoryController.HistoryVisible(), true);
    QCOMPARE(emptyRepositoryController.CommitHistory()->rowCount(), 0);
    QCOMPARE(emptyRepositoryController.CommitFiles()->rowCount(), 0);
    QCOMPARE(emptyRepositoryController.StatusMessage(), QStringLiteral("No commits yet."));
}

void TestDesktopGitCore::FormatDiffForDisplay()
{
    const QString formatted = GitDiffFormatter::FormatForDisplay(QStringLiteral(
        "diff --git a/file.txt b/file.txt\n"
        "index 1111111..2222222 100644\n"
        "--- a/file.txt\n"
        "+++ b/file.txt\n"
        "@@ -1 +1 @@\n"
        "-old line\n"
        "+new line\n"));

    QVERIFY(!formatted.contains(QStringLiteral("diff --git")));
    QVERIFY(!formatted.contains(QStringLiteral("@@")));
    QVERIFY(formatted.contains(QStringLiteral("-old line")));
    QVERIFY(formatted.contains(QStringLiteral("+new line")));
}

void TestDesktopGitCore::ReadStatusAndDiffFromRepository()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString repositoryPath = temporaryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("new line\n")));

    GitRepository repository;
    repository.SetPath(repositoryPath);

    QVERIFY(repository.IsValid());

    const QList<GitStatusFile> statusFiles = repository.Status();
    QCOMPARE(statusFiles.size(), 1);
    QCOMPARE(statusFiles.first().path, QStringLiteral("file.txt"));
    QCOMPARE(statusFiles.first().worktreeStatus, QStringLiteral("M"));
    QCOMPARE(statusFiles.first().additions, 1);
    QCOMPARE(statusFiles.first().deletions, 1);
    QCOMPARE(statusFiles.first().Changes(), 2);

    const QString diff = repository.Diff(QStringLiteral("file.txt"));
    QVERIFY(diff.contains(QStringLiteral("-old line")));
    QVERIFY(diff.contains(QStringLiteral("+new line")));
    QVERIFY(!diff.contains(QStringLiteral("diff --git")));
    QVERIFY(!diff.contains(QStringLiteral("@@")));
}

void TestDesktopGitCore::StageAndUnstageRepositoryFile()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString repositoryPath = temporaryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("new line\n")));

    GitRepository repository;
    repository.SetPath(repositoryPath);

    QList<GitStatusFile> statusFiles = repository.Status();
    QCOMPARE(statusFiles.size(), 1);
    QCOMPARE(statusFiles.first().indexStatus, QString());
    QCOMPARE(statusFiles.first().worktreeStatus, QStringLiteral("M"));

    QVERIFY(repository.StageFile(QStringLiteral("file.txt")));

    statusFiles = repository.Status();
    QCOMPARE(statusFiles.size(), 1);
    QCOMPARE(statusFiles.first().indexStatus, QStringLiteral("M"));
    QCOMPARE(statusFiles.first().worktreeStatus, QString());
    QCOMPARE(statusFiles.first().additions, 1);
    QCOMPARE(statusFiles.first().deletions, 1);

    const QString stagedDiff = repository.Diff(QStringLiteral("file.txt"));
    QVERIFY(stagedDiff.contains(QStringLiteral("-old line")));
    QVERIFY(stagedDiff.contains(QStringLiteral("+new line")));

    QVERIFY(repository.UnstageFile(QStringLiteral("file.txt")));

    statusFiles = repository.Status();
    QCOMPARE(statusFiles.size(), 1);
    QCOMPARE(statusFiles.first().indexStatus, QString());
    QCOMPARE(statusFiles.first().worktreeStatus, QStringLiteral("M"));
}

void TestDesktopGitCore::StageAndUnstageSelectedFilesFromController()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString repositoryPath = temporaryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");

    const QString firstFilePath = QDir(repositoryPath).filePath(QStringLiteral("first.txt"));
    const QString secondFilePath = QDir(repositoryPath).filePath(QStringLiteral("second.txt"));
    QVERIFY(WriteTextFile(firstFilePath, QStringLiteral("old first\n")));
    QVERIFY(WriteTextFile(secondFilePath, QStringLiteral("old second\n")));
    QVERIFY2(runner.Run({
        QStringLiteral("add"),
        QStringLiteral("first.txt"),
        QStringLiteral("second.txt")
    }, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    QVERIFY(WriteTextFile(firstFilePath, QStringLiteral("new first\n")));
    QVERIFY(WriteTextFile(secondFilePath, QStringLiteral("new second\n")));

    AppController controller;
    controller.OpenRepositoryPath(repositoryPath);

    StatusFileModel *model = controller.StatusFiles();
    QCOMPARE(model->rowCount(), 2);

    controller.SelectAllFiles();
    QCOMPARE(controller.SelectedFileCount(), 2);

    controller.StageSelectedFiles();
    QCOMPARE(controller.SelectedFileCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        QCOMPARE(model->data(index, StatusFileModel::IndexStatusRole).toString(), QStringLiteral("M"));
        QCOMPARE(model->data(index, StatusFileModel::WorktreeStatusRole).toString(), QString());
    }

    controller.UnstageSelectedFiles();
    QCOMPARE(controller.SelectedFileCount(), 2);
    QCOMPARE(model->rowCount(), 2);
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        QCOMPARE(model->data(index, StatusFileModel::IndexStatusRole).toString(), QString());
        QCOMPARE(model->data(index, StatusFileModel::WorktreeStatusRole).toString(), QStringLiteral("M"));
    }
}

void TestDesktopGitCore::CommitRepositoryChanges()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString repositoryPath = temporaryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("new line\n")));

    GitRepository repository;
    repository.SetPath(repositoryPath);

    QVERIFY(repository.StageFile(QStringLiteral("file.txt")));

    const GitCommandResult commitResult = repository.Commit(QStringLiteral("Update file"));
    QVERIFY2(commitResult.Success(), qPrintable(commitResult.standardError));
    QCOMPARE(repository.Status().size(), 0);

    const GitCommandResult logResult = runner.Run({
        QStringLiteral("log"),
        QStringLiteral("-1"),
        QStringLiteral("--pretty=%s")
    }, repositoryPath);
    QVERIFY2(logResult.Success(), qPrintable(logResult.standardError));
    QCOMPARE(logResult.standardOutput.trimmed(), QStringLiteral("Update file"));
}

void TestDesktopGitCore::CommitStagedFilesFromController()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString repositoryPath = temporaryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("new line\n")));

    AppController controller;
    QSignalSpy commitCreatedSpy(&controller, &AppController::commitCreated);

    controller.OpenRepositoryPath(repositoryPath);
    QCOMPARE(controller.StatusFiles()->rowCount(), 1);

    controller.CommitStagedFiles(QStringLiteral("No staged files"));
    QCOMPARE(commitCreatedSpy.count(), 0);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Stage at least one file before committing."));

    controller.SelectAllFiles();
    controller.StageSelectedFiles();
    QCOMPARE(controller.StagedFileCount(), 1);

    controller.CommitStagedFiles(QStringLiteral("   "));
    QCOMPARE(commitCreatedSpy.count(), 0);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Commit message cannot be empty."));

    controller.CommitStagedFiles(QStringLiteral("Update file"));
    QCOMPARE(commitCreatedSpy.count(), 1);
    QCOMPARE(controller.StatusFiles()->rowCount(), 0);
    QCOMPARE(controller.StagedFileCount(), 0);
    QCOMPARE(controller.SelectedFileCount(), 0);
    QCOMPARE(controller.SelectedFilePath(), QString());
    QCOMPARE(controller.CurrentDiff(), QString());
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Commit created."));

    const GitCommandResult logResult = runner.Run({
        QStringLiteral("log"),
        QStringLiteral("-1"),
        QStringLiteral("--pretty=%s")
    }, repositoryPath);
    QVERIFY2(logResult.Success(), qPrintable(logResult.standardError));
    QCOMPARE(logResult.standardOutput.trimmed(), QStringLiteral("Update file"));
}

void TestDesktopGitCore::PushRepositoryChanges()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    const GitCommandResult branchResult = runner.Run({
        QStringLiteral("branch"),
        QStringLiteral("--show-current")
    }, repositoryPath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());

    QVERIFY2(runner.Run({
        QStringLiteral("remote"),
        QStringLiteral("add"),
        QStringLiteral("origin"),
        remotePath
    }, repositoryPath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("push"),
        QStringLiteral("-u"),
        QStringLiteral("origin"),
        branchName
    }, repositoryPath).Success(), "git push -u failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("new line\n")));

    GitRepository repository;
    repository.SetPath(repositoryPath);

    QVERIFY(repository.StageFile(QStringLiteral("file.txt")));
    const GitCommandResult commitResult = repository.Commit(QStringLiteral("Update file"));
    QVERIFY2(commitResult.Success(), qPrintable(commitResult.standardError));

    const GitChangeSummary pushSummary = repository.OutgoingChangeSummary();
    QCOMPARE(pushSummary.filesChanged, 1);
    QCOMPARE(pushSummary.additions, 1);
    QCOMPARE(pushSummary.deletions, 1);
    QCOMPARE(pushSummary.LineChanges(), 2);

    const GitCommandResult pushResult = repository.Push();
    QVERIFY2(pushResult.Success(), qPrintable(pushResult.standardError));

    const GitCommandResult logResult = runner.Run({
        QStringLiteral("--git-dir"),
        remotePath,
        QStringLiteral("log"),
        QStringLiteral("-1"),
        QStringLiteral("--pretty=%s")
    }, repositoryPath);
    QVERIFY2(logResult.Success(), qPrintable(logResult.standardError));
    QCOMPARE(logResult.standardOutput.trimmed(), QStringLiteral("Update file"));
}

void TestDesktopGitCore::PushRepositorySetsUpstreamWhenMissing()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");
    QVERIFY2(runner.Run({
        QStringLiteral("remote"),
        QStringLiteral("add"),
        QStringLiteral("origin"),
        remotePath
    }, repositoryPath).Success(), "git remote add failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("first line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    GitRepository repository;
    repository.SetPath(repositoryPath);

    const GitCommandResult pushResult = repository.Push();
    QVERIFY2(pushResult.Success(), qPrintable(pushResult.standardError));

    const GitCommandResult upstreamResult = runner.Run({
        QStringLiteral("rev-parse"),
        QStringLiteral("--abbrev-ref"),
        QStringLiteral("@{u}")
    }, repositoryPath);
    QVERIFY2(upstreamResult.Success(), qPrintable(upstreamResult.standardError));
    QVERIFY(upstreamResult.standardOutput.trimmed().startsWith(QStringLiteral("origin/")));

    const GitCommandResult logResult = runner.Run({
        QStringLiteral("--git-dir"),
        remotePath,
        QStringLiteral("log"),
        QStringLiteral("-1"),
        QStringLiteral("--pretty=%s")
    }, repositoryPath);
    QVERIFY2(logResult.Success(), qPrintable(logResult.standardError));
    QCOMPARE(logResult.standardOutput.trimmed(), QStringLiteral("Initial commit"));
}

void TestDesktopGitCore::PushRepositoryFromController()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    GitCommandRunner runner;

    AppController unopenedController;
    QSignalSpy unopenedPushSpy(&unopenedController, &AppController::pushCompleted);
    unopenedController.PushRepository();
    QCOMPARE(unopenedPushSpy.count(), 0);
    QCOMPARE(unopenedController.StatusMessage(), QStringLiteral("Open a Git repository first."));

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.email"),
        QStringLiteral("test@example.local")
    }, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({
        QStringLiteral("config"),
        QStringLiteral("user.name"),
        QStringLiteral("DesktopGit Test")
    }, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("commit"),
        QStringLiteral("-m"),
        QStringLiteral("Initial commit")
    }, repositoryPath).Success(), "git commit failed");

    const GitCommandResult branchResult = runner.Run({
        QStringLiteral("branch"),
        QStringLiteral("--show-current")
    }, repositoryPath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());

    QVERIFY2(runner.Run({
        QStringLiteral("remote"),
        QStringLiteral("add"),
        QStringLiteral("origin"),
        remotePath
    }, repositoryPath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({
        QStringLiteral("push"),
        QStringLiteral("-u"),
        QStringLiteral("origin"),
        branchName
    }, repositoryPath).Success(), "git push -u failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("new line\n")));

    AppController controller;
    QSignalSpy commitCreatedSpy(&controller, &AppController::commitCreated);
    QSignalSpy pushCompletedSpy(&controller, &AppController::pushCompleted);

    controller.OpenRepositoryPath(repositoryPath);
    controller.SelectAllFiles();
    controller.StageSelectedFiles();
    controller.CommitStagedFiles(QStringLiteral("Update file"));

    QCOMPARE(commitCreatedSpy.count(), 1);
    QCOMPARE(controller.StatusFiles()->rowCount(), 0);
    QCOMPARE(controller.HasUpstream(), true);
    QCOMPARE(controller.AheadCount(), 1);
    QCOMPARE(controller.BehindCount(), 0);
    QCOMPARE(controller.SyncStatusText(), QStringLiteral("ahead 1"));

    controller.PushRepository();

    QCOMPARE(controller.PushInProgress(), true);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Pushing changes..."));
    QVERIFY(pushCompletedSpy.wait(5000));
    QCOMPARE(pushCompletedSpy.count(), 1);
    QCOMPARE(controller.PushInProgress(), false);
    QCOMPARE(controller.LastPushFilesChanged(), 1);
    QCOMPARE(controller.LastPushLineChanges(), 2);
    QCOMPARE(controller.PushSummaryVisible(), true);
    QCOMPARE(controller.AheadCount(), 0);
    QCOMPARE(controller.BehindCount(), 0);
    QCOMPARE(controller.SyncStatusText(), QStringLiteral("synced"));
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Push completed."));

    controller.ClosePushSummary();
    QCOMPARE(controller.PushSummaryVisible(), false);

    const GitCommandResult logResult = runner.Run({
        QStringLiteral("--git-dir"),
        remotePath,
        QStringLiteral("log"),
        QStringLiteral("-1"),
        QStringLiteral("--pretty=%s")
    }, repositoryPath);
    QVERIFY2(logResult.Success(), qPrintable(logResult.standardError));
    QCOMPARE(logResult.standardOutput.trimmed(), QStringLiteral("Update file"));
}

void TestDesktopGitCore::FetchRepositoryChanges()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    QTemporaryDir workspaceDirectory;
    QVERIFY(workspaceDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    const QString collaboratorPath = QDir(workspaceDirectory.path()).filePath(QStringLiteral("collaborator"));
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    const GitCommandResult branchResult = runner.Run({QStringLiteral("branch"), QStringLiteral("--show-current")}, repositoryPath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());

    QVERIFY2(runner.Run({QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remotePath}, repositoryPath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), branchName}, repositoryPath).Success(), "git push -u failed");
    QVERIFY2(runner.Run({QStringLiteral("--git-dir"), remotePath, QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/") + branchName}, repositoryPath).Success(), "git symbolic-ref failed");

    QVERIFY2(runner.Run({QStringLiteral("clone"), remotePath, collaboratorPath}, workspaceDirectory.path()).Success(), "git clone failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, collaboratorPath).Success(), "collaborator git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Collaborator")}, collaboratorPath).Success(), "collaborator git config user.name failed");
    QVERIFY(WriteTextFile(QDir(collaboratorPath).filePath(QStringLiteral("file.txt")), QStringLiteral("remote line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, collaboratorPath).Success(), "collaborator git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Remote update")}, collaboratorPath).Success(), "collaborator git commit failed");
    QVERIFY2(runner.Run({QStringLiteral("push")}, collaboratorPath).Success(), "collaborator git push failed");

    GitRepository repository;
    repository.SetPath(repositoryPath);

    const GitCommandResult fetchResult = repository.Fetch();
    QVERIFY2(fetchResult.Success(), qPrintable(fetchResult.standardError));

    const GitCommandResult remoteHeadResult = runner.Run({QStringLiteral("--git-dir"), remotePath, QStringLiteral("rev-parse"), branchName}, repositoryPath);
    const GitCommandResult trackingHeadResult = runner.Run({QStringLiteral("rev-parse"), QStringLiteral("origin/") + branchName}, repositoryPath);
    QVERIFY2(remoteHeadResult.Success(), qPrintable(remoteHeadResult.standardError));
    QVERIFY2(trackingHeadResult.Success(), qPrintable(trackingHeadResult.standardError));
    QCOMPARE(trackingHeadResult.standardOutput.trimmed(), remoteHeadResult.standardOutput.trimmed());
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("old line\n"));
}

void TestDesktopGitCore::PullRepositoryChanges()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    QTemporaryDir workspaceDirectory;
    QVERIFY(workspaceDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    const QString collaboratorPath = QDir(workspaceDirectory.path()).filePath(QStringLiteral("collaborator"));
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    const GitCommandResult branchResult = runner.Run({QStringLiteral("branch"), QStringLiteral("--show-current")}, repositoryPath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());

    QVERIFY2(runner.Run({QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remotePath}, repositoryPath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), branchName}, repositoryPath).Success(), "git push -u failed");
    QVERIFY2(runner.Run({QStringLiteral("--git-dir"), remotePath, QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/") + branchName}, repositoryPath).Success(), "git symbolic-ref failed");

    QVERIFY2(runner.Run({QStringLiteral("clone"), remotePath, collaboratorPath}, workspaceDirectory.path()).Success(), "git clone failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, collaboratorPath).Success(), "collaborator git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Collaborator")}, collaboratorPath).Success(), "collaborator git config user.name failed");
    QVERIFY(WriteTextFile(QDir(collaboratorPath).filePath(QStringLiteral("file.txt")), QStringLiteral("remote line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, collaboratorPath).Success(), "collaborator git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Remote update")}, collaboratorPath).Success(), "collaborator git commit failed");
    QVERIFY2(runner.Run({QStringLiteral("push")}, collaboratorPath).Success(), "collaborator git push failed");

    GitRepository repository;
    repository.SetPath(repositoryPath);

    const GitCommandResult pullResult = repository.Pull();
    QVERIFY2(pullResult.Success(), qPrintable(pullResult.standardError));
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("remote line\n"));

    const GitCommandResult logResult = runner.Run({QStringLiteral("log"), QStringLiteral("-1"), QStringLiteral("--pretty=%s")}, repositoryPath);
    QVERIFY2(logResult.Success(), qPrintable(logResult.standardError));
    QCOMPARE(logResult.standardOutput.trimmed(), QStringLiteral("Remote update"));
}

void TestDesktopGitCore::FetchAndPullRepositoryFromController()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    QTemporaryDir workspaceDirectory;
    QVERIFY(workspaceDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    const QString collaboratorPath = QDir(workspaceDirectory.path()).filePath(QStringLiteral("collaborator"));
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    const GitCommandResult branchResult = runner.Run({QStringLiteral("branch"), QStringLiteral("--show-current")}, repositoryPath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());

    QVERIFY2(runner.Run({QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remotePath}, repositoryPath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), branchName}, repositoryPath).Success(), "git push -u failed");
    QVERIFY2(runner.Run({QStringLiteral("--git-dir"), remotePath, QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/") + branchName}, repositoryPath).Success(), "git symbolic-ref failed");

    QVERIFY2(runner.Run({QStringLiteral("clone"), remotePath, collaboratorPath}, workspaceDirectory.path()).Success(), "git clone failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, collaboratorPath).Success(), "collaborator git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Collaborator")}, collaboratorPath).Success(), "collaborator git config user.name failed");
    QVERIFY(WriteTextFile(QDir(collaboratorPath).filePath(QStringLiteral("file.txt")), QStringLiteral("remote line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, collaboratorPath).Success(), "collaborator git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Remote update")}, collaboratorPath).Success(), "collaborator git commit failed");
    QVERIFY2(runner.Run({QStringLiteral("push")}, collaboratorPath).Success(), "collaborator git push failed");

    AppController controller;
    QSignalSpy fetchCompletedSpy(&controller, &AppController::fetchCompleted);
    QSignalSpy pullCompletedSpy(&controller, &AppController::pullCompleted);

    controller.OpenRepositoryPath(repositoryPath);
    QCOMPARE(controller.HasUpstream(), true);
    QCOMPARE(controller.AheadCount(), 0);
    QCOMPARE(controller.BehindCount(), 0);
    QCOMPARE(controller.SyncStatusText(), QStringLiteral("synced"));

    controller.FetchRepository();
    QCOMPARE(controller.FetchInProgress(), true);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Fetching changes..."));
    QVERIFY(fetchCompletedSpy.wait(5000));
    QCOMPARE(controller.FetchInProgress(), false);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Fetch completed."));
    QCOMPARE(controller.AheadCount(), 0);
    QCOMPARE(controller.BehindCount(), 1);
    QCOMPARE(controller.SyncStatusText(), QStringLiteral("behind 1"));
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("old line\n"));

    controller.PullRepository();
    QCOMPARE(controller.PullInProgress(), true);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Pulling changes..."));
    QVERIFY(pullCompletedSpy.wait(5000));
    QCOMPARE(controller.PullInProgress(), false);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Pull completed."));
    QCOMPARE(controller.AheadCount(), 0);
    QCOMPARE(controller.BehindCount(), 0);
    QCOMPARE(controller.SyncStatusText(), QStringLiteral("synced"));
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("remote line\n"));
}

void TestDesktopGitCore::CloneRepositoryFromController()
{
    QTemporaryDir sourceDirectory;
    QVERIFY(sourceDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    QTemporaryDir cloneParentDirectory;
    QVERIFY(cloneParentDirectory.isValid());

    const QString sourcePath = sourceDirectory.path();
    const QString remotePath = remoteDirectory.path();
    const QString cloneParentPath = cloneParentDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, sourcePath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, sourcePath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, sourcePath).Success(), "git config user.name failed");

    QVERIFY(WriteTextFile(QDir(sourcePath).filePath(QStringLiteral("file.txt")), QStringLiteral("controller clone line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, sourcePath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, sourcePath).Success(), "git commit failed");
    QVERIFY2(runner.Run({QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remotePath}, sourcePath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("HEAD")}, sourcePath).Success(), "git push failed");
    const GitCommandResult branchResult = runner.Run({QStringLiteral("branch"), QStringLiteral("--show-current")}, sourcePath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());
    QVERIFY2(runner.Run({QStringLiteral("--git-dir"), remotePath, QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/") + branchName}, sourcePath).Success(), "git symbolic-ref failed");

    AppController controller;
    QSignalSpy cloneCompletedSpy(&controller, &AppController::cloneCompleted);

    QCOMPARE(controller.DefaultCloneFolderName(remotePath), QFileInfo(remotePath).fileName());

    controller.CloneRepository(remotePath, cloneParentPath, QStringLiteral("controller-clone"));
    QCOMPARE(controller.CloneInProgress(), true);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Cloning repository..."));
    QVERIFY(cloneCompletedSpy.wait(5000));

    const QString clonedRepositoryPath = QDir(cloneParentPath).filePath(QStringLiteral("controller-clone"));
    QCOMPARE(controller.CloneInProgress(), false);
    QCOMPARE(controller.RepositoryPath(), clonedRepositoryPath);
    QCOMPARE(controller.RepositoryInitialized(), true);
    QCOMPARE(controller.RemoteConnected(), true);
    QCOMPARE(controller.RemoteUrl(), remotePath);
    QCOMPARE(controller.CurrentBranch(), branchName);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Repository cloned."));
    QCOMPARE(ReadTextFile(QDir(clonedRepositoryPath).filePath(QStringLiteral("file.txt"))), QStringLiteral("controller clone line\n"));

    controller.CloneRepository(remotePath, cloneParentPath, QStringLiteral("controller-clone"));
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Clone target folder already exists."));

    controller.CloneRepository(QStringLiteral("bad url with spaces"), cloneParentPath, QStringLiteral("bad-clone"));
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Remote URL is invalid."));
}

void TestDesktopGitCore::ReadBranchSyncStatus()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    QTemporaryDir remoteDirectory;
    QVERIFY(remoteDirectory.isValid());

    QTemporaryDir workspaceDirectory;
    QVERIFY(workspaceDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString remotePath = remoteDirectory.path();
    const QString collaboratorPath = QDir(workspaceDirectory.path()).filePath(QStringLiteral("collaborator"));
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("init"), QStringLiteral("--bare")}, remotePath).Success(), "git init --bare failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    QVERIFY(WriteTextFile(filePath, QStringLiteral("old line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    GitRepository repository;
    repository.SetPath(repositoryPath);

    GitBranchSyncStatus syncStatus = repository.BranchSyncStatus();
    QCOMPARE(syncStatus.hasUpstream, false);
    QCOMPARE(syncStatus.ahead, 0);
    QCOMPARE(syncStatus.behind, 0);

    const GitCommandResult branchResult = runner.Run({QStringLiteral("branch"), QStringLiteral("--show-current")}, repositoryPath);
    QVERIFY2(branchResult.Success(), qPrintable(branchResult.standardError));
    const QString branchName = branchResult.standardOutput.trimmed();
    QVERIFY(!branchName.isEmpty());

    QVERIFY2(runner.Run({QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remotePath}, repositoryPath).Success(), "git remote add failed");
    QVERIFY2(runner.Run({QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), branchName}, repositoryPath).Success(), "git push -u failed");
    QVERIFY2(runner.Run({QStringLiteral("--git-dir"), remotePath, QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"), QStringLiteral("refs/heads/") + branchName}, repositoryPath).Success(), "git symbolic-ref failed");

    syncStatus = repository.BranchSyncStatus();
    QCOMPARE(syncStatus.hasUpstream, true);
    QCOMPARE(syncStatus.ahead, 0);
    QCOMPARE(syncStatus.behind, 0);

    QVERIFY(WriteTextFile(filePath, QStringLiteral("local line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add local failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Local update")}, repositoryPath).Success(), "git commit local failed");

    syncStatus = repository.BranchSyncStatus();
    QCOMPARE(syncStatus.hasUpstream, true);
    QCOMPARE(syncStatus.ahead, 1);
    QCOMPARE(syncStatus.behind, 0);

    QVERIFY2(runner.Run({QStringLiteral("clone"), remotePath, collaboratorPath}, workspaceDirectory.path()).Success(), "git clone failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, collaboratorPath).Success(), "collaborator git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Collaborator")}, collaboratorPath).Success(), "collaborator git config user.name failed");
    QVERIFY(WriteTextFile(QDir(collaboratorPath).filePath(QStringLiteral("remote.txt")), QStringLiteral("remote line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("remote.txt")}, collaboratorPath).Success(), "collaborator git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Remote update")}, collaboratorPath).Success(), "collaborator git commit failed");
    QVERIFY2(runner.Run({QStringLiteral("push")}, collaboratorPath).Success(), "collaborator git push failed");

    QVERIFY(repository.Fetch().Success());

    syncStatus = repository.BranchSyncStatus();
    QCOMPARE(syncStatus.hasUpstream, true);
    QCOMPARE(syncStatus.ahead, 1);
    QCOMPARE(syncStatus.behind, 1);

    QVERIFY2(runner.Run({QStringLiteral("reset"), QStringLiteral("--hard"), QStringLiteral("origin/") + branchName}, repositoryPath).Success(), "git reset failed");

    syncStatus = repository.BranchSyncStatus();
    QCOMPARE(syncStatus.hasUpstream, true);
    QCOMPARE(syncStatus.ahead, 0);
    QCOMPARE(syncStatus.behind, 0);
}

void TestDesktopGitCore::ReadAndChangeLocalBranches()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    QVERIFY(WriteTextFile(QDir(repositoryPath).filePath(QStringLiteral("file.txt")), QStringLiteral("main line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    GitRepository repository;
    repository.SetPath(repositoryPath);

    const QString initialBranch = repository.CurrentBranch();
    QVERIFY(!initialBranch.isEmpty());
    QVERIFY(repository.ValidateBranchName(QStringLiteral("feature/test")));
    QVERIFY(!repository.ValidateBranchName(QStringLiteral("bad branch name")));

    QVERIFY2(repository.CreateBranch(QStringLiteral("feature/test")).Success(), "git checkout -b failed");
    QCOMPARE(repository.CurrentBranch(), QStringLiteral("feature/test"));

    QList<GitBranchInfo> branches = repository.LocalBranches();
    QCOMPARE(branches.size(), 2);

    bool foundInitialBranch = false;
    bool foundFeatureBranch = false;
    for (const GitBranchInfo &branch : branches) {
        if (branch.name == initialBranch) {
            foundInitialBranch = true;
            QCOMPARE(branch.isCurrent, false);
        } else if (branch.name == QStringLiteral("feature/test")) {
            foundFeatureBranch = true;
            QCOMPARE(branch.isCurrent, true);
        }
    }

    QVERIFY(foundInitialBranch);
    QVERIFY(foundFeatureBranch);

    QVERIFY2(repository.CheckoutBranch(initialBranch).Success(), "git checkout initial branch failed");
    QCOMPARE(repository.CurrentBranch(), initialBranch);

    branches = repository.LocalBranches();
    for (const GitBranchInfo &branch : branches) {
        if (branch.name == initialBranch) {
            QCOMPARE(branch.isCurrent, true);
        }
    }
}

void TestDesktopGitCore::ManageBranchesFromController()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    QVERIFY(WriteTextFile(QDir(repositoryPath).filePath(QStringLiteral("file.txt")), QStringLiteral("main line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    AppController controller;
    controller.OpenRepositoryPath(repositoryPath);

    const QString initialBranch = controller.CurrentBranch();
    QVERIFY(!initialBranch.isEmpty());

    controller.OpenBranches();
    QCOMPARE(controller.BranchesVisible(), true);
    QCOMPARE(controller.HistoryVisible(), false);
    QCOMPARE(controller.Branches()->rowCount(), 1);
    QCOMPARE(controller.SelectedBranchName(), initialBranch);

    controller.CreateBranch(QStringLiteral("feature/controller"));
    QCOMPARE(controller.CurrentBranch(), QStringLiteral("feature/controller"));
    QCOMPARE(controller.SelectedBranchName(), QStringLiteral("feature/controller"));
    QCOMPARE(controller.Branches()->rowCount(), 2);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Created and checked out feature/controller."));

    controller.CreateBranch(QStringLiteral("bad branch name"));
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Branch name is invalid."));

    controller.SelectBranch(initialBranch);
    QCOMPARE(controller.SelectedBranchName(), initialBranch);

    controller.CheckoutSelectedBranch();
    QCOMPARE(controller.CurrentBranch(), initialBranch);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Checked out %1.").arg(initialBranch));

    controller.SelectBranch(QStringLiteral("missing"));
    QCOMPARE(controller.SelectedBranchName(), QString());
}

void TestDesktopGitCore::ManageStashes()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("base line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    GitRepository repository;
    repository.SetPath(repositoryPath);

    const QString branchName = repository.CurrentBranch();
    QVERIFY(!branchName.isEmpty());

    QVERIFY(WriteTextFile(filePath, QStringLiteral("stashed line\n")));
    const GitCommandResult pushResult = repository.StashPush(QStringLiteral("Work in progress"));
    QVERIFY2(pushResult.Success(), qPrintable(pushResult.standardError));
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("base line\n"));

    QList<GitStashInfo> stashes = repository.Stashes();
    QCOMPARE(stashes.size(), 1);
    QCOMPARE(stashes.first().index, 0);
    QCOMPARE(stashes.first().name, QStringLiteral("stash@{0}"));
    QCOMPARE(stashes.first().branch, branchName);
    QCOMPARE(stashes.first().message, QStringLiteral("Work in progress"));

    QVERIFY2(repository.StashApply(QStringLiteral("stash@{0}")).Success(), "git stash apply failed");
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("stashed line\n"));
    QCOMPARE(repository.Stashes().size(), 1);

    QVERIFY2(runner.Run({QStringLiteral("reset"), QStringLiteral("--hard"), QStringLiteral("HEAD")}, repositoryPath).Success(), "git reset failed");
    QVERIFY2(repository.StashPop(QStringLiteral("stash@{0}")).Success(), "git stash pop failed");
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("stashed line\n"));
    QCOMPARE(repository.Stashes().size(), 0);

    QVERIFY2(runner.Run({QStringLiteral("reset"), QStringLiteral("--hard"), QStringLiteral("HEAD")}, repositoryPath).Success(), "git reset second failed");
    QVERIFY(WriteTextFile(filePath, QStringLiteral("drop line\n")));
    QVERIFY2(repository.StashPush(QStringLiteral("Drop me")).Success(), "git stash push drop failed");
    QCOMPARE(repository.Stashes().size(), 1);
    QVERIFY2(repository.StashDrop(QStringLiteral("stash@{0}")).Success(), "git stash drop failed");
    QCOMPARE(repository.Stashes().size(), 0);
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("base line\n"));
}

void TestDesktopGitCore::ManageStashesFromController()
{
    QTemporaryDir repositoryDirectory;
    QVERIFY(repositoryDirectory.isValid());

    const QString repositoryPath = repositoryDirectory.path();
    const QString filePath = QDir(repositoryPath).filePath(QStringLiteral("file.txt"));
    GitCommandRunner runner;

    QVERIFY2(runner.Run({QStringLiteral("init")}, repositoryPath).Success(), "git init failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.local")}, repositoryPath).Success(), "git config user.email failed");
    QVERIFY2(runner.Run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("DesktopGit Test")}, repositoryPath).Success(), "git config user.name failed");

    QVERIFY(WriteTextFile(filePath, QStringLiteral("base line\n")));
    QVERIFY2(runner.Run({QStringLiteral("add"), QStringLiteral("file.txt")}, repositoryPath).Success(), "git add failed");
    QVERIFY2(runner.Run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("Initial commit")}, repositoryPath).Success(), "git commit failed");

    AppController controller;
    controller.OpenRepositoryPath(repositoryPath);

    controller.OpenStash();
    QCOMPARE(controller.StashVisible(), true);
    QCOMPARE(controller.HistoryVisible(), false);
    QCOMPARE(controller.BranchesVisible(), false);
    QCOMPARE(controller.Stashes()->rowCount(), 0);
    QCOMPARE(controller.StatusMessage(), QStringLiteral("No stash entries."));

    controller.ApplySelectedStash();
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Select a stash first."));

    QVERIFY(WriteTextFile(filePath, QStringLiteral("controller stash line\n")));
    controller.RefreshRepository();
    QCOMPARE(controller.StatusFiles()->rowCount(), 1);

    controller.StashPush(QStringLiteral("Controller stash"));
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Stash created."));
    QCOMPARE(controller.Stashes()->rowCount(), 1);
    QCOMPARE(controller.SelectedStashName(), QStringLiteral("stash@{0}"));
    QCOMPARE(controller.StatusFiles()->rowCount(), 0);
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("base line\n"));

    controller.ApplySelectedStash();
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Stash applied."));
    QCOMPARE(controller.Stashes()->rowCount(), 1);
    QCOMPARE(controller.StatusFiles()->rowCount(), 1);
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("controller stash line\n"));

    QVERIFY2(runner.Run({QStringLiteral("reset"), QStringLiteral("--hard"), QStringLiteral("HEAD")}, repositoryPath).Success(), "git reset failed");
    controller.RefreshRepository();
    controller.PopSelectedStash();
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Stash popped."));
    QCOMPARE(controller.Stashes()->rowCount(), 0);
    QCOMPARE(controller.SelectedStashName(), QString());
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("controller stash line\n"));

    QVERIFY2(runner.Run({QStringLiteral("reset"), QStringLiteral("--hard"), QStringLiteral("HEAD")}, repositoryPath).Success(), "git reset second failed");
    controller.RefreshRepository();
    controller.StashPush(QStringLiteral("No changes"));
    QCOMPARE(controller.StatusMessage(), QStringLiteral("No local changes to stash."));
    QCOMPARE(controller.Stashes()->rowCount(), 0);

    QVERIFY(WriteTextFile(filePath, QStringLiteral("drop from controller\n")));
    controller.RefreshRepository();
    controller.StashPush(QStringLiteral("Drop from controller"));
    QCOMPARE(controller.Stashes()->rowCount(), 1);
    controller.DropSelectedStash();
    QCOMPARE(controller.StatusMessage(), QStringLiteral("Stash dropped."));
    QCOMPARE(controller.Stashes()->rowCount(), 0);
    QCOMPARE(ReadTextFile(filePath), QStringLiteral("base line\n"));
}

QTEST_MAIN(TestDesktopGitCore)

#include "TestDesktopGitCore.moc"
