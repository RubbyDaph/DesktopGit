#include "StatusFileModel.h"

StatusFileModel::StatusFileModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int StatusFileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return files.size();
}

QVariant StatusFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= files.size()) {
        return {};
    }

    const GitStatusFile &file = files.at(index.row());

    switch (role) {
    case PathRole:
        return file.path;
    case IndexStatusRole:
        return file.indexStatus;
    case WorktreeStatusRole:
        return file.worktreeStatus;
    case DisplayStatusRole:
        return file.DisplayStatus();
    case StagedRole:
        return file.IsStaged();
    default:
        return {};
    }
}

QHash<int, QByteArray> StatusFileModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {IndexStatusRole, "indexStatus"},
        {WorktreeStatusRole, "worktreeStatus"},
        {DisplayStatusRole, "displayStatus"},
        {StagedRole, "staged"}
    };
}

void StatusFileModel::SetFiles(const QList<GitStatusFile> &files)
{
    beginResetModel();
    this->files = files;
    endResetModel();
}

void StatusFileModel::Clear()
{
    SetFiles({});
}
