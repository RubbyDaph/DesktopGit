#pragma once

#include "GitRepository.h"

#include <QAbstractListModel>
#include <QList>
#include <QString>

class BranchModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        CurrentRole,
        UpstreamRole
    };

    explicit BranchModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void SetBranches(const QList<GitBranchInfo> &branches);
    void Clear();
    [[nodiscard]] bool ContainsBranch(const QString &name) const;

private:
    QList<GitBranchInfo> branches;
};
