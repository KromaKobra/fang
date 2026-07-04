#pragma once

#include "util/Enums.h"

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QtQml/qqmlregistration.h>

namespace fang {

class MountMonitor;

/// Sidebar model: XDG user dirs, editable bookmarks, devices, mounted gvfs
/// network paths and Trash — usable directly in a ListView (docs/03 §11,
/// docs/04).
class Places : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Places is owned by Application")

public:
    enum Roles {
        LabelRole = Qt::UserRole + 1,
        IconNameRole,
        UrlRole,
        SectionRole,
        SectionNameRole, // string form, convenient for ListView.section
        IsMountedRole,
        IsEjectableRole,
        UsedBytesRole,
        TotalBytesRole,
        ItemCountRole,
        IdRole,
        IsBookmarkRole,
    };
    Q_ENUM(Roles)

    explicit Places(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addBookmark(const QString& url, const QString& label);
    Q_INVOKABLE void removeBookmark(const QString& id);
    Q_INVOKABLE void moveBookmark(int from, int to);
    Q_INVOKABLE void mount(const QString& id);
    Q_INVOKABLE void unmount(const QString& id);
    Q_INVOKABLE void eject(const QString& id);
    /// Resolves a place URL for opening (exists for override convenience).
    Q_INVOKABLE QString activate(const QString& url) const;

    Q_INVOKABLE bool udisksAvailable() const;

    /// Bookmarks file (default $XDG_CONFIG_HOME/fang/bookmarks.toml).
    void setBookmarksPath(const QString& path);
    void reload();

    static QString trashFilesPath();

signals:
    void error(const QString& title, const QString& detail);

private:
    struct Entry {
        QString label;
        QString iconName;
        QString url; // local path
        PlaceSection section = PlaceSection::Places;
        bool isMounted = true;
        bool isEjectable = false;
        qint64 usedBytes = -1;
        qint64 totalBytes = -1;
        int itemCount = -1;
        QString id;
        bool isBookmark = false;
    };

    void rebuild();
    void loadBookmarks();
    void saveBookmarks();
    static QList<Entry> xdgEntries();
    int trashItemCount() const;

    QList<Entry> m_entries;
    struct Bookmark {
        QString label;
        QString url;
        QString icon;
    };
    QList<Bookmark> m_bookmarks;
    QString m_bookmarksPath;
    MountMonitor* m_mounts = nullptr;
    QFileSystemWatcher m_trashWatcher;
};

} // namespace fang
