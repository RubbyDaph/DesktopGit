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
    void RunGitVersionCommand();
    void FormatDiffForDisplay();
    void ReadStatusAndDiffFromRepository();
    void StageAndUnstageRepositoryFile();
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

QTEST_MAIN(TestDesktopGitCore)

#include "TestDesktopGitCore.moc"
