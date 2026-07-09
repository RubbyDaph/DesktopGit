#pragma once

#include "GitRepository.h"

#include <QAbstractListModel>
#include <QList>
#include <QString>

class StashModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IndexRole = Qt::UserRole + 1,
        NameRole,
        BranchRole,
        MessageRole
    };

    explicit StashModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void SetStashes(const QList<GitStashInfo> &stashes);
    void Clear();
    [[nodiscard]] bool ContainsStash(const QString &name) const;

private:
    QList<GitStashInfo> stashes;
};
