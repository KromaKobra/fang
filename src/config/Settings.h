#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace fang {

class ConfigStore;

/// Typed, persistent view of config.toml (docs/06). Every property reads
/// through the ConfigStore (defaults documented there) and writes back
/// atomically; one notify signal per group plus changed().
class Settings : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Settings is owned by Application")

    // [general]
    Q_PROPERTY(QString defaultLocation READ defaultLocation WRITE setDefaultLocation NOTIFY generalChanged)
    Q_PROPERTY(bool restoreSession READ restoreSession WRITE setRestoreSession NOTIFY generalChanged)
    Q_PROPERTY(bool confirmDelete READ confirmDelete WRITE setConfirmDelete NOTIFY generalChanged)
    Q_PROPERTY(bool confirmTrash READ confirmTrash WRITE setConfirmTrash NOTIFY generalChanged)
    Q_PROPERTY(bool singleInstance READ singleInstance CONSTANT) // FIXED false in v1
    Q_PROPERTY(QString terminalCommand READ terminalCommand WRITE setTerminalCommand NOTIFY generalChanged)

    // [view]
    Q_PROPERTY(QString defaultMode READ defaultMode WRITE setDefaultMode NOTIFY viewChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY viewChanged)
    Q_PROPERTY(bool foldersFirst READ foldersFirst WRITE setFoldersFirst NOTIFY viewChanged)
    Q_PROPERTY(QString sortRole READ sortRole WRITE setSortRole NOTIFY viewChanged)
    Q_PROPERTY(bool sortDescending READ sortDescending WRITE setSortDescending NOTIFY viewChanged)
    Q_PROPERTY(bool caseSensitiveSort READ caseSensitiveSort WRITE setCaseSensitiveSort NOTIFY viewChanged)
    Q_PROPERTY(int iconSize READ iconSize WRITE setIconSize NOTIFY viewChanged)
    Q_PROPERTY(QStringList columns READ columns WRITE setColumns NOTIFY viewChanged)
    Q_PROPERTY(QVariantMap columnWidths READ columnWidths WRITE setColumnWidths NOTIFY viewChanged)
    Q_PROPERTY(bool previewPane READ previewPane WRITE setPreviewPane NOTIFY viewChanged)
    Q_PROPERTY(int previewPaneWidth READ previewPaneWidth WRITE setPreviewPaneWidth NOTIFY viewChanged)

    // [behavior]
    Q_PROPERTY(QString openWith READ openWith WRITE setOpenWith NOTIFY behaviorChanged)
    Q_PROPERTY(QString selectWith READ selectWith WRITE setSelectWith NOTIFY behaviorChanged)
    Q_PROPERTY(QString typeAhead READ typeAhead WRITE setTypeAhead NOTIFY behaviorChanged)
    Q_PROPERTY(QString dragDefault READ dragDefault WRITE setDragDefault NOTIFY behaviorChanged)
    Q_PROPERTY(QString newTabPosition READ newTabPosition WRITE setNewTabPosition NOTIFY behaviorChanged)
    Q_PROPERTY(bool closeConfirmMultipleTabs READ closeConfirmMultipleTabs WRITE setCloseConfirmMultipleTabs NOTIFY behaviorChanged)

    // [previews]
    Q_PROPERTY(bool previewsEnabled READ previewsEnabled WRITE setPreviewsEnabled NOTIFY previewsChanged)
    Q_PROPERTY(bool previewsImages READ previewsImages WRITE setPreviewsImages NOTIFY previewsChanged)
    Q_PROPERTY(int maxFileSizeMb READ maxFileSizeMb WRITE setMaxFileSizeMb NOTIFY previewsChanged)
    Q_PROPERTY(int textPeekKb READ textPeekKb WRITE setTextPeekKb NOTIFY previewsChanged)
    Q_PROPERTY(bool useSharedCache READ useSharedCache WRITE setUseSharedCache NOTIFY previewsChanged)

    // [search]
    Q_PROPERTY(bool contentSearchDefault READ contentSearchDefault WRITE setContentSearchDefault NOTIFY searchChanged)
    Q_PROPERTY(bool searchFollowSymlinks READ searchFollowSymlinks WRITE setSearchFollowSymlinks NOTIFY searchChanged)
    Q_PROPERTY(int searchMaxDepth READ searchMaxDepth WRITE setSearchMaxDepth NOTIFY searchChanged)
    Q_PROPERTY(bool searchIgnoreHidden READ searchIgnoreHidden WRITE setSearchIgnoreHidden NOTIFY searchChanged)
    Q_PROPERTY(QStringList searchIgnorePaths READ searchIgnorePaths WRITE setSearchIgnorePaths NOTIFY searchChanged)

    // [appearance]
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY appearanceChanged)
    Q_PROPERTY(bool followSystem READ followSystem WRITE setFollowSystem NOTIFY appearanceChanged)
    Q_PROPERTY(QString lightTheme READ lightTheme WRITE setLightTheme NOTIFY appearanceChanged)
    Q_PROPERTY(QString darkTheme READ darkTheme WRITE setDarkTheme NOTIFY appearanceChanged)
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme NOTIFY appearanceChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY appearanceChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY appearanceChanged)

