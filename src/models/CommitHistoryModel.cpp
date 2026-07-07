#include "CommitHistoryModel.h"

CommitHistoryModel::CommitHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommitHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return commits.size();
}

QVariant CommitHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= commits.size()) {
        return {};
    }

    const GitCommitInfo &commit = commits.at(index.row());

    switch (role) {
    case HashRole:
        return commit.hash;
    case ShortHashRole:
        return commit.shortHash;
    case SubjectRole:
        return commit.subject;
    case BodyRole:
        return commit.body;
    case AuthorNameRole:
        return commit.authorName;
    case AuthorEmailRole:
        return commit.authorEmail;
    case DateRole:
        return commit.date;
    default:
        return {};
    }
}

QHash<int, QByteArray> CommitHistoryModel::roleNames() const
{
    return {
        {HashRole, "hash"},
        {ShortHashRole, "shortHash"},
        {SubjectRole, "subject"},
        {BodyRole, "body"},
        {AuthorNameRole, "authorName"},
        {AuthorEmailRole, "authorEmail"},
        {DateRole, "date"}
    };
}

void CommitHistoryModel::SetCommits(const QList<GitCommitInfo> &commits)
{
    beginResetModel();
    this->commits = commits;
    endResetModel();
}

void CommitHistoryModel::Clear()
{
    SetCommits({});
}

bool CommitHistoryModel::ContainsHash(const QString &hash) const
{
    for (const GitCommitInfo &commit : commits) {
        if (commit.hash == hash) {
            return true;
        }
    }

    return false;
}

GitCommitInfo CommitHistoryModel::CommitByHash(const QString &hash) const
{
    for (const GitCommitInfo &commit : commits) {
        if (commit.hash == hash) {
            return commit;
        }
    }

    return {};
}
