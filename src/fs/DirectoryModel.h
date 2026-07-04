#pragma once

#include "fs/FileInfo.h"
#include "util/Enums.h"

#include <QAbstractItemModel>
#include <QCollator>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <vector>

namespace fang {

class FileSystemWatcher;
class Selection;

/// Backs every file view. Flat for Icons/List/Details (top-level rows);
/// hierarchical with lazy fetchMore for Tree. Populated asynchronously in
/// batches by a scan worker; kept live by FileSystemWatcher (docs/03 §4).
class DirectoryModel : public QAbstractItemModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("DirectoryModel is owned by PaneController")

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
    };
    Q_ENUM(Roles)

    explicit DirectoryModel(QObject* parent = nullptr);
    ~DirectoryModel() override;

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool hasChildren(const QModelIndex& parent = {}) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    // Population / navigation
    void setPath(const QString& absPath);
    QString path() const { return m_path; }
    void refresh();
    bool isLoading() const { return m_loading; }

    // Display options
    void setShowHidden(bool show);
    bool showHidden() const { return m_showHidden; }
    void setFilterText(const QString& text);
    QString filterText() const { return m_filterText; }
    void setSort(SortRole role, bool descending);
    void setFoldersFirst(bool foldersFirst);
    void setCaseSensitiveSort(bool cs);
    SortRole sortRole() const { return m_sortRole; }
    bool sortDescending() const { return m_sortDescending; }
    bool foldersFirst() const { return m_foldersFirst; }

    void setSelection(Selection* selection);

    // Top-level helpers (row = visible top-level row)
    int count() const;
    qint64 totalSize() const;
    const FileInfo* infoAt(int row) const;
    QString pathAt(int row) const;
    int rowOf(const QString& path) const;
    /// dataChanged for SelectedRole across everything (called by Selection).
    void notifySelectionChanged();

    /// Direct filesystem role lookup for any node index (used by tree views).
    const FileInfo* infoForIndex(const QModelIndex& index) const;

signals:
    void loadingChanged(bool loading);
    void countChanged();
    void error(const QString& title, const QString& detail);
    void pathChanged(const QString& path);

private:
    struct Node {
        FileInfo info;
        Node* parent = nullptr;
        std::vector<std::unique_ptr<Node>> all; // owned, unfiltered
        std::vector<Node*> visible;             // filtered + sorted view of `all`
        bool loaded = false;
        bool loading = false;
    };

    Node* nodeFor(const QModelIndex& index) const;
    QModelIndex indexFor(Node* node) const;
    Node* findNodeByPath(const QString& absPath) const;
    int visibleRowOf(const Node* node) const;

    bool passesFilter(const FileInfo& info, bool topLevel) const;
    bool lessThan(const FileInfo& a, const FileInfo& b) const;
    void rebuildAllVisible();
    void sortVisible(std::vector<Node*>& vec) const;

    void startScan(Node* node, const QString& absPath);
    void onScanBatch(quint64 generation, const QString& dirPath,
                     const std::vector<FileInfo>& batch);
    void onScanDone(quint64 generation, const QString& dirPath, bool ok, const QString& err);

    void onWatcherAdded(const QString& dir, const QStringList& paths);
    void onWatcherRemoved(const QString& dir, const QStringList& paths);
    void onWatcherChanged(const QString& dir, const QStringList& paths);
    void insertEntrySorted(Node* parent, std::unique_ptr<Node> child);
    void removeEntry(Node* parent, const QString& name);

    void setLoading(bool loading);
    void updateCollator();

    std::unique_ptr<Node> m_root;
    QString m_path;
    quint64 m_generation = 0;
    bool m_loading = false;

    bool m_showHidden = false;
    QString m_filterText;
    SortRole m_sortRole = SortRole::ByName;
    bool m_sortDescending = false;
    bool m_foldersFirst = true;
    bool m_caseSensitiveSort = false;

    QCollator m_collator;
    FileSystemWatcher* m_watcher = nullptr;
    Selection* m_selection = nullptr;
};

} // namespace fang
