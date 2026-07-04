#pragma once

#include "util/Enums.h"

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

Q_MOC_INCLUDE("fs/DirectoryModel.h")
Q_MOC_INCLUDE("nav/Navigation.h")
Q_MOC_INCLUDE("nav/Selection.h")
Q_MOC_INCLUDE("ops/FileOperationEngine.h")

namespace fang {

class DirectoryModel;
class FileOperations;
class Navigation;
class Selection;
class WindowController;

/// One navigable file view: location, model, selection, view options and
/// per-pane history (docs/03 §3, docs/04).
class PaneController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Panes are created by their TabController")

    Q_PROPERTY(QString location READ location NOTIFY locationChanged)
    Q_PROPERTY(fang::DirectoryModel* model READ model CONSTANT)
    Q_PROPERTY(fang::Navigation* navigation READ navigation CONSTANT)
    Q_PROPERTY(fang::Selection* selection READ selection CONSTANT)
    Q_PROPERTY(fang::FileOperations* fileOperations READ fileOperations CONSTANT)
    Q_PROPERTY(fang::ViewMode viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(fang::SortRole sortRole READ sortRole WRITE setSortRole NOTIFY sortChanged)
    Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending NOTIFY sortChanged)
    Q_PROPERTY(bool foldersFirst READ foldersFirst WRITE setFoldersFirst NOTIFY sortChanged)
    Q_PROPERTY(QStringList columns READ columns WRITE setColumns NOTIFY columnsChanged)
    /// column key -> width px (additive helper for the Details/Tree header).
    Q_PROPERTY(QVariantMap columnWidths READ columnWidths NOTIFY columnsChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(int iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)
    Q_PROPERTY(int zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY zoomLevelChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY itemCountChanged)
    Q_PROPERTY(qint64 totalSize READ totalSize NOTIFY itemCountChanged)
    /// Free bytes on the current location's volume (additive to API v1).
    Q_PROPERTY(qint64 freeBytes READ freeBytes NOTIFY locationChanged)

public:
    explicit PaneController(WindowController* window, QObject* parent = nullptr);

    QString location() const { return m_location; }
    DirectoryModel* model() const { return m_model; }
    Navigation* navigation() const { return m_navigation; }
    Selection* selection() const { return m_selection; }
    FileOperations* fileOperations() const { return m_fileOperations; }

    ViewMode viewMode() const { return m_viewMode; }
    void setViewMode(ViewMode mode);
    SortRole sortRole() const;
    void setSortRole(SortRole role);
    bool sortDescending() const;
    void setSortDescending(bool desc);
    bool foldersFirst() const;
    void setFoldersFirst(bool ff);
    QStringList columns() const { return m_columns; }
    QVariantMap columnWidths() const { return m_columnWidths; }
    bool showHidden() const;
    void setShowHidden(bool show);
    QString filterText() const;
    void setFilterText(const QString& text);
    int iconSize() const { return m_iconSize; }
    void setIconSize(int px);
    int zoomLevel() const { return m_zoomLevel; }
    void setZoomLevel(int level);
    bool loading() const;
    int itemCount() const;
    qint64 totalSize() const;
    qint64 freeBytes() const;

    Q_INVOKABLE void open(const QString& pathOrUrl);
    Q_INVOKABLE void activate(int row);
    /// Row-independent activation (tree nodes; additive to API v1).
    Q_INVOKABLE void activatePath(const QString& path);
    Q_INVOKABLE void openInNewTab(int row);
    Q_INVOKABLE void openInNewWindow(int row);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setColumns(const QStringList& columns);
    Q_INVOKABLE void moveColumn(int from, int to);
    Q_INVOKABLE void setColumnWidth(const QString& key, int px);
    Q_INVOKABLE void setSort(fang::SortRole role, bool desc);
    Q_INVOKABLE void createFolder(const QString& name);
    Q_INVOKABLE void createFile(const QString& name);
    Q_INVOKABLE void renameAt(int row, const QString& newName);

    /// Navigation without pushing the current location to history
    /// (back/forward use this).
    void openWithoutHistory(const QString& path);

    WindowController* window() const { return m_window; }

signals:
    void locationChanged();
    void activated(int row);
    void error(const QString& title, const QString& detail);
    void viewModeChanged();
    void sortChanged();
    void columnsChanged();
    void showHiddenChanged();
    void filterTextChanged();
    void iconSizeChanged();
    void zoomLevelChanged();
    void loadingChanged();
    void itemCountChanged();

private:
    static QString normalizePath(const QString& pathOrUrl);
    bool navigateTo(const QString& pathOrUrl, bool recordHistory);

    WindowController* m_window;
    DirectoryModel* m_model;
    Navigation* m_navigation;
    Selection* m_selection;
    FileOperations* m_fileOperations;

    QString m_location;
    ViewMode m_viewMode = ViewMode::Details;
    QStringList m_columns{QStringLiteral("name"), QStringLiteral("size"), QStringLiteral("type"),
                          QStringLiteral("modified")};
    QVariantMap m_columnWidths;
    int m_iconSize = 48;
    int m_zoomLevel = 0;
};

} // namespace fang
