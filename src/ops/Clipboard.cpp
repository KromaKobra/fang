#include "Clipboard.h"

#include "ops/FileOperationEngine.h"
#include "util/Logging.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

namespace fang {

static const char* kGnomeFilesMime = "x-special/gnome-copied-files";

Clipboard::Clipboard(FileOperationEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
    if (auto* cb = QGuiApplication::clipboard()) {
        connect(cb, &QClipboard::dataChanged, this, &Clipboard::readFromSystem);
        readFromSystem();
    }
}

QStringList Clipboard::cutPaths() const
{
    if (m_mode != ClipboardMode::Cut)
        return {};
    QStringList paths;
    for (const QUrl& url : m_urls)
        if (url.isLocalFile())
            paths << url.toLocalFile();
    return paths;
}

void Clipboard::copy(const QList<QUrl>& urls)
{
    write(urls, ClipboardMode::Copy);
}

void Clipboard::cut(const QList<QUrl>& urls)
{
    write(urls, ClipboardMode::Cut);
}

void Clipboard::write(const QList<QUrl>& urls, ClipboardMode mode)
{
    auto* cb = QGuiApplication::clipboard();
    if (!cb || urls.isEmpty())
        return;

    auto* mime = new QMimeData; // ownership passes to the clipboard
    QByteArray gnome = mode == ClipboardMode::Cut ? "cut" : "copy";
    QStringList plain;
    for (const QUrl& url : urls) {
        gnome += '\n' + url.toEncoded();
        plain << (url.isLocalFile() ? url.toLocalFile() : url.toString());
    }
    mime->setData(QLatin1String(kGnomeFilesMime), gnome);
    mime->setUrls(urls);
    mime->setText(plain.join(QLatin1Char('\n')));
    cb->setMimeData(mime);
    // dataChanged fires and readFromSystem() updates our state.
}

void Clipboard::readFromSystem()
{
    const QMimeData* mime = QGuiApplication::clipboard()->mimeData();
    QList<QUrl> urls;
    ClipboardMode mode = ClipboardMode::Copy;
    if (mime) {
        if (mime->hasFormat(QLatin1String(kGnomeFilesMime))) {
            const QList<QByteArray> lines =
                mime->data(QLatin1String(kGnomeFilesMime)).split('\n');
            for (const QByteArray& line : lines) {
                const QByteArray token = line.trimmed();
                if (token == "cut")
                    mode = ClipboardMode::Cut;
                else if (token == "copy")
                    mode = ClipboardMode::Copy;
                else if (!token.isEmpty())
                    urls << QUrl::fromEncoded(token);
            }
        } else if (mime->hasUrls()) {
            urls = mime->urls();
        }
    }
    if (urls == m_urls && mode == m_mode)
        return;
    m_urls = urls;
    m_mode = mode;
    emit changed();
}

Operation* Clipboard::paste(const QUrl& destDir)
{
    if (m_urls.isEmpty())
        return nullptr;
    Operation* op = nullptr;
    if (m_mode == ClipboardMode::Cut) {
        op = m_engine->move(m_urls, destDir);
        clear(); // a cut may only be pasted once
    } else {
        op = m_engine->copy(m_urls, destDir);
    }
    return op;
}

void Clipboard::clear()
{
    if (auto* cb = QGuiApplication::clipboard())
        cb->clear();
    if (m_urls.isEmpty())
        return;
    m_urls.clear();
    m_mode = ClipboardMode::Copy;
    emit changed();
}

void Clipboard::copyText(const QString& text)
{
    if (auto* cb = QGuiApplication::clipboard())
        cb->setText(text);
}

void Clipboard::copyPaths(const QList<QUrl>& urls)
{
    QStringList paths;
    for (const QUrl& url : urls)
        paths << (url.isLocalFile() ? url.toLocalFile() : url.toString());
    copyText(paths.join(QLatin1Char('\n')));
}

} // namespace fang
