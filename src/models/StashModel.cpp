#include "StashModel.h"

StashModel::StashModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int StashModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return stashes.size();
}

QVariant StashModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= stashes.size()) {
        return {};
    }

    const GitStashInfo &stash = stashes.at(index.row());

    switch (role) {
    case IndexRole:
        return stash.index;
    case NameRole:
        return stash.name;
    case BranchRole:
        return stash.branch;
    case MessageRole:
        return stash.message;
    default:
        return {};
    }
}

QHash<int, QByteArray> StashModel::roleNames() const
{
    return {
        {IndexRole, "index"},
        {NameRole, "name"},
        {BranchRole, "branch"},
        {MessageRole, "message"}
    };
}

void StashModel::SetStashes(const QList<GitStashInfo> &stashes)
{
    beginResetModel();
    this->stashes = stashes;
    endResetModel();
}

void StashModel::Clear()
{
    SetStashes({});
}

bool StashModel::ContainsStash(const QString &name) const
{
    for (const GitStashInfo &stash : stashes) {
        if (stash.name == name) {
            return true;
        }
    }

    return false;
}
