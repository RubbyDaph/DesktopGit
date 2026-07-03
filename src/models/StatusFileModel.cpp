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
    case AdditionsRole:
        return file.additions;
    case DeletionsRole:
        return file.deletions;
    case ChangesRole:
        return file.Changes();
    case SelectedRole:
        return selectedPaths.contains(file.path);
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
        {StagedRole, "staged"},
        {AdditionsRole, "additions"},
        {DeletionsRole, "deletions"},
        {ChangesRole, "changes"},
        {SelectedRole, "selected"}
    };
}

void StatusFileModel::SetFiles(const QList<GitStatusFile> &files)
{
    QSet<QString> nextSelectedPaths;
    for (const GitStatusFile &file : files) {
        if (selectedPaths.contains(file.path)) {
            nextSelectedPaths.insert(file.path);
        }
    }

    beginResetModel();
    this->files = files;
    selectedPaths = nextSelectedPaths;
    endResetModel();
}

void StatusFileModel::Clear()
{
    SetFiles({});
}

QStringList StatusFileModel::SelectedPaths() const
{
    QStringList paths;
    paths.reserve(selectedPaths.size());

    for (const GitStatusFile &file : files) {
        if (selectedPaths.contains(file.path)) {
            paths.append(file.path);
        }
    }

    return paths;
}

int StatusFileModel::SelectedCount() const
{
    return selectedPaths.size();
}

int StatusFileModel::StagedCount() const
{
    int count = 0;
    for (const GitStatusFile &file : files) {
        if (file.IsStaged()) {
            ++count;
        }
    }

    return count;
}

bool StatusFileModel::IsSelected(const QString &path) const
{
    return selectedPaths.contains(path);
}

void StatusFileModel::ToggleSelected(const QString &path)
{
    if (path.isEmpty()) {
        return;
    }

    int row = -1;
    for (int index = 0; index < files.size(); ++index) {
        if (files.at(index).path == path) {
            row = index;
            break;
        }
    }

    if (row < 0) {
        return;
    }

    if (selectedPaths.contains(path)) {
        selectedPaths.remove(path);
    } else {
        selectedPaths.insert(path);
    }

    const QModelIndex changedIndex = index(row, 0);
    emit dataChanged(changedIndex, changedIndex, {SelectedRole});
}

void StatusFileModel::SelectAll()
{
    if (files.isEmpty()) {
        return;
    }

    selectedPaths.clear();
    for (const GitStatusFile &file : files) {
        selectedPaths.insert(file.path);
    }

    emit dataChanged(index(0, 0), index(files.size() - 1, 0), {SelectedRole});
}

void StatusFileModel::ClearSelection()
{
    if (selectedPaths.isEmpty() || files.isEmpty()) {
        selectedPaths.clear();
        return;
    }

    selectedPaths.clear();
    emit dataChanged(index(0, 0), index(files.size() - 1, 0), {SelectedRole});
}
