#include "BranchModel.h"

BranchModel::BranchModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BranchModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return branches.size();
}

QVariant BranchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= branches.size()) {
        return {};
    }

    const GitBranchInfo &branch = branches.at(index.row());

    switch (role) {
    case NameRole:
        return branch.name;
    case CurrentRole:
        return branch.isCurrent;
    case UpstreamRole:
        return branch.upstream;
    default:
        return {};
    }
}

QHash<int, QByteArray> BranchModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {CurrentRole, "current"},
        {UpstreamRole, "upstream"}
    };
}

void BranchModel::SetBranches(const QList<GitBranchInfo> &branches)
{
    beginResetModel();
    this->branches = branches;
    endResetModel();
}

void BranchModel::Clear()
{
    SetBranches({});
}

bool BranchModel::ContainsBranch(const QString &name) const
{
    for (const GitBranchInfo &branch : branches) {
        if (branch.name == name) {
            return true;
        }
    }

    return false;
}
