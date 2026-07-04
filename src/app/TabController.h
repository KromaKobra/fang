#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

Q_MOC_INCLUDE("app/PaneController.h")

namespace fang {

class PaneController;
class WindowController;

/// One tab: a left pane, optionally a right pane (split view), and the
/// active-pane pointer (docs/03 §2, docs/04).
class TabController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Tabs are created by their WindowController")

    Q_PROPERTY(fang::PaneController* leftPane READ leftPane NOTIFY splitViewChanged)
    Q_PROPERTY(fang::PaneController* rightPane READ rightPane NOTIFY splitViewChanged)
    Q_PROPERTY(fang::PaneController* activePane READ activePane NOTIFY activePaneChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool splitView READ splitView WRITE setSplitView NOTIFY splitViewChanged)

public:
    explicit TabController(WindowController* window, const QString& path,
                           QObject* parent = nullptr);

    PaneController* leftPane() const { return m_left; }
    PaneController* rightPane() const { return m_right; }
    PaneController* activePane() const { return m_active; }
    QString title() const;

    bool splitView() const { return m_right != nullptr; }
    void setSplitView(bool split);

    Q_INVOKABLE void setActivePane(fang::PaneController* pane);
    void focusOtherPane();

signals:
    void splitViewChanged();
    void activePaneChanged();
    void titleChanged();

private:
    WindowController* m_window;
    PaneController* m_left = nullptr;
    PaneController* m_right = nullptr;
    PaneController* m_active = nullptr;
};

} // namespace fang
