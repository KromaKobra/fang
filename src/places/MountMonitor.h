#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QStorageInfo>
#include <QTimer>

namespace fang {

/// Enumerates mounted volumes (QStorageInfo + /proc/self/mountinfo), watches
/// for mount table changes, classifies removable/network mounts, and offers
/// optional udisks2 mount/unmount/eject (hidden when udisks2 is absent).
class MountMonitor : public QObject {
    Q_OBJECT

public:
    struct Mount {
        QString device;     // /dev/sda1 (or gvfs identifier)
        QString mountPoint;
        QString label;      // display label
        QString fsType;
        qint64 bytesTotal = 0;
        qint64 bytesFree = 0;
        bool isRemovable = false;
        bool isOptical = false;
        bool isNetwork = false; // gvfs
    };

    explicit MountMonitor(QObject* parent = nullptr);

    QList<Mount> mounts() const { return m_mounts; }
    bool udisksAvailable() const { return m_udisksAvailable; }

    /// udisks2 operations (best-effort, async completion via mountsChanged).
    void mount(const QString& device);
    void unmount(const QString& device);
    void eject(const QString& device);

signals:
    void mountsChanged();
    void error(const QString& title, const QString& detail);

private:
    void refresh();
    static QList<Mount> enumerate();
    QString blockObjectPath(const QString& device) const;

    QList<Mount> m_mounts;
    QTimer m_pollTimer;
    QSocketNotifier* m_notifier = nullptr;
    int m_mountinfoFd = -1;
    bool m_udisksAvailable = false;
};

} // namespace fang
