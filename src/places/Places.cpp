#include "Places.h"

#include "places/MountMonitor.h"
#include "util/Logging.h"

#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

#include <toml++/toml.hpp>

namespace fang {

namespace {

QString userDirsPath(const QString& key)
{
    // Parse ~/.config/user-dirs.dirs for XDG_<KEY>_DIR entries that
    // QStandardPaths does not cover (PUBLICSHARE, TEMPLATES).
    QFile f(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
            + QStringLiteral("/user-dirs.dirs"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    const QString needle = QStringLiteral("XDG_%1_DIR=").arg(key);
    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (!line.startsWith(needle))
            continue;
        QString value = line.mid(needle.size());
        value.remove(QLatin1Char('"'));
        value.replace(QLatin1String("$HOME"), QDir::homePath());
        return value;
    }
    return {};
}

} // namespace

Places::Places(QObject* parent)
    : QAbstractListModel(parent)
{
    m_bookmarksPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/fang/bookmarks.toml");
    m_mounts = new MountMonitor(this);
    connect(m_mounts, &MountMonitor::mountsChanged, this, [this] { rebuild(); });
    connect(m_mounts, &MountMonitor::error, this, &Places::error);

    const QString trashFiles = trashFilesPath();
    if (QDir().mkpath(trashFiles))
        m_trashWatcher.addPath(trashFiles);
    connect(&m_trashWatcher, &QFileSystemWatcher::directoryChanged, this, [this] { rebuild(); });

    loadBookmarks();
    rebuild();
}

void Places::setBookmarksPath(const QString& path)
{
    m_bookmarksPath = path;
    reload();
}

void Places::reload()
{
    loadBookmarks();
    rebuild();
}

QString Places::trashFilesPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/Trash/files");
}

int Places::trashItemCount() const
{
    return int(QDir(trashFilesPath())
                   .entryList(QDir::AllEntries | QDir::Hidden | QDir::System
                              | QDir::NoDotAndDotDot)
                   .size());
}

QList<Places::Entry> Places::xdgEntries()
{
    QList<Entry> out;
    auto add = [&out](const QString& path, const QString& label, const QString& icon) {
        if (path.isEmpty() || !QDir(path).exists())
            return;
        for (const Entry& e : std::as_const(out))
            if (e.url == path)
                return;
        Entry entry;
        entry.label = label;
        entry.iconName = icon;
        entry.url = path;
        entry.section = PlaceSection::Places;
        entry.id = QStringLiteral("place:") + path;
        out << entry;
    };

    add(QDir::homePath(), QStringLiteral("Home"), QStringLiteral("user-home"));
    add(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        QStringLiteral("Desktop"), QStringLiteral("user-desktop"));
    add(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        QStringLiteral("Documents"), QStringLiteral("folder-documents"));
    add(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
        QStringLiteral("Downloads"), QStringLiteral("folder-download"));
    add(QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
        QStringLiteral("Music"), QStringLiteral("folder-music"));
    add(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        QStringLiteral("Pictures"), QStringLiteral("folder-pictures"));
    add(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        QStringLiteral("Videos"), QStringLiteral("folder-videos"));
    add(userDirsPath(QStringLiteral("PUBLICSHARE")), QStringLiteral("Public"),
        QStringLiteral("folder-publicshare"));
    add(userDirsPath(QStringLiteral("TEMPLATES")), QStringLiteral("Templates"),
        QStringLiteral("folder-templates"));
    return out;
}

void Places::rebuild()
{
    beginResetModel();
    m_entries.clear();
    m_entries += xdgEntries();

    for (const Bookmark& b : std::as_const(m_bookmarks)) {
        Entry e;
        e.label = b.label;
        e.iconName = b.icon.isEmpty() ? QStringLiteral("folder-bookmark") : b.icon;
        e.url = QUrl(b.url).isLocalFile() ? QUrl(b.url).toLocalFile() : b.url;
        e.section = PlaceSection::Bookmarks;
        e.id = QStringLiteral("bookmark:") + b.url;
        e.isBookmark = true;
        m_entries << e;
    }

    const QList<MountMonitor::Mount> mounts = m_mounts->mounts();
    for (const MountMonitor::Mount& m : mounts) {
        Entry e;
        e.label = m.label;
        e.url = m.mountPoint;
        e.section = m.isNetwork ? PlaceSection::Network : PlaceSection::Devices;
        e.isMounted = true;
        e.isEjectable = (m.isRemovable || m.isOptical) && m_mounts->udisksAvailable();
        e.totalBytes = m.bytesTotal;
        e.usedBytes = m.bytesTotal > 0 ? m.bytesTotal - m.bytesFree : -1;
        e.id = QStringLiteral("device:") + m.device;
        e.iconName = m.isNetwork ? QStringLiteral("folder-remote")
            : m.isOptical        ? QStringLiteral("media-optical")
            : m.isRemovable      ? QStringLiteral("drive-removable-media")
                                 : QStringLiteral("drive-harddisk");
        m_entries << e;
    }

    Entry trash;
    trash.label = QStringLiteral("Trash");
    trash.iconName = trashItemCount() > 0 ? QStringLiteral("user-trash-full")
                                          : QStringLiteral("user-trash");
    trash.url = trashFilesPath();
    trash.section = PlaceSection::Trash;
    trash.itemCount = trashItemCount();
    trash.id = QStringLiteral("trash");
    m_entries << trash;

    endResetModel();
}

int Places::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QHash<int, QByteArray> Places::roleNames() const
{
    return {
        {LabelRole, "label"},
        {IconNameRole, "iconName"},
        {UrlRole, "url"},
        {SectionRole, "section"},
        {SectionNameRole, "sectionName"},
        {IsMountedRole, "isMounted"},
        {IsEjectableRole, "isEjectable"},
        {UsedBytesRole, "usedBytes"},
        {TotalBytesRole, "totalBytes"},
        {ItemCountRole, "itemCount"},
        {IdRole, "id"},
        {IsBookmarkRole, "isBookmark"},
    };
}

QVariant Places::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const Entry& e = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case LabelRole:
        return e.label;
    case IconNameRole:
        return e.iconName;
    case UrlRole:
        return e.url;
    case SectionRole:
        return QVariant::fromValue(e.section);
    case SectionNameRole:
        switch (e.section) {
        case PlaceSection::Places:
            return QStringLiteral("Places");
        case PlaceSection::Bookmarks:
            return QStringLiteral("Bookmarks");
        case PlaceSection::Devices:
            return QStringLiteral("Devices");
        case PlaceSection::Network:
            return QStringLiteral("Network");
        case PlaceSection::Trash:
            return QStringLiteral("Trash");
        }
        return {};
    case IsMountedRole:
        return e.isMounted;
    case IsEjectableRole:
        return e.isEjectable;
    case UsedBytesRole:
        return e.usedBytes;
    case TotalBytesRole:
        return e.totalBytes;
    case ItemCountRole:
        return e.itemCount;
    case IdRole:
        return e.id;
    case IsBookmarkRole:
        return e.isBookmark;
    default:
        return {};
    }
}

