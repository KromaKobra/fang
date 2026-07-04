#pragma once

#include <QDateTime>
#include <QObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace fang {

/// One search's parameters (docs/03 §9). A QML-creatable object so panels
/// can build queries declaratively; the engine snapshots it into a value
/// before handing it to the worker thread.
class SearchQuery : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString rootPath MEMBER rootPath NOTIFY changed)
    Q_PROPERTY(QString namePattern MEMBER namePattern NOTIFY changed)
    Q_PROPERTY(bool contentSearch MEMBER contentSearch NOTIFY changed)
    Q_PROPERTY(QString contentQuery MEMBER contentQuery NOTIFY changed)
    Q_PROPERTY(bool caseSensitive MEMBER caseSensitive NOTIFY changed)
    Q_PROPERTY(bool regex MEMBER regex NOTIFY changed)
    Q_PROPERTY(bool includeHidden MEMBER includeHidden NOTIFY changed)
    Q_PROPERTY(bool followSymlinks MEMBER followSymlinks NOTIFY changed)
    Q_PROPERTY(int maxDepth MEMBER maxDepth NOTIFY changed) // 0 = unlimited
    Q_PROPERTY(qint64 minSize MEMBER minSize NOTIFY changed) // -1 = unset
    Q_PROPERTY(qint64 maxSize MEMBER maxSize NOTIFY changed) // -1 = unset
    Q_PROPERTY(QDateTime modifiedAfter MEMBER modifiedAfter NOTIFY changed)
    Q_PROPERTY(QDateTime modifiedBefore MEMBER modifiedBefore NOTIFY changed)
    Q_PROPERTY(QStringList mimeTypes MEMBER mimeTypes NOTIFY changed)
    Q_PROPERTY(QStringList ignorePaths MEMBER ignorePaths NOTIFY changed)

public:
    explicit SearchQuery(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    QString rootPath;
    QString namePattern;
    bool contentSearch = false;
    QString contentQuery;
    bool caseSensitive = false;
    bool regex = false;
    bool includeHidden = false;
    bool followSymlinks = false;
    int maxDepth = 0;
    qint64 minSize = -1;
    qint64 maxSize = -1;
    QDateTime modifiedAfter;
    QDateTime modifiedBefore;
    QStringList mimeTypes;
    QStringList ignorePaths{QStringLiteral("/proc"), QStringLiteral("/sys"),
                            QStringLiteral("/dev"), QStringLiteral("/run")};

signals:
    void changed();
};

} // namespace fang
