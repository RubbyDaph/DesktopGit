#include "AppController.h"
#include "GitCommandRunner.h"
#include "GitDiffFormatter.h"
#include "GitRepository.h"
#include "GitStatusParser.h"
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
    void RunGitVersionCommand();
    void FormatDiffForDisplay();
    void ReadStatusAndDiffFromRepository();
    void StageAndUnstageRepositoryFile();
    void StageAndUnstageSelectedFilesFromController();
    void CommitRepositoryChanges();
    void CommitStagedFilesFromController();
    void PushRepositoryChanges();
    void PushRepositoryFromController();
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

void TestDesktopGitCore::RunGitVersionCommand()
{
    GitCommandRunner runner;
    const GitCommandResult result = runner.Run({QStringLiteral("--version")});

    QVERIFY2(result.Success(), qPrintable(result.standardError));
    QVERIFY(result.standardOutput.startsWith(QStringLiteral("git version")));
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
    QSignalSpy commitCreatedSpy(&controller, &AppController::CommitCreated);

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
    QSignalSpy unopenedPushSpy(&unopenedController, &AppController::PushCompleted);
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
    QSignalSpy commitCreatedSpy(&controller, &AppController::CommitCreated);
    QSignalSpy pushCompletedSpy(&controller, &AppController::PushCompleted);

    controller.OpenRepositoryPath(repositoryPath);
    controller.SelectAllFiles();
    controller.StageSelectedFiles();
    controller.CommitStagedFiles(QStringLiteral("Update file"));

    QCOMPARE(commitCreatedSpy.count(), 1);
    QCOMPARE(controller.StatusFiles()->rowCount(), 0);

    controller.PushRepository();

    QCOMPARE(pushCompletedSpy.count(), 1);
    QCOMPARE(controller.LastPushFilesChanged(), 1);
    QCOMPARE(controller.LastPushLineChanges(), 2);
    QCOMPARE(controller.PushSummaryVisible(), true);
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

QTEST_MAIN(TestDesktopGitCore)

#include "TestDesktopGitCore.moc"
