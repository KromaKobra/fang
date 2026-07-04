#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <memory>

Q_MOC_INCLUDE("search/SearchResultsModel.h")
Q_MOC_INCLUDE("search/SearchQuery.h")

namespace fang {

class SearchQuery;
class SearchResultsModel;

/// The `Search` controller (docs/03 §9, docs/04): owns the results model
/// and a default query object, runs the recursive walk + content grep on a
/// worker thread, streams matches in queued batches, and is cancellable.
class SearchEngine : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Search)
    QML_UNCREATABLE("The Search controller is owned by the application")

    Q_PROPERTY(fang::SearchResultsModel* results READ results CONSTANT)
    Q_PROPERTY(fang::SearchQuery* query READ query CONSTANT)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int dirsScanned READ dirsScanned NOTIFY progressChanged)
    Q_PROPERTY(int matchCount READ matchCount NOTIFY progressChanged)

public:
    explicit SearchEngine(QObject* parent = nullptr);
    ~SearchEngine() override;

    SearchResultsModel* results() const { return m_results; }
    SearchQuery* query() const { return m_query; }
    bool running() const { return m_running; }
    int dirsScanned() const { return m_dirsScanned; }
    int matchCount() const { return m_matchCount; }

    Q_INVOKABLE void start(fang::SearchQuery* query = nullptr);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void clear();

signals:
    void runningChanged();
    void progressChanged();
    void finished();

private:
    void setRunning(bool running);

    SearchResultsModel* m_results;
    SearchQuery* m_query;
    std::shared_ptr<std::atomic<bool>> m_cancel;
    quint64 m_generation = 0; ///< stale worker results are dropped
    bool m_running = false;
    int m_dirsScanned = 0;
    int m_matchCount = 0;
};

} // namespace fang
