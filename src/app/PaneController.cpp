#include "PaneController.h"

#include "app/Application.h"
#include "app/WindowController.h"
#include "fs/DirectoryModel.h"
#include "config/Settings.h"
#include "fs/MimeResolver.h"
#include "nav/Navigation.h"
#include "nav/Selection.h"
#include "ops/FileOperationEngine.h"
#include "ops/Operation.h"
#include "util/Logging.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QUrl>

namespace fang {

PaneController::PaneController(WindowController* window, QObject* parent)
    : QObject(parent)
    , m_window(window)
{
    m_model = new DirectoryModel(this);
    m_navigation = new Navigation(this);
    m_selection = new Selection(m_model, this);
    m_model->setSelection(m_selection);
    m_fileOperations = new FileOperations(this);

    m_columnWidths.insert(QStringLiteral("name"), 320);
    m_columnWidths.insert(QStringLiteral("size"), 90);
    m_columnWidths.insert(QStringLiteral("type"), 160);
    m_columnWidths.insert(QStringLiteral("modified"), 160);

    // New panes start from the configured [view] defaults (docs/06).
    if (Application::instance() && Application::instance()->settings()) {
        const Settings* s = Application::instance()->settings();
        m_viewMode = viewModeFromString(s->defaultMode());
        m_iconSize = qBound(16, s->iconSize(), 256);
        if (!s->columns().isEmpty())
            m_columns = s->columns();
        const QVariantMap widths = s->columnWidths();
        for (auto it = widths.constBegin(); it != widths.constEnd(); ++it)
            m_columnWidths.insert(it.key(), it.value());
        m_model->setShowHidden(s->showHidden());
        m_model->setFoldersFirst(s->foldersFirst());
        m_model->setCaseSensitiveSort(s->caseSensitiveSort());
        m_model->setSort(sortRoleFromString(s->sortRole()), s->sortDescending());
    }

    connect(m_model, &DirectoryModel::loadingChanged, this, &PaneController::loadingChanged);
    connect(m_model, &DirectoryModel::countChanged, this, &PaneController::itemCountChanged);
    connect(m_model, &DirectoryModel::error, this, &PaneController::error);

    // All pane failures surface as app-level notifications.
    connect(this, &PaneController::error, this, [](const QString& title, const QString& detail) {
        if (Application::instance())
            Application::instance()->notify(title, detail);
    });
}

qint64 PaneController::freeBytes() const
{
    return QStorageInfo(m_location).bytesAvailable();
}

QString PaneController::normalizePath(const QString& pathOrUrl)
{
    QString path = pathOrUrl.trimmed();
    if (path.startsWith(QLatin1String("file://")))
        path = QUrl(path).toLocalFile();
    if (path == QLatin1String("~") || path.startsWith(QLatin1String("~/")))
        path = QDir::homePath() + path.mid(1);
    if (path.isEmpty())
        path = QDir::homePath();
    return QDir::cleanPath(QDir(path).absolutePath());
}

bool PaneController::navigateTo(const QString& pathOrUrl, bool recordHistory)
{
    const QString path = normalizePath(pathOrUrl);
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) {
        emit error(tr("Cannot open folder"), path);
        return false;
    }
    if (path == m_location)
        return true;

    const QString previous = m_location;
    m_location = path;
    m_model->setPath(path);
    if (recordHistory)
        m_navigation->recordVisit(previous);
    else
        m_navigation->historyReset();
    emit locationChanged();
    return true;
}

void PaneController::open(const QString& pathOrUrl)
{
    navigateTo(pathOrUrl, true);
}

void PaneController::openWithoutHistory(const QString& path)
{
    navigateTo(path, false);
}

void PaneController::activate(int row)
{
    const FileInfo* fi = m_model->infoAt(row);
    if (!fi)
        return;
    emit activated(row);
    if (fi->isDir) {
        open(fi->path);
    } else {
        if (!MimeResolver::instance()->openDefault({QUrl::fromLocalFile(fi->path)}))
            emit error(tr("Could not open file"), fi->path);
    }
}

void PaneController::activatePath(const QString& path)
{
    const QFileInfo fi(path);
    if (!fi.exists())
        return;
    if (fi.isDir()) {
        open(path);
    } else if (!MimeResolver::instance()->openDefault({QUrl::fromLocalFile(path)})) {
        emit error(tr("Could not open file"), path);
    }
}

void PaneController::openInNewTab(int row)
{
    const FileInfo* fi = m_model->infoAt(row);
    if (fi && fi->isDir && m_window)
        m_window->addTab(fi->path);
}

void PaneController::openInNewWindow(int row)
{
    const FileInfo* fi = m_model->infoAt(row);
    if (fi && fi->isDir && Application::instance())
        Application::instance()->openPath(fi->path);
}

void PaneController::refresh()
{
    m_model->refresh();
}

// --------------------------------------------------------------- options

void PaneController::setViewMode(ViewMode mode)
{
    if (m_viewMode == mode)
        return;
    m_viewMode = mode;
    emit viewModeChanged();
}

SortRole PaneController::sortRole() const
{
    return m_model->sortRole();
}

