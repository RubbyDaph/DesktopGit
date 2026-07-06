#pragma once

#include "GitRepository.h"

#include <QAbstractListModel>
#include <QList>

class CommitHistoryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        HashRole = Qt::UserRole + 1,
        ShortHashRole,
        SubjectRole,
        AuthorNameRole,
        AuthorEmailRole,
        DateRole
    };

    explicit CommitHistoryModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void SetCommits(const QList<GitCommitInfo> &commits);
    void Clear();
    [[nodiscard]] bool ContainsHash(const QString &hash) const;

private:
    QList<GitCommitInfo> commits;
};
