#include "Selection.h"

#include "fs/DirectoryModel.h"

namespace fang {

Selection::Selection(DirectoryModel* model, QObject* parent)
    : QObject(parent)
    , m_model(model)
{
    // A new listing invalidates path-based selection.
    connect(model, &DirectoryModel::pathChanged, this, &Selection::clear);
}

QList<QUrl> Selection::selectedUrls() const
{
    QList<QUrl> urls;
    urls.reserve(m_paths.size());
    // Keep listing order for predictable operation batches.
    for (int row = 0; row < m_model->count(); ++row) {
        const FileInfo* fi = m_model->infoAt(row);
        if (fi && m_paths.contains(fi->path))
            urls << QUrl::fromLocalFile(fi->path);
    }
    if (urls.size() < m_paths.size()) { // selected entries filtered out of view
        for (const QString& p : m_paths) {
            const QUrl u = QUrl::fromLocalFile(p);
            if (!urls.contains(u))
                urls << u;
        }
    }
    return urls;
}

QStringList Selection::selectedPaths() const
{
    QStringList out;
    const QList<QUrl> urls = selectedUrls();
    out.reserve(urls.size());
    for (const QUrl& u : urls)
        out << u.toLocalFile();
    return out;
}

qint64 Selection::selectedSize() const
{
    qint64 total = 0;
    for (int row = 0; row < m_model->count(); ++row) {
        const FileInfo* fi = m_model->infoAt(row);
        if (fi && fi->size > 0 && m_paths.contains(fi->path))
            total += fi->size;
    }
    return total;
}

bool Selection::allSelected() const
{
    const int n = m_model->count();
    return n > 0 && count() >= n;
}

void Selection::setCurrent(int row)
{
    if (m_current == row)
        return;
    m_current = row;
    emit currentChanged();
}

void Selection::select(int row)
{
    m_paths.clear();
    const QString p = m_model->pathAt(row);
    if (!p.isEmpty())
        m_paths.insert(p);
    m_current = row;
    m_anchor = row;
    notify();
}

void Selection::toggle(int row)
{
    const QString p = m_model->pathAt(row);
    if (p.isEmpty())
        return;
    if (m_paths.contains(p))
        m_paths.remove(p);
    else
        m_paths.insert(p);
    m_current = row;
    m_anchor = row;
    notify();
}

void Selection::range(int from, int to)
{
    if (from > to)
        std::swap(from, to);
    m_paths.clear();
    for (int row = from; row <= to; ++row) {
        const QString p = m_model->pathAt(row);
        if (!p.isEmpty())
            m_paths.insert(p);
    }
    m_current = to;
    notify();
}

void Selection::selectAll()
{
    m_paths.clear();
    for (int row = 0; row < m_model->count(); ++row)
        m_paths.insert(m_model->pathAt(row));
    notify();
}

void Selection::clear()
{
    if (m_paths.isEmpty() && m_current < 0)
        return;
    m_paths.clear();
    m_current = -1;
    m_anchor = -1;
    notify();
    emit currentChanged();
}

void Selection::invert()
{
    QSet<QString> inverted;
    for (int row = 0; row < m_model->count(); ++row) {
        const QString p = m_model->pathAt(row);
        if (!m_paths.contains(p))
            inverted.insert(p);
    }
    m_paths = inverted;
    notify();
}

bool Selection::isSelected(int row) const
{
    const QString p = m_model->pathAt(row);
    return !p.isEmpty() && m_paths.contains(p);
}

void Selection::selectPath(const QString& path)
{
    m_paths.clear();
    m_paths.insert(path);
    m_current = m_model->rowOf(path);
    m_anchor = m_current;
    notify();
}

bool Selection::isPathSelected(const QString& path) const
{
    return m_paths.contains(path);
}

void Selection::notify()
{
    emit changed();
    m_model->notifySelectionChanged();
}

} // namespace fang
