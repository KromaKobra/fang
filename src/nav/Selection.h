#pragma once

#include <QObject>
#include <QSet>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace fang {

class DirectoryModel;

/// Selection set for one pane. Rows refer to the pane model's top-level
/// listing; internally selection is tracked by absolute path so it survives
/// re-sorts and refreshes (docs/03 §3, docs/04).
class Selection : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Selection is owned by PaneController")

    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QList<QUrl> selectedUrls READ selectedUrls NOTIFY changed)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY changed)
    Q_PROPERTY(qint64 selectedSize READ selectedSize NOTIFY changed)
    Q_PROPERTY(int current READ current WRITE setCurrent NOTIFY currentChanged)
    Q_PROPERTY(int anchor READ anchor NOTIFY changed)
    Q_PROPERTY(bool allSelected READ allSelected NOTIFY changed)

public:
    explicit Selection(DirectoryModel* model, QObject* parent = nullptr);

    int count() const { return int(m_paths.size()); }
    bool hasSelection() const { return !m_paths.isEmpty(); }
    QList<QUrl> selectedUrls() const;
    QStringList selectedPaths() const;
    qint64 selectedSize() const;
    int current() const { return m_current; }
    void setCurrent(int row);
    int anchor() const { return m_anchor; }
    bool allSelected() const;

    Q_INVOKABLE void select(int row);
    Q_INVOKABLE void toggle(int row);
    Q_INVOKABLE void range(int from, int to);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void invert();
    Q_INVOKABLE bool isSelected(int row) const;

    /// Path-based access (additive to API v1; used by tree delegates and
    /// programmatic selection like --select).
    Q_INVOKABLE void selectPath(const QString& path);
    Q_INVOKABLE bool isPathSelected(const QString& path) const;

signals:
    void changed();
    void currentChanged();

private:
    void notify();

    DirectoryModel* m_model;
    QSet<QString> m_paths;
    int m_current = -1;
    int m_anchor = -1;
};

} // namespace fang
