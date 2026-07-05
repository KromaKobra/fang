#include "WindowController.h"

#include "app/Application.h"
#include "app/PaneController.h"
#include "app/TabController.h"
#include "config/Settings.h"

#include <QDir>

namespace fang {

WindowController::WindowController(QObject* parent)
    : QObject(parent)
{
}

QQmlListProperty<TabController> WindowController::tabsProp()
{
    return QQmlListProperty<TabController>(
        this, nullptr,
        [](QQmlListProperty<TabController>* prop) {
            return qsizetype(static_cast<WindowController*>(prop->object)->m_tabs.size());
        },
        [](QQmlListProperty<TabController>* prop, qsizetype i) -> TabController* {
            const auto& tabs = static_cast<WindowController*>(prop->object)->m_tabs;
            return (i >= 0 && i < tabs.size()) ? tabs.at(i) : nullptr;
        });
}

TabController* WindowController::activeTab() const
{
    return (m_activeTabIndex >= 0 && m_activeTabIndex < m_tabs.size())
        ? m_tabs.at(m_activeTabIndex)
        : nullptr;
}

TabController* WindowController::tabAt(int index) const
{
    return (index >= 0 && index < m_tabs.size()) ? m_tabs.at(index) : nullptr;
}

void WindowController::setActiveTabIndex(int index)
{
    if (index < 0 || index >= m_tabs.size() || index == m_activeTabIndex)
        return;
    m_activeTabIndex = index;
    emit activeTabChanged();
    emit splitViewChanged();
}

bool WindowController::splitView() const
{
    return activeTab() && activeTab()->splitView();
}

void WindowController::setSplitView(bool split)
{
    if (TabController* tab = activeTab())
        tab->setSplitView(split);
}

TabController* WindowController::addTab(const QString& path)
{
    const Settings* settings =
        Application::instance() ? Application::instance()->settings() : nullptr;
    QString target = path;
    if (target.isEmpty())
        target = settings ? settings->defaultLocation() : QDir::homePath();
    auto* tab = new TabController(this, target, this);
    connect(tab, &TabController::splitViewChanged, this, [this, tab] {
        if (tab == activeTab())
            emit splitViewChanged();
    });

    // behavior.new_tab_position (docs/06): "after" inserts right of the
    // active tab, "end" appends.
    const bool atEnd = settings && settings->newTabPosition() == QLatin1String("end");
    const int insertAt = (!atEnd && m_activeTabIndex >= 0 && m_activeTabIndex < m_tabs.size())
        ? m_activeTabIndex + 1
        : int(m_tabs.size());
    m_tabs.insert(insertAt, tab);
    emit tabsChanged();
    setActiveTabIndex(insertAt);
    if (m_tabs.size() == 1) { // first tab: setActiveTabIndex(0) was a no-op guard
        m_activeTabIndex = 0;
        emit activeTabChanged();
        emit splitViewChanged();
    }
    return tab;
}

void WindowController::closeTab(int index)
{
    if (index < 0 || index >= m_tabs.size())
        return;
    TabController* tab = m_tabs.takeAt(index);
    tab->deleteLater();

    if (m_tabs.isEmpty()) {
        m_activeTabIndex = -1;
        emit tabsChanged();
        emit activeTabChanged();
        emit closeRequested();
        return;
    }
    if (m_activeTabIndex >= index)
        m_activeTabIndex = qBound(0, m_activeTabIndex - 1, int(m_tabs.size()) - 1);
    emit tabsChanged();
    emit activeTabChanged();
    emit splitViewChanged();
}

void WindowController::moveTab(int from, int to)
{
    if (from < 0 || to < 0 || from >= m_tabs.size() || to >= m_tabs.size() || from == to)
        return;
    m_tabs.move(from, to);
    if (m_activeTabIndex == from)
        m_activeTabIndex = to;
    else if (from < m_activeTabIndex && to >= m_activeTabIndex)
        --m_activeTabIndex;
    else if (from > m_activeTabIndex && to <= m_activeTabIndex)
        ++m_activeTabIndex;
    emit tabsChanged();
    emit activeTabChanged();
}

void WindowController::duplicateTab(int index)
{
    TabController* tab = tabAt(index);
    if (!tab || !tab->activePane())
        return;
    addTab(tab->activePane()->location());
}

void WindowController::toggleSplit()
{
    setSplitView(!splitView());
}

void WindowController::focusOtherPane()
{
    if (TabController* tab = activeTab())
        tab->focusOtherPane();
}

} // namespace fang
