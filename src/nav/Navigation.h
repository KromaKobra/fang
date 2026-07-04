#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace fang {

class PaneController;

/// Per-pane navigation: back/forward/up history, breadcrumb segments, and
/// location-bar editing/validation (docs/03, docs/04).
class Navigation : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Navigation is owned by PaneController")

    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY historyChanged)
    /// list of { label: string, url: string } from root to current dir.
    Q_PROPERTY(QVariantList breadcrumbs READ breadcrumbs NOTIFY historyChanged)
    Q_PROPERTY(QString editableText READ editableText NOTIFY historyChanged)

public:
    explicit Navigation(PaneController* pane);

    bool canGoBack() const { return !m_back.isEmpty(); }
    bool canGoForward() const { return !m_forward.isEmpty(); }
    bool canGoUp() const;
    QVariantList breadcrumbs() const;
    QString editableText() const;

    Q_INVOKABLE void back();
    Q_INVOKABLE void forward();
    Q_INVOKABLE void up();
    Q_INVOKABLE void home();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void goTo(const QString& url);
    /// Parses/validates a typed location; emits pane error() when invalid.
    Q_INVOKABLE void commitEdit(const QString& text);
    /// Subdirectory names of `dirPath` (capped), for breadcrumb sibling
    /// dropdowns and location-bar autocomplete. Additive to API v1.
    Q_INVOKABLE QStringList childDirs(const QString& dirPath) const;

    /// Called by PaneController when the location actually changes.
    void recordVisit(const QString& fromPath);
    void historyReset() { emit historyChanged(); }

signals:
    void historyChanged();

private:
    PaneController* m_pane;
    QStringList m_back;
    QStringList m_forward;
};

} // namespace fang