public:
    explicit Settings(ConfigStore* store, QObject* parent = nullptr);

    // [general]
    QString defaultLocation() const;
    void setDefaultLocation(const QString& v);
    bool restoreSession() const;
    void setRestoreSession(bool v);
    bool confirmDelete() const;
    void setConfirmDelete(bool v);
    bool confirmTrash() const;
    void setConfirmTrash(bool v);
    bool singleInstance() const { return false; }
    QString terminalCommand() const;
    void setTerminalCommand(const QString& v);

    // [view]
    QString defaultMode() const;
    void setDefaultMode(const QString& v);
    bool showHidden() const;
    void setShowHidden(bool v);
    bool foldersFirst() const;
    void setFoldersFirst(bool v);
    QString sortRole() const;
    void setSortRole(const QString& v);
    bool sortDescending() const;
    void setSortDescending(bool v);
    bool caseSensitiveSort() const;
    void setCaseSensitiveSort(bool v);
    int iconSize() const;
    void setIconSize(int v);
    QStringList columns() const;
    void setColumns(const QStringList& v);
    QVariantMap columnWidths() const;
    void setColumnWidths(const QVariantMap& v);
    bool previewPane() const;
    void setPreviewPane(bool v);
    int previewPaneWidth() const;
    void setPreviewPaneWidth(int v);

    // [behavior]
    QString openWith() const;
    void setOpenWith(const QString& v);
    QString selectWith() const;
    void setSelectWith(const QString& v);
    QString typeAhead() const;
    void setTypeAhead(const QString& v);
    QString dragDefault() const;
    void setDragDefault(const QString& v);
    QString newTabPosition() const;
    void setNewTabPosition(const QString& v);
    bool closeConfirmMultipleTabs() const;
    void setCloseConfirmMultipleTabs(bool v);

    // [previews]
    bool previewsEnabled() const;
    void setPreviewsEnabled(bool v);
    bool previewsImages() const;
    void setPreviewsImages(bool v);
    int maxFileSizeMb() const;
    void setMaxFileSizeMb(int v);
    int textPeekKb() const;
    void setTextPeekKb(int v);
    bool useSharedCache() const;
    void setUseSharedCache(bool v);

    // [search]
    bool contentSearchDefault() const;
    void setContentSearchDefault(bool v);
    bool searchFollowSymlinks() const;
    void setSearchFollowSymlinks(bool v);
    int searchMaxDepth() const;
    void setSearchMaxDepth(int v);
    bool searchIgnoreHidden() const;
    void setSearchIgnoreHidden(bool v);
    QStringList searchIgnorePaths() const;
    void setSearchIgnorePaths(const QStringList& v);

    // [appearance]
    QString theme() const;
    void setTheme(const QString& v);
    bool followSystem() const;
    void setFollowSystem(bool v);
    QString lightTheme() const;
    void setLightTheme(const QString& v);
    QString darkTheme() const;
    void setDarkTheme(const QString& v);
    QString iconTheme() const;
    void setIconTheme(const QString& v);
    QString fontFamily() const;
    void setFontFamily(const QString& v);
    int fontSize() const;
    void setFontSize(int v);

    /// [shortcuts] override for `name`, else `fallback` (docs/05 table).
    Q_INVOKABLE QString shortcut(const QString& name, const QString& fallback) const;

    /// Re-emit every group signal (config file reloaded from disk).
    void notifyAllChanged();

signals:
    void generalChanged();
    void viewChanged();
    void behaviorChanged();
    void previewsChanged();
    void searchChanged();
    void appearanceChanged();
    void shortcutsChanged();
    void changed();

private:
    ConfigStore* m_store;
};

} // namespace fang
