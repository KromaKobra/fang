#include "DirectoryModel.h"

#include "fs/FileSystemWatcher.h"
#include "nav/Selection.h"
#include "util/HumanFormat.h"
#include "util/Logging.h"

#include <QDir>
#include <QDirIterator>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

namespace fang {

namespace {
constexpr int kBatchSize = 400;
} // namespace

DirectoryModel::DirectoryModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_root(std::make_unique<Node>())
{
    updateCollator();
    m_watcher = new FileSystemWatcher(this);
    connect(m_watcher, &FileSystemWatcher::added, this, &DirectoryModel::onWatcherAdded);
    connect(m_watcher, &FileSystemWatcher::removed, this, &DirectoryModel::onWatcherRemoved);
    connect(m_watcher, &FileSystemWatcher::changed, this, &DirectoryModel::onWatcherChanged);
    connect(m_watcher, &FileSystemWatcher::directoryGone, this, [this](const QString& dir) {
        if (dir == m_path)
            emit error(tr("Folder no longer exists"), dir);
    });
}

DirectoryModel::~DirectoryModel()
{
    ++m_generation; // orphan any in-flight scan deliveries
}

void DirectoryModel::updateCollator()
{
    m_collator.setNumericMode(true);
    m_collator.setCaseSensitivity(m_caseSensitiveSort ? Qt::CaseSensitive : Qt::CaseInsensitive);
}

// ---------------------------------------------------------------- model API

DirectoryModel::Node* DirectoryModel::nodeFor(const QModelIndex& index) const
{
    if (!index.isValid())
        return m_root.get();
    return static_cast<Node*>(index.internalPointer());
}

int DirectoryModel::visibleRowOf(const Node* node) const
{
    if (!node || !node->parent)
        return -1;
    const auto& vec = node->parent->visible;
    const auto it = std::find(vec.begin(), vec.end(), node);
    return it == vec.end() ? -1 : int(it - vec.begin());
}

QModelIndex DirectoryModel::index(int row, int column, const QModelIndex& parent) const
{
    const Node* p = nodeFor(parent);
    if (!p || row < 0 || column != 0 || size_t(row) >= p->visible.size())
        return {};
    return createIndex(row, column, p->visible[size_t(row)]);
}

QModelIndex DirectoryModel::parent(const QModelIndex& child) const
{
    const Node* node = nodeFor(child);
    if (!node || !node->parent || node->parent == m_root.get())
        return {};
    const int row = visibleRowOf(node->parent);
    if (row < 0)
        return {};
    return createIndex(row, 0, node->parent);
}

QModelIndex DirectoryModel::indexFor(Node* node) const
{
    if (!node || node == m_root.get())
        return {};
    const int row = visibleRowOf(node);
    if (row < 0)
        return {};
    return createIndex(row, 0, node);
}

int DirectoryModel::rowCount(const QModelIndex& parent) const
{
    const Node* p = nodeFor(parent);
    return p ? int(p->visible.size()) : 0;
}

int DirectoryModel::columnCount(const QModelIndex&) const
{
    return 1; // columns are rendered from roles, not model columns
}

bool DirectoryModel::hasChildren(const QModelIndex& parent) const
{
    const Node* p = nodeFor(parent);
    if (!p)
        return false;
    if (p == m_root.get() || !p->loaded)
        return p->info.isDir || p == m_root.get();
    return !p->visible.empty();
}

bool DirectoryModel::canFetchMore(const QModelIndex& parent) const
{
    const Node* p = nodeFor(parent);
    return p && p != m_root.get() && p->info.isDir && !p->loaded && !p->loading;
}

void DirectoryModel::fetchMore(const QModelIndex& parent)
{
    Node* p = nodeFor(parent);
    if (!p || p == m_root.get() || !p->info.isDir || p->loaded || p->loading)
        return;
    m_watcher->watchDirectory(p->info.path);
    startScan(p, p->info.path);
}

QHash<int, QByteArray> DirectoryModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {PathRole, "path"},
        {UrlRole, "url"},
        {IsDirRole, "isDir"},
        {IsSymlinkRole, "isSymlink"},
        {SizeRole, "size"},
        {SizeTextRole, "sizeText"},
        {ModifiedRole, "modified"},
        {CreatedRole, "created"},
        {AccessedRole, "accessed"},
        {MimeRole, "mimeType"},
        {MimeCommentRole, "typeText"},
        {PermissionsRole, "permissions"},
        {OwnerRole, "owner"},
        {GroupRole, "group"},
        {IconNameRole, "iconName"},
        {ThumbnailRole, "thumbnail"},
        {IsHiddenRole, "isHidden"},
        {SelectedRole, "selected"},
        {ExtensionRole, "extension"},
    };
}

