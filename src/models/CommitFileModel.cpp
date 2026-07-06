#include "CommitFileModel.h"

CommitFileModel::CommitFileModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommitFileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return files.size();
}

QVariant CommitFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= files.size()) {
        return {};
    }

    const GitCommitFile &file = files.at(index.row());

    switch (role) {
    case PathRole:
        return file.path;
    case StatusRole:
        return file.status;
    case AdditionsRole:
        return file.additions;
    case DeletionsRole:
        return file.deletions;
    case ChangesRole:
        return file.Changes();
    default:
        return {};
    }
}

QHash<int, QByteArray> CommitFileModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {StatusRole, "status"},
        {AdditionsRole, "additions"},
        {DeletionsRole, "deletions"},
        {ChangesRole, "changes"}
    };
}

void CommitFileModel::SetFiles(const QList<GitCommitFile> &files)
{
    beginResetModel();
    this->files = files;
    endResetModel();
}

void CommitFileModel::Clear()
{
    SetFiles({});
}

bool CommitFileModel::ContainsPath(const QString &path) const
{
    for (const GitCommitFile &file : files) {
        if (file.path == path) {
            return true;
        }
    }

    return false;
}