void PaneController::setSortRole(SortRole role)
{
    setSort(role, m_model->sortDescending());
}

bool PaneController::sortDescending() const
{
    return m_model->sortDescending();
}

void PaneController::setSortDescending(bool desc)
{
    setSort(m_model->sortRole(), desc);
}

void PaneController::setSort(SortRole role, bool desc)
{
    if (m_model->sortRole() == role && m_model->sortDescending() == desc)
        return;
    m_model->setSort(role, desc);
    emit sortChanged();
}

bool PaneController::foldersFirst() const
{
    return m_model->foldersFirst();
}

void PaneController::setFoldersFirst(bool ff)
{
    if (m_model->foldersFirst() == ff)
        return;
    m_model->setFoldersFirst(ff);
    emit sortChanged();
}

void PaneController::setColumns(const QStringList& columns)
{
    if (m_columns == columns)
        return;
    m_columns = columns;
    emit columnsChanged();
}

void PaneController::moveColumn(int from, int to)
{
    if (from < 0 || to < 0 || from >= m_columns.size() || to >= m_columns.size() || from == to)
        return;
    m_columns.move(from, to);
    emit columnsChanged();
}

void PaneController::setColumnWidth(const QString& key, int px)
{
    m_columnWidths.insert(key, qMax(40, px));
    emit columnsChanged();
}

bool PaneController::showHidden() const
{
    return m_model->showHidden();
}

void PaneController::setShowHidden(bool show)
{
    if (m_model->showHidden() == show)
        return;
    m_model->setShowHidden(show);
    emit showHiddenChanged();
}

QString PaneController::filterText() const
{
    return m_model->filterText();
}

void PaneController::setFilterText(const QString& text)
{
    if (m_model->filterText() == text)
        return;
    m_model->setFilterText(text);
    emit filterTextChanged();
}

void PaneController::setIconSize(int px)
{
    px = qBound(16, px, 256);
    if (m_iconSize == px)
        return;
    m_iconSize = px;
    emit iconSizeChanged();
}

void PaneController::setZoomLevel(int level)
{
    level = qBound(-3, level, 6);
    if (m_zoomLevel == level)
        return;
    m_zoomLevel = level;
    emit zoomLevelChanged();
}

bool PaneController::loading() const
{
    return m_model->isLoading();
}

int PaneController::itemCount() const
{
    return m_model->count();
}

qint64 PaneController::totalSize() const
{
    return m_model->totalSize();
}

// ------------------------------------------------------- mutating helpers

// Creation and rename go through the engine so they land on the undo stack;
// the engine notifies the user on failure.

void PaneController::createFolder(const QString& name)
{
    FileOperationEngine::instance()->createFolder(QUrl::fromLocalFile(m_location), name);
}

void PaneController::createFile(const QString& name)
{
    FileOperationEngine::instance()->createFile(QUrl::fromLocalFile(m_location), name);
}

void PaneController::renameAt(int row, const QString& newName)
{
    const FileInfo* fi = m_model->infoAt(row);
    if (!fi)
        return;
    FileOperationEngine::instance()->rename(QUrl::fromLocalFile(fi->path), newName);
}

// ------------------------------------------------------ string conversions

QString PaneController::viewModeToString(ViewMode mode)
{
    switch (mode) {
    case ViewMode::Icons: return QStringLiteral("icons");
    case ViewMode::List: return QStringLiteral("list");
    case ViewMode::Details: return QStringLiteral("details");
    case ViewMode::Tree: return QStringLiteral("tree");
    }
    return QStringLiteral("details");
}

ViewMode PaneController::viewModeFromString(const QString& s)
{
    if (s == QLatin1String("icons"))
        return ViewMode::Icons;
    if (s == QLatin1String("list"))
        return ViewMode::List;
    if (s == QLatin1String("tree"))
        return ViewMode::Tree;
    return ViewMode::Details;
}

QString PaneController::sortRoleToString(SortRole role)
{
    switch (role) {
    case SortRole::ByName: return QStringLiteral("name");
    case SortRole::BySize: return QStringLiteral("size");
    case SortRole::ByType: return QStringLiteral("type");
    case SortRole::ByModified: return QStringLiteral("modified");
    case SortRole::ByCreated: return QStringLiteral("created");
    case SortRole::ByAccessed: return QStringLiteral("accessed");
    case SortRole::ByExtension: return QStringLiteral("extension");
    case SortRole::ByOwner: return QStringLiteral("owner");
    }
    return QStringLiteral("name");
}

SortRole PaneController::sortRoleFromString(const QString& s)
{
    if (s == QLatin1String("size"))
        return SortRole::BySize;
    if (s == QLatin1String("type"))
        return SortRole::ByType;
    if (s == QLatin1String("modified"))
        return SortRole::ByModified;
    if (s == QLatin1String("created"))
        return SortRole::ByCreated;
    if (s == QLatin1String("accessed"))
        return SortRole::ByAccessed;
    if (s == QLatin1String("extension"))
        return SortRole::ByExtension;
    if (s == QLatin1String("owner"))
        return SortRole::ByOwner;
    return SortRole::ByName;
}

} // namespace fang