QVariant DirectoryModel::data(const QModelIndex& index, int role) const
{
    const Node* node = nodeFor(index);
    if (!node || node == m_root.get())
        return {};
    const FileInfo& fi = node->info;
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return fi.name;
    case PathRole:
        return fi.path;
    case UrlRole:
        return fi.url();
    case IsDirRole:
        return fi.isDir;
    case IsSymlinkRole:
        return fi.isSymlink;
    case SizeRole:
        return fi.size;
    case SizeTextRole:
        return fi.size < 0 ? QString() : HumanFormat::size(fi.size);
    case ModifiedRole:
        return fi.modified;
    case CreatedRole:
        return fi.created;
    case AccessedRole:
        return fi.accessed;
    case MimeRole:
        return fi.mimeType;
    case MimeCommentRole:
        return fi.mimeComment;
    case PermissionsRole:
        return fi.permissions;
    case OwnerRole:
        return fi.owner;
    case GroupRole:
        return fi.group;
    case IconNameRole:
        return fi.iconName;
    case ThumbnailRole:
        return fi.isImage() ? QStringLiteral("image://fang-thumb") + fi.path : QString();
    case IsHiddenRole:
        return fi.isHidden;
    case SelectedRole:
        return m_selection && m_selection->isPathSelected(fi.path);
    case ExtensionRole:
        return fi.extension;
    default:
        return {};
    }
}

// ------------------------------------------------------------ population

void DirectoryModel::setPath(const QString& absPath)
{
    const QString cleaned = QDir::cleanPath(absPath);
    ++m_generation;
    m_watcher->clear();

    beginResetModel();
    m_root = std::make_unique<Node>();
    m_root->info = FileInfo::fromPath(cleaned, false);
    endResetModel();
    emit countChanged();

    m_path = cleaned;
    emit pathChanged(m_path);

    m_watcher->watchDirectory(cleaned);
    startScan(m_root.get(), cleaned);
}

void DirectoryModel::refresh()
{
    if (!m_path.isEmpty())
        setPath(m_path);
}

void DirectoryModel::setLoading(bool loading)
{
    if (m_loading == loading)
        return;
    m_loading = loading;
    emit loadingChanged(loading);
}

void DirectoryModel::startScan(Node* node, const QString& absPath)
{
    const quint64 gen = m_generation;
    const bool isRoot = node == m_root.get();
    node->loading = true;
    if (isRoot)
        setLoading(true);

    auto deliverBatch = [this, gen, absPath](std::vector<FileInfo> batch) {
        QMetaObject::invokeMethod(
            this,
            [this, gen, absPath, batch = std::move(batch)] { onScanBatch(gen, absPath, batch); },
            Qt::QueuedConnection);
    };
    auto deliverDone = [this, gen, absPath](bool ok, const QString& err) {
        QMetaObject::invokeMethod(
            this, [this, gen, absPath, ok, err] { onScanDone(gen, absPath, ok, err); },
            Qt::QueuedConnection);
    };

    // `stream` — the root listing streams into the view in batches; lazy tree
    // children arrive as one sorted insert so expansion state is preserved.
    const bool stream = isRoot;
    auto future = QtConcurrent::run([absPath, stream, deliverBatch, deliverDone] {
        const QDir dir(absPath);
        if (!dir.exists()) {
            deliverDone(false, QStringLiteral("Folder does not exist"));
            return;
        }
        if (!dir.isReadable()) {
            deliverDone(false, QStringLiteral("Permission denied"));
            return;
        }
        std::vector<FileInfo> batch;
        QDirIterator it(absPath,
                        QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            batch.push_back(FileInfo::fromPath(it.next()));
            if (stream && int(batch.size()) >= kBatchSize)
                deliverBatch(std::exchange(batch, {}));
        }
        if (!batch.empty())
            deliverBatch(std::move(batch));
        deliverDone(true, QString());
    });
    Q_UNUSED(future)
}