QString Places::activate(const QString& url) const
{
    return url;
}

bool Places::udisksAvailable() const
{
    return m_mounts->udisksAvailable();
}

// -------------------------------------------------------------- bookmarks

void Places::loadBookmarks()
{
    m_bookmarks.clear();
    QFile f(m_bookmarksPath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QByteArray data = f.readAll();
    try {
        const toml::table root =
            toml::parse(std::string_view(data.constData(), size_t(data.size())));
        if (const toml::array* arr = root["bookmark"].as_array()) {
            for (const toml::node& n : *arr) {
                const toml::table* t = n.as_table();
                if (!t)
                    continue;
                Bookmark b;
                b.label = QString::fromStdString((*t)["label"].value_or<std::string>(""));
                b.url = QString::fromStdString((*t)["url"].value_or<std::string>(""));
                b.icon = QString::fromStdString((*t)["icon"].value_or<std::string>(""));
                if (!b.url.isEmpty())
                    m_bookmarks << b;
            }
        }
    } catch (const toml::parse_error& err) {
        qCWarning(fangPlaces) << "bookmarks.toml parse error:" << err.description().data();
    }
}

void Places::saveBookmarks()
{
    toml::table root;
    root.insert("schema_version", 1);
    toml::array arr;
    for (const Bookmark& b : std::as_const(m_bookmarks)) {
        toml::table t;
        t.insert("label", b.label.toStdString());
        t.insert("url", b.url.toStdString());
        if (!b.icon.isEmpty())
            t.insert("icon", b.icon.toStdString());
        arr.push_back(std::move(t));
    }
    root.insert("bookmark", std::move(arr));

    QDir().mkpath(QFileInfo(m_bookmarksPath).absolutePath());
    QSaveFile out(m_bookmarksPath);
    if (!out.open(QIODevice::WriteOnly)) {
        emit error(tr("Could not save bookmarks"), m_bookmarksPath);
        return;
    }
    std::ostringstream ss;
    ss << root;
    const std::string str = ss.str();
    out.write(str.data(), qint64(str.size()));
    if (!out.commit())
        emit error(tr("Could not save bookmarks"), m_bookmarksPath);
}

void Places::addBookmark(const QString& url, const QString& label)
{
    Bookmark b;
    b.url = url.startsWith(QLatin1Char('/')) ? QUrl::fromLocalFile(url).toString() : url;
    b.label = label.isEmpty() ? QFileInfo(QUrl(b.url).toLocalFile()).fileName() : label;
    m_bookmarks << b;
    saveBookmarks();
    rebuild();
}

void Places::removeBookmark(const QString& id)
{
    const QString url = id.startsWith(QLatin1String("bookmark:")) ? id.mid(9) : id;
    const auto before = m_bookmarks.size();
    m_bookmarks.removeIf([&](const Bookmark& b) { return b.url == url; });
    if (m_bookmarks.size() != before) {
        saveBookmarks();
        rebuild();
    }
}

void Places::moveBookmark(int from, int to)
{
    if (from < 0 || to < 0 || from >= m_bookmarks.size() || to >= m_bookmarks.size()
        || from == to)
        return;
    m_bookmarks.move(from, to);
    saveBookmarks();
    rebuild();
}

// ---------------------------------------------------------------- devices

void Places::mount(const QString& id)
{
    if (id.startsWith(QLatin1String("device:")))
        m_mounts->mount(id.mid(7));
}

void Places::unmount(const QString& id)
{
    if (id.startsWith(QLatin1String("device:")))
        m_mounts->unmount(id.mid(7));
}

void Places::eject(const QString& id)
{
    if (id.startsWith(QLatin1String("device:")))
        m_mounts->eject(id.mid(7));
}

} // namespace fang
