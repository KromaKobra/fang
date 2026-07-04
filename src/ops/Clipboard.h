#pragma once

#include "util/Enums.h"

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

Q_MOC_INCLUDE("ops/Operation.h")

namespace fang {

class FileOperationEngine;
class Operation;

/// System-clipboard file transfer (docs/04): writes/reads the
/// x-special/gnome-copied-files convention plus text/uri-list, tracks
/// copy-vs-cut mode, and turns paste() into an engine operation.
class Clipboard : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("The Clipboard is owned by the application")

    Q_PROPERTY(bool hasFiles READ hasFiles NOTIFY changed)
    Q_PROPERTY(fang::ClipboardMode mode READ mode NOTIFY changed)
    /// Local paths currently cut (empty in copy mode) — FileView binds this
    /// for cut-dimming. Additive to API v1.
    Q_PROPERTY(QStringList cutPaths READ cutPaths NOTIFY changed)

public:
    explicit Clipboard(FileOperationEngine* engine, QObject* parent = nullptr);

    bool hasFiles() const { return !m_urls.isEmpty(); }
    ClipboardMode mode() const { return m_mode; }
    QStringList cutPaths() const;
    QList<QUrl> urls() const { return m_urls; }

    Q_INVOKABLE void copy(const QList<QUrl>& urls);
    Q_INVOKABLE void cut(const QList<QUrl>& urls);
    Q_INVOKABLE fang::Operation* paste(const QUrl& destDir);
    Q_INVOKABLE void clear();
    /// Plain-text convenience ("Copy Path"). Additive to API v1.
    Q_INVOKABLE void copyText(const QString& text);
    /// "Copy Path": newline-joined local paths as plain text. Additive.
    Q_INVOKABLE void copyPaths(const QList<QUrl>& urls);

signals:
    void changed();

private:
    void write(const QList<QUrl>& urls, ClipboardMode mode);
    void readFromSystem();

    FileOperationEngine* m_engine;
    QList<QUrl> m_urls;
    ClipboardMode m_mode = ClipboardMode::Copy;
};

} // namespace fang