void DirectoryModel::onScanBatch(quint64 generation, const QString& dirPath,
                                 const std::vector<FileInfo>& batch)
{
    if (generation != m_generation)
        return;
    Node* node = findNodeByPath(dirPath);
    if (!node)
        return;

    const bool topLevel = node == m_root.get();
    std::vector<Node*> fresh;
    for (const FileInfo& fi : batch) {
        auto child = std::make_unique<Node>();
        child->info = fi;
        child->parent = node;
        if (passesFilter(fi, topLevel))
            fresh.push_back(child.get());
        node->all.push_back(std::move(child));
    }
    if (fresh.empty())
        return;

    if (node->visible.empty()) {
        sortVisible(fresh);
        const QModelIndex parentIdx = indexFor(node);
        beginInsertRows(parentIdx, 0, int(fresh.size()) - 1);
        node->visible = std::move(fresh);
        endInsertRows();
    } else {
        // Streaming append; the final onScanDone pass sorts everything.
        const QModelIndex parentIdx = indexFor(node);
        const int first = int(node->visible.size());
        beginInsertRows(parentIdx, first, first + int(fresh.size()) - 1);
        node->visible.insert(node->visible.end(), fresh.begin(), fresh.end());
        endInsertRows();
    }
    if (topLevel)
        emit countChanged();
}

void DirectoryModel::onScanDone(quint64 generation, const QString& dirPath, bool ok,
                                const QString& err)
{
    if (generation != m_generation)
        return;
    Node* node = findNodeByPath(dirPath);
    if (!node)
        return;
    node->loading = false;
    node->loaded = true;

    if (!ok) {
        emit error(tr("Could not read folder"), dirPath + QStringLiteral(": ") + err);
    } else if (node == m_root.get() && node->visible.size() > 1) {
        // Final ordering pass over the streamed listing.
        beginResetModel();
        sortVisible(node->visible);
        endResetModel();
    }
    if (node == m_root.get()) {
        setLoading(false);
        emit countChanged();
    }
}

// ------------------------------------------------------- filtering/sorting

bool DirectoryModel::passesFilter(const FileInfo& info, bool topLevel) const
{
    if (info.isHidden && !m_showHidden)
        return false;
    if (topLevel && !m_filterText.isEmpty()
        && !info.name.contains(m_filterText, Qt::CaseInsensitive))
        return false;
    return true;
}

