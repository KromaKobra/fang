#pragma once

#include "fs/FileInfo.h"

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

namespace fang {

/// Streaming list of search matches (docs/03 §9): DirectoryModel's roles
/// plus matchContext (content-hit snippet) and parentDir.
class SearchResultsModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("SearchResultsModel is owned by the Search controller")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        UrlRole,
        IsDirRole,
        IsSymlinkRole,
        SizeRole,
        SizeTextRole,
        ModifiedRole,
        CreatedRole,
        AccessedRole,
        MimeRole,
        MimeCommentRole,
        PermissionsRole,
        OwnerRole,
        GroupRole,
        IconNameRole,
        ThumbnailRole,
        IsHiddenRole,
        SelectedRole,
        ExtensionRole,
        MatchContextRole,
        ParentDirRole,
    };
    Q_ENUM(Roles)

    struct Result {
        FileInfo info;
        QString matchContext;
    };

    explicit SearchResultsModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE QString parentDirAt(int row) const;
    Q_INVOKABLE bool isDirAt(int row) const;

public slots:
    void appendBatch(const QList<fang::SearchResultsModel::Result>& results);
    void clear();

signals:
    void countChanged();

private:
    QList<Result> m_results;
};

} // namespace fang

Q_DECLARE_METATYPE(fang::SearchResultsModel::Result)
