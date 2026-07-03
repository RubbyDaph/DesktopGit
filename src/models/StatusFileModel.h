#pragma once

#include "GitStatusFile.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QStringList>

class StatusFileModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        PathRole = Qt::UserRole + 1,
        IndexStatusRole,
        WorktreeStatusRole,
        DisplayStatusRole,
        StagedRole,
        AdditionsRole,
        DeletionsRole,
        ChangesRole,
        SelectedRole
    };

    explicit StatusFileModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void SetFiles(const QList<GitStatusFile> &files);
    void Clear();
    [[nodiscard]] QStringList SelectedPaths() const;
    [[nodiscard]] int SelectedCount() const;
    [[nodiscard]] int StagedCount() const;
    [[nodiscard]] bool IsSelected(const QString &path) const;
    void ToggleSelected(const QString &path);
    void SelectAll();
    void ClearSelection();

private:
    QList<GitStatusFile> files;
    QSet<QString> selectedPaths;
};
