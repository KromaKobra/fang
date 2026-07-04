#include "SearchEngine.h"

#include "search/SearchQuery.h"
#include "search/SearchResultsModel.h"
#include "util/Logging.h"

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QMimeDatabase>
#include <QPointer>
#include <QRegularExpression>
#include <QThread>

namespace fang {

namespace {

constexpr qint64 kContentSizeCap = 64 * 1024 * 1024;
constexpr int kBatchSize = 64;
constexpr int kBatchIntervalMs = 100;

/// Value snapshot of a SearchQuery, safe to move across threads.
struct QuerySnapshot {
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
    QStringList ignorePaths;
};

QuerySnapshot snapshot(const SearchQuery* q)
{
    QuerySnapshot s;
    s.rootPath = q->rootPath;
    s.namePattern = q->namePattern;
    s.contentSearch = q->contentSearch;
    s.contentQuery = q->contentQuery;
    s.caseSensitive = q->caseSensitive;
    s.regex = q->regex;
    s.includeHidden = q->includeHidden;
    s.followSymlinks = q->followSymlinks;
    s.maxDepth = q->maxDepth;
    s.minSize = q->minSize;
    s.maxSize = q->maxSize;
    s.modifiedAfter = q->modifiedAfter;
    s.modifiedBefore = q->modifiedBefore;
    s.mimeTypes = q->mimeTypes;
    s.ignorePaths = q->ignorePaths;
    return s;
}

bool mimeGlobMatch(const QStringList& patterns, const QString& mime)
{
    for (const QString& p : patterns) {
        if (p.endsWith(QLatin1String("/*"))) {
            if (mime.startsWith(QStringView(p).left(p.size() - 1)))
                return true;
        } else if (p == mime) {
            return true;
        }
    }
    return false;
}

} // namespace

/// Worker object living on its own thread. Emits batches + progress via
/// queued connections; checks the shared cancel flag per entry.
class SearchWorker : public QObject {
    Q_OBJECT

public:
    SearchWorker(QuerySnapshot query, std::shared_ptr<std::atomic<bool>> cancel)
        : m_q(std::move(query))
        , m_cancel(std::move(cancel))
    {
    }

signals:
    void batchReady(const QList<fang::SearchResultsModel::Result>& results);
    void progress(int dirsScanned, int matches);
    void done();

public slots:
    void run()
    {
        m_nameNeedle = m_q.namePattern;
        if (m_q.regex) {
            m_nameRe = QRegularExpression(
                m_q.namePattern,
                m_q.caseSensitive ? QRegularExpression::NoPatternOption
                                  : QRegularExpression::CaseInsensitiveOption);
            m_useRe = m_nameRe.isValid() && !m_q.namePattern.isEmpty();
        } else if (m_q.namePattern.contains(QLatin1Char('*'))
                   || m_q.namePattern.contains(QLatin1Char('?'))) {
            m_nameRe = QRegularExpression::fromWildcard(
                m_q.namePattern, m_q.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
            m_useRe = true;
        }

        m_flushClock.start();
        const QString root = QDir::cleanPath(m_q.rootPath);
        if (QFileInfo(root).isDir())
            walk(root, 0);
        flush(true);
        emit progress(m_dirs, m_matches);
        emit done();
    }

private:
    bool cancelled() const { return m_cancel->load(); }

    bool ignored(const QString& path) const
    {
        for (const QString& ig : m_q.ignorePaths) {
            if (path == ig || path.startsWith(ig + QLatin1Char('/')))
                return true;
        }
        return false;
    }

    void walk(const QString& dir, int depth)
    {
        if (cancelled())
            return;
        ++m_dirs;

        QDir::Filters filters =
            QDir::AllEntries | QDir::System | QDir::NoDotAndDotDot;
        if (m_q.includeHidden)
            filters |= QDir::Hidden;
        QDirIterator it(dir, filters);
        while (it.hasNext() && !cancelled()) {
            it.next();
            const QFileInfo e = it.fileInfo();
            const QString path = e.absoluteFilePath();

            // Pseudo-filesystems are skipped unless the search root is
            // inside them (docs/03 §9).
            if (ignored(path) && !m_q.rootPath.startsWith(path))
                continue;

            consider(e);

            if (e.isDir() && (!e.isSymLink() || m_q.followSymlinks)
                && (m_q.maxDepth <= 0 || depth + 1 < m_q.maxDepth))
                walk(path, depth + 1);
        }

        if (m_flushClock.elapsed() >= kBatchIntervalMs) {
            flush(false);
            emit progress(m_dirs, m_matches);
        }
    }

    void consider(const QFileInfo& e)
    {
        if (!matchesName(e.fileName()))
            return;
        if (m_q.minSize >= 0 && (!e.isFile() || e.size() < m_q.minSize))
            return;
        if (m_q.maxSize >= 0 && e.isFile() && e.size() > m_q.maxSize)
            return;
        if (m_q.modifiedAfter.isValid() && e.lastModified() < m_q.modifiedAfter)
            return;
        if (m_q.modifiedBefore.isValid() && e.lastModified() > m_q.modifiedBefore)
            return;
        if (!m_q.mimeTypes.isEmpty()) {
            const QString mime =
                m_mimeDb.mimeTypeForFile(e.absoluteFilePath(), QMimeDatabase::MatchExtension)
                    .name();
            if (!mimeGlobMatch(m_q.mimeTypes, mime))
                return;
        }

        QString context;
        if (m_q.contentSearch && !m_q.contentQuery.isEmpty()) {
            if (!e.isFile() || !contentMatches(e, &context))
                return;
        }

        SearchResultsModel::Result r;
        r.info = FileInfo::fromPath(e.absoluteFilePath());
        r.matchContext = context;
        m_pending.append(r);
        ++m_matches;
        if (m_pending.size() >= kBatchSize)
            flush(false);
    }

    bool matchesName(const QString& name) const
    {
        if (m_q.namePattern.isEmpty())
            return true;
        if (m_useRe)
            return m_nameRe.match(name).hasMatch();
        return name.contains(m_nameNeedle,
                             m_q.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }

    bool contentMatches(const QFileInfo& e, QString* context) const
    {
        if (e.size() > kContentSizeCap)
            return false;
        QFile f(e.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly))
            return false;
        const QByteArray head = f.peek(8192);
        if (head.isEmpty() || head.contains('\0'))
            return false; // binary skip

        const QByteArray needle = m_q.contentQuery.toUtf8();
        QByteArray carry;
        while (!f.atEnd() && !cancelled()) {
            QByteArray chunk = carry + f.read(256 * 1024);
            const int idx = m_q.caseSensitive
                ? int(chunk.indexOf(needle))
                : int(chunk.toLower().indexOf(needle.toLower()));
            if (idx >= 0) {
                // Snippet: the line around the first hit, capped.
                int lineStart = chunk.lastIndexOf('\n', idx);
                lineStart = lineStart < 0 ? 0 : lineStart + 1;
                int lineEnd = chunk.indexOf('\n', idx);
                if (lineEnd < 0)
                    lineEnd = int(chunk.size());
                *context = QString::fromUtf8(chunk.mid(lineStart, lineEnd - lineStart))
                               .trimmed()
                               .left(160);
                return true;
            }
            // Keep a tail so matches spanning chunk borders are found.
            carry = chunk.right(needle.size() - 1);
        }
        return false;
    }

    void flush(bool force)
    {
        if (m_pending.isEmpty() && !force)
            return;
        if (!m_pending.isEmpty()) {
            emit batchReady(m_pending);
            m_pending.clear();
        }
        m_flushClock.restart();
    }

    const QuerySnapshot m_q;
    std::shared_ptr<std::atomic<bool>> m_cancel;
    QMimeDatabase m_mimeDb;
    QRegularExpression m_nameRe;
    QString m_nameNeedle;
    bool m_useRe = false;
    int m_dirs = 0;
    int m_matches = 0;
    QList<SearchResultsModel::Result> m_pending;
    QElapsedTimer m_flushClock;
};

SearchEngine::SearchEngine(QObject* parent)
    : QObject(parent)
    , m_results(new SearchResultsModel(this))
    , m_query(new SearchQuery(this))
{
    qRegisterMetaType<QList<SearchResultsModel::Result>>();
}

SearchEngine::~SearchEngine()
{
    cancel();
}

void SearchEngine::start(SearchQuery* query)
{
    cancel();
    m_results->clear();
    m_dirsScanned = 0;
    m_matchCount = 0;
    emit progressChanged();

    const QuerySnapshot snap = snapshot(query ? query : m_query);
    m_cancel = std::make_shared<std::atomic<bool>>(false);
    const quint64 generation = ++m_generation;

    auto* worker = new SearchWorker(snap, m_cancel);
    auto* thread = new QThread(this);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &SearchWorker::run);
    connect(worker, &SearchWorker::batchReady, this,
            [this, generation](const QList<SearchResultsModel::Result>& results) {
                if (generation == m_generation)
                    m_results->appendBatch(results);
            });
    connect(worker, &SearchWorker::progress, this,
            [this, generation](int dirs, int matches) {
                if (generation != m_generation)
                    return;
                m_dirsScanned = dirs;
                m_matchCount = matches;
                emit progressChanged();
            });
    connect(worker, &SearchWorker::done, this, [this, generation] {
        if (generation == m_generation) {
            setRunning(false);
            emit finished();
        }
    });
    connect(worker, &SearchWorker::done, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    setRunning(true);
    thread->start();
}

void SearchEngine::cancel()
{
    if (m_cancel)
        m_cancel->store(true);
    setRunning(false);
}

void SearchEngine::clear()
{
    cancel();
    m_results->clear();
    m_dirsScanned = 0;
    m_matchCount = 0;
    emit progressChanged();
}

void SearchEngine::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged();
}

} // namespace fang

#include "SearchEngine.moc"