bool DirectoryModel::lessThan(const FileInfo& a, const FileInfo& b) const
{
    if (m_foldersFirst && a.isDir != b.isDir)
        return a.isDir;

    int cmp = 0;
    switch (m_sortRole) {
    case SortRole::ByName:
        cmp = m_collator.compare(a.name, b.name);
        break;
    case SortRole::BySize:
        cmp = a.size < b.size ? -1 : (a.size > b.size ? 1 : 0);
        break;
    case SortRole::ByType:
        cmp = QString::compare(a.mimeComment, b.mimeComment, Qt::CaseInsensitive);
        break;
    case SortRole::ByModified:
        cmp = a.modified < b.modified ? -1 : (a.modified > b.modified ? 1 : 0);
        break;
    case SortRole::ByCreated:
        cmp = a.created < b.created ? -1 : (a.created > b.created ? 1 : 0);
        break;
    case SortRole::ByAccessed:
        cmp = a.accessed < b.accessed ? -1 : (a.accessed > b.accessed ? 1 : 0);
        break;
    case SortRole::ByExtension:
        cmp = QString::compare(a.extension, b.extension, Qt::CaseInsensitive);
        break;
    case SortRole::ByOwner:
        cmp = QString::compare(a.owner, b.owner, Qt::CaseInsensitive);
        break;
    }
    if (m_sortDescending)
        cmp = -cmp;
    if (cmp != 0)
        return cmp < 0;
    return m_collator.compare(a.name, b.name) < 0; // stable tie-break
}

void DirectoryModel::sortVisible(std::vector<Node*>& vec) const
{
    std::stable_sort(vec.begin(), vec.end(),
                     [this](const Node* a, const Node* b) { return lessThan(a->info, b->info); });
}

void DirectoryModel::rebuildAllVisible()
{
    beginResetModel();
    const std::function<void(Node*)> rebuild = [&](Node* node) {
        node->visible.clear();
        const bool topLevel = node == m_root.get();
        for (const auto& child : node->all) {
            if (passesFilter(child->info, topLevel))
                node->visible.push_back(child.get());
            if (child->loaded)
                rebuild(child.get());
        }
        sortVisible(node->visible);
    };
    rebuild(m_root.get());
    endResetModel();
    emit countChanged();
}

void DirectoryModel::setShowHidden(bool show)
{
    if (m_showHidden == show)
        return;
    m_showHidden = show;
    rebuildAllVisible();
}

void DirectoryModel::setFilterText(const QString& text)
{
    if (m_filterText == text)
        return;
    m_filterText = text;
    rebuildAllVisible();
}

void DirectoryModel::setSort(SortRole role, bool descending)
{
    if (m_sortRole == role && m_sortDescending == descending)
        return;
    m_sortRole = role;
    m_sortDescending = descending;
    rebuildAllVisible();
}

void DirectoryModel::setFoldersFirst(bool foldersFirst)
{
    if (m_foldersFirst == foldersFirst)
        return;
    m_foldersFirst = foldersFirst;
    rebuildAllVisible();
}

void DirectoryModel::setCaseSensitiveSort(bool cs)
{
    if (m_caseSensitiveSort == cs)
        return;
    m_caseSensitiveSort = cs;
    updateCollator();
    rebuildAllVisible();
}

void DirectoryModel::setSelection(Selection* selection)
{
    m_selection = selection;
}

// ------------------------------------------------------------- accessors

int DirectoryModel::count() const
{
    return int(m_root->visible.size());
}

qint64 DirectoryModel::totalSize() const
{
    qint64 total = 0;
    for (const Node* n : m_root->visible)
        if (n->info.size > 0)
            total += n->info.size;
    return total;
}

const FileInfo* DirectoryModel::infoAt(int row) const
{
    if (row < 0 || size_t(row) >= m_root->visible.size())
        return nullptr;
    return &m_root->visible[size_t(row)]->info;
}

QString DirectoryModel::pathAt(int row) const
{
    const FileInfo* fi = infoAt(row);
    return fi ? fi->path : QString();
}

bool DirectoryModel::isDirAt(int row) const
{
    const FileInfo* fi = infoAt(row);
    return fi && fi->isDir;
}

int DirectoryModel::rowOf(const QString& path) const
{
    for (size_t i = 0; i < m_root->visible.size(); ++i)
        if (m_root->visible[i]->info.path == path)
            return int(i);
    return -1;
}

const FileInfo* DirectoryModel::infoForIndex(const QModelIndex& index) const
{
    const Node* node = nodeFor(index);
    return (node && node != m_root.get()) ? &node->info : nullptr;
}

