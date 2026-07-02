#pragma once

#include "GitStatusFile.h"

#include <QAbstractListModel>
#include <QList>

class StatusFileModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        PathRole = Qt::UserRole + 1,
        IndexStatusRole,
        WorktreeStatusRole,
        DisplayStatusRole,
        StagedRole
    };

    explicit StatusFileModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void SetFiles(const QList<GitStatusFile> &files);
    void Clear();

private:
    QList<GitStatusFile> files;
};
