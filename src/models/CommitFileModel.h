#pragma once

#include "GitRepository.h"

#include <QAbstractListModel>
#include <QList>

class CommitFileModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        PathRole = Qt::UserRole + 1,
        StatusRole,
        AdditionsRole,
        DeletionsRole,
        ChangesRole
    };

    explicit CommitFileModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void SetFiles(const QList<GitCommitFile> &files);
    void Clear();
    [[nodiscard]] bool ContainsPath(const QString &path) const;

private:
    QList<GitCommitFile> files;
};