void DirectoryModel::notifySelectionChanged()
{
    const std::function<void(Node*)> notify = [&](Node* node) {
        if (!node->visible.empty()) {
            emit dataChanged(createIndex(0, 0, node->visible.front()),
                             createIndex(int(node->visible.size()) - 1, 0, node->visible.back()),
                             {SelectedRole});
        }
        for (const auto& child : node->all)
            if (child->loaded)
                notify(child.get());
    };
    notify(m_root.get());
}

// --------------------------------------------------------------- watching

DirectoryModel::Node* DirectoryModel::findNodeByPath(const QString& absPath) const
{
    const std::function<Node*(Node*)> walk = [&](Node* node) -> Node* {
        if (node->info.path == absPath)
            return node;
        const QString prefix = node->info.path.endsWith(QLatin1Char('/'))
            ? node->info.path
            : node->info.path + QLatin1Char('/');
        if (!absPath.startsWith(prefix))
            return nullptr;
        for (const auto& child : node->all) {
            if (Node* hit = walk(child.get()))
                return hit;
        }
        return nullptr;
    };
    return walk(m_root.get());
}

void DirectoryModel::insertEntrySorted(Node* parent, std::unique_ptr<Node> child)
{
    Node* raw = child.get();
    parent->all.push_back(std::move(child));
    if (!passesFilter(raw->info, parent == m_root.get()))
        return;
    auto& vec = parent->visible;
    const auto pos = std::lower_bound(
        vec.begin(), vec.end(), raw,
        [this](const Node* a, const Node* b) { return lessThan(a->info, b->info); });
    const int row = int(pos - vec.begin());
    beginInsertRows(indexFor(parent), row, row);
    vec.insert(pos, raw);
    endInsertRows();
    if (parent == m_root.get())
        emit countChanged();
}

void DirectoryModel::removeEntry(Node* parent, const QString& name)
{
    const auto allIt = std::find_if(parent->all.begin(), parent->all.end(),
                                    [&](const auto& n) { return n->info.name == name; });
    if (allIt == parent->all.end())
        return;
    Node* node = allIt->get();
    const int row = visibleRowOf(node);
    if (row >= 0) {
        beginRemoveRows(indexFor(parent), row, row);
        parent->visible.erase(parent->visible.begin() + row);
        parent->all.erase(allIt);
        endRemoveRows();
        if (parent == m_root.get())
            emit countChanged();
    } else {
        parent->all.erase(allIt);
    }
}

void DirectoryModel::onWatcherAdded(const QString& dir, const QStringList& paths)
{
    Node* node = findNodeByPath(dir);
    if (!node || (!node->loaded && !node->loading))
        return;
    for (const QString& p : paths) {
        if (std::any_of(node->all.begin(), node->all.end(),
                        [&](const auto& n) { return n->info.path == p; }))
            continue;
        auto child = std::make_unique<Node>();
        child->info = FileInfo::fromPath(p);
        child->parent = node;
        insertEntrySorted(node, std::move(child));
    }
}

void DirectoryModel::onWatcherRemoved(const QString& dir, const QStringList& paths)
{
    Node* node = findNodeByPath(dir);
    if (!node)
        return;
    for (const QString& p : paths)
        removeEntry(node, p.section(QLatin1Char('/'), -1));
}

void DirectoryModel::onWatcherChanged(const QString& dir, const QStringList& paths)
{
    Node* node = findNodeByPath(dir);
    if (!node)
        return;
    for (const QString& p : paths) {
        const auto it = std::find_if(node->all.begin(), node->all.end(),
                                     [&](const auto& n) { return n->info.path == p; });
        if (it == node->all.end())
            continue;
        (*it)->info = FileInfo::fromPath(p);
        const QModelIndex idx = indexFor(it->get());
        if (idx.isValid())
            emit dataChanged(idx, idx);
    }
}

} // namespace fang
