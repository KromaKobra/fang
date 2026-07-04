#include "Navigation.h"

#include "app/PaneController.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QVariantMap>

namespace fang {

Navigation::Navigation(PaneController* pane)
    : QObject(pane)
    , m_pane(pane)
{
}

bool Navigation::canGoUp() const
{
    const QString loc = m_pane->location();
    return !loc.isEmpty() && loc != QLatin1String("/");
}

QVariantList Navigation::breadcrumbs() const
{
    QVariantList crumbs;
    const QString loc = m_pane->location();
    if (loc.isEmpty())
        return crumbs;

    QVariantMap rootCrumb;
    rootCrumb[QStringLiteral("label")] = QStringLiteral("/");
    rootCrumb[QStringLiteral("url")] = QStringLiteral("/");
    crumbs << rootCrumb;

    QString acc;
    const QStringList parts = loc.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        acc += QLatin1Char('/') + part;
        QVariantMap crumb;
        crumb[QStringLiteral("label")] = part;
        crumb[QStringLiteral("url")] = acc;
        crumbs << crumb;
    }
    return crumbs;
}

QString Navigation::editableText() const
{
    return m_pane->location();
}

void Navigation::back()
{
    if (m_back.isEmpty())
        return;
    const QString target = m_back.takeLast();
    m_forward.append(m_pane->location());
    m_pane->openWithoutHistory(target);
    emit historyChanged();
}

void Navigation::forward()
{
    if (m_forward.isEmpty())
        return;
    const QString target = m_forward.takeLast();
    m_back.append(m_pane->location());
    m_pane->openWithoutHistory(target);
    emit historyChanged();
}

void Navigation::up()
{
    if (!canGoUp())
        return;
    const QFileInfo fi(m_pane->location());
    m_pane->open(fi.absolutePath());
}

void Navigation::home()
{
    m_pane->open(QDir::homePath());
}

void Navigation::refresh()
{
    m_pane->refresh();
}

void Navigation::goTo(const QString& url)
{
    m_pane->open(url);
}

void Navigation::commitEdit(const QString& text)
{
    QString path = text.trimmed();
    if (path.isEmpty())
        return;
    if (path.startsWith(QLatin1String("file://")))
        path = QUrl(path).toLocalFile();
    if (path == QLatin1String("~") || path.startsWith(QLatin1String("~/")))
        path = QDir::homePath() + path.mid(1);
    const QFileInfo fi(path);
    if (!fi.exists()) {
        emit m_pane->error(tr("Location does not exist"), text);
        return;
    }
    // A typed file path opens its folder (and could select the file).
    m_pane->open(fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath());
}

QStringList Navigation::childDirs(const QString& dirPath) const
{
    QStringList out = QDir(dirPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                              QDir::Name | QDir::IgnoreCase);
    if (out.size() > 200)
        out = out.mid(0, 200);
    return out;
}

void Navigation::recordVisit(const QString& fromPath)
{
    if (!fromPath.isEmpty()) {
        m_back.append(fromPath);
        while (m_back.size() > 100)
            m_back.removeFirst();
    }
    m_forward.clear();
    emit historyChanged();
}

} // namespace fang
