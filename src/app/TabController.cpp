#include "TabController.h"

#include "app/PaneController.h"

#include <QFileInfo>

namespace fang {

TabController::TabController(WindowController* window, const QString& path, QObject* parent)
    : QObject(parent)
    , m_window(window)
{
    m_left = new PaneController(window, this);
    m_active = m_left;
    m_left->open(path);
    connect(m_left, &PaneController::locationChanged, this, &TabController::titleChanged);
}

QString TabController::title() const
{
    const QString loc = m_active ? m_active->location() : QString();
    if (loc.isEmpty() || loc == QLatin1String("/"))
        return QStringLiteral("/");
    const QString base = QFileInfo(loc).fileName();
    return base.isEmpty() ? loc : base;
}

void TabController::setSplitView(bool split)
{
    if (split == splitView())
        return;
    if (split) {
        m_right = new PaneController(m_window, this);
        m_right->open(m_left->location());
        connect(m_right, &PaneController::locationChanged, this, [this] {
            if (m_active == m_right)
                emit titleChanged();
        });
    } else {
        if (m_active == m_right)
            setActivePane(m_left);
        m_right->deleteLater();
        m_right = nullptr;
    }
    emit splitViewChanged();
    emit titleChanged();
}

void TabController::setActivePane(PaneController* pane)
{
    if (!pane || pane == m_active || (pane != m_left && pane != m_right))
        return;
    m_active = pane;
    emit activePaneChanged();
    emit titleChanged();
}

void TabController::focusOtherPane()
{
    if (!m_right)
        return;
    setActivePane(m_active == m_left ? m_right : m_left);
}

} // namespace fang
