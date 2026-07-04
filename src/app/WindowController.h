#pragma once

#include <QObject>
#include <QQmlListProperty>
#include <QtQml/qqmlregistration.h>

Q_MOC_INCLUDE("app/TabController.h")

namespace fang {

class TabController;

/// One per window: owns the tab list, the active tab, and window-level
/// actions (docs/03 §2, docs/04).
class WindowController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Create via Application.newWindow()")

    Q_PROPERTY(QQmlListProperty<fang::TabController> tabs READ tabsProp NOTIFY tabsChanged)
    Q_PROPERTY(int tabCount READ tabCount NOTIFY tabsChanged)
    Q_PROPERTY(int activeTabIndex READ activeTabIndex WRITE setActiveTabIndex
                   NOTIFY activeTabChanged)
    Q_PROPERTY(fang::TabController* activeTab READ activeTab NOTIFY activeTabChanged)
    Q_PROPERTY(bool splitView READ splitView WRITE setSplitView NOTIFY splitViewChanged)

public:
    explicit WindowController(QObject* parent = nullptr);

    QQmlListProperty<TabController> tabsProp();
    const QList<TabController*>& tabList() const { return m_tabs; }
    int tabCount() const { return int(m_tabs.size()); }

    int activeTabIndex() const { return m_activeTabIndex; }
    void setActiveTabIndex(int index);
    TabController* activeTab() const;

    bool splitView() const;
    void setSplitView(bool split);

    Q_INVOKABLE fang::TabController* addTab(const QString& path = QString());
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void moveTab(int from, int to);
    Q_INVOKABLE void duplicateTab(int index);
    Q_INVOKABLE void toggleSplit();
    Q_INVOKABLE void focusOtherPane();
    Q_INVOKABLE fang::TabController* tabAt(int index) const;

signals:
    void tabsChanged();
    void activeTabChanged();
    void splitViewChanged();
    /// Emitted when the last tab closes; the QML window should close.
    void closeRequested();

private:
    QList<TabController*> m_tabs;
    int m_activeTabIndex = -1;
};

} // namespace fang
