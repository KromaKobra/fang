#include "SearchResultsModel.h"

#include "util/HumanFormat.h"

#include <QFileInfo>
#include <QUrl>

namespace fang {

SearchResultsModel::SearchResultsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int SearchResultsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_results.size());
}

QVariant SearchResultsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_results.size())
        return {};
    const Result& r = m_results.at(index.row());
    const FileInfo& fi = r.info;

    switch (role) {
    case NameRole: return fi.name;
    case PathRole: return fi.path;
    case UrlRole: return fi.url();
    case IsDirRole: return fi.isDir;
    case IsSymlinkRole: return fi.isSymlink;
    case SizeRole: return fi.size;
    case SizeTextRole: return fi.isDir ? QString() : HumanFormat::size(fi.size);
    case ModifiedRole: return fi.modified;
    case CreatedRole: return fi.created;
    case AccessedRole: return fi.accessed;
    case MimeRole: return fi.mimeType;
    case MimeCommentRole: return fi.mimeComment;
    case PermissionsRole: return fi.permissions;
    case OwnerRole: return fi.owner;
    case GroupRole: return fi.group;
    case IconNameRole: return fi.iconName;
    case ThumbnailRole:
        return fi.isImage() ? QStringLiteral("image://fang-thumb") + fi.path : QString();
    case IsHiddenRole: return fi.isHidden;
    case SelectedRole: return false;
    case ExtensionRole: return fi.extension;
    case MatchContextRole: return r.matchContext;
    case ParentDirRole: return QFileInfo(fi.path).absolutePath();
    }
    return {};
}

QHash<int, QByteArray> SearchResultsModel::roleNames() const
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
        {MatchContextRole, "matchContext"},
        {ParentDirRole, "parentDir"},
    };
}

QString SearchResultsModel::pathAt(int row) const
{
    return row >= 0 && row < m_results.size() ? m_results.at(row).info.path : QString();
}

QString SearchResultsModel::parentDirAt(int row) const
{
    const QString path = pathAt(row);
    return path.isEmpty() ? QString() : QFileInfo(path).absolutePath();
}

bool SearchResultsModel::isDirAt(int row) const
{
    return row >= 0 && row < m_results.size() && m_results.at(row).info.isDir;
}

void SearchResultsModel::appendBatch(const QList<Result>& results)
{
    if (results.isEmpty())
        return;
    beginInsertRows({}, int(m_results.size()), int(m_results.size() + results.size() - 1));
    m_results.append(results);
    endInsertRows();
    emit countChanged();
}

void SearchResultsModel::clear()
{
    if (m_results.isEmpty())
        return;
    beginResetModel();
    m_results.clear();
    endResetModel();
    emit countChanged();
}

} // namespace fang
